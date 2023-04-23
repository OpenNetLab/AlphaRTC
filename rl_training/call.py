import json
import random
import subprocess


'''
- link_bandwidths: a list of link bandwidth (mm-link)
- delays: a list of one-way delay (mm-delay)
'''
class Call:
    def __init__(self, link_bandwidths, delays):
        self.link_bandwidths = link_bandwidths
        self.delays = delays
        self.num_calls = len(link_bandwidths) * len(delays)

        # Call and network configs
        self.receiver_configs = {}  # 'receiver_pyinferX.json'
        self.sender_configs = {}    # 'sender_pyinferX.json'
        self.network_configs = {}

        # calls
        self.receiver_apps = {}
        self.sender_apps = {}

        self._init_configs()

    def _init_configs(self):
        # for call_idx in range(0, self.num_calls):
        call_idx = 0
        for link_bw in self.link_bandwidths:
            for delay in self.delays:
                # Call configs
                self.receiver_configs[call_idx] = f'receiver_pyinfer{call_idx}.json'
                self.sender_configs[call_idx] = f'sender_pyinfer{call_idx}.json'

                # Network configs
                self.network_configs[call_idx] = (link_bw, delay)

                call_idx += 1

        print(f'CALL self.network_configs {self.network_configs}')

    def _fetch_stats(self, line):
        line = line.strip()
        if 'AlphaCC: SendState' in line:
            lrdr = line.split(',')
            stats_dict = {}
            stats_dict['loss_rate'] = float(lrdr[1].split(' ')[1])
            stats_dict['rtt'] = float(lrdr[2].split(' ')[1])
            stats_dict['delay_interval'] = float(lrdr[3].split(' ')[1])
            stats_dict['recv_thp'] = float(lrdr[4].split(' ')[1])
            return stats_dict
        else:
            return None

    '''
    Generate a random free tcp6 port.
    Goal: dynamically binding an unused port for e2e call
    '''
    def _generate_random_port(self, call_idx):
        MIN_PORT = 1024
        MAX_PORT = 65535

        used_ports = []
        free_port = -1

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

        while(free_port < 0 or free_port in used_ports):
            free_port = random.randint(MIN_PORT, MAX_PORT)

        # Write the free port to the designated port file
        port_path = f'port/call{call_idx}.port'
        with open(port_path, "w") as out:
            out.write(str(free_port))

        with open('port_assignment.log', "a+") as out:
            out.write(f'Call {call_idx}: assigned {free_port} written to {port_path}\n')

        return port_path

    def check_result(self):
        for call_idx in range(0, self.num_calls):
            receiver_app = self.receiver_apps[call_idx]
            sender_app = self.sender_apps[call_idx]

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

    def create_and_run_call(self, call_idx):
        receiver_config = '$ALPHARTC_HOME/receiver_pyinfer.json'
        sender_config = '$ALPHARTC_HOME/sender_pyinfer.json'
        call_app = f'$ALPHARTC_HOME/peerconnection_serverless.origin{call_idx}'
        link_bandwidth = self.network_configs[call_idx][0]
        delay = self.network_configs[call_idx][1]

        # Randomly assign different port for this video call
        port_path = self._generate_random_port(call_idx)

        # Run the video call (env) in separate processes
        receiver_cmd = f"{call_app} {receiver_config} {port_path}"
        sender_cmd = f"sleep 5; mm-delay {delay} mm-link traces/{link_bandwidth} traces/{link_bandwidth} {call_app} {sender_config} {port_path}"
        receiver_app = subprocess.Popen(receiver_cmd, shell=True)
        sender_app = subprocess.Popen(sender_cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        self.receiver_apps[call_idx] = receiver_app
        self.sender_apps[call_idx] = sender_app
        print(f'CALL Video call started: Call {call_idx} link BW {link_bandwidth}, one-way delay {delay}ms app {call_app} configs {receiver_config} {sender_config}')

    def run_calls(self):
        # Create and run calls
        # for every combination of link bandwidth x delay
        for call_idx in range(0, self.num_calls):
            self.create_and_run_call(call_idx)

        return self.receiver_apps, self.sender_apps
