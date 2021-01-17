#!/usr/bin/env python3

import argparse
import numpy as np
import sys


def find_time_range(column_line):
    columns = column_line.split(";")
    time_start = -1
    for i, c in enumerate(columns):
        if c.startswith("time_ms"):
            if time_start == -1:
                time_start = i
        else:
            if time_start != -1:
                return time_start, i


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("time_file", nargs="?", default=sys.stdin,
                        type=argparse.FileType())
    parser.add_argument("out_file", nargs="?", default=sys.stdout,
                        type=argparse.FileType(mode="w"))
    args = parser.parse_args()
    with args.time_file as tf:
        tf_lines = tf.readlines()

    fs_line = tf_lines[0].rstrip()
    t_start, t_end = find_time_range(fs_line)
    all_times = []
    for tf_line in tf_lines[1:]:
        tf_line = tf_line.rstrip()
        times = np.array([float(t) for t in tf_line.split(";")[t_start:t_end]])
        all_times.append(times)

    avgs = np.mean(all_times, axis=0)
    print(avgs)
    print(f"Variance: {np.std(avgs)}")
    print(f"Average: {np.mean(avgs)}")
    return 0


if __name__ == "__main__":
    exit(main())
