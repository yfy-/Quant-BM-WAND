#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <numeric>

using cache_t = std::unordered_map<std::string, double>;

std::pair<std::string, std::string> split(const std::string& qline) {
  std::size_t qstart = qline.find(";");
  std::string first = qline.substr(0, qstart);
  std::string second = qline.substr(qstart + 1);
  return {first, second};
}

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

std::vector<std::string> gen_subsets(const std::vector<std::string>& tokens,
                                     std::uint32_t r) {
  std::uint32_t n = tokens.size();
  std::vector<std::string> subsets;

  if (r <= n && r > 0) {
    std::vector<std::uint32_t> ptrs(r);
    std::iota(std::begin(ptrs), std::end(ptrs), 0);

    std::vector<std::uint32_t> end_ptrs(r);
    std::iota(std::begin(end_ptrs), std::end(end_ptrs), n - r);

    do {
      std::string subset = tokens[ptrs[0]];

      for (int i = 1; i < r; i++)
        subset += " " + tokens[ptrs[i]];

      subsets.push_back(subset);

    } while (next_subset(ptrs, end_ptrs, r));
  }

  return subsets;
}

double hr2_threshold(const std::vector<std::string>& tokens,
                     const cache_t& cache) {
  double threshold = 0.0;
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


int main(int argc, char* argv[]) {
  std::cout << "Loading queries\n";
  std::vector<std::vector<std::string>> query_tokens;
  std::vector<std::string> queries;
  std::ifstream qfile(argv[1]);
  if (qfile.is_open()) {
    std::string qline;
    while (std::getline(qfile, qline)) {
      std::pair<std::string, std::string> pair = split(qline);
      queries.push_back(pair.second);
      std::istringstream iss(pair.second);
      std::string token;
      std::vector<std::string> qtokens;
      while (std::getline(iss, token, ' '))
        qtokens.push_back(token);

      query_tokens.push_back(qtokens);
    }
  }

  cache_t cache;
  std::ifstream cfile(argv[2]);
  if (cfile.is_open()) {
    std::string cline;
    while (std::getline(cfile, cline)) {
      std::pair<std::string, std::string> pair = split(cline);
      cache[pair.first] = std::stod(pair.second);
    }
  }

  int processed = 0;
  auto tot_t = std::chrono::duration<double>::zero();

  for (int i = 0, s = queries.size(); i < s; ++i) {
    if (cache.find(queries[i]) != cache.end())
      continue;

    processed++;
    auto start = std::chrono::high_resolution_clock::now();
    double threshold = hr2_threshold(query_tokens[i], cache);
    auto end = std::chrono::high_resolution_clock::now();
    tot_t += end - start;
  }

  std::cout << "Avg processing time(ms): " <<
      std::chrono::duration_cast<std::chrono::microseconds>(tot_t).count() / processed / 1000.0 << "\n";
  return 0;
}
