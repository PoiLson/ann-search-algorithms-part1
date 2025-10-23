#!/usr/bin/env python3
import sys
import struct


def subset_fvecs(in_path: str, out_path: str, n: int) -> int:
    written = 0
    with open(in_path, 'rb') as fin, open(out_path, 'wb') as fout:
        while written < n:
            d_bytes = fin.read(4)
            if len(d_bytes) < 4:
                break  # EOF
            (d,) = struct.unpack('<i', d_bytes)
            vec_bytes = fin.read(4 * d)
            if len(vec_bytes) < 4 * d:
                break  # Truncated
            fout.write(d_bytes)
            fout.write(vec_bytes)
            written += 1
    return written


def main():
    if len(sys.argv) < 4:
        print("Usage: subset_fvecs.py <input.fvecs> <output.fvecs> <count>")
        sys.exit(1)
    in_path = sys.argv[1]
    out_path = sys.argv[2]
    try:
        count = int(sys.argv[3])
    except ValueError:
        print("count must be an integer")
        sys.exit(1)

    written = subset_fvecs(in_path, out_path, count)
    print(f"Wrote {written} vectors to {out_path}")


if __name__ == "__main__":
    main()
