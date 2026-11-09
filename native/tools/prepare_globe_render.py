#!/usr/bin/env python3
"""Prepare the exact existing renderer sample without changing the approved map."""
from pathlib import Path
import argparse,hashlib,json
from PIL import Image
Image.MAX_IMAGE_PIXELS=None
parser=argparse.ArgumentParser();parser.add_argument('--data',type=Path,required=True);args=parser.parse_args()
source=args.data/'globe/surface.png';target=source.with_name('surface-render.png')
with Image.open(source) as image:
 width,height=image.size;tw=min(width,8192);th=max(1,int(height*tw/width))
 assert image.mode=="P", "Expected the approved indexed globe image"
 xs=[x*width//tw for x in range(tw)];pixels=image.tobytes();sample=bytearray(tw*th)
 for y in range(th):
  row=(y*height//th)*width;sample[y*tw:(y+1)*tw]=bytes(pixels[row+x] for x in xs)
 prepared=Image.frombytes(image.mode,(tw,th),bytes(sample))
 if image.mode=='P':prepared.putpalette(image.getpalette())
 prepared.save(target,optimize=True)
metadata={'source':source.name,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'source_dimensions':[width,height],'render':target.name,'render_dimensions':[tw,th],'sampling':'floor(dst_coordinate * source_size / destination_size)','render_sha256':hashlib.sha256(target.read_bytes()).hexdigest()}
target.with_suffix('.json').write_text(json.dumps(metadata,indent=2)+'\n');print(json.dumps(metadata))
