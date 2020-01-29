#!/usr/bin/env python3

import argparse
from collections import defaultdict
from run_variance import find_time_range
import os.path as osp
import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--time-files", nargs="*", required=True)
    # parser.add_argument("-n", "--names", nargs="*", required=True)
    parser.add_argument("-q", "--qlen", type=int, default=5)
    parser.add_argument("-o", "--old", action="store_true", default=False)
    parser.add_argument("-m", "--metric", choices=["mean", "median", "min"],
                        default="mean")
    args = parser.parse_args()
    metric_map = {"mean": np.mean, "median": np.median, "min": np.min}
    metric = metric_map[args.metric]
    avg_times = dict()

    names = [osp.basename(tf).split("-")[0] for tf in args.time_files]
    load_func = load_tf_old if args.old else load_tf
    for i, tf in enumerate(args.time_files):
        all_runs = load_func(tf, metric)
        all_times = np.array([r.time_ms for r in all_runs])
        crr_name = names[i]
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

        if not args.old:
            avg_threshold_found = np.mean(
                [r.low_threshold / r.act_threshold for r
                 in all_runs if r.act_threshold > 0])
            # low_sum = np.sum(r.low_threshold for r in all_runs
            #                  if r.act_threshold > 0)
            # act_sum = np.sum(r.act_threshold for r in all_runs
            #                  if r.act_threshold > 0)
            # avg_threshold_found = low_sum / act_sum
            print("Average threshold ratio = {0:.3f}".format(
                avg_threshold_found))

    if len(names) == 2:
        first_avg = avg_times[names[0]]
        second_avg = avg_times[names[1]]

        print("Gain = {0:.3f}".format(abs(first_avg - second_avg) /
                                      max(first_avg, second_avg)))


class RunInfo:
    def __init__(self, num_terms, time_ms, low_threshold=0,
                 act_threshold=0):
        self.num_terms = num_terms
        self.time_ms = time_ms
        self.low_threshold = low_threshold
        self.act_threshold = act_threshold


def load_tf(time_file, metric):
    with open(time_file) as tf:
        tf_lines = tf.readlines()

    ts, te = find_time_range(tf_lines[0])
    all_runs = []
    for tl in tf_lines[1:]:
        tokens = tl.split(";")
        if not bool(int(tokens[te + 1])):
            time_ms = metric([float(t) for t in tokens[ts:te]])
            all_runs.append(RunInfo(int(tokens[6]), time_ms,
                                    float(tokens[te + 2]),
                                    float(tokens[te + 3])))

    return all_runs


def load_tf_old(time_file):
    with open(time_file) as tf:
        tf_lines = tf.readlines()

    all_runs = []
    for tl in tf_lines[1:]:
        tokens = tl.split(";")
        if int(tokens[1]) > 0:
            all_runs.append(RunInfo(int(tokens[6]), float(tokens[7])))

    return all_runs


if __name__ == "__main__":
    main()
