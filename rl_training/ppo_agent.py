#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import torch
import time
from rl_training.actor_critic import ActorCritic


class PPO:
    def __init__(self, state_dim, action_dim, exploration_param, lr, betas, gamma, K_epochs, ppo_clip, use_gae=False):
        self.lr = lr
        self.betas = betas
        self.gamma = gamma
        self.ppo_clip = ppo_clip
        self.K_epochs = K_epochs
        self.use_gae = use_gae

        self.device = torch.device("cpu")
        self.policy = ActorCritic(state_dim, action_dim, exploration_param, self.device).to(self.device)
        self.optimizer = torch.optim.Adam(self.policy.parameters(), lr=lr, betas=betas)

        self.policy_old = ActorCritic(state_dim, action_dim, exploration_param, self.device).to(self.device)
        self.policy_old.load_state_dict(self.policy.state_dict())

    def select_action(self, state, storage):
        # get action from the current policy
        state = torch.FloatTensor(state.reshape(1, -1)).to(self.device)
        action, action_logprobs, value = self.policy_old.forward(state)

        storage.states.append(state)
        storage.actions.append(action)
        storage.logprobs.append(action_logprobs)
        storage.values.append(value)
        return action.cpu()

    def get_value(self, state):
        return self.policy_old.critic(state)

    def update(self, storage):
        episode_actor_loss = 0
        episode_critic_loss = 0
        if self.use_gae:
            raise NotImplementedError

        # TODO: Refactor L51-53 as written in storage.py:L9
        # TODO: Check the correctness of subtracting values from returns to get advantages
        # (cf. https://github.com/nikhilbarhate99/PPO-PyTorch/blob/master/PPO.py#L204)
        advantages = (torch.tensor(storage.returns) - torch.tensor(storage.values)).detach()
        # standardization
        advantages = (advantages - advantages.mean()) / (advantages.std() + 1e-5) # 0.00001

        old_states = torch.squeeze(torch.stack(storage.states).to(self.device), 1).detach()
        old_actions = torch.squeeze(torch.stack(storage.actions).to(self.device), 1).detach()
        old_action_logprobs = torch.squeeze(torch.stack(storage.logprobs), 1).to(self.device).detach()
        old_returns = torch.squeeze(torch.stack(storage.returns), 1).to(self.device).detach()

        # Optimize policy for K epochs
        for _ in range(self.K_epochs):
            # Evaluate old actions and values
            action_logprobs, state_values, dist_entropy = self.policy.evaluate(old_states, old_actions)
            ratios = torch.exp(action_logprobs - old_action_logprobs)

            # Calculate surrogate loss
            surr1 = ratios * advantages
            surr2 = torch.clamp(ratios, 1-self.ppo_clip, 1+self.ppo_clip) * advantages
            actor_loss = -torch.min(surr1, surr2)
            # 0.5 * MSE loss(state_values, rewards)
            critic_loss = 0.5 * (state_values - old_returns).pow(2).mean()
            # Final loss of clipped objective PPO.
            # As we sum up actor_loss and critic_loss, we do loss.mean().backward
            # as in https://github.com/nikhilbarhate99/PPO-PyTorch/blob/master/PPO.py#L234
            # and mean() is not applied on actor_loss
            # as in  https://github.com/nikhilbarhate99/PPO-PyTorch/blob/master/PPO.py#L230)
            loss = actor_loss + critic_loss - 0.01 * dist_entropy

            # TODO: Shouldn't we separate backprop of actor_loss and critic_loss
            # as in https://github.com/ericyangyu/PPO-for-Beginners/blob/master/ppo.py ?
            self.optimizer.zero_grad()
            loss.mean().backward()
            self.optimizer.step()
            episode_actor_loss += actor_loss.detach()
            episode_critic_loss += critic_loss.detach()

        # Copy new weights into old policy
        self.policy_old.load_state_dict(self.policy.state_dict())
        return episode_actor_loss / self.K_epochs, episode_critic_loss / self.K_epochs

    def save_model(self, data_path):
        torch.save(self.policy.state_dict(), '{}ppo_{}.pth'.format(data_path, time.strftime("%Y_%m_%d_%H_%M_%S", time.localtime())))
