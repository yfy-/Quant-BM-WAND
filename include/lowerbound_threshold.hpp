#ifndef LOWERBOUND_THRESHOLD_HPP
#define LOWERBOUND_THRESHOLD_HPP

#include <unordered_map>
#include "query.hpp"
#include "score_cache.hpp"

typedef ScoreCache cache_t;

// Naive threshold, always return 0
double naive_threshold(const query_t& query, const cache_t& cache);

/* Heuristic-1
 * Generate 3 length subsets and return max of them if one of the subsets is
 * in the cache. Otherwise repeat for 2 and 1
 */
double hr1_threshold(const query_t& query, const cache_t& cache);

/* Heuristic-2
 * Generate 3, 2 and 1 length subsets and return max of them if in cache.
 */
double hr2_threshold(const query_t& query, const cache_t& cache);

/* Heuristic-3
 * Generate n-1 subsets and return max of them if in cache.
 */
double hr3_threshold(const query_t& query, const cache_t& cache);

/* Heuristic-4
 * Generate subsets in decreased order by the token document frequency
 * Return first found, since it is guaranteed to be maximum of remaining to be
 * generated subsets
 */
double hr4_threshold(const query_t& query, const cache_t& cache);

double bayes_threshold(const query_t& query, const cache_t& cache);

double all_threshold(const query_t& query, const cache_t& cache);

double ts_threshold(const query_t& query, const cache_t& cache,
                    const cache_t& term_cache);

/* Heuristic-1 with Term cache
   Use Heuristic-1 with an additional term cache where terms score are
   pre-computed and are in a separate cache.
 */
double hr1_ts_threshold(const query_t& query, const cache_t& cache,
                        const cache_t& term_cache);

double hr2_ts_threshold(const query_t& query, const cache_t& cache,
                        const cache_t& term_cache);

#endif  // LOWERBOUND_THRESHOLD_HPP
