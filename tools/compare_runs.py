#!/usr/bin/env python3

import argparse
from collections import defaultdict
from matplotlib import pyplot as plt
import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-t", "--time-files", nargs="*", required=True)
    parser.add_argument("-n", "--names", nargs="*", required=True)
    parser.add_argument("-q", "--qlen", type=int, default=20)
    parser.add_argument("out")
    args = parser.parse_args()

    to_csv = None

    for i, tf in enumerate(args.time_files):
        qlens, qcounts, avg_times = avg_time_qlen(tf, args.qlen)

        if to_csv is None:
            to_csv = np.hstack((v2cmtrx(qlens), v2cmtrx(qcounts),
                                v2cmtrx(avg_times)))
        else:
            to_csv = np.hstack((to_csv, v2cmtrx(avg_times)))

        plt.plot(qlens, avg_times, 'o', label=args.names[i])

    with open(args.out, "w") as of:
        of.write("query_length,")
        of.write("query_count")

        for name in args.names:
            of.write(",{}".format(name))

        of.write("\n")

        np.savetxt(of, to_csv, fmt="%.3f", delimiter=',')

    plt.xlabel("Query Lengths")
    plt.ylabel("Avg Time (ms)")
    plt.xticks(np.arange(1, args.qlen + 1))
    plt.legend()
    plt.show()


def v2cmtrx(vec):
    return np.reshape(vec, (vec.size, 1))


def avg_time_qlen(time_file, until_qlen):
    with open(time_file) as tf:
        time_file_lines = tf.readlines()

    qlen_to_tot_time = defaultdict(float)
    qlen_count = defaultdict(int)

    # First line is header
    for tfl in time_file_lines[1:]:
        # Only obtain query length and time
        qlen_str, time_str = tfl.split(';')[6: 8]
        qlen = int(qlen_str)
        time = float(time_str)
        if qlen and qlen <= until_qlen:
            qlen_to_tot_time[qlen] += time
            qlen_count[qlen] += 1

    qlens = []
    avg_times = []
    qcounts = []

    for qlen, qc in qlen_count.items():
        qlens.append(qlen)
        qcounts.append(qc)
        avg_times.append(qlen_to_tot_time[qlen] / qlen_count[qlen])

    qlens = np.array(qlens)
    avg_times = np.array(avg_times)
    qcounts = np.array(qcounts)
    qlen_sort_ind = qlens.argsort()

    return qlens[qlen_sort_ind], qcounts[qlen_sort_ind], \
        avg_times[qlen_sort_ind]


if __name__ == "__main__":
    main()
