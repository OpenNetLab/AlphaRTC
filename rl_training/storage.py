#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import torch
import numpy as np

# TODO: Refactor 'Storage' to 'RolloutBuffer', as normally used in PPO implementations.
# (cf. https://github.com/nikhilbarhate99/PPO-PyTorch/blob/d5c883783ac6406c4d58a1e1e9eb6f08a6462d89/PPO.py#L20)
# TODO: Decouple compute_returns from this class and move it to PPO#update.
class Storage:
    def __init__(self):
        self.actions = []
        self.values = []
        self.states = []
        self.logprobs = []
        self.rewards = []
        self.returns = []

    def compute_returns(self, next_value, gamma):
        # compute returns for advantages
        returns = np.zeros(len(self.rewards)+1)
        returns[-1] = next_value
        for i in reversed(range(len(self.rewards))):
            # Original: returns[i] = returns[i+1] * gamma * (1-self.is_terminals[i]) + self.rewards[i]
            # i.e. if done == True, returns[i] = self.rewards[i]
            returns[i] = returns[i+1] * gamma + self.rewards[i]
            self.returns.append(torch.tensor([returns[i]]))
        self.returns.reverse()

    def clear_storage(self):
        self.actions.clear()
        self.values.clear()
        self.states.clear()
        self.logprobs.clear()
        self.rewards.clear()
        self.returns.clear()
