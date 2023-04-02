#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
from statistics import mean

# For bps
UNIT_K = 1000
MAX_RECV_THP_KBPS = 200
MIN_RECV_THP_KBPS = 1
LOG_MAX_RECV_THP_KBPS = np.log(MAX_RECV_THP_KBPS)
LOG_MIN_RECV_THP_KBPS = np.log(MIN_RECV_THP_KBPS)

# For ms
MAX_RTT_US = 100000   # 100ms
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
        self.packet_stats_dict['receiver_side_thp_fluct'] = []
        self.packet_stats_dict['receiver_side_thp_actual'] = [] # for debugging
        self.packet_stats_dict['loss_rate'] = []
        self.packet_stats_dict['rtt'] = []
        self.packet_stats_dict['bwe'] = []

    def normalize_recv_thp(self, value):
        # from 1Kbps-200Kbps to 0-1
        clipped_value = np.clip(value/UNIT_K, MIN_RECV_THP_KBPS, MAX_RECV_THP_KBPS)
        log_value = np.log(clipped_value)
        # (x - min) / (max - min) where x, min, max is log values
        norm_value = (log_value - LOG_MIN_RECV_THP_KBPS) / (LOG_MAX_RECV_THP_KBPS - LOG_MIN_RECV_THP_KBPS)
        return norm_value

    def normalize_rtt(self, value):
        # from 1-100ms to 0-1
        clipped_value = np.clip(value * UNIT_K, MIN_RTT_US, MAX_RTT_US)
        log_value = np.log(clipped_value)
        # (x - min) / (max - min) where x, min, max is log values
        norm_value = (log_value - LOG_MIN_RTT_US) / (LOG_MAX_RTT_US - LOG_MIN_RTT_US)
        return norm_value

    # Add normalized receiver-side throughput.
    # 1Kbps-200Kbps (empirically) to 0-1
    def add_receiver_side_thp(self, receiver_side_thp):
        # Calculate fluctuation in receiver-side throughput
        latest_norm_recv_thp = self.packet_stats_dict['receiver_side_thp'][-1]
        this_norm_recv_thp = self.normalize_recv_thp(receiver_side_thp)
        self.packet_stats_dict['receiver_side_thp_fluct'].append(abs(this_norm_recv_thp - latest_norm_recv_thp))

        # For debugging fluctuation in receiver-side throughput
        latest_recv_thp = self.packet_stats_dict['receiver_side_thp_actual'][-1]
        actual_recv_thp_fluct = abs(receiver_side_thp - latest_recv_thp)
        norm_recv_thp_fluct = self.packet_stats_dict['receiver_side_thp_fluct'][-1]
        print(f'recv thp fluct: Actual {actual_recv_thp_fluct} Normalized {norm_recv_thp_fluct}')
        self.packet_stats_dict['receiver_side_thp_actual'].append(receiver_side_thp)

        # Calculate normalized receiver-side throughput
        # print(f'Receiver-side thp {receiver_side_thp} normalized to {normalized_thp}')
        self.packet_stats_dict['receiver_side_thp'].append(this_norm_recv_thp)


    # Add normalized RTT.
    # 1-100ms to 0-1
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

    # TODO: dequeue the items when using them
    def _get_latest_history_len_stats(self, key):
        assert self.history_len > 0
        latest_history_len_stats = self.packet_stats_dict[key][-self.history_len:]
        # print(f'latest {self.history_len} {key}: {latest_history_len_stats}')
        return latest_history_len_stats if len(latest_history_len_stats) > 0 else [0]

    def calculate_statistics(self):
        '''
        Calulate average of latest history_len number of receiver-side throughputs (bps),
        RTTs (ms) and loss rates (0-1).
        '''
        avg_receiver_side_thp = mean(self._get_latest_history_len_stats(key='receiver_side_thp'))
        avg_loss_rate = mean(self._get_latest_history_len_stats(key='loss_rate'))
        avg_rtt = mean(self._get_latest_history_len_stats(key='rtt'))
        avg_receiver_side_thp_fluct = mean(self._get_latest_history_len_stats(key='receiver_side_thp_fluct'))

        return avg_receiver_side_thp, avg_loss_rate, avg_rtt, avg_receiver_side_thp_fluct
