import itertools
import sys

import pefile


def main():
    files = [pefile.PE(path, fast_load=True) for path in sys.argv[1:]]

    pair_counts = [0 for _ in range(2 ** 16)]
    total_pairs_count = 0

    for pe in files:
        for section in pe.sections:
            if not section.IMAGE_SCN_MEM_EXECUTE:
                continue
            data = section.get_data()
            total_pairs_count += len(data) - 1
            for a, b in zip(data[:], data[1:]):
                pair_counts[a * 0x100 + b] += 1

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
