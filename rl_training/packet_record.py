#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
from statistics import mean

# For bps
UNIT_M = 1000000
MAX_BANDWIDTH_MBPS = 10     # 10Mbps
MIN_BANDWIDTH_MBPS = 0.001  # 10Kbps
LOG_MAX_BANDWIDTH_MBPS = np.log(MAX_BANDWIDTH_MBPS)
LOG_MIN_BANDWIDTH_MBPS = np.log(MIN_BANDWIDTH_MBPS)

# For ms
UNIT_K = 1000
MAX_RTT_US = 1000000   # 1000ms
MIN_RTT_US = 1000      # 1ms
LOG_MAX_RTT_US = np.log(MAX_RTT_US)
LOG_MIN_RTT_US = np.log(MIN_RTT_US)

class PacketRecord:
    def __init__(self):
        # Calculate a state with an average of stats from the latest 10 RTCP packets
        self.history_len = 10
        self.reset()

    def reset(self):
        self.packet_stats_dict = {}
        self.packet_stats_dict['receiver_side_thp'] = []
        self.packet_stats_dict['rtt'] = []
        self.packet_stats_dict['loss_rate'] = []
        self.packet_stats_dict['bwe'] = []

    def normalize_bps(self, value):
        # from 10Kbps-10Mbps to 0-1
        clipped_value = np.clip(value / UNIT_M, MIN_BANDWIDTH_MBPS, MAX_BANDWIDTH_MBPS)
        log_value = np.log(clipped_value)
        # (x - min) / (max - min) where x, min, max is log values
        norm_value = (log_value - LOG_MIN_BANDWIDTH_MBPS) / (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS)
        # print(f'linear to log: receiver-side thp (10Kbps-10Mbps) orig {value} clipped {clipped_value} log-based {log_value}, normalized to {norm_value}')
        return norm_value

    def normalize_rtt(self, value):
        # from 1-1000ms to 0-1
        clipped_value = np.clip(value * UNIT_K, MIN_RTT_US, MAX_RTT_US)
        log_value = np.log(clipped_value)
        # (x - min) / (max - min) where x, min, max is log values
        norm_value = (log_value - LOG_MIN_RTT_US) / (LOG_MAX_RTT_US - LOG_MIN_RTT_US)
        return norm_value

    # Add normalized throughput (10Kbps-10Mbps to 0-1)
    def add_receiver_side_thp(self, receiver_side_thp):
        normalized_thp = self.normalize_bps(receiver_side_thp)
        # print(f'Receiver-side thp {receiver_side_thp} normalized to {normalized_thp}')
        self.packet_stats_dict['receiver_side_thp'].append(normalized_thp)

    # Add normalized rtt (1-1000ms to 0-1)
    def add_rtt(self, rtt):
        normalized_rtt = self.normalize_rtt(rtt)
        # print(f'RTT {rtt} normalized to {normalized_rtt}')
        self.packet_stats_dict['rtt'].append(normalized_rtt)

    def add_loss_rate(self, loss_rate):
        self.packet_stats_dict['loss_rate'].append(loss_rate)

    def add_bwe(self, bwe):
        self.packet_stats_dict['bwe'].append(bwe)

    def get_bwe(self):
        return self.packet_stats_dict['bwe']

    def _get_latest_history_len_stats(self, key):
        assert self.history_len > 0
        latest_history_len_stats = self.packet_stats_dict[key][-self.history_len:]
        # print(f'latest {self.history_len} {key}: {latest_history_len_stats}')
        return latest_history_len_stats if len(latest_history_len_stats) > 0 else [0]

    def calculate_state(self):
        '''
        Calulate average of latest history_len number of receiver-side throughputs (bps),
        RTTs (ms) and loss rates (0-1).
        '''
        avg_receiver_side_thp = mean(self._get_latest_history_len_stats(key='receiver_side_thp'))
        avg_rtt = mean(self._get_latest_history_len_stats(key='rtt'))
        avg_loss_rate = mean(self._get_latest_history_len_stats(key='loss_rate'))

        return avg_receiver_side_thp, avg_rtt, avg_loss_rate
