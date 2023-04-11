#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
from statistics import mean
import logging
logging.basicConfig(
    filename='example.log', encoding='utf-8',
    format='%(asctime)s,%(msecs)d %(levelname)-8s [%(pathname)s:%(lineno)d in ' \
           'function %(funcName)s] %(message)s',
    datefmt='%Y-%m-%d:%H:%M:%S',
    level=logging.DEBUG
)

# For estimated bandwidth (action) or receiver-side thp (bps)
UNIT_K = 1000
MAX_KBPS = 1000 # 1Mbps
MIN_KBPS = 1    # 1Kbps
LOG_MAX_KBPS = np.log(MAX_KBPS)
LOG_MIN_KBPS = np.log(MIN_KBPS)

# For RTT (ms)
MAX_RTT_MS = 100    # 100ms
MIN_RTT_MS = 1      # 1ms
LOG_MAX_RTT_MS = np.log(MAX_RTT_MS)
LOG_MIN_RTT_MS = np.log(MIN_RTT_MS)


class PacketRecord:
    def __init__(self):
        # Calculate a state with an average of stats from the latest 10 RTCP packets
        self.history_len = 10
        self.logger = logging.getLogger(__name__)
        self.reset()

    # Initial observation for env.reset()
    def reset(self):
        self.packet_stats_dict = {}
        # For observation
        self.packet_stats_dict['loss_rate'] = [0.0]
        self.packet_stats_dict['rtt_norm'] = [0.0]
        self.packet_stats_dict['delay_interval_norm'] = [0.0]
        self.packet_stats_dict['receiver_side_thp_norm'] = [1.0]
        # For reward
        self.packet_stats_dict['receiver_side_thp'] = [300000]
        self.packet_stats_dict['rtt'] = [0.0, 0.0]
        self.packet_stats_dict['receiver_side_thp_fluct'] = [0]
        # Action
        self.packet_stats_dict['bwe'] = [300000]
        self.call_start = False
        print(f'PacketRecord reset done: internal state init for a new episode')

    def normalize_obs_bps(self, bps):
        # 1Kbps-1Mbps to 0~1
        clipped_kbps = np.clip(bps/UNIT_K, MIN_KBPS, MAX_KBPS)
        # log_value = np.log(clipped_value)
        # (x - min) / (max - min) where x, min, max is log values
        # norm_value = (log_value - LOG_MIN_KBPS) / (LOG_MAX_KBPS - LOG_MIN_KBPS)
        norm_kbps = (clipped_kbps - MIN_KBPS) / (MAX_KBPS - MIN_KBPS)
        print(f'bps (1Kbps-1Mbps) {bps} clipped kbps (1Kbps-1Mbps) {clipped_kbps} norm kbps (0-1) {norm_kbps}')
        return norm_kbps

    def normalize_obs_ms(self, ms):
        # 1-100ms to 0~1
        clipped_ms = np.clip(ms, MIN_RTT_MS, MAX_RTT_MS)
        # log_value = np.log(clipped_value)
        # (x - min) / (max - min) where x, min, max is log values
        # norm_value = (log_value - LOG_MIN_RTT_US) / (LOG_MAX_RTT_US - LOG_MIN_RTT_US)
        norm_ms = (clipped_ms - MIN_RTT_MS) / (MAX_RTT_MS - MIN_RTT_MS)
        logging.debug(f'RTT (1-100ms) {ms} clipped RTT (1-100ms) {clipped_ms} norm RTT (0-1) {norm_ms}')
        return norm_ms

    # Referred to https://stats.stackexchange.com/questions/178626/how-to-normalize-data-between-1-and-1
    # Using 1Kbps ~ 1Mbps as the range of the action.
    def normalize_action(self, action_bps):
        # 1Kbps~1Mbps to -1~1
        norm_action_kbps = 2 * (action_bps/UNIT_K - MIN_KBPS) / (MAX_KBPS - MIN_KBPS) - 1
        print(f'Action: (1Kbps-1Mbps) {action_bps} to (-1~1) {norm_action_kbps}')
        return norm_action_kbps

    # Referred to https://stats.stackexchange.com/questions/178626/how-to-normalize-data-between-1-and-1
    def rescale_action(self, norm_action_kbps):
        # -1~1 to 1Kbps~1Mbps
        rescaled_action_bps = ((MAX_KBPS - MIN_KBPS) * (norm_action_kbps + 1) / 2 + MIN_KBPS) * UNIT_K
        print(f'Action: (-1~1) {norm_action_kbps} to (1Kbps-1Mbps) {rescaled_action_bps}')
        return rescaled_action_bps

    def get_packet_stats(self):
        # get packet stats from zeromq
        pass

    # Add normalized receiver-side throughput.
    # 1Kbps-1Mbps (empirically) to 0-1
    def add_receiver_side_thp(self, receiver_side_thp):
        # Fluctuation in receiver-side throughput (for reward)
        latest_recv_thp = self.packet_stats_dict['receiver_side_thp'][-1]
        recv_side_thp_fluct = abs(receiver_side_thp - latest_recv_thp)
        self.packet_stats_dict['receiver_side_thp_fluct'].append(recv_side_thp_fluct)
        # Receiver-side throughput (for reward)
        self.packet_stats_dict['receiver_side_thp'].append(receiver_side_thp)
        # Normalized receiver-side throughput (for observation)
        recv_side_thp_norm = self.normalize_obs_bps(receiver_side_thp)
        self.packet_stats_dict['receiver_side_thp_norm'].append(recv_side_thp_norm)
        print(f'Added recv thp (1Kbps-1Mbps) {receiver_side_thp} recv thp (0-1) {recv_side_thp_norm} recv thp fluct (1Kbps-1Mbps) {recv_side_thp_fluct}')
        if not self.call_start:
            self.call_start = True

    # Add normalized RTT.
    # 1-100ms to 0-1
    def add_rtt(self, rtt):
        self.packet_stats_dict['rtt'].append(rtt)
        norm_rtt = self.normalize_obs_ms(rtt)
        self.packet_stats_dict['rtt_norm'].append(norm_rtt)
        print(f'Added RTT (ms) {rtt} RTT (0-1) {norm_rtt}')
        print(f"@@@@@@@@@@@@@@@ CURRENT PacketRecord['rtt']: {self.packet_stats_dict['rtt']}")
        if not self.call_start:
            self.call_start = True


    def add_loss_rate(self, loss_rate):
        self.packet_stats_dict['loss_rate'].append(loss_rate)
        print(f'Added loss rate {loss_rate}')
        print(f"@@@@@@@@@@@@@@@ CURRENT PacketRecord['loss_rate']: {self.packet_stats_dict['loss_rate']}")
        if not self.call_start:
            self.call_start = True


    # TODO: measure delay interval instead of diff in consecutive RTTs
    def add_delay_interval(self):
        delay_interval = abs(self.packet_stats_dict['rtt'][-1] \
            - self.packet_stats_dict['rtt'][-2])
        norm_delay_interval = abs(self.normalize_obs_ms(self.packet_stats_dict['rtt'][-1]) \
            - self.normalize_obs_ms(self.packet_stats_dict['rtt'][-2]))
        self.packet_stats_dict['delay_interval_norm'].append(norm_delay_interval)
        print(f'Added delay interval (ms) {delay_interval} delay interval (0-1) {norm_delay_interval}')
        print(f"@@@@@@@@@@@@@@@ CURRENT PacketRecord['delay_interval_norm']: {self.packet_stats_dict['delay_interval_norm']}")
        if not self.call_start:
            self.call_start = True


    # TODO: dequeue the items when using them
    def _get_latest_history_len_stats(self, key):
        assert self.history_len > 0
        latest_history_len_stats = self.packet_stats_dict[key][-self.history_len:]
        print(f'latest {self.history_len} {key}: {latest_history_len_stats}')
        return latest_history_len_stats if len(latest_history_len_stats) > 0 else [0]

    '''
    Calulate average of latest history_len number of receiver-side throughputs (bps),
    RTTs (ms) and loss rates (0-1).
    '''
    def calculate_obs(self):
        loss_rate = mean(self._get_latest_history_len_stats(key='loss_rate'))
        norm_rtt = mean(self._get_latest_history_len_stats(key='rtt_norm'))
        norm_delay_interval = mean(self._get_latest_history_len_stats(key='delay_interval_norm'))
        norm_recv_thp = mean(self._get_latest_history_len_stats(key='receiver_side_thp_norm'))
        return [loss_rate, norm_rtt, norm_delay_interval, norm_recv_thp]

    '''
    Incentivize increase in receiver-side thp,
    penalize increase in loss, RTT, and thp fluctuation.
    Using actual values (with different magnitudes) + normalizing coefficients in OnRL
    '''
    def calculate_reward(self):
        recv_thp = mean(self._get_latest_history_len_stats(key='receiver_side_thp'))
        loss_rate = mean(self._get_latest_history_len_stats(key='loss_rate'))
        rtt = mean(self._get_latest_history_len_stats(key='rtt'))
        recv_thp_fluct = mean(self._get_latest_history_len_stats(key='receiver_side_thp_fluct'))

        # 50 * norm_recv_thp - 50 * loss_rate - 10 * norm_rtt - 30 * norm_recv_thp_fluct
        reward = 50 * recv_thp - 50 * loss_rate - 10 * rtt - 30 * recv_thp_fluct
        return reward, recv_thp, loss_rate, rtt, recv_thp_fluct
