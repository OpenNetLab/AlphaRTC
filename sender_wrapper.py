import json
import os
import subprocess
import sys
import traceback
import logging
logging.basicConfig(filename='step_obs_reward_action.log', encoding='utf-8', level=logging.INFO)

def fetch_stats(line):
    line = line.strip()
    if 'AlphaCC: SendState' in line:
        lrdr = line.split(',')
        stats_dict = {}
        stats_dict['loss_rate'] = float(lrdr[1].split(' ')[1])
        stats_dict['rtt'] = float(lrdr[2].split(' ')[1])
        stats_dict['delay_interval'] = float(lrdr[3].split(' ')[1])
        stats_dict['recv_thp'] = float(lrdr[4].split(' ')[1])
        logging.info(f'fetch_stats: {stats_dict}')
        return stats_dict
    else:
        return None

def sender_wrapper(ifd, ofd):
    while True:
        # Read from sender_app.stdout
        line = ifd.readline()
        if not line:
            break
        if isinstance(line, bytes):
            line = line.decode("utf-8")

        # Extract packet statistics
        stats_dict = fetch_stats(line)
        if stats_dict:
            with open('packet_stats.json', 'w') as f:
                json.dump(stats_dict, f)
            continue

        ofd.write(line)
        ofd.flush()

def main():
    try:
        logging.info(f'sender_wrapper: starting')
        sender_app = subprocess.Popen(
            [os.path.join(os.path.dirname(__file__), 'peerconnection_serverless.origin')] + ['sender_pyinfer.json'],
            bufsize=1,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)
        sender_wrapper(sender_app.stdout, sender_app.stdin)
        sender_app.wait()
        logging.info('sender_wrapper: ended')
    except:
        sender_app.terminate()
        sender_app.wait()
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


if __name__ == '__main__':
    main()
