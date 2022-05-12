#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import numpy as np
import glob
import json
from datetime import datetime

from gym import spaces

# sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "gym"))
from rl_training.packet_info import PacketInfo
from rl_training.packet_record import PacketRecord


UNIT_M = 1000000
MAX_BANDWIDTH_MBPS = 8
MIN_BANDWIDTH_MBPS = 0.01
LOG_MAX_BANDWIDTH_MBPS = np.log(MAX_BANDWIDTH_MBPS)
LOG_MIN_BANDWIDTH_MBPS = np.log(MIN_BANDWIDTH_MBPS)


def liner_to_log(value):
    # from 10kbps~8Mbps to 0~1
    value = np.clip(value / UNIT_M, MIN_BANDWIDTH_MBPS, MAX_BANDWIDTH_MBPS)
    log_value = np.log(value)
    return (log_value - LOG_MIN_BANDWIDTH_MBPS) / (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS)


def log_to_linear(value):
    # from 0~1 to 10kbps to 8Mbps
    value = np.clip(value, 0, 1)
    log_bwe = value * (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS) + LOG_MIN_BANDWIDTH_MBPS
    return np.exp(log_bwe) * UNIT_M


class GymEnv:
    def __init__(self, step_time=60):
        self.gym_env = None
        self.step_time = step_time
        trace_dir = os.path.join(os.path.dirname(__file__), "traces")
        self.trace_set = glob.glob(f'{trace_dir}/**/*.json', recursive=True)
        self.action_space = spaces.Box(low=0.0, high=1.0, shape=(1,), dtype=np.float64)
        self.observation_space = spaces.Box(
            low=np.array([0.0, 0.0, 0.0, 0.0]),
            high=np.array([1.0, 1.0, 1.0, 1.0]),
            dtype=np.float64)
        self.latest_seq_no = -1

    def reset(self):
        # self.gym_env = alphartc_gym.Gym()
        # self.gym_env.reset(trace_path=random.choice(self.trace_set),
        #     report_interval_ms=self.step_time,
        #     duration_time_ms=0)
        self.packet_record = PacketRecord()
        self.packet_record.reset()
        return [0.0, 0.0, 0.0, 0.0]

    def step(self, action):
        # action: log to linear
        estimated_bandwidth = str(log_to_linear(action).item())
        # print(f'latest action {estimated_bandwidth} log_to_linear(action) {log_to_linear(action).item()}')

        # Send the action to a running WebRTC receiver
        # by storing the action in a file named `estimated_bandwidth`.
        # Overwrite the contents if exists
        # with open('estimated_bandwidth', 'w') as f:
        #     f.write(estimated_bandwidth)

        ts = datetime.timestamp(datetime.now())
        ts_bwe = {'timestamp': ts, 'bandwidth': estimated_bandwidth}
        print(f'ts_bwe {ts_bwe}')

        with open('estimated_bandwidth.json', 'w') as f:
            json.dump(ts_bwe, f)


        # # Wait until new packet statistics arrive
        # while True:
        #     # Receive packet statistics in json format
        #     # from the receiver's RemoteEstimatorProxy::IncompingPackets.
        #     with open('packet_info.json') as f:
        #         rep = f.read()
        #         # TODO: add signals when training ended, and return done = True when so.

        #     # dict type
        #     packet = json.loads(rep)
        #     if packet["sequence_number"] > self.latest_seq_no:
        #         break

        # Receive packet statistics in json format
        # from the receiver's RemoteEstimatorProxy::IncompingPackets.
        with open('packet_info.json') as f:
            rep = f.read()
            # TODO: add signals when training ended, and return done = True when so.

        # dict type
        packet = json.loads(rep)

        # for packet in packet:
            # print(f'{packet['payload_type']} {type(packet['payload_type'])}')
        # print(packet['payload_type'])
        packet_info = PacketInfo()
        packet_info.payload_type = packet["payload_type"]
        packet_info.ssrc = packet["ssrc"]
        packet_info.sequence_number = packet["sequence_number"]
        packet_info.send_timestamp = packet["send_time_ms"]
        packet_info.receive_timestamp = packet["arrival_time_ms"]
        packet_info.padding_length = packet["padding_length"]
        packet_info.header_length = packet["header_length"]
        packet_info.payload_size = packet["payload_size"]
        packet_info.bandwidth_prediction = int(float(estimated_bandwidth))
        self.packet_record.calculate_per_packet_stats(packet_info)

        # calculate state
        states = []
        receiving_rate = self.packet_record.calculate_receiving_rate(interval=self.step_time)
        states.append(liner_to_log(receiving_rate))
        delay = self.packet_record.calculate_average_delay(interval=self.step_time)
        states.append(min(delay/1000, 1))
        loss_ratio = self.packet_record.calculate_loss_ratio(interval=self.step_time)
        states.append(loss_ratio)
        latest_prediction = self.packet_record.calculate_latest_prediction()
        # print(f'packet_record.calculate_latest_prediction() {latest_prediction} {type(latest_prediction)}')
        states.append(liner_to_log(latest_prediction))

        # calculate reward
        reward = states[0] - states[1] - states[2]

        # TODO: currently 'done' is always false
        return states, reward, False, {}
