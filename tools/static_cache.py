#!/usr/bin/env python3

import argparse

parser = argparse.ArgumentParser("Create static cache file")
parser.add_argument("trec_file", help="TREC output of processed queries")
parser.add_argument("query_file", help="Query file")
parser.add_argument("out_file", help="Out file")
parser.add_argument("-k", "--top-k", type=int, help="Cache k'th document")
parser.add_argument("-m", "--merge", help="Merge with a previous cache file")
args = parser.parse_args()

query_threshold = dict()

if args.merge:
    with open(args.merge) as mf:
        merge_cache_lines = mf.readlines()

    for mcl in merge_cache_lines:
        q, t = mcl.rstrip().split(";")
        query_threshold[q] = t

trec_tokens = None

with open(args.trec_file) as tf, open(args.query_file) as qf:
    q_count = 1
    for qf_line in qf:
        if q_count % 10000 == 0:
            print("Processed {0}\r".format(q_count), end="")

        q_count += 1
        query_id, query = qf_line.rstrip().split(';')
        score = 0.0

        while True:
            if trec_tokens is None:
                trec_tokens = tf.readline().split('\t')

            read_id = trec_tokens[0]

            if read_id != query_id:
                break

            rank = int(trec_tokens[3])

            if rank == args.top_k:
                score = float(trec_tokens[4])

            trec_tokens = None

        if score > 0.0:
            query_threshold[query] = score

with open(args.out_file, "w") as of:
    for query, threshold in query_threshold.items():
        of.write("{};{}\n".format(query, threshold))
