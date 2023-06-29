
class Estimator(object):
    def __init__(self):
        self.cnt = 0
        self.bandwidths = [1e6, 2e6]

    def report_states(self, stats: dict):
        '''
        stats is a dict with the following items
        {
            "send_time_ms": uint,
            "arrival_time_ms": uint,
            "payload_type": int,
            "sequence_number": uint,
            "ssrc": int,
            "padding_length": uint,
            "header_length": uint,
            "payload_size": uint
        }
        '''
        pass

    def get_estimated_bandwidth(self)->int:
        bandwidth = self.bandwidths[self.cnt % len(self.bandwidths)]
        self.cnt += 1
        return bandwidth
