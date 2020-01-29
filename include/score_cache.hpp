// Created by yfy at January 26, 2020

#ifndef SCORECACHE_HPP
#define SCORECACHE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

class ScoreCache {
  std::unordered_map<std::string, double> cache_;
  std::unordered_map<std::string, std::uint32_t> term_freq_;

 public:
  void Insert(const std::string& query, double score);

  void Clear();

  // Returns 0.0 if the query does not exists
  double ScoreOf(const std::string& query) const;

  std::uint32_t FreqOf(const std::string& term) const;
};

#endif // SCORECACHE_HPP
