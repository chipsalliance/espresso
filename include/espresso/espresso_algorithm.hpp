/**
 * @file espresso_algorithm.hpp
 * @brief Modern C++23 implementation of the core Espresso minimization algorithm.
 *
 * This implements the main espresso() function and the expand-reduce-irredundant loop
 * using modern C++23 features and the EspressoContext for state management.
 */
#pragma once

#include <espresso/espresso_context.hpp>
#include <espresso/cube_algebra.hpp>
#include <optional>
#include <concepts>

namespace espresso::algorithm {

/**
 * @brief Core Espresso minimization algorithm.
 * 
 * This is the main entry point for the Espresso logic minimization algorithm.
 * It implements the expand-reduce-irredundant loop with essential prime extraction
 * and various optimization strategies.
 * 
 * @param context Algorithm context containing options and temporary buffers
 * @param F ON-set of the function to minimize
 * @param D Don't-care set (optional)
 * @param R OFF-set (for expansion bounds)
 * @return Minimized cover representing the same logic function
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> espresso(EspressoContext<WIDTH>& context,
                                   const Cover<WIDTH>& F,
                                   const Cover<WIDTH>& D = {},
                                   const Cover<WIDTH>& R = {});

/**
 * @brief Expand operation - grow cubes to cover more minterms.
 * 
 * Each cube in the cover is expanded to a larger cube (prime implicant)
 * that covers more minterms of the ON-set while staying disjoint from the OFF-set.
 * 
 * @param context Algorithm context
 * @param F Cover to expand
 * @param R OFF-set (expansion bounds)
 * @param nonsparse If true, only expand non-sparse variables
 * @return Expanded cover
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> expand(EspressoContext<WIDTH>& context,
                                 const Cover<WIDTH>& F,
                                 const Cover<WIDTH>& R,
                                 bool nonsparse = false);

/**
 * @brief Reduce operation - shrink cubes while maintaining coverage.
 * 
 * Each cube is reduced to the smallest cube that can replace it in the cover
 * without changing the logic function. This explores larger regions of the
 * optimization space.
 * 
 * @param context Algorithm context
 * @param F Cover to reduce
 * @param D Don't-care set
 * @return Reduced cover
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> reduce(EspressoContext<WIDTH>& context,
                                 const Cover<WIDTH>& F,
                                 const Cover<WIDTH>& D);

/**
 * @brief Irredundant operation - remove redundant cubes.
 * 
 * Identifies and removes cubes that are redundant (covered by other cubes)
 * to create a minimal cover.
 * 
 * @param context Algorithm context
 * @param F Cover to make irredundant
 * @param D Don't-care set
 * @return Irredundant cover
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> irredundant(EspressoContext<WIDTH>& context,
                                      const Cover<WIDTH>& F,
                                      const Cover<WIDTH>& D);

/**
 * @brief Extract essential prime implicants.
 * 
 * Essential primes are cubes that must be included in any minimal cover.
 * They are extracted separately and added back at the end.
 * 
 * @param context Algorithm context
 * @param F Cover to extract essentials from (modified in place)
 * @param D Don't-care set (modified in place)
 * @return Essential prime implicants
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> essential(EspressoContext<WIDTH>& context,
                                    Cover<WIDTH>& F,
                                    Cover<WIDTH>& D);

/**
 * @brief Last gasp optimization strategy.
 * 
 * Performs final optimization attempts to improve the solution by trying
 * various perturbations and local optimizations.
 * 
 * @param context Algorithm context
 * @param F Current cover
 * @param D Don't-care set
 * @param R OFF-set
 * @return Optimized cover
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> last_gasp(EspressoContext<WIDTH>& context,
                                    const Cover<WIDTH>& F,
                                    const Cover<WIDTH>& D,
                                    const Cover<WIDTH>& R);

/**
 * @brief Make sparse optimization.
 * 
 * Attempts to create a sparse PLA representation by minimizing the number
 * of literals in the final cover.
 * 
 * @param context Algorithm context
 * @param F Cover to optimize
 * @param D Don't-care set
 * @param R OFF-set
 * @return Sparse-optimized cover
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> make_sparse(EspressoContext<WIDTH>& context,
                                      const Cover<WIDTH>& F,
                                      const Cover<WIDTH>& D,
                                      const Cover<WIDTH>& R);

/**
 * @brief Sort cubes for optimal reduction ordering.
 * 
 * Orders cubes to optimize the reduction process using distance and size metrics.
 * 
 * @param context Algorithm context
 * @param F Cover to sort
 * @return Sorted cover
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> sort_reduce(EspressoContext<WIDTH>& context,
                                      const Cover<WIDTH>& F);

/**
 * @brief Sort cubes using column sum heuristics.
 * 
 * Orders cubes based on their correlation with column sums for better optimization.
 * 
 * @param context Algorithm context
 * @param F Cover to sort
 * @param ascending Sort in ascending order if true, descending if false
 * @return Sorted cover
 */
template<size_t WIDTH>
[[nodiscard]] Cover<WIDTH> mini_sort(EspressoContext<WIDTH>& context,
                                    const Cover<WIDTH>& F,
                                    bool ascending = true);

} // namespace espresso::algorithm
