#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import torch
import numpy as np


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
