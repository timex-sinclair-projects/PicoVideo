#!/usr/bin/env python3
"""Expand {{file:path}} tokens (HTML-escaped listings), then run the skill's build scripts."""
import html, re, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
SK = Path('~/.claude/skills/ts-program-manual/scripts').expanduser()

def expand(src, dst):
    s = (ROOT / src).read_text()
    s = re.sub(r'\{\{file:([^}]+)\}\}', lambda m: html.escape((ROOT / m.group(1)).read_text().rstrip('\n'), quote=False), s)
    (ROOT / 'tools/tmp').mkdir(exist_ok=True)
    (ROOT / dst).write_text(s)

what = sys.argv[1:] or ['html', 'zine']
if 'html' in what:
    expand('manual_src.html', 'tools/tmp/manual_full.html')
    subprocess.run([sys.executable, SK / 'build_manual.py', 'tools/tmp/manual_full.html', 'PicoVideo Manual.html', '--images', 'images'], cwd=ROOT, check=True)
if 'zine' in what:
    expand('zine_src.html', 'tools/tmp/zine_full.html')
    subprocess.run([sys.executable, SK / 'build_zine.py', 'tools/tmp/zine_full.html', 'PicoVideo Manual (half-letter).pdf', '--images', 'images',
                    '--booklet', 'PicoVideo Manual (booklet, print duplex).pdf', '--preview', 'tools/previews'], cwd=ROOT, check=True)
