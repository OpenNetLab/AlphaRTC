from statistics import mean, median
import os
import matplotlib.pyplot as plt
import numpy as np


def parse_log():
    bwe_l = []
    with open('webrtc-sender.log', 'r') as f:
        for line in f:
            line = line.rstrip()
            if 'Current BWE' in line:
                l = line.split()
                bwe_l.append(float(l[3])/1000000) # Mbps

    return bwe_l

def main():
    bwe_l = parse_log()

    os.remove('rl-bwe-file')
    for line in bwe_l:
        with open('rl-bwe-file', 'a') as f:
            f.write(str(line)+'\n')

    a = np.array(bwe_l)
    average = "{:.2f}".format(mean(bwe_l))
    med = "{:.2f}".format(median(bwe_l))
    p90 = "{:.2f}".format(np.percentile(a, 90)) # 90th percentile
    p99 = "{:.2f}".format(np.percentile(a, 99)) # 99th percentile

    title = f'RL-based CC BWE: avg {average} med {med} p90 {p90} p99 {p99} Mbps'

    x_axis = []
    len_bwe_l = len(bwe_l)
    for i in range(0, len_bwe_l):
        # bwe is logged at every 100ms
        x_axis.append(i * 100)
    y_axis = bwe_l

    plt.plot(x_axis, y_axis)
    plt.title(title)
    plt.xlabel('Time (ms)')
    plt.ylabel('Estimated Bandwidth (Mbps)')
    plt.savefig('rlcc-bwe.pdf')


if __name__ == "__main__":
    main()