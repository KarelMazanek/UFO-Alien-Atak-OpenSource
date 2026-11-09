#!/usr/bin/env python3
"""Reject incomplete or wrong-architecture replacement editor payloads."""
import argparse, hashlib, json, pathlib, struct, sys
from mapeditor_data import inspect_pack, resource, validate_config

def validate(root, target):
    root = pathlib.Path(root)
    binary = root / ('uaa-mapeditor.exe' if target == 'win32' else 'uaa-mapeditor')
    pack = root / 'uaa-data.pak'
    if not binary.is_file() or not pack.is_file():
        raise ValueError('Supply actual UAA Mapeditor 0.1.4 binary and uaa-data.pak; placeholder editor is forbidden')
    data = binary.read_bytes()
    if target == 'win32':
        if len(data) < 64 or data[:2] != b'MZ':
            raise ValueError('Editor is not a Windows executable')
        offset = struct.unpack_from('<I', data, 60)[0]
        if offset + 26 > len(data) or data[offset:offset+4] != b'PE\0\0':
            raise ValueError('Invalid PE header')
        if struct.unpack_from('<H', data, offset+4)[0] != 0x14c or struct.unpack_from('<H', data, offset+24)[0] != 0x10b:
            raise ValueError('Editor must be PE32 i386, not x64')
    else:
        if len(data) < 52 or data[:6] != b'\x7fELF\x01\x01' or struct.unpack_from('<H', data, 18)[0] != 3:
            raise ValueError('Editor must be little-endian ELF32 i386')
        if not binary.stat().st_mode & 0o111:
            raise ValueError('Linux editor is not executable')
    blob = pack.read_bytes()
    index, base = inspect_pack(blob)
    validate_config(resource(blob, index, base, 'config.json'), index)
    resource(blob, index, base, 'font.ttf')
    if len(index) != 1668:
        raise ValueError('Expected 1668 original editor resources')
    del blob
    if not (root / 'LICENSES').is_dir() or not any((root / 'LICENSES').iterdir()):
        raise ValueError('Include original editor LICENSES')
    info = {'version': '0.1.4', 'target': target, 'binary': binary.name, 'resources': len(index), 'sha256': {}}
    for file in [binary, pack]:
        h = hashlib.sha256()
        with file.open('rb') as stream:
            for part in iter(lambda: stream.read(1024 * 1024), b''):
                h.update(part)
        info['sha256'][file.name] = h.hexdigest()
    return info

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('root')
    parser.add_argument('target', choices=['win32', 'linux32'])
    parser.add_argument('--output')
    args = parser.parse_args()
    try:
        result = validate(args.root, args.target)
        rendered = json.dumps(result, indent=2)
        if args.output:
            pathlib.Path(args.output).write_text(rendered + '\n')
        print(rendered)
    except (OSError, ValueError, struct.error) as exc:
        print('MAPEDITOR INTEGRATION BLOCKED:', exc, file=sys.stderr)
        sys.exit(1)
