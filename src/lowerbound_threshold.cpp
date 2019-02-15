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

  // Normally the interval would be inclusive
  // but for practical reasons it is exclusive
  if (r > 0 && r < n) {
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

  std::vector<std::string> subsets3 = gen_subsets(tokens, 3);

  if (subset_max_exists(subsets3, threshold, cache))
    return threshold;

  std::vector<std::string> subsets2 = gen_subsets(tokens, 2);

  if (subset_max_exists(subsets2, threshold, cache))
    return threshold;

  std::vector<std::string> subsets1 = gen_subsets(tokens, 1);
  subset_max_exists(subsets1, threshold, cache);

  return threshold;
}

double hr2_threshold(const query_t& query, const cache_t& cache) {
  double threshold = 0.0;
  std::vector<query_token> tokens = query.tokens;
  std::vector<std::string> subsets = gen_subsets(tokens, 3);
  std::vector<std::string> subsets2 = gen_subsets(tokens, 2);

  subsets.insert(std::end(subsets), std::begin(subsets2), std::end(subsets2));

  std::vector<std::string> subsets1 = gen_subsets(tokens, 1);

  subsets.insert(std::end(subsets), std::begin(subsets1), std::end(subsets1));
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
