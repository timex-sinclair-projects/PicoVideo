"""Run host assertions using actual bus, renderer, controller and ISR code."""
import argparse,os,shlex,shutil,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser()
parser.add_argument('--cc',default=os.environ.get('CC'))
parser.add_argument('--upstream-dir',type=Path,help='Unmodified picoCode directory, for the expected-failure control')
args=parser.parse_args()
compiler=shlex.split(args.cc) if args.cc else [next((c for c in ['cl','cc','gcc','clang'] if shutil.which(c)),'')]
if not compiler[0]:parser.error('No C compiler found; use --cc or run from a compiler command prompt')
msvc=Path(compiler[0]).stem.lower()=='cl'
build=HERE/'build';build.mkdir(exist_ok=True)
source=(args.upstream_dir or HERE.parent/'picoCode').resolve()
headers=HERE.parent/'picoCode'
def compile_run(name,unit,extra):
 exe=build/(name+('.exe' if os.name=='nt' else ''))
 sources=[unit,source/'globals.c',*extra]
 if msvc:
  command=compiler+['/nologo','/TC','/std:c11','/O2','/W3',f'/I{HERE/"stubs"}',f'/I{headers}',f'/Fe:{exe}']+list(map(str,sources))
 else:
  command=compiler+['-std=c11','-O2','-Wall','-Wextra','-I'+str(HERE/'stubs'),'-I'+str(headers),'-o',str(exe)]+list(map(str,sources))
 subprocess.run(command,cwd=build,check=True)
 return subprocess.run([str(exe)],cwd=build,text=True,capture_output=True)
result=compile_run('video_regression',HERE/'video_regression.c',[source/'coreB/busMain.c',source/'coreV/displayProc80.c'])
print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr)
if args.upstream_dir:
 if result.returncode==0 or 'writes[gameplay].mode==2' not in result.stdout+result.stderr:
  raise SystemExit('Expected the unchanged source to fail the native-mode transition assertion')
 print('PASS: unchanged-source control detects the native-mode transition difference')
else:
 if result.returncode:raise SystemExit(result.returncode)
 subprocess.run([sys.executable,str(HERE/'extract_video_state.py')],check=True)
 result=compile_run('video_state_regression',build/'video_state_regression.c',[source/'coreV/displayProc80.c'])
 print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr)
 raise SystemExit(result.returncode)
