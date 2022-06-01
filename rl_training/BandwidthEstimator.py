import torch
import numpy as np
import os

from rl_training.packet_info import PacketInfo
from rl_training.packet_record import PacketRecord
from rl_training.ppo_agent import PPO
from rl_training.storage import Storage

UNIT_M = 1000000
MAX_BANDWIDTH_MBPS = 8      # Max: 8 Mbps
MIN_BANDWIDTH_MBPS = 0.01   # Min: 10 Kbps
LOG_MAX_BANDWIDTH_MBPS = np.log(MAX_BANDWIDTH_MBPS)
LOG_MIN_BANDWIDTH_MBPS = np.log(MIN_BANDWIDTH_MBPS)

def log_to_linear(value):
    # from 0~1 to 10kbps to 8Mbps
    value = np.clip(value, 0, 1)
    log_bwe = value * (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS) + LOG_MIN_BANDWIDTH_MBPS
    return np.exp(log_bwe) * UNIT_M

def liner_to_log(value):
    # from 10kbps~8Mbps to 0~1
    value = np.clip(value / UNIT_M, MIN_BANDWIDTH_MBPS, MAX_BANDWIDTH_MBPS)
    log_value = np.log(value)
    return (log_value - LOG_MIN_BANDWIDTH_MBPS) / (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS)


'''
Estimator class for emulator or in-situ training.
'''
class Estimator(object):
    def __init__(self):
        # Hyperparameters for RL training
        # 1 episode = num_steps_per_episode steps. Policy is updated whenever an episode is finished.
        # 1 step = whenever a reward is generated, i.e. an action is applied and new state is observed as a result of it
        self.num_steps_per_episode = 10     # cf. set as 4000 in gym-example
        self.num_steps = 0
        self.num_episodes = 0

        # PPO agent for RL policy
        state_dim = 4
        action_dim = 1
        exploration_param = 0.05            # the std var of action distribution
        lr = 3e-5                           # 0.00003. for Adam
        betas = (0.9, 0.999)
        self.gamma = 0.99                   # discount factor
        K_epochs = 37                       # update policy for K_epochs epochs in one PPO update
        ppo_clip = 0.2                      # clip parameter
        self.ppo = PPO(state_dim, action_dim, exploration_param, lr, betas, self.gamma, K_epochs, ppo_clip)

        # Initial state and action
        self.state = [0.0, 0.0, 0.0, 0.0]
        torch_tensor_states = torch.FloatTensor(torch.Tensor(self.state).reshape(1, -1)).to(torch.device("cpu"))
        action, _, _ = self.ppo.policy.forward(torch_tensor_states)
        self.bwe = log_to_linear(action)

        # To calculate average reward per step in each episode
        self.accum_reward  = 0
        self.avg_reward_per_step_list = []

        # Accumulating states
        self.packet_record = PacketRecord()
        self.packet_record.reset()
        self.storage = Storage()

        # Path to save model ckpt
        self.ckpt_dir = './rl_model/ckpt/'
        if not os.path.exists(self.ckpt_dir):
            os.makedirs(self.ckpt_dir)
        # Save ckpt at every 10 episodes
        self.episodes_between_ckpt = 10

        # Path to save reward curve
        self.reward_curve_dir = './rl_model/reward_curves/'
        if not os.path.exists(self.reward_curve_dir):
            os.makedirs(self.reward_curve_dir)


    '''
    Calculate state and reward from received packet statistics
    from the receiver's RemoteEstimatorProxy::IncompingPackets.
    Should be called once per every step.
    '''
    def report_states(self, stats: dict):
        # Calculate state and reward
        state, reward = self.calculate_state_reward(stats)
        self.state = torch.Tensor(state)

        # BWE: action of the current policy
        action = self.ppo.select_action(self.state, self.storage)
        self.bwe = log_to_linear(action)
        print(f'Episode {self.num_episodes} Step {self.num_steps} Action (BWE): {self.bwe}')

        # Calculate cumulative sum of per step reward
        self.storage.rewards.append(reward)
        self.accum_reward += reward

        # Increase step counter
        self.num_steps += 1

        # Check whether to update the policy (episode finished)
        if self.num_steps % self.num_steps_per_episode == 0:
            # Update the policy
            self.policy_update()

            # Clear per-episode data and reset step counter
            self.storage.clear_storage()
            self.accum_reward = 0
            self.num_steps = 0

            # Save model ckpt
            if self.num_episodes > 0 and self.num_episodes % self.episodes_between_ckpt == 0:
                self.ppo.save_model(self.ckpt_dir)

            # Increase episode counter
            self.num_episodes += 1

        return self.bwe


    '''
    PPO policy update.
    Currently updated at every BWE request.
    '''
    def policy_update(self):
        next_value = self.ppo.get_value(self.state)
        self.storage.compute_returns(next_value, self.gamma)

        policy_loss, val_loss = self.ppo.update(self.storage)
        avg_reward_per_step = self.accum_reward / self.num_steps
        self.avg_reward_per_step_list.append(avg_reward_per_step)
        print(f'Episode {self.num_episodes} finished, PPO policy updated: policy loss {policy_loss} value loss {val_loss} avg. reward per step {avg_reward_per_step}')


    '''
    Received packet statistics are the result of running current self._bwe.
    From this, calculate state and reward.
    '''
    def calculate_state_reward(self, packet):
        packet_info = PacketInfo()
        packet_info.payload_type = packet["payload_type"]
        packet_info.ssrc = packet["ssrc"]
        packet_info.sequence_number = packet["sequence_number"]
        packet_info.send_timestamp = packet["send_time_ms"]
        packet_info.receive_timestamp = packet["arrival_time_ms"]
        packet_info.padding_length = packet["padding_length"]
        packet_info.header_length = packet["header_length"]
        packet_info.payload_size = packet["payload_size"]
        # this should be the bwe that resulted this packet statistics
        # TODO: make the WebRTC receiver send bwe also
        packet_info.bandwidth_prediction = int(float(self.bwe))
        # Calculate loss count, delay, last interval time and accumulate them in a single record
        self.packet_record.calculate_per_packet_stats(packet_info)

        # Calculate state
        states = []
        receiving_rate = self.packet_record.calculate_receiving_rate()
        states.append(liner_to_log(receiving_rate))
        delay = self.packet_record.calculate_average_delay()
        states.append(min(delay/1000, 1))
        loss_ratio = self.packet_record.calculate_loss_ratio()
        states.append(loss_ratio)
        latest_prediction = self.packet_record.calculate_latest_prediction()
        states.append(liner_to_log(latest_prediction))

        # Calculate reward.
        # Incentivize increase in throughput, penalize increase in delay.
        # TODO: Add normalizing coefficients
        reward = states[0] - states[1]

        return states, reward


