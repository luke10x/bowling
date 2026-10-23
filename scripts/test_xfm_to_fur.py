"""End-to-end checks for the C++ Furnace converter and its Python wrapper."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
WRAPPER = ROOT / 'scripts/xfm_to_fur.py'
SONGS = ROOT / 'sounds/builtin_songs'
EMPTY = '.......'
OP = 'OP {n} 0 1 0 0 31 0 0 0 0 15 0'


def rows_in_first_pattern(data):
    assert data[:16] == b'-Furnace module-'
    pos = 32
    while pos < len(data):
        tag = data[pos:pos + 4]
        size = struct.unpack_from('<I', data, pos + 4)[0]
        body = data[pos + 8:pos + 8 + size]
        pos += 8 + size
        if tag != b'PATN' or body[1] != 0 or struct.unpack_from('<H', body, 2)[0] != 0:
            continue
        cursor = body.index(0, 4) + 1
        rows = []
        while cursor < len(body):
            mask = body[cursor]
            cursor += 1
            if mask == 255:
                break
            if mask & 128:
                rows.extend([None] * ((mask & 127) + 2))
                continue
            extra = body[cursor] if mask & 32 else 0
            cursor += bool(mask & 32)
            fields = []
            for bit in (1, 2, 4, 8, 16):
                fields.append(body[cursor] if mask & bit else None)
                cursor += bool(mask & bit)
            for bit in range(2, 8):
                cursor += bool(extra & (1 << bit))
            rows.append(fields)
        return rows
    raise AssertionError('missing channel 0, order 0 PATN')


def song_header(lines):
    pattern = '\n'.join('|'.join([first] + [EMPTY] * 5) for first in lines)
    operators = '\n'.join(OP.format(n=n) for n in range(1, 5))
    return f'''XFM_SONG_BEGIN(R"xfmname(Test)xfmname")
XFM_TICK_RATE(60)
XFM_SPEED(4)
XFM_ROWS_PER_BEAT(4)
XFM_TUNING_MODE(0)
XFM_LFO_ENABLED(0)
XFM_LFO_FREQUENCY(0)
XFM_PATTERN(R"xfmpattern(
{len(lines)}
{pattern}
)xfmpattern")
XFM_INSTRUMENTS(R"xfminstruments(
INST 01
PATCH 0 0 0 0
{operators}
ENDINST
)xfminstruments")
'''


class ConverterTests(unittest.TestCase):
    def convert(self, header, player_match=False, master_volume=None):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            source, target = path / 'test.h', path / 'test.fur'
            source.write_text(header)
            command = [sys.executable, str(WRAPPER)]
            if player_match:
                command.append('--player-match')
            if master_volume is not None:
                command.extend(('--master-volume', master_volume))
            command.extend((str(source), str(target)))
            subprocess.run(command,
                           cwd=ROOT, check=True, capture_output=True)
            return target.read_bytes()

    def test_inherited_and_live_row_volumes_use_cpp_playback(self):
        data = self.convert(song_header([
            'C-40120', EMPTY, '.....10', 'D-401..',
            '.......0A10', 'E-401..', 'OFF....', 'F-401..',
        ]))
        rows = rows_in_first_pattern(data)
        self.assertEqual([rows[i][2] for i in (0, 2, 3, 5, 7)],
                         [0x20, 0x10, 0x10, 0x11, 0x13])
        self.assertTrue(all(value is None for value in rows[1]))
        self.assertEqual(rows[4][3:], [0x0A, 0x10])
        self.assertIsNone(rows[4][2])
        self.assertIsNone(rows[6][2])

    def test_skipped_parts_do_not_change_inherited_volume(self):
        header = song_header(['C-40120', '.....10', '.....00', 'D-401..'])
        header = header.replace('C-40120|', 'PART First\nC-40120|')
        header = header.replace('.....00|', 'SKIP Unused\n.....00|')
        header = header.replace('D-401..|', 'PART Last\nD-401..|')
        data = self.convert(header)
        # The next order's first note must retain 0x10, not the skipped 0x00.
        self.assertEqual(rows_in_first_pattern(data)[0][2], 0x20)
        pos = 32
        volumes = []
        while pos < len(data):
            tag = data[pos:pos + 4]
            size = struct.unpack_from('<I', data, pos + 4)[0]
            body = data[pos + 8:pos + 8 + size]
            if tag == b'PATN' and body[1] == 0 and struct.unpack_from('<H', body, 2)[0] == 1:
                cursor = body.index(0, 4) + 1
                mask = body[cursor]
                self.assertEqual(mask & 7, 7)
                volumes.append(body[cursor + 3])
            pos += 8 + size
        self.assertEqual(volumes, [0x10])

    def test_all_builtin_songs_convert(self):
        with tempfile.TemporaryDirectory() as directory:
            subprocess.run([sys.executable, str(WRAPPER), '--all', str(SONGS), directory],
                           cwd=ROOT, check=True, capture_output=True)
            outputs = list(Path(directory).glob('*.fur'))
            self.assertEqual(len(outputs), 7)
            self.assertTrue(all(path.stat().st_size > 1000 for path in outputs))

    def test_operator_tl_and_macros_follow_furnace_slot_order(self):
        header = song_header(['C-4017F'])
        for number, tl in enumerate((11, 22, 33, 44), 1):
            header = header.replace(OP.format(n=number),
                                    f'OP {number} 0 1 {tl} 0 31 0 0 0 0 15 0')
        header = header.replace('ENDINST',
                                'MACRO 2 1 0 255 21\nMACRO 3 1 0 255 31\nENDINST')
        data = self.convert(header)
        pos = 32
        while data[pos:pos + 4] != b'INS2':
            pos += 8 + struct.unpack_from('<I', data, pos + 4)[0]
        size = struct.unpack_from('<I', data, pos + 4)[0]
        body = data[pos + 8:pos + 8 + size]
        cursor = 4  # instrument version, type, and flags
        features = {}
        while body[cursor:cursor + 2] != b'EN':
            tag = body[cursor:cursor + 2]
            length = struct.unpack_from('<H', body, cursor + 2)[0]
            features[tag] = body[cursor + 4:cursor + 4 + length]
            cursor += 4 + length
        fm = features[b'FM']
        self.assertEqual([fm[5 + slot * 8 + 1] for slot in range(4)],
                         [11, 33, 22, 44])
        self.assertEqual(features[b'O2'][-2], 31)
        self.assertEqual(features[b'O3'][-2], 21)

    def test_ym3438_at_each_master_volume(self):
        header = song_header(['C-40140', 'D-40120'])
        standard = self.convert(header)
        half = self.convert(header, master_volume='0.5')
        quarter = self.convert(header, master_volume='0.25')
        matched = self.convert(header, player_match=True)
        self.assertEqual(quarter, matched)

        def blocks(data):
            pos = 32
            result = []
            while pos < len(data):
                size = struct.unpack_from('<I', data, pos + 4)[0]
                result.append((data[pos:pos + 4], data[pos + 8:pos + 8 + size]))
                pos += 8 + size
            return result

        standard_blocks = blocks(standard)
        half_blocks = blocks(half)
        quarter_blocks = blocks(quarter)
        self.assertEqual([code for code, _ in standard_blocks],
                         [b'INF2', b'SNG2', b'FLAG', b'INS2'] + [b'PATN'] * 6)
        for variant in (standard_blocks, half_blocks, quarter_blocks):
            self.assertEqual(next(body for code, body in variant if code == b'FLAG'),
                             b'chipType=0\n\0')
            self.assertEqual([body for code, body in standard_blocks if code != b'INF2'],
                             [body for code, body in variant if code != b'INF2'])

        def master_volume(inf):
            cursor = 0
            for _ in range(8):
                cursor = inf.index(0, cursor) + 1
            return struct.unpack_from('<f', inf, cursor + 5)[0]

        self.assertEqual(master_volume(standard_blocks[0][1]), 1.0)
        self.assertEqual(master_volume(half_blocks[0][1]), 0.5)
        self.assertEqual(master_volume(quarter_blocks[0][1]), 0.25)

    def test_all_levels_names_each_ym3438_master_value(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, output = root / 'songs', root / 'exports'
            source.mkdir()
            (source / 'test.h').write_text(song_header(['C-4017F']))
            subprocess.run([sys.executable, str(WRAPPER), '--all', '--all-levels',
                            str(source), str(output)], cwd=ROOT, check=True,
                           capture_output=True)
            self.assertEqual({path.name for path in output.glob('*.fur')}, {
                'test-ym3438-master-1_0.fur',
                'test-ym3438-master-0_5.fur',
                'test-ym3438-master-0_25.fur',
            })


if __name__ == '__main__':
    unittest.main()
