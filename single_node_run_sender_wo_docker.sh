#!/bin/bash

echo -e "\n\nRunning the sender...\n\n"
python training_loop.py sender_pyinfer.json 2>&1 | tee sender_stdout.log

