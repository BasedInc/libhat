import itertools
import sys

import numpy as np
import pefile


def main():
    files = [pefile.PE(path, fast_load=True) for path in sys.argv[1:]]

    pair_counts = np.zeros(1 << 16, dtype=np.int64)
    total_pairs_count = 0

    for pe in files:
        for section in pe.sections:
            if not section.IMAGE_SCN_MEM_EXECUTE:
                continue
            data = section.get_data()
            total_pairs_count += len(data) - 1

            a = np.frombuffer(data[:-1], dtype=np.uint8).astype(np.uint16)
            b = np.frombuffer(data[1:], dtype=np.uint8)
            pairs = (a << 8) | b
            count = np.bincount(pairs, minlength=1 << 16)
            pair_counts += count

    top_n_pairs = 512
    sorted_pairs = sorted(
        enumerate(sorted(range(len(pair_counts)), key=lambda x: pair_counts[x], reverse=True)[:top_n_pairs]),
        key=lambda x: x[1])

    top_pairs_count = 0
    for batch in itertools.batched(sorted_pairs, 8):
        for _, packed in batch:
            a, b = divmod(packed, 0x100)
            top_pairs_count += pair_counts[packed]
            print(f'p(0x{a:02X}, 0x{b:02X}), ', end='')
        print()

    print()
    for batch in itertools.batched(sorted_pairs, 16):
        for score, _ in batch:
            print(f'0x{score:03X}, ', end='')
        print()

    print(f'% of all: {top_pairs_count / total_pairs_count:0.4%}')


if __name__ == '__main__':
    main()
