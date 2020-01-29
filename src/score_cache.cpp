// Created by yfy at January 26, 2020

#include <cstdint>
#include <string>
#include <sstream>
#include "score_cache.hpp"


void ScoreCache::Insert(const std::string& query, double score) {
  cache_[query] = score;
  std::istringstream qss(query);
  std::string term;
  while (std::getline(qss, term, ' '))
    term_freq_[term]++;
}

double ScoreCache::ScoreOf(const std::string& query) const {
  auto found = cache_.find(query);
  return found == cache_.end() ? 0.0 : found->second;
}

std::uint32_t ScoreCache::FreqOf(const std::string& term) const {
  auto found = term_freq_.find(term);
  return found == term_freq_.end() ? 0 : found->second;
}

void ScoreCache::Clear() {
  cache_.clear();
  term_freq_.clear();
}
