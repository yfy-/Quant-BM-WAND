#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "query.hpp"
#include "invidx.hpp"
#include "generic_rank.hpp"
#include "impact.hpp"
#include "bm25.hpp"
#include "util.hpp"

typedef struct cmdargs {
  std::string collection_dir;
  std::string query_file;
  std::string postings_file;
  std::string doclen_file;
  std::string global_file;
  std::string output_prefix;
  std::string index_type_file;
  uint64_t k;
  double F_boost;
  query_traversal traversal;
  std::string traversal_string;
  std::string cache_file;
  std::string term_cache_file;
  bool dyn_cache;
  bool report_only_time;
  std::uint32_t num_runs;
  std::string threshold_method;
} cmdargs_t;

void print_usage(std::string program) {
  std::cerr << program << " -c <collection>"
            << " -q <query_file>"
            << " -k <no. items to retrieve>"
            << " -z <F: aggression parameter. 1.0 is rank-safe>"
            << " -o <output file handle>"
            << " -t <traversal type: AND|OR>"
            << " -f <static cache file>"
            << " -d <cache dynamically>"
            << " -r <only report time logs>"
            << " -n <number of runs>"
            << " -m <threshold method: HR1|HR2|HR3|HR4|ALL|TS|HR1_TS|HR2_TS|BAYES|BAYES_TS, default is NAIVE>"
            << " -e <term static cache file>"
            << std::endl;
  exit(EXIT_FAILURE);
}

cmdargs_t
parse_args(int argc, char* const argv[])
{
  cmdargs_t args;
  int op;
  args.collection_dir = "";
  args.output_prefix = "";
  args.traversal = UNKNOWN;
  args.traversal_string = "";
  args.k = 10;
  args.F_boost = 1.0;
  args.cache_file = "";
  args.term_cache_file = "";
  args.dyn_cache = false;
  args.report_only_time = false;
  args.num_runs = 3;
  args.threshold_method = "NAIVE";
  while ((op=getopt(argc,argv,"c:q:k:z:o:t:f:e:drn:m:")) != -1) {
    switch (op) {
      case 'c':
        args.collection_dir = optarg;
        args.postings_file = args.collection_dir + "/WANDbl_postings.idx";
        args.doclen_file = args.collection_dir +"/doc_lens.txt";
        args.global_file = args.collection_dir +"/global.txt";
        args.index_type_file = args.collection_dir + "/index_info.txt";
        break;
      case 'o':
        args.output_prefix = optarg;
        break;
      case 'q':
        args.query_file = optarg;
        break;
      case 'k':
        args.k = std::strtoul(optarg,NULL,10);
        break;
      case 'z':
        args.F_boost = atof(optarg);
        break;
      case 't':
        args.traversal_string = optarg;
        if (args.traversal_string == "OR")
          args.traversal = OR;
        else if (args.traversal_string == "AND")
          args.traversal = AND;
        else
          print_usage(argv[0]);
        break;
      case 'f':
        args.cache_file = optarg;
        break;
      case 'e':
        args.term_cache_file = optarg;
        break;
      case 'd':
        args.dyn_cache = true;
        break;
      case 'r':
        args.report_only_time = true;
        break;
      case 'n':
        args.num_runs = std::stoul(optarg);
        break;
      case 'm':
        args.threshold_method = optarg;
        break;
      case '?':
      default:
        print_usage(argv[0]);
    }
  }
  if (args.collection_dir=="" || args.query_file=="" || args.F_boost < 1 ||
      args.traversal == UNKNOWN) {
    std::cerr << "Missing/Incorrect command line parameters.\n";
    print_usage(argv[0]);
  }
  return args;
}

int
main (int argc,char* const argv[])
{
  /* define types */
  using plist_type = block_postings_list<128>;
  using my_index_t = idx_invfile<plist_type, generic_rank>;
  using clock = std::chrono::high_resolution_clock;

  /* parse command line */
  cmdargs_t args = parse_args(argc,argv);

  std::cout << "NOTE: Global F boost = " << args.F_boost << std::endl;

  // Read the index and traversal type
  std::ifstream read_type(args.index_type_file);
  std::string t_traversal, t_postings;
  read_type >> t_traversal;
  read_type >> t_postings;

  // Wand or BMW index?
  index_form t_index_type;
  if (t_traversal == STRING_WAND)
    t_index_type = WAND;
  else if (t_traversal == STRING_BMW)
    t_index_type = BMW;
  else {
    std::cerr << "Index is corrupted. Please rebuild." << std::endl;
    exit(EXIT_FAILURE);
  }

  // TF or a quant index?
  postings_form t_postings_type;
  if (t_postings == STRING_FREQ) {
    t_postings_type = FREQUENCY;
  }
  else if (t_postings == STRING_QUANT) {
    t_postings_type = QUANTIZED;
  }
  else {
    std::cerr << "Index is corrupted. Please rebuild." << std::endl;
    exit(EXIT_FAILURE);
  }


  /* parse queries */
  std::cout << "Parsing query file '" << args.query_file << "'" << std::endl;
  auto queries = query_parser::parse_queries(args.collection_dir,args.query_file);
  std::cout << "Found " << queries.size() << " queries." << std::endl;

  std::string index_name(basename(strdup(args.collection_dir.c_str())));

  /* load the index */
  my_index_t index;

  auto load_start = clock::now();
  // Construct index instance.
  construct(index, args.postings_file, args.F_boost);

  // Prepare Ranker
  uint64_t temp;

  std::vector<uint64_t> doc_lens;
  ifstream doclen_file(args.doclen_file);
  if(!doclen_file.is_open()){
    std::cerr << "Couldn't open: " << args.doclen_file << std::endl;
    exit(EXIT_FAILURE);
  }

  std::cout << "Reading document lengths." << std::endl;
  /*Read the lengths of each document from asc file into vector*/
  while(doclen_file >> temp){
    doc_lens.push_back(temp);
  }
  ifstream global_file(args.global_file);
  if(!global_file.is_open()) {
    std::cerr << "Couldn't open: " << args.global_file << std::endl;
    exit(EXIT_FAILURE);
  }
  // Load the ranker
  uint64_t total_docs, total_terms;
  global_file >> total_docs >> total_terms;
  index.load(doc_lens, total_terms, total_docs, t_postings_type);
  index.set_dyn_cache(args.dyn_cache);
  index.set_threshold_method(args.threshold_method);

  auto load_stop = clock::now();
  auto load_time_sec = std::chrono::duration_cast<std::chrono::seconds>(load_stop-load_start);
  std::cout << "Index loaded in " << load_time_sec.count() << " seconds." << std::endl;

  /* process the queries */
  std::map<uint64_t,std::vector<std::chrono::microseconds>> query_times;
  std::map<uint64_t,result> query_results;
  std::map<uint64_t,uint64_t> query_lengths;
  std::map<uint64_t, std::string> rewritten_queries;
  std::map<uint64_t, query_stat> query_stats;

  if (args.term_cache_file != "")
    index.load_term_cache(args.term_cache_file);

  for (size_t i = 0; i < args.num_runs; i++) {
    if (args.cache_file != "") {
      std::cout << "Loading static cache with " << args.cache_file << "\n";
      index.reset_cache();
      index.load_cache(args.cache_file);
    }

    // std::cout << "Query pass no " << i + 1 << std::endl;
    // For each query
    for(auto& query: queries) {
      uint64_t id = query.query_id;
      std::vector<query_token> qry_tokens = query.tokens;
      std::cerr << "[" << id << "] |Q|=" << qry_tokens.size();
      std::cerr.flush();

      query_stat stat;
      // run the query
      auto qry_start = clock::now();
      auto results = index.search(query, args.k, t_index_type, args.traversal,
                                  stat);
      auto qry_stop = clock::now();

      auto query_time = std::chrono::duration_cast<std::chrono::microseconds>(
          qry_stop - qry_start);
      std::cerr << " TIME = " << std::setprecision(5)
                << query_time.count() / 1000.0 << " ms\r";

      if (i == 0) {
        query_results[id] = results;
        query_lengths[id] = qry_tokens.size();
        query_stats[id] = stat;
        rewritten_queries[id] = query.query_str;
      }

      query_times[id].push_back(query_time);
    }
    std::cerr << "\n";
  }

  std::cout << "Cache hit rate is " << index.hit_rate() << "\n";
  std::cout << "Subset found rate is " << index.subset_found_rate() << "\n";


  // generate output string
  args.output_prefix = args.output_prefix + "-" // user specified
                       + t_postings + "-"  // quantized or frequency
                       + t_traversal + "-" // wand or bmw
                       + args.traversal_string + "-" // OR, AND, etc
                       + std::to_string(args.k) + "-" // no. results
                       + std::to_string(args.F_boost);

  std::string time_file = args.output_prefix + "-time.log";

  /* output */
  std::cout << "Writing timing results to '" << time_file << "'" << std::endl;
  std::ofstream resfs(time_file);
  if (resfs.is_open()) {
    resfs << "query;num_results;postings_eval;docs_fully_eval;"
        "docs_added_to_heap;threshold;num_terms;";
    for (size_t i = 0; i < args.num_runs; ++i)
      resfs << "time_ms" << i << ";";

    resfs << "traversal_type;cache_hit;low_threshold;act_threshold" <<
        std::endl;
    for (const auto& timing: query_times) {
      auto qry_id = timing.first;
      std::vector<std::chrono::microseconds> qry_times = timing.second;
      auto results = query_results[qry_id];
      query_stat stat = query_stats[qry_id];
      resfs << qry_id << ";" << results.list.size() << ";"
            << results.postings_evaluated << ";"
            << results.docs_fully_evaluated << ";"
            << results.docs_added_to_heap << ";"
            << results.final_threshold << ";"
            << query_lengths[qry_id] << ";";
      for (auto qt : qry_times)
        resfs << qt.count() / 1000.0 << ";";

      resfs << args.traversal_string << ";"
            << stat.cache_hit << ";"
            << stat.lowerbound_threshold << ";"
            << stat.actual_threshold << std::endl;
    }
  } else {
    perror ("Could not output results to file.");
  }

  // Write TREC output file.

  /* load the docnames map */
  std::unordered_map<uint64_t,std::string> id_mapping;
  std::string doc_names_file = args.collection_dir + "/doc_names.txt";
  std::ifstream dfs(doc_names_file);
  size_t j=0;
  std::string name_mapping;
  while( std::getline(dfs,name_mapping) ) {
    id_mapping[j] = name_mapping;
    j++;
  }

  if (!args.report_only_time) {
    std::string trec_file = args.output_prefix + "-trec.run";
    std::cout << "Writing trec output to " << trec_file << std::endl;
    std::ofstream trec_out(trec_file);
    if(trec_out.is_open()) {
      for(const auto& result: query_results) {
        auto qry_id = result.first;
        auto qry_res = result.second.list;
        for(size_t i=1;i<=qry_res.size();i++) {
          trec_out << qry_id << "\t"
                   << "Q0" << "\t"
                   << id_mapping[qry_res[i-1].doc_id] << "\t"
                   << i << "\t"
                   << qry_res[i-1].score << "\t"
                   << "WANDbl" << std::endl;
        }
      }
    } else {
      perror ("Could not output results to file.");
    }

    std::string rewrite_file = args.output_prefix + "-rewrite.qry";
    std::cout << "Writing rewritten queries to " << rewrite_file << std::endl;
    std::ofstream rewrite_out(rewrite_file);

    if (rewrite_out.is_open()) {
      for (const auto& rwrt: rewritten_queries)
        rewrite_out << rwrt.first << ";" << rwrt.second << "\n";
    } else {
      perror ("Could not output results to file.");
    }
  }


  return EXIT_SUCCESS;
}
