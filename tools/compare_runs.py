#!/usr/bin/env python3

import argparse
from collections import defaultdict
import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--time-files", nargs="*", required=True)
    parser.add_argument("-n", "--names", nargs="*", required=True)
    parser.add_argument("-q", "--qlen", type=int, default=5)
    args = parser.parse_args()
    avg_times = dict()

    for i, tf in enumerate(args.time_files):
        all_runs = load_tf(tf)
        all_times = np.array([r.time_ms for r in all_runs])
        crr_name = args.names[i]
        avg_times[crr_name] = np.mean(all_times)
        print("Run results for {}".format(crr_name))
        print("Mean = {0:.3f}".format(avg_times[crr_name]))
        print("Median = {0:.3f}".format(np.median(all_times)))
        print("P95 = {0:.3f}".format(np.percentile(all_times, 95)))
        print("P99 = {0:.3f}".format(np.percentile(all_times, 99)))

        times_grouped = defaultdict(list)
        for r in all_runs:
            if r.num_terms >= args.qlen:
                times_grouped[args.qlen].append(r.time_ms)
            else:
                times_grouped[r.num_terms].append(r.time_ms)
        avg_by_qlen = args.qlen * [None]

        for i in range(1, args.qlen + 1):
            avg_by_qlen[i - 1] = np.mean(times_grouped[i])

        avg_times_str = ["{0:.3f}".format(aq) for aq in avg_by_qlen]
        print("Mean by num_terms = {}".format(avg_times_str))

        avg_threshold_found = np.mean([r.low_threshold / r.act_threshold for r
                                       in all_runs if r.act_threshold > 0])
        print("Average threshold ratio = {0:.3f}".format(avg_threshold_found))

    if len(args.names) == 2:
        first_avg = avg_times[args.names[0]]
        second_avg = avg_times[args.names[1]]

        print("Gain = {0:.3f}".format(abs(first_avg - second_avg) /
                                      max(first_avg, second_avg)))

class RunInfo:
    def __init__(self, num_terms, time_ms, low_threshold,
                 act_threshold):
        self.num_terms = num_terms
        self.time_ms = time_ms
        self.low_threshold = low_threshold
        self.act_threshold = act_threshold


def load_tf(time_file):
    with open(time_file) as tf:
        tf_lines = tf.readlines()

    all_runs = []
    for tl in tf_lines[1:]:
        tokens = tl.split(";")
        if not bool(int(tokens[9])):
            all_runs.append(RunInfo(int(tokens[6]), float(tokens[7]),
                                    int(tokens[10]), int(tokens[11])))

    return all_runs

if __name__ == "__main__":
    main()
