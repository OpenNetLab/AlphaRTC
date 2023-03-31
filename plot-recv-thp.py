from statistics import mean, median
import os
import matplotlib.pyplot as plt
import numpy as np

LINK_BW='12mbps'

def parse_log():
    recv_thp_l = []
    with open(f'webrtc-sender-{LINK_BW}', 'r') as f:
        for line in f:
            line = line.rstrip()
            if 'Sending avg receiver side thp' in line:
                l = line.split()
                recv_thp_l.append(l[8])

    return recv_thp_l


def main():
    recv_thp_l_bps = parse_log()
    recv_thp_l = []
    for line in recv_thp_l_bps:
        recv_thp_l.append(float(line.rstrip()))

    os.remove('recv-thp-file')
    for line in recv_thp_l:
        with open('recv-thp-file', 'a') as f:
            f.write(str(line)+'\n')

    a = np.array(recv_thp_l)
    average = "{:.2f}".format(mean(recv_thp_l))
    med = "{:.2f}".format(median(recv_thp_l))
    p90 = "{:.2f}".format(np.percentile(a, 90)) # 90th percentile
    p99 = "{:.2f}".format(np.percentile(a, 99)) # 99th percentile

    title = f'Receiver-side thp: avg {average} med {med} p90 {p90} p99 {p99} bps'

    plt.ylim(0, 300000)
    plt.plot(recv_thp_l)
    plt.title(title)
    plt.xlabel('Time')
    plt.ylabel('Receiver-side throughput (bps)')
    plt.savefig(f'recv-thp-{LINK_BW}.pdf')


if __name__ == "__main__":
    main()