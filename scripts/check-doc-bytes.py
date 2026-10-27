#!/usr/bin/env python3
"""Check packed/UTF-8 CP437 DolDoc resources against the native decoder."""
import ctypes
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

with tempfile.TemporaryDirectory(prefix='zeal-doc-bytes-') as name:
    temp = Path(name)
    source = temp/'probe.c'
    source.write_text('#include "doc_bytes.h"\n'
        'int probe(const unsigned char *p,size_t n,unsigned char **out,size_t *size)'
        '{return doc_bytes_normalize(p,n,out,size);}\n'
        'void release(void *p){free(p);}\n')
    library = temp/'probe.so'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra',
                    '-Werror', '-shared', '-fPIC', '-I'+str(ROOT/'src/zc'),
                    str(source), '-o', str(library)], check=True)
    native = ctypes.CDLL(str(library))
    native.probe.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                            ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_size_t)]
    native.probe.restype = ctypes.c_int
    native.release.argtypes = [ctypes.c_void_p]

    def normalize(data):
        original = ctypes.create_string_buffer(data)
        owned, size = ctypes.c_void_p(), ctypes.c_size_t()
        status = native.probe(original, len(data), ctypes.byref(owned), ctypes.byref(size))
        result = ctypes.string_at(owned, size.value) if owned.value else data
        if owned.value:
            native.release(owned)
        return status, result

    def record(data, number=1):
        return struct.pack('<4I', number, 0, len(data), 2)+data

    assert normalize(b'I64 F(){return 42;}\n') == (0, b'I64 F(){return 42;}\n')
    rng = random.Random(42)
    for length in [0, 1, 3, 256, 1024, *range(10, 100)]:
        payload = bytes(range(256)) if length == 256 else rng.randbytes(length)
        tail = record(payload)+record(b'\0\xff\x80', 2)
        plain = b'// UTF-8 text remains untouched: \xe2\x88\x9e\n\0'
        original = plain+tail
        assert normalize(original) == (0, original)
        encoded = plain+tail.decode('cp437').encode('utf-8')
        status, decoded = normalize(encoded)
        assert decoded == original
        assert status == (0 if encoded == original else 1)
    for malformed in [b'\0'+b'\0'*15, b'\0'+struct.pack('<4I', 1, 0, 100, 1),
                      b'\0\xf0\x9f\x98\x80', b'\0\xc0\x80', b'\0\xed\xa0\x80']:
        assert normalize(malformed)[0] == -1

    converted = 0
    for tree in ('Apps', 'Demo'):
        for path in sorted((ROOT/'upstream/pinned'/tree).rglob('*')):
            if not path.is_file() or path.suffix not in ('.ZC', '.DD', '.HH'):
                continue
            data = path.read_bytes()
            if b'\0' not in data:
                continue
            text, tail = data.split(b'\0', 1)
            status, decoded = normalize(data)
            if status == 0:
                continue
            expected = text+b'\0'+tail.decode('utf-8').encode('cp437')
            assert status == 1 and decoded == expected, path
            converted += 1
    assert converted == 54, converted
    print('PASS native binary passthrough, complete CP437 byte range, malformed tails, '
          'and exact decoding of all 54 pinned Apps/Demo documents')
