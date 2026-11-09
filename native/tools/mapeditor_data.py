#!/usr/bin/env python3
"""Recover the two original non-image resources and rebuild an editor asset pack.

This tool does not invent catalog metadata or modify any graphics.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

MAGIC = b'UAAPAK1\0'
FOOTER = b'UAAXPK1\0'
MAX_INDEX = 16 * 1024 * 1024


def inspect_pack(blob):
    start = 0
    limit = len(blob)
    if len(blob) >= 16 and blob[-16:-8] == FOOTER:
        start = struct.unpack_from('<Q', blob, len(blob) - 8)[0]
        limit -= 16
    if start + 12 > limit or blob[start:start+8] != MAGIC:
        raise ValueError('Invalid UAA pack header or executable footer')
    size = struct.unpack_from('<I', blob, start+8)[0]
    base = start + 12 + size
    if size > MAX_INDEX or size == 0 or base > limit:
        raise ValueError('Invalid or truncated pack index')
    def unique_object(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError('Duplicate pack resource: ' + key)
            result[key] = value
        return result
    index = json.loads(blob[start+12:base], object_pairs_hook=unique_object)
    if not isinstance(index, dict):
        raise ValueError('Pack index must be an object')
    spans = []
    for name, pair in index.items():
        if not isinstance(name, str) or not isinstance(pair, list) or len(pair) != 2:
            raise ValueError('Invalid pack entry')
        offset, length = pair
        if type(offset) is not int or type(length) is not int or offset < 0 or length <= 0:
            raise ValueError('Invalid pack range: ' + name)
        if base + offset + length > limit:
            raise ValueError('Truncated resource: ' + name)
        spans.append((offset, offset+length, name))
    spans.sort()
    for before, after in zip(spans, spans[1:]):
        if before[1] > after[0]:
            raise ValueError('Overlapping resources: ' + before[2] + ', ' + after[2])
    return index, base


def resource(blob, index, base, name):
    if name not in index:
        raise ValueError('Original pack is missing ' + name)
    offset, length = index[name]
    return blob[base+offset:base+offset+length]


def validate_config(raw, available):
    config = json.loads(raw)
    if not isinstance(config, dict):
        raise ValueError('Invalid editor config')
    for name in ('demo', 'demo20', 'groups', 'catalog', 'removed'):
        if name not in config:
            raise ValueError('Original editor config is missing ' + name)
    if not isinstance(config['demo'], dict) or not isinstance(config['demo20'], dict):
        raise ValueError('Invalid demo maps')
    for name in ('groups', 'catalog', 'removed'):
        if not isinstance(config[name], list):
            raise ValueError('Invalid editor config collection: ' + name)
    missing = set()
    def visit(value):
        if isinstance(value, dict):
            for child in value.values():
                visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)
        elif isinstance(value, str) and value.startswith(('images/', 'ui/', 'user-demo/')):
            if value not in available:
                missing.add(value)
    visit(config)
    if missing:
        raise ValueError('Config references missing graphics: ' + ', '.join(sorted(missing)[:10]))
    return config


def extract_supplement(source, output):
    if source.suffix.lower() == '.zip':
        with zipfile.ZipFile(source) as archive:
            names = [n for n in archive.namelist() if n.lower().endswith('.exe')]
            if len(names) != 1:
                raise ValueError('Expected exactly one editor EXE in the ZIP')
            if archive.getinfo(names[0]).file_size > 512 * 1024 * 1024:
                raise ValueError('Editor EXE is unexpectedly large')
            blob = archive.read(names[0])
    else:
        blob = source.read_bytes()
    index, base = inspect_pack(blob)
    files = {name: resource(blob, index, base, name) for name in ('config.json', 'font.ttf')}
    validate_config(files['config.json'], index)
    if files['font.ttf'][:4] not in (b'\x00\x01\x00\x00', b'OTTO', b'true', b'ttcf'):
        raise ValueError('Invalid original font header')
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED) as archive:
        for name, data in files.items():
            archive.writestr(name, data)
    print(json.dumps({'output': str(output), 'bytes': output.stat().st_size,
                      'resources': {name: {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
                                    for name, data in files.items()}}, indent=2))


def build_pack(graphics, config, font, output):
    files = {p.relative_to(graphics).as_posix(): p for p in graphics.rglob('*.png')}
    if not files:
        raise ValueError('No graphics supplied')
    if not config.is_file() or not font.is_file():
        raise ValueError('Original config.json and font.ttf are required; image ZIPs alone are insufficient')
    validate_config(config.read_bytes(), files)
    files.update({'config.json': config, 'font.ttf': font})
    if font.read_bytes()[:4] not in (b'\x00\x01\x00\x00', b'OTTO', b'true', b'ttcf'):
        raise ValueError('Invalid font header')
    index = {}
    offset = 0
    for name, path in sorted(files.items()):
        length = path.stat().st_size
        index[name] = [offset, length]
        offset += length
    raw = json.dumps(index, ensure_ascii=False, separators=(',', ':')).encode()
    if len(raw) > MAX_INDEX:
        raise ValueError('Pack index exceeds editor limit')
    output.parent.mkdir(parents=True, exist_ok=True)
    temp = output.with_suffix(output.suffix + '.tmp')
    try:
        with temp.open('wb') as stream:
            stream.write(MAGIC + struct.pack('<I', len(raw)) + raw)
            for name in index:
                with files[name].open('rb') as source:
                    for chunk in iter(lambda: source.read(1024*1024), b''):
                        stream.write(chunk)
        inspect_pack(temp.read_bytes())
        temp.replace(output)
    except BaseException:
        temp.unlink(missing_ok=True)
        raise
    print(json.dumps({'output': str(output), 'resources': len(index), 'bytes': output.stat().st_size}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    extract = commands.add_parser('extract-supplement', help='Extract config.json and font.ttf from the original ZIP/EXE/PAK')
    extract.add_argument('source', type=Path)
    extract.add_argument('output', type=Path)
    build = commands.add_parser('build', help='Rebuild the original resource pack from PNGs, configuration and font')
    for name in ('graphics', 'config', 'font', 'output'):
        build.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.command == 'extract-supplement':
            extract_supplement(args.source, args.output)
        else:
            build_pack(args.graphics, args.config, args.font, args.output)
    except (OSError, ValueError, KeyError, struct.error, zipfile.BadZipFile) as exc:
        parser.exit(1, 'UAA data: '+str(exc)+'\n')

if __name__ == '__main__':
    main()
