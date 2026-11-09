import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('mapeditor_data', Path(__file__).parents[1]/'tools/mapeditor_data.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

class PackTests(unittest.TestCase):
    def test_roundtrip_and_extraction(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root/'images').mkdir()
            (root/'images'/'one.png').write_bytes(b'PNG fixture')
            config = {'demo': {}, 'demo20': {}, 'groups': [], 'catalog': [], 'removed': [], 'logo': 'images/one.png'}
            (root/'config.json').write_text(json.dumps(config))
            (root/'font.ttf').write_bytes(b'\x00\x01\x00\x00'+b'font fixture')
            pack = root/'result.pak'
            m.build_pack(root, root/'config.json', root/'font.ttf', pack)
            blob = pack.read_bytes()
            index, base = m.inspect_pack(blob)
            self.assertEqual(set(index), {'images/one.png','config.json','font.ttf'})
            self.assertEqual(m.resource(blob,index,base,'config.json'), (root/'config.json').read_bytes())
            executable = root/'editor.exe'
            prefix = b'MZ fixture'
            executable.write_bytes(prefix+blob+m.FOOTER+struct.pack('<Q',len(prefix)))
            output = root/'supplement.zip'
            m.extract_supplement(executable,output)
            with m.zipfile.ZipFile(output) as archive:
                self.assertEqual(set(archive.namelist()), {'config.json','font.ttf'})
                self.assertEqual(archive.read('config.json'), (root/'config.json').read_bytes())
    def test_rejects_truncation(self):
        index = json.dumps({'font.ttf':[0,100]}).encode()
        with self.assertRaisesRegex(ValueError,'Truncated'):
            m.inspect_pack(m.MAGIC+struct.pack('<I',len(index))+index+b'x')
    def test_rejects_duplicate_entry(self):
        raw = b'{"x":[0,1],"x":[1,1]}'
        with self.assertRaisesRegex(ValueError,'Duplicate'):
            m.inspect_pack(m.MAGIC+struct.pack('<I',len(raw))+raw+b'xx')
    def test_rejects_overlap(self):
        raw=json.dumps({'a':[0,5],'b':[4,3]}).encode()
        with self.assertRaisesRegex(ValueError,'Overlapping'):
            m.inspect_pack(m.MAGIC+struct.pack('<I',len(raw))+raw+b'1234567')
    def test_rejects_missing_graphics(self):
        raw=json.dumps({'demo':{},'demo20':{},'groups':[],'catalog':[],'removed':[],'logo':'ui/missing.png'}).encode()
        with self.assertRaisesRegex(ValueError,'missing graphics'):
            m.validate_config(raw,set())

if __name__ == '__main__':unittest.main()
