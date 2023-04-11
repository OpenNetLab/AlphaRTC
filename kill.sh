ps -ef | grep 'training_loop.py' | grep -v grep | awk '{print $2}' | xargs -r kill -9

