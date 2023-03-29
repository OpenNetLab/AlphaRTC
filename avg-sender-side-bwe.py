from statistics import mean, median
import argparse
import numpy as np

def main(args):
    filename = args.filename
    print(filename)
    with open(filename, 'r') as f:
        lines = [float(line.rstrip()) for line in f]

    a = np.array(lines)
    p50 = np.percentile(a, 50) / 1000000 # return 50th percentile, e.g median.
    p90 = np.percentile(a, 90) / 1000000 # return 50th percentile, e.g median.
    p99 = np.percentile(a, 99) / 1000000 # return 50th percentile, e.g median.

    average = mean(lines)/1000000
    med = median(lines)/1000000

    print(f'BWE: average {average} median {med} p90 {p90} p99 {p99} Mbps')

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--filename", help="sender side bwe extracted from webrtc log")
    args = parser.parse_args()
    main(args)
