"""Extract the published V4.2 source into a new directory and apply the series."""
import argparse,hashlib,subprocess,zipfile
from pathlib import Path,PurePosixPath
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser()
parser.add_argument('destination',type=Path,help='New output directory; existing directories are refused')
parser.add_argument('--archive',type=Path,default=HERE.parents[1]/'picoTsVid4_2.zip')
parser.add_argument('--photo-palette',action='store_true',help='Apply the separate optional palette patch too')
a=parser.parse_args()
expected='9ff9ccacd694e35cda72ed3443e35e4f911a6af729f16375415bc07cc24f2a7b'
if hashlib.sha256(a.archive.read_bytes()).hexdigest()!=expected:parser.error('Archive hash differs from the reviewed V4.2 source')
if a.destination.exists():parser.error('Destination already exists; choose a new directory')
a.destination.mkdir(parents=True)
with zipfile.ZipFile(a.archive) as z:
 for info in z.infolist():
  p=PurePosixPath(info.filename)
  if p.is_absolute() or '..' in p.parts:raise ValueError('Unsafe archive path')
  if not p.parts or p.parts[0]!='picoCode' or any(c in p.parts for c in ['build','.vscode']) or info.is_dir():continue
  out=a.destination.joinpath(*p.parts);out.parent.mkdir(parents=True,exist_ok=True);data=z.read(info)
  if out.suffix in ('.c','.h','.txt','.cmake','.pio'):data=data.replace(b'\r\n',b'\n')
  out.write_bytes(data)
for name in (HERE/'series.txt').read_text().splitlines():
 subprocess.run(['git','apply','--check',str(HERE/name)],cwd=a.destination,check=True)
 subprocess.run(['git','apply',str(HERE/name)],cwd=a.destination,check=True)
if a.photo_palette:
 subprocess.run(['git','apply','--check',str(HERE/'optional-photo-palette.patch')],cwd=a.destination,check=True)
 subprocess.run(['git','apply',str(HERE/'optional-photo-palette.patch')],cwd=a.destination,check=True)
print('Applied core series'+(' and optional photo palette' if a.photo_palette else ''), 'to',a.destination.resolve())
