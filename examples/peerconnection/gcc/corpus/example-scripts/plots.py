#!/usr/bin/env python3
import matplotlib.pyplot as plt
import sys
import argparse
import numpy as np

def throughput_timeseries(dequeues, enqueues, drops, mm_timestamp, min_ts, dt=1000):
    dq_times = [t for t, _, _ in dequeues]
    dq_bytes = [b for _,b,_ in dequeues]
    dq_queue_delay = [qd for _,_,qd in dequeues]
    dq_times_ms = [(t-min_ts)*(1000/dt) for t in dq_times]
    drop_times_ms = [(t-min_ts)*(1000/dt) for t in drops]
    enq_time_ms = [(t-min_ts)*(1000/dt) for t in enqueues]
    # throughput
    throughputs = [0 for _ in range(0, 1+int(max(dq_times_ms)))]
    for i in range(len(dq_times)):
        time_ms = int(dq_times_ms[i])
        pkt_len = dq_bytes[i]
        throughputs[int(time_ms)] += pkt_len
    throughputs = [(t/((1024*1024)/8))*(1000/dt) for t in throughputs]
    times = [t*(dt/1000) for t in range(len(throughputs))]
    throughput = (times, throughputs)
    # average queuing delay
    delays = [[] for _ in range(0, 1+int(max(dq_times_ms)))]
    for i in range(len(dq_times)):
        time_ms = int(dq_times_ms[i])
        qdelay = dq_queue_delay[i]
        delays[time_ms].append(qdelay)
    delays = [np.mean(d) if d else 0 for d in delays]
    delay = (times, delays)
    # packet drop rate
    packets_eq = [0 for _ in range(0, 1+int(max(enq_time_ms)))]
    packets_dq = [0 for _ in range(0, 1+int(max(enq_time_ms)))]
    packets_dropped = [0 for _ in range(0, 1+int(max(enq_time_ms)))]
    for i in range(len(enq_time_ms)):
        time_ms = int(enq_time_ms[i])
        packets_eq[time_ms] += 1
    for i in range(len(dq_times_ms)):
        time_ms = int(dq_times_ms[i])
        packets_dq[time_ms] += 1
    for i in range(len(drop_times_ms)):
        time_ms = int(drop_times_ms[i])
        packets_dropped[time_ms] += 1
    drop_rate = []
    for i in range(len(packets_eq)):
        if packets_eq[i]>0:
            drop_rate.append(100*packets_dropped[i] / packets_eq[i])
        else:
            drop_rate.append(0)
    drop = (times, drop_rate)
    return throughput, delay, drop



def plot_twin(xs1, ys1, xs2, ys2, labels1, labels2, xlabel, ylabel1, ylabel2, save, ylim1=-1, ylim2=-1, trange=-1):
    fig, ax1 = plt.subplots(figsize=(8, 6))

    # Plot data for the first y-axis (left)
    lines = ["-", "-.", "o-"]
    colors = ["b", "g", "k"]
    for i in range(len(ys1)):
        ax1.plot(xs1[i], ys1[i], label=labels1[i], marker='o', linestyle='-', color='b')
    ax1.set_xlabel(xlabel)
    ax1.set_ylabel(ylabel1, color='tab:blue')
    ax1.tick_params(axis='y', labelcolor='tab:blue')
    if ylim1!=-1:
        ax1.set_ylim(ylim1)

    # Create second y-axis (right)
    ax2 = ax1.twinx()
    colors = ["r", "cyan", "gray"]
    markers = ["s", None]
    
    # Plot data for the second y-axis (right)
    for i in range(len(ys2)):
        ax2.plot(xs2[i], ys2[i], label=labels2[i], marker='s', linestyle=lines[i], color=colors[i])
    ax2.set_ylabel(ylabel2, color='tab:red')
    ax2.tick_params(axis='y', labelcolor='tab:red')
    if ylim2!=-1:
        ax2.set_ylim(ylim2)
    
    if trange!=-1:
        ax1.set_xlim(trange)
        ax2.set_xlim(trange)

    # Add legend
    lines1, labels1 = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    ax1.legend(lines1 + lines2, labels1 + labels2, loc='upper right')

    plt.grid(True)
    plt.tight_layout()
    plt.savefig(save)  # Save the plot

def plot(xs, ys, labels, xlabel, ylabel, save, ylim=-1):
    plt.figure(figsize=(8, 6))
    for i in range(len(ys)):
        plt.plot(xs[i], ys[i], label=labels[i], marker='o', linestyle='-')
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    if ylim!=-1:
        plt.ylim(ylim)
    plt.grid(True)
    plt.legend()
    plt.savefig(save)  # Save the plot

def time_series_analysis(args, mm_timestamp, delays, frame_writes, mm_dequeues, mm_enqueues, mm_drops, frame_drops, frame_drops_rate_limiter, frame_sizes):
    dt_ = 100
    timestamps_delays = [t for t,_ in delays]
    min_t = min(min(timestamps_delays), min(frame_writes), mm_timestamp)
    print(mm_timestamp, min(timestamps_delays), min(frame_writes), min_t)
    avg_throughput, avg_delay, drop_rate = throughput_timeseries(mm_dequeues, mm_enqueues, mm_drops, mm_timestamp, min_t, dt=dt_)
    timestamps_secs = [t - min_t for t in frame_writes]
    frame_drops_ = [t - min_t for t in frame_drops]
    frame_drops_rate_limiter_ = [t - min_t for t in frame_drops_rate_limiter]
    delays = [(t-min_t,d) for t,d in delays]
    # frame rate variations
    frame_rate = [0 for _ in range(0, 1+int(max(timestamps_secs)))]
    frame_drops_secs = [0 for _ in range(0, 1+int(max(timestamps_secs)))]
    frame_drops_rate_limiter_secs = [0 for _ in range(0, 1+int(max(timestamps_secs)))]
    frame_rate = [0 for _ in range(0, 1+int(max(timestamps_secs)))]
    frame_bitrate = [0 for t in avg_throughput[0]]
    for t in timestamps_secs:
        # print(t, 1000000*(t+min_t))
        frame_rate[int(t)] += 1
    for t in frame_drops_:
        frame_drops_secs[int(t)] += 1
    for t in frame_drops_rate_limiter_:
        frame_drops_rate_limiter_secs[int(t)] += 1
    for t, byte in frame_sizes:
        frame_bitrate[int((t-min_t) * (1000 / dt_))] += (1000/dt_) * (byte * 8) / (10**6)

    
    plot([range(1, len(frame_rate) + 1), range(1, len(frame_rate) + 1), range(1, len(frame_rate) + 1)], [frame_rate, frame_drops_secs, frame_drops_rate_limiter_secs], ["frame rate", "frame drops", "frame drops (rate limiter)"], 'Time (s)', 'Frame Rate', '%s/frame_rate_plot.png' %(args.d))
    plot_twin([range(1, len(frame_rate) + 1)], [frame_rate], [avg_throughput[0], avg_throughput[0]], [frame_bitrate, avg_throughput[1]], ["frame rate"], ["video bitrate", "throughput"], 'Time (s)', 'Frame Rate', 'Throughput (Mbps)', '%s/frame_rate_tpt_plot.png' %(args.d), ylim1=(0,65), ylim2=(0,50), trange=-1)
    plot_twin([range(1, len(frame_rate) + 1)], [frame_rate], [drop_rate[0]], [drop_rate[1]], ["frame rate"], ['Droprate (%)'], 'Time (s)', 'Frame Rate', 'Droprate (%)', '%s/frame_rate_drops_plot.png' %(args.d), ylim1=(0,65), ylim2=(0,100))
    # per-second delay
    delays_by_second = [[] for _ in range(0, 1+int(max([t for t,_ in delays])))]
    for i in range(len(delays)):
        ts, delay = delays[i][0], delays[i][1]
        delays_by_second[int(ts)].append(delay)
    avg_delay_per_second = [np.mean(d) if d else 0 for d in delays_by_second]

    plot([range(1, len(avg_delay_per_second) + 1), avg_delay[0]], [avg_delay_per_second, avg_delay[1]], ["Overall Delay", "Queuing Delay"], 'Time (s)', 'Average Delay (ms)', '%s/avg_delay_plot.png' %(args.d), ylim=(0,400))
    

def main(args):
    delays = []
    frame_write = []
    lines = []
    dequeues = []
    enqueues = []
    drops = []
    base_mm_timestamp = -1
    frame_drops = []
    frame_sizes = []
    frame_drops_rate_limiter = []
    with open(args.d+"/receiver.log", 'r') as receiver_log:
        for line in receiver_log:
            if 'E2E FRAME DELAY' in line or 'FRAME WRITE:' in line:
                lines.append(line)
    with open(args.d+"/sender.log", 'r') as receiver_log:
        for line in receiver_log:
            if 'Frame Dropped' in line:
                frame_drops.append(int(line.split()[-1])/1000)
            if 'ENCODED FRAME AT:' in line:
                frame_sizes.append((int(line.split(": ")[2].split(" ")[0])/1000, int(line.split()[-1])))
            if 'Frame Dropped by rate limiter' in line:
                frame_drops_rate_limiter.append(int(line.split()[-1])/1000)
    with open(args.d+"/mahimahi.log", 'r') as mm_log:
        for line in mm_log:
            if ' - ' in line:
                dequeues.append(((int(line.split()[0]) + base_mm_timestamp )/ 1000, int(line.split()[2]), int(line.split()[3])))
            if ' + ' in line:
                enqueues.append((int(line.split()[0]) + base_mm_timestamp )/ 1000)
            if ' d ' in line:
                drops.append((int(line.split()[0]) + base_mm_timestamp) / 1000)
            if 'init timestamp:' in line:
                base_mm_timestamp = int(line.split()[-1])

    for i in range(len(lines)):
        line = lines[i]
        if 'E2E FRAME DELAY' in line:
            delays.append((int(line.split()[-5])/1000, int(line.split()[-1])))
        if 'FRAME WRITE' in line:
            frame_write.append(int(line.split()[-1])/1000)

    print('Delay measurement count:', len([d for _,d in delays]))
    print('Median per-frame delay (ms):', np.median([d for _,d in delays]))
    print('P95 per-frame delay (ms):', np.percentile([d for _,d in delays], 95))

    time_series_analysis(args, base_mm_timestamp/1000, delays, frame_write, dequeues, enqueues, drops, frame_drops, frame_drops_rate_limiter, frame_sizes)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', metavar='DIRECTORY',
                        required=True, help='save plot directory')
    args = parser.parse_args()

    main(args)
