"""Extract the versioned asset pack appended to the Windows executable."""
import argparse,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
with a.exe.open('rb') as f:
 f.seek(-16,2);magic,offset=struct.unpack('<8sQ',f.read(16))
 if magic!=b'UAAXPK1\0':raise SystemExit('Executable has no embedded UAA asset pack')
 size=f.tell()-16-offset;f.seek(offset)
 if f.read(8)!=b'UAAPAK1\0':raise SystemExit('Invalid pack header')
 f.seek(offset);a.output.parent.mkdir(parents=True,exist_ok=True)
 with a.output.open('wb') as out:
  while size:
   block=f.read(min(size,1024*1024))
   if not block:raise SystemExit('Truncated asset pack')
   out.write(block);size-=len(block)
print(a.output)
