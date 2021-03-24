
class Estimator(object):
    def report_states(self, stats: dict):
        pass

    def get_estimated_bandwidth(self)->int:
        return int(1e6) # 1Mbps
