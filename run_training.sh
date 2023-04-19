#!/bin/sh

rm training-log && ./kill.sh && python training_loop.py 2>&1 | tee training-log
