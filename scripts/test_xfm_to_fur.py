"""Regression checks for the standalone Furnace converter."""
import pathlib
import struct
import unittest
import os

import xfm_to_fur as fur

SONGS = pathlib.Path(os.environ.get(
    'XFM_SONGS_DIR',
    pathlib.Path(__file__).resolve().parents[1] / 'sounds' / 'builtin_songs'))


def read_blocks(data):
    assert data[:16] == b'-Furnace module-'
    pos = 32
    while pos < len(data):
        code = data[pos:pos + 4]
        size = struct.unpack_from('<I', data, pos + 4)[0]
        yield code, data[pos + 8:pos + 8 + size]
        pos += 8 + size
    assert pos == len(data)


class ConverterTests(unittest.TestCase):
    def test_exported_pitch_matches_tracker_label(self):
        self.assertEqual(fur.parse_cell('C-4....', 0, 0).note, 108)  # Furnace C-4
        self.assertEqual(fur.parse_cell('C#4....', 0, 0).note, 109)
        self.assertEqual(fur.parse_cell('A-3....', 0, 0).note, 105)

    def test_builtin_songs_have_parts_and_valid_blocks(self):
        songs = sorted(SONGS.glob('*.h'))
        self.assertGreaterEqual(len(songs), 7)
        for path in songs:
            with self.subTest(song=path.name):
                song = fur.parse_song(path)
                parts = fur.split_parts(song)
                data = fur.convert(song)
                self.assertEqual(data, fur.convert(song))
                codes = [code for code, _ in read_blocks(data)]
                self.assertEqual(codes.count(b'INF2'), 1)
                self.assertEqual(codes.count(b'SNG2'), 1)
                self.assertEqual(codes.count(b'INS2'), len(song.instruments))
                self.assertEqual(codes.count(b'PATN'), len(parts) * 6)
                self.assertLessEqual(sum(len(p.rows) for p in parts), len(song.rows))
                self.assertLessEqual(max(len(p.rows) for p in parts), 256)
                self.assertTrue(all(not p.name.startswith('SKIP ') for p in parts))

    def test_detune_mapping_matches_both_chip_tables(self):
        furnace_hardware = (7, 6, 5, 0, 1, 2, 3, 4)
        for signed, furnace_index in fur.DT_TO_FUR.items():
            xfm_hardware = 8 + signed if signed < 0 else signed
            self.assertEqual(furnace_hardware[furnace_index], xfm_hardware)

    def test_parts_skip_and_pattern_break(self):
        song = fur.parse_song(SONGS / 'drizzle_lane.h')
        parts = fur.split_parts(song)
        self.assertEqual(len(parts), 16)
        self.assertEqual(max(len(p.rows) for p in parts), 64)
        self.assertEqual(len(fur.convert(song)) > 1000, True)

    def test_macro_loop_and_release_bytes(self):
        song = fur.parse_song(SONGS / 'pin_crusher.h')
        macro = next(m for i in song.instruments for m in i.macros if m.target == 1)
        encoded = fur.encode_macro(macro, 6)
        self.assertEqual(encoded[:4], bytes((6, 32, 0, 21)))
        self.assertEqual(encoded[8:], bytes(macro.values))


if __name__ == '__main__':
    unittest.main()
