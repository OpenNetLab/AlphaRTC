#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import torch
from torch import nn
from torch.distributions import MultivariateNormal


class ActorCritic(nn.Module):
    def __init__(self, state_dim, action_dim, exploration_param=0.05, device="cpu"):
        super(ActorCritic, self).__init__()
        # output of actor in [0, 1]
        self.actor = nn.Sequential(
                nn.Linear(state_dim, 128),
                nn.ReLU(),
                nn.Linear(128, 64),
                nn.ReLU(),
                nn.Linear(64, 32),
                nn.ReLU(),
                nn.Linear(32, action_dim),
                nn.Sigmoid()
                )
        # critic
        self.critic = nn.Sequential(
                nn.Linear(state_dim, 128),
                nn.ReLU(),
                nn.Linear(128, 64),
                nn.ReLU(),
                nn.Linear(64,32),
                nn.ReLU(),
                nn.Linear(32, 1)
                )
        self.device = device
        self.action_var = torch.full((action_dim,), exploration_param**2).to(self.device)
        self.random_action = True

    def forward(self, state):
        # actor's output
        action_mean = self.actor(state)
        # critic's output
        value = self.critic(state)

        action_cov_mat = torch.diag(self.action_var).to(self.device)
        action_dist = MultivariateNormal(action_mean, action_cov_mat)

        if not self.random_action:
            # simply return the actor's output
            action = action_mean
        else:
            action = action_dist.sample()

        action_logprobs = action_dist.log_prob(action)

        return action.detach(), action_logprobs, value

    def evaluate(self, state, action):
        # actor's output
        action_mean = self.actor(state)
        # critic's output
        value = self.critic(state)

        action_cov_mat = torch.diag(self.action_var).to(self.device)
        action_dist = MultivariateNormal(action_mean, action_cov_mat)
        action_logprobs = action_dist.log_prob(action)
        action_dist_entropy = action_dist.entropy()

        return action_logprobs, torch.squeeze(value), action_dist_entropy

