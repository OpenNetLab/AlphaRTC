import random
import subprocess
import logging
logging.basicConfig(filename='step_obs_reward_action.log', encoding='utf-8', level=logging.INFO)

# Hello World client in Python
# Connects REQ socket to tcp://localhost:5555
# Sends "Hello" to server, expects "World" back
import zmq

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

def relay_packet_stats(ifd, ofd, env):
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
            env.packet_record.add_loss_rate(stats_dict['loss_rate'])
            env.packet_record.add_rtt(stats_dict['rtt'])
            env.packet_record.add_delay_interval()
            env.packet_record.add_receiver_side_thp(stats_dict['receiver_side_thp'])
            continue

        ofd.write(line)
        ofd.flush()

    # context = zmq.Context()

    # # Socket to talk to server
    # logging.info('Connecting to hello world server...')
    # socket = context.socket(zmq.REQ)
    # socket.connect("tcp://localhost:5555")

    # # Do 10 requests, waiting each time for a response
    # for request in range(10):
    #     logging.info(f'Sending {request}th request')
    #     socket.send("Hello")

    #     # Get the reply.
    #     message = socket.recv()
    #     logging.info(f'Received {request}th reply: {message}')

def check_call_result(receiver_app, sender_app):
    # Log whether the call ended successfully
    call_result = ''
    if receiver_app.returncode == 0 and sender_app.returncode == 0:
        call_result = f'Call finished successfully!\n'
    else:
        call_result = f'Call finished with errors! \
            receiver\'s return code {receiver_app.returncode} \
            sender\'s return code {sender_app.returncode}\n'

    with open("call_result.log", "a+") as out:
        out.write(call_result)

'''
Generate a random free tcp6 port.
Goal: dynamically binding an unused port for e2e call
'''
def generate_random_port():
    MIN_PORT = 1024
    MAX_PORT = 65535

    used_ports = []
    ret_port = -1

    out = subprocess.check_output('netstat -tnlp | grep tcp6', shell=True)
    lines = out.decode("utf-8").split("\n")
    # Figure out all the used ports
    for line in lines:
        # Proto    Recv-Q    Send-Q    Local Address    Foreign Address    State    PID/Program name
        line_elements = line.split()
        if len(line_elements) > 4:
            local_address = line.split()[3] # e.g., ::1:39673 :::22
            port = int(local_address.split(':')[-1])
            used_ports.append(port)

    while(ret_port < 0 or ret_port in used_ports):
        ret_port = random.randint(MIN_PORT, MAX_PORT)

    # Write the free port as a file
    with open("free_port", "w") as out:
        out.write(str(ret_port))

    with open("port_assignment.log", "a+") as out:
        out.write(str(ret_port)+"\n")


def run_call(env, link_bandwidth, delay):
    logging.info(f'run_call: starting')
    # `peerconnection_serverless.origin` is an e2e WebRTC app built by examples/BUILD.gn
    # Run this e2e app on separate processes in parallel
    # with AlphaCC config file as an argument (e.g. receiver_pyinfer.json)
    receiver_cmd = f"$ALPHARTC_HOME/peerconnection_serverless.origin receiver_pyinfer.json"
    sender_cmd = f"sleep 5; mm-link traces/{link_bandwidth} traces/{link_bandwidth} $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer.json"

    # Randomly assign different port for this video call
    generate_random_port()

    # Run the video call
    receiver_app = subprocess.Popen(receiver_cmd, shell=True)
    sender_app = subprocess.Popen(sender_cmd, bufsize=1, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    logging.info(f'Running a video call env: link BW {link_bandwidth}, one-way delay {delay}ms')
    relay_packet_stats(sender_app.stdout, sender_app.stdin, env)

    receiver_app.wait()
    sender_app.wait()

    check_call_result(receiver_app, sender_app)
    logging.info('run_call: ended')


# def run_sender(env, link_bandwidth):
#     logging.info(f'sender_wrapper main arg: {env}')

#     # sender_app = subprocess.Popen(
#     #     [os.path.join(os.path.dirname(__file__), 'peerconnection_serverless.origin')] + ['sender_pyinfer.json'],
#     #     bufsize=1,
#     #     stdin=subprocess.PIPE,
#     #     stdout=subprocess.PIPE,
#     #     stderr=subprocess.STDOUT)
#     logging.info(f'sender_wrapper: starting')
#     # sender_cmd = f"sleep 5; mm-link traces/{link_bandwidth} traces/{link_bandwidth} $ALPHARTC_HOME/peerconnection_serverless.origin sender_pyinfer.json"
#     # sender_app = subprocess.Popen(sender_cmd, bufsize=1, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
#     run_call(env, link_bandwidth)
#     logging.info('sender_wrapper: ended')
