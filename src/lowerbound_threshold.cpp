#include <string>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <vector>
#include "lowerbound_threshold.hpp"

bool subset_max_exists(const std::vector<std::string>& subsets,
                       double& max_threshold, const cache_t& cache) {
  bool exists = false;

  for (const auto& s : subsets) {
    bool in_cache = cache.find(s) != cache.end();

    if (in_cache) {
      exists = true;
      double threshold = cache.at(s);
      if (threshold > max_threshold)
        max_threshold = threshold;
    }
  }

  return exists;
}

bool next_subset(std::vector<std::uint32_t>& ptrs,
                 const std::vector<std::uint32_t>& end_ptrs,
                 std::uint32_t r) {
  for (int i = r - 1; i > -1; i--) {
    if (ptrs[i] < end_ptrs[i]) {
      ptrs[i]++;
      std::uint32_t moved = ptrs[i];

      for (int j = i + 1; j < r; j++) {
        ptrs[j] = moved + 1;
        moved++;
      }

      return true;
    }
  }

  return false;
}

std::vector<std::string> gen_subsets(const std::vector<query_token>& tokens,
                                     std::uint32_t r) {
  std::uint32_t n = tokens.size();
  std::vector<std::string> subsets;

  if (r <= n && r > 0) {
    std::vector<std::uint32_t> ptrs(r);
    std::iota(std::begin(ptrs), std::end(ptrs), 0);

    std::vector<std::uint32_t> end_ptrs(r);
    std::iota(std::begin(end_ptrs), std::end(end_ptrs), n - r);

    do {
      std::string subset = tokens[ptrs[0]].token_str;

      for (int i = 1; i < r; i++)
        subset += " " + tokens[ptrs[i]].token_str;

      subsets.push_back(subset);

    } while (next_subset(ptrs, end_ptrs, r));
  }

  return subsets;
}

double naive_threshold(const query_t& query, const cache_t& cache) {
  return 0.0;
}

double hr1_threshold(const query_t& query, const cache_t& cache) {
  double threshold = 0.0;
  std::vector<query_token> tokens = query.tokens;
  int len_tokens = tokens.size();

  if (len_tokens > 3) {
    if (subset_max_exists(gen_subsets(tokens, 3), threshold, cache))
      return threshold;
  }

  if (len_tokens > 2) {
    if (subset_max_exists(gen_subsets(tokens, 2), threshold, cache))
      return threshold;
  }

  if (len_tokens > 1)
    subset_max_exists(gen_subsets(tokens, 1), threshold, cache);

  return threshold;
}

double hr2_threshold(const query_t& query, const cache_t& cache) {
  double threshold = 0.0;
  std::vector<query_token> tokens = query.tokens;
  int len_tokens = tokens.size();
  std::vector<std::string> subsets;

  if (len_tokens > 3) {
    std::vector<std::string> subsets3 = gen_subsets(tokens, 3);
    subsets.insert(subsets.end(), subsets3.begin(), subsets3.end());
  }

  if (len_tokens > 2) {
    std::vector<std::string> subsets2 = gen_subsets(tokens, 2);
    subsets.insert(subsets.end(), subsets2.begin(), subsets2.end());
  }

  if (len_tokens > 1) {
    std::vector<std::string> subsets1 = gen_subsets(tokens, 1);
    subsets.insert(subsets.end(), subsets1.begin(), subsets1.end());
  }

  subset_max_exists(subsets, threshold, cache);
  return threshold;
}

double hr3_threshold(const query_t& query, const cache_t& cache) {
  double threshold = 0.0;
  std::vector<query_token> tokens = query.tokens;
  std::vector<std::string> subsets = gen_subsets(tokens, tokens.size() - 1);
  subset_max_exists(subsets, threshold, cache);
  return threshold;
}

double hr4_threshold(const query_t& query, const cache_t& cache) {
  std::vector<query_token> tokens = query.tokens;
  std::vector<query_token> tokens_df_sorted(std::begin(tokens),
                                            std::end(tokens));
  // Sort descending by doc freq
  std::sort(std::begin(tokens_df_sorted), std::end(tokens_df_sorted),
            [](const query_token& a, const query_token& b) -> bool {
              return a.df > b.df;
            });
  int tokens_len = tokens.size();
  for (int i = tokens_len - 1; i >= 1; i--) {
    auto tok_head = tokens_df_sorted.begin();
    std::vector<query_token> sub_tokens(tok_head, tok_head + i);
    std::sort(sub_tokens.begin(), sub_tokens.end());
    std::string subset = sub_tokens[0].token_str;

    for (int j = 1; j < sub_tokens.size(); j++)
      subset += " " + sub_tokens[j].token_str;

    if (cache.find(subset) != cache.end())
      return cache.at(subset);
  }

  return 0.0;
}

double ts_threshold(const query_t& query, const cache_t& cache,
                    const cache_t& term_cache) {
  std::vector<std::string> subsets1 = gen_subsets(query.tokens, 1);
  double max_threshold = 0.0;
  subset_max_exists(subsets1, max_threshold, term_cache);
  return max_threshold;
}

double hr1_ts_threshold(const query_t& query, const cache_t& cache,
                        const cache_t& term_cache) {
  double max_threshold = ts_threshold(query, cache, term_cache);

  std::vector<query_token> tokens = query.tokens;
  int len_tokens = tokens.size();

  if (len_tokens > 3) {
    std::vector<std::string> subsets3 = gen_subsets(tokens, 3);
    if (subset_max_exists(subsets3, max_threshold, cache))
      return max_threshold;
  }

  if (len_tokens > 2) {
    std::vector<std::string> subsets2 = gen_subsets(tokens, 2);
    subset_max_exists(subsets2, max_threshold, cache);
  }

  return max_threshold;
}

double hr2_ts_threshold(const query_t& query, const cache_t& cache,
                        const cache_t& term_cache) {
  double max_threshold = ts_threshold(query, cache, term_cache);

  std::vector<query_token> tokens = query.tokens;
  int len_tokens = tokens.size();
  std::vector<std::string> subsets;

  if (len_tokens > 3) {
    std::vector<std::string> subsets3 = gen_subsets(tokens, 3);
    subsets.insert(subsets.end(), subsets3.begin(), subsets3.end());
  }

  if (len_tokens > 2) {
    std::vector<std::string> subsets2 = gen_subsets(tokens, 2);
    subsets.insert(subsets.end(), subsets2.begin(), subsets2.end());
  }

  subset_max_exists(subsets, max_threshold, cache);
  return max_threshold;
}
