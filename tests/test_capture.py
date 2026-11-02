import csv
import ctypes as ct
from capture import consume
from protocol import Parser, pack


def test_saved_binary_replays_real_c_diagnostics(native,tmp_path):
    output=(ct.c_ubyte*96)()
    size=native.test_diagnostic(output,96)
    frame=bytes(output[:size])
    source=b"startup noise"+frame+frame
    chunks=[source[i:i+3] for i in range(0,len(source),3)]
    csv_path=tmp_path/"capture.csv"
    summary=consume(chunks,csv_path)
    with csv_path.open(newline="") as stream:
        rows=list(csv.DictReader(stream))
    assert len(rows)==2 and int(rows[0]["sequence"])==0x12345678
    assert summary["duplicate_samples"]==1
    assert summary["bad_checksums"]==0


def test_parser_recovers_after_truncated_frame():
    good=pack(3,bytes(7))
    # A plausible header truncated in transit must not poison later frames.
    broken=b"\xaa\xff\xf1\x3d"+bytes(12)
    parser=Parser()
    frames=parser.feed(broken+good*8)
    assert len(frames)>=4
    assert all(frame==(3,bytes(7)) for frame in frames)
