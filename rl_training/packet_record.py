#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np


class PacketRecord:
    def __init__(self, base_delay_ms=0):
        self.base_delay_ms = base_delay_ms
        self.history_len = 10 # calculate a state based on the latest 10 packets
        self.reset()

    def reset(self):
        self.packet_num = 0
        self.packet_list = []   # ms
        self.last_seqNo = {}
        self.timer_delta = None # ms
        self.min_seen_delay = self.base_delay_ms  # ms

    def clear(self):
        self.packet_num = 0
        self.packet_list = []

    def add_packet_stats(self, packet_info):
        assert (len(self.packet_list) == 0
                or packet_info.receive_timestamp
                >= self.packet_list[-1]['timestamp']), \
            "The incoming packets receive_timestamp disordered"

        # Calculate the loss count
        loss_count = 0
        if packet_info.ssrc in self.last_seqNo:
            loss_count = max(0,
                packet_info.sequence_number - self.last_seqNo[packet_info.ssrc] - 1)
        self.last_seqNo[packet_info.ssrc] = packet_info.sequence_number

        # Calculate packet delay
        # if self.timer_delta is None:
        #     # shift delay of the first packet to base delay
        #     self.timer_delta = self.base_delay_ms - \
        #         (packet_info.receive_timestamp - packet_info.send_timestamp)
        # delay = self.timer_delta + \
        #     (packet_info.receive_timestamp - packet_info.send_timestamp)
        # self.min_seen_delay = min(delay, self.min_seen_delay)
        delay = packet_info.receive_timestamp - packet_info.send_timestamp

        # Record result in current packet
        packet_result = {
            'timestamp': packet_info.receive_timestamp,  # ms
            'delay': delay,  # ms
            'payload_byte': packet_info.payload_size,  # B
            'loss_count': loss_count,  # p
            'bandwidth_prediction': packet_info.bandwidth_prediction  # bps
        }
        print(f'Packet {packet_info.sequence_number} result {packet_result}')
        self.packet_list.append(packet_result)
        self.packet_num += 1


    def _get_latest_history_len_number_of_packets(self, key):
        assert self.history_len > 0

        if self.packet_num == 0:
            return []

        # start_time = self.packet_list[-1]['timestamp'] - self.history_len
        index = self.packet_num - 1
        history_len = 0
        latest_history_len_number_of_packets = []

        if key is None:
            # result_list of at most history_len packets
            while index >= 0 and history_len < self.history_len:
                latest_history_len_number_of_packets.append(self.packet_list[index])
                index -= 1
                history_len += 1
        else:
            # result_list of at most history_len packets
            while index >= 0 and history_len < self.history_len:
                latest_history_len_number_of_packets.append(self.packet_list[index][key])
                index -= 1
                history_len += 1

        return latest_history_len_number_of_packets

    def calculate_receiving_rate(self):
        '''
        Calulate the receiving rate in the last history_len number of packets.
        return type: bps
        '''
        assert self.history_len > 0

        received_size_list = self._get_latest_history_len_number_of_packets(key='payload_byte')
        received_list = self._get_latest_history_len_number_of_packets(key=None)
        print(f'received_size_list {received_size_list}')
        if received_size_list:
            # aggregate payload of latest history_len number of packets
            received_nbytes = np.sum(received_size_list)
            start_time = received_list[0]['timestamp']
            end_time = received_list[-1]['timestamp']
            elapsed_ms = end_time - start_time
            print(f'received_nbyptes {received_nbytes} elapsed_ms {elapsed_ms}')
            return received_nbytes * 8 / elapsed_ms * 1000
        else:
            return 0

    def calculate_average_delay(self):
        '''
        Calulate the average delay in the last history_len number of packets.
        The unit of return value: ms
        '''
        assert self.history_len > 0

        delay_list = self._get_latest_history_len_number_of_packets(key='delay')
        print(f'delay_list {delay_list}')

        if delay_list:
            return np.mean(delay_list) - self.base_delay_ms
        else:
            return 0

    def calculate_loss_ratio(self):
        '''
        Calulate the loss ratio in the last history_len number of packets.
        returns: %
        '''
        assert self.history_len > 0

        loss_list = self._get_latest_history_len_number_of_packets(key='loss_count')
        if loss_list:
            loss_num = np.sum(loss_list)
            received_num = len(loss_list)
            return loss_num / (loss_num + received_num)
        else:
            return 0


    def calculate_latest_prediction(self):
        '''
        The unit of return value: bps
        '''
        assert self.history_len > 0

        if self.packet_num > 0:
            return self.packet_list[-1]['bandwidth_prediction']
        else:
            return 0