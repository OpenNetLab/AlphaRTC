import os
import shutil
import subprocess
import sys
import time

def main(num_calls):
    calls = int(num_calls)
    for call_idx in range(0, calls):
        # Copy rl_agent_file to ./modules/third_party/rl_agent/rl_agent.cc
        src = f'rl_agents/rl_agent{call_idx}.cc'
        dst = "modules/third_party/rl_agent/rl_agent.cc"
        shutil.copyfile(src, dst)

        # execute 'make all'
        build_cmd = 'make all && ls -la'
        # build_app = subprocess.Popen(build_cmd, shell=True)
        #build_app.wait()
        os.system(build_cmd)
        print(f'Building peerconnection app for call {call_idx} finished!')
        time.sleep(0.5)

        # execute 'mv peerconnection_serverless.origin peerconnection_serverless.origin{call_idx}'
        mv_cmd = f'mv peerconnection_serverless.origin peerconnection_serverless.origin{call_idx} && ls -la'
        # mv_app = subprocess.Popen(mv_cmd, shell=True)
        # mv_app.wait()
        os.system(mv_cmd)
        print(f'Renaming peerconnection app for call {call_idx} finished!')
        time.sleep(0.5)

if __name__ == '__main__':
    main(sys.argv[1])