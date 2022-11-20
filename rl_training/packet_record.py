#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from statistics import mean


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
        self.packet_action_dict['bwe'] = []

    def add_receiver_side_thp(self, receiver_side_thp):
        self.packet_stats_dict['receiver_side_thp'].append(receiver_side_thp)

    def add_rtt(self, rtt):
        self.packet_stats_dict['rtt'].append(rtt)

    def add_loss_rate(self, loss_rate):
        self.packet_stats_dict['loss_rate'].append(loss_rate)

    def add_bwe(self, bwe):
        self.packet_stats_dict['bwe'].append(bwe)

    def get_bwe(self):
        return self.packet_stats_dict['bwe']

    def _get_latest_history_len_stats(self, key):
        assert self.history_len > 0
        latest_history_len_stats = self.packet_stats_dict[key][:self.history_len]
        print(f'latest {self.history_len} {key}: {latest_history_len_stats}')
        return latest_history_len_stats if len(latest_history_len_stats) > 0 else [0]

    def calculate_state(self):
        '''
        Calulate average of latest history_len number of receiver-side throughputs (bps),
        RTTs (ms) and loss rates (0-1).
        '''
        avg_receiver_side_thp = mean(self._get_latest_history_len_stats(key='receiver_side_thp'))
        avg_rtt = mean(self._get_latest_history_len_stats(key='rtt'))
        avg_loss_rate = mean(self._get_latest_history_len_stats(key='loss_rate'))

        state = [avg_receiver_side_thp, avg_rtt, avg_loss_rate]

        return state
