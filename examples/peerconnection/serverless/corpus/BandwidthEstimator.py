import torch
import numpy as np
from utils.packet_info import PacketInfo
from utils.packet_record import PacketRecord
from deep_rl.actor_critic import ActorCritic
import time

UNIT_M = 1000000
MAX_BANDWIDTH_MBPS = 8
MIN_BANDWIDTH_MBPS = 0.01
LOG_MAX_BANDWIDTH_MBPS = np.log(MAX_BANDWIDTH_MBPS)
LOG_MIN_BANDWIDTH_MBPS = np.log(MIN_BANDWIDTH_MBPS)

'''
Return ssrc-sequence_number string from the stats dict
'''
def get_packet_info(stats):
    print(f"Packet info of {stats}:\n{stats['ssrc']}-{stats['sequence_number']}")
    return f"{stats['ssrc']}-{stats['sequence_number']}"

def get_current_timestamp_ms():
        return time.time() * 1000



def log_to_linear(value):
    # from 0~1 to 10kbps to 8Mbps
    value = np.clip(value, 0, 1)
    log_bwe = value * (LOG_MAX_BANDWIDTH_MBPS - LOG_MIN_BANDWIDTH_MBPS) + LOG_MIN_BANDWIDTH_MBPS
    return np.exp(log_bwe) * UNIT_M


'''
Estimator class for emulator or in-situ training.
'''
class Estimator(object):
    def __init__(self, model_path="./model/pretrained_model.pth", step_time=60):
        state_dim = 4
        action_dim = 1
        # stdvar of action distribution
        exploration_param = 0.05

        # Load model
        # self.device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
        # self.model = ActorCritic(state_dim, action_dim, exploration_param, self.device).to(self.device)
        # self.model.load_state_dict(torch.load(model_path))

        # Model ckpt path and interval (ms)
        self.ckpt_path = model_path
        self.ckpt_interval = 60000 # 1 min

        # For states
        self.packet_record = PacketRecord()
        self.packet_record.reset()
        self.step_time = step_time

        # PPO agent
        self.ppo = PPO(state_dim, action_dim, exploration_param, lr, betas, gamma, K_epochs, ppo_clip)

        # Initial state
        states = [0.0, 0.0, 0.0, 0.0]
        torch_tensor_states = torch.FloatTensor(torch.Tensor(states).reshape(1, -1)).to(self.device)

        # Initial action
        action, action_logprobs, value = self.model.forward(torch_tensor_states)
        self.bandwidth_prediction = log_to_linear(action)

        # Update the policy after at least update_interval amount of time (ms)
        # TODO: Update policy after receiving n number of packets? Refer to OnRL
        self.update_interval = 100
        self.latest_ts = get_current_timestamp_ms()

        # Lastest packet info (ssrc-sequence_number) that is already used in training
        self.latest_packet_info = ""

        # Status marker
        self.last_call = "init"


    def is_update_interval_passed(self):
        return self.latest_ts - get_current_timestamp_ms() > self.update_interval

    def is_ckpt_interval_passed(self):
        return self.latest_ts - get_current_timestamp_ms() > self.ckpt_interval


    # training loop 에서처럼 packet stats 받아 처리
    def report_states(self, stats: dict):
        '''
        stats is a dict with the following items
        {
            "send_time_ms": uint,
            "arrival_time_ms": uint,
            "payload_type": int,
            "sequence_number": uint,
            "ssrc": int,
            "padding_length": uint,
            "header_length": uint,
            "payload_size": uint
        }
        '''
        self.last_call = "report_states"

        pass

    # training loop 에서처럼 latest action 전달
    def get_estimated_bandwidth(self)->int:
        if self.last_call == "report_states":
            # Get and update the latest bandwidth only after update_interval passed.
            if self.is_update_interval_passed():
                # Calculate state.
                # Exclude all packet stats already used for training
                # by referring to self.latest_packet_info
                states = []
                # ...
                # Update latest packet info used in training
                # self.latest_packet_info = ...

                # Do model update: calculate loss and do backprop.
                self.update()

            # Do inference: call model.forward.
            # Input the state to the model, and get the model output (latest action)
            torch_tensor_states = torch.FloatTensor(torch.Tensor(states).reshape(1, -1)).to(self.device)
            action, action_logprobs, value = self.model.forward(torch_tensor_states)
            self.bandwidth_prediction = log_to_linear(action)

        # Return latest action (bps), e.g. int(1e6) equals to 1Mbps
        return self.bandwidth_prediction

    # training loop 참고해 update_interval 마다 model update
    def update(self):
        # Calculate loss and do backprop

        # Save model ckpt
        if self.is_ckpt_interval_passed():
            self.ppo.save_model(self.ckpt_path)
        # plt.plot(range(len(record_episode_reward)), record_episode_reward)
        # plt.xlabel('Episode')
        # plt.ylabel('Averaged episode reward')
        # plt.savefig('%sreward_record.jpg' % (self.ckpt_path))


