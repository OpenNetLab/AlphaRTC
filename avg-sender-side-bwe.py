from statistics import mean, median
import argparse

def main(args):
    filename = args.filename
    print(filename)
    with open(filename, 'r') as f:
        lines = [float(line.rstrip()) for line in f]

    print(f'BWE: average {mean(lines)/1000000} Mbps, median {median(lines)/1000000} Mbps')

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--filename", help="sender side bwe extracted from webrtc log")
    args = parser.parse_args()
    main(args)
