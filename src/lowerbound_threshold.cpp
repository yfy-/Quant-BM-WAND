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
    if (double threshold = cache.ScoreOf(s)) {
      exists = true;
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

double hr4_threshold_old(const query_t& query, const cache_t& cache) {
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

    if (double t = cache.ScoreOf(subset))
      return t;
  }

  return 0.0;
}

double bayes_threshold(const query_t& query, const cache_t& cache) {
  using term_score = std::pair<query_token, std::uint64_t>;
  std::vector<term_score> t_scores;

  // We need to compute normalized doc-freq and term-freq in cache
  for (auto& t : query.tokens)
    t_scores.push_back(std::make_pair(t, cache.FreqOf(t.token_str) * t.df));

  auto bys_sort = [&](const term_score& x, const term_score& y) -> bool {
                    return x.second > y.second;
                  };

  std::sort(t_scores.begin(), t_scores.end(), bys_sort);
  auto ss_end = t_scores.size() - 1;
  std::vector<std::string> s_terms;
  for (int i = 0; i < ss_end; ++i) {
    if (t_scores[i].second)
      s_terms.push_back(t_scores[i].first.token_str);
    else
      ss_end = i;
  }

  if (!s_terms.size())
    return 0.0;

  do {
    auto start = s_terms.begin();
    std::vector<std::string> s_terms_srt(start, start + ss_end);
    std::sort(s_terms_srt.begin(), s_terms_srt.end());
    std::string subset = s_terms_srt[0];
    for (auto s = s_terms_srt.begin() + 1, e = s_terms_srt.end(); s != e; ++s)
      subset += " " + *s;

    if (double t = cache.ScoreOf(subset))
      return t;

    ss_end--;
  } while (ss_end);

  return 0.0;
}

double hr4_threshold(const query_t& query, const cache_t& cache) {
  std::vector<query_token> tokens = query.tokens;
  std::vector<query_token> tokens_df_sorted(tokens.begin(), tokens.end());

  // Sort descending by doc freq
  std::sort(std::begin(tokens_df_sorted), std::end(tokens_df_sorted),
            [](const query_token& a, const query_token& b) -> bool {
              return a.df > b.df;
            });

  std::size_t max_subset_len = std::min<std::size_t>(
      3, tokens_df_sorted.size() - 1);

  auto tdf_begin = tokens_df_sorted.begin();
  std::vector<query_token> max_subset(tdf_begin, tdf_begin + max_subset_len);
  std::sort(max_subset.begin(), max_subset.end());
  std::vector<std::string> subsets;
  for (int i = max_subset_len; i > 0; --i) {
    std::vector<std::string> subsets_i = gen_subsets(max_subset, i);
    subsets.insert(subsets.end(), subsets_i.begin(), subsets_i.end());
  }

  double threshold = 0.0;
  subset_max_exists(subsets, threshold, cache);
  return threshold;
}

double all_threshold(const query_t& query, const cache_t& cache) {
  std::vector<query_token> tokens = query.tokens;
  std::vector<std::string> subsets;
  std::size_t start_len_subset = tokens.size() - 1;

  for (std::size_t i = start_len_subset; i > 0; --i) {
    std::vector<std::string> ilen_subsets = gen_subsets(tokens, i);
    subsets.insert(subsets.end(), ilen_subsets.begin(), ilen_subsets.end());
  }

  double threshold = 0.0;
  subset_max_exists(subsets, threshold, cache);
  return threshold;
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

  for (int i = std::min(len_tokens - 1, 3); i > 1; --i) {
    std::vector<std::string> subsets_i = gen_subsets(tokens, i);
    subsets.insert(subsets.end(), subsets_i.begin(), subsets_i.end());
  }

  subset_max_exists(subsets, max_threshold, cache);
  return max_threshold;
}
