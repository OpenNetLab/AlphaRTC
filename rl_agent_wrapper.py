import json
import os
import subprocess
import sys
import time
import traceback

import rl_training.rl_agent as rl_agent


def main():
    e2e_app = os.path.join(os.path.dirname(__file__), 'peerconnection_serverless.origin')
    # config_file = 'sender_pyinfer-30sec-480x270.json'
    config_file = 'sender_pyinfer.json'
    app = subprocess.Popen(
        [e2e_app] + [config_file],
        bufsize=1,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT)
    try:
        print(f'Starting the training loop (rl_agent.main)')
        rl_agent.main(app.stdout, app.stdin)
        print('Finished the training loop (rl_agent.main)')
        app.wait()
    except:
        app.terminate()
        app.wait()
        error_message = traceback.format_exc()
        error_message = "\n{}".format(error_message)
        sys.stderr.write(error_message)
        if len(sys.argv[1:]) == 0:
            return
        config_file = json.load(open(config_file, "r"))
        if "logging" not in config_file:
            return
        if "enabled" not in config_file["logging"] or not config_file["logging"]["enabled"]:
            return
        with open(config_file["logging"]["log_output_path"], "a") as log_file:
            log_file.write(error_message)
    time.sleep(10)


if __name__ == "__main__":
    main()
