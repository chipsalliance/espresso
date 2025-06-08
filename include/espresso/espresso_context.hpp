/**
 * @file espresso_context.hpp
 * @brief Modern C++23 EspressoContext for managing algorithm state and temporary buffers.
 *
 * This replaces the global variables from the legacy espresso implementation
 * with a context object that owns all temporary data and algorithm parameters.
 */
#pragma once

#include <espresso/bitcube.hpp>
#include <espresso/cube_algebra.hpp>
#include <memory>
#include <vector>
#include <cstdint>
#include <chrono>

namespace espresso {

/**
 * @brief Cost metrics for evaluating cover quality.
 */
struct CostMetrics {
    size_t cubes = 0;      ///< Number of cubes in the cover
    size_t literals = 0;   ///< Total number of literals
    size_t total = 0;      ///< Total cost (weighted sum)
    size_t out = 0;        ///< Output cost

    auto operator<=>(const CostMetrics&) const = default;
};

/**
 * @brief Configuration options for the Espresso algorithm.
 */
struct EspressoOptions {
    // Tracing and debugging
    bool trace = false;                    ///< Print trace information
    bool debug = false;                    ///< Print debug information
    
    // Algorithm behavior
    bool remove_essential = true;          ///< Remove essential primes
    bool single_expand = false;            ///< Stop after first expand/irredundant
    bool force_irredundant = false;        ///< Force minimal solution
    bool skip_make_sparse = false;         ///< Skip make_sparse step
    
    // Strategy options
    bool use_super_gasp = false;           ///< Use super_gasp instead of last_gasp
    bool recompute_onset = false;          ///< Recompute onset using complement
    bool unwrap_onset = true;              ///< Unwrap function output part
    
    // Iteration limits
    size_t max_iterations = 1000;         ///< Maximum algorithm iterations
    std::chrono::milliseconds timeout{0}; ///< Algorithm timeout (0 = no timeout)
    
    // Cost thresholds
    size_t large_output_threshold = 5000;  ///< Threshold for large output
};

/**
 * @brief Context object that manages Espresso algorithm state and temporary buffers.
 * 
 * This class replaces the global variables from the legacy implementation
 * and provides a clean interface for the modernized algorithm.
 */
template<size_t WIDTH>
class EspressoContext {
public:
    using CubeType = BitCube<WIDTH>;
    using CoverType = Cover<WIDTH>;
    
    explicit EspressoContext(const EspressoOptions& options = {})
        : options_(options), start_time_(std::chrono::steady_clock::now()) {}
    
    // Non-copyable but movable
    EspressoContext(const EspressoContext&) = delete;
    EspressoContext& operator=(const EspressoContext&) = delete;
    EspressoContext(EspressoContext&&) = default;
    EspressoContext& operator=(EspressoContext&&) = default;
    
    /**
     * @brief Get algorithm options.
     */
    [[nodiscard]] const EspressoOptions& options() const noexcept { return options_; }
    
    /**
     * @brief Get mutable algorithm options.
     */
    [[nodiscard]] EspressoOptions& options() noexcept { return options_; }
    
    /**
     * @brief Calculate cost metrics for a cover.
     */
    [[nodiscard]] CostMetrics calculate_cost(const CoverType& cover) const noexcept;
    
    /**
     * @brief Check if algorithm should timeout.
     */
    [[nodiscard]] bool should_timeout() const noexcept;
    
    /**
     * @brief Get elapsed time since context creation.
     */
    [[nodiscard]] std::chrono::milliseconds elapsed_time() const noexcept;
    
    /**
     * @brief Log trace information if tracing is enabled.
     */
    template<typename... Args>
    void trace(const std::string& format, Args&&... args) const;
    
    /**
     * @brief Log debug information if debugging is enabled.
     */
    template<typename... Args>
    void debug(const std::string& format, Args&&... args) const;
    
    /**
     * @brief Get a temporary cover for intermediate calculations.
     * 
     * Returns a reference to a temporary cover that can be used for
     * intermediate calculations. The cover is automatically cleared
     * when returned.
     */
    [[nodiscard]] CoverType& get_temp_cover();
    
    /**
     * @brief Get a temporary cube for intermediate calculations.
     */
    [[nodiscard]] CubeType& get_temp_cube();
    
    /**
     * @brief Release all temporary buffers back to the pool.
     */
    void release_temp_buffers();
    
    /**
     * @brief Get statistics about temporary buffer usage.
     */
    [[nodiscard]] size_t temp_covers_allocated() const noexcept { return temp_covers_.size(); }
    [[nodiscard]] size_t temp_cubes_allocated() const noexcept { return temp_cubes_.size(); }
    
private:
    EspressoOptions options_;
    std::chrono::steady_clock::time_point start_time_;
    
    // Temporary buffer pools
    mutable std::vector<std::unique_ptr<CoverType>> temp_covers_;
    mutable std::vector<std::unique_ptr<CubeType>> temp_cubes_;
    mutable size_t temp_cover_index_ = 0;
    mutable size_t temp_cube_index_ = 0;
    
    void ensure_temp_covers(size_t count) const;
    void ensure_temp_cubes(size_t count) const;
};

// Template implementation

template<size_t WIDTH>
CostMetrics EspressoContext<WIDTH>::calculate_cost(const CoverType& cover) const noexcept {
    CostMetrics cost;
    cost.cubes = cover.size();
    
    for (const auto& cube : cover) {
        cost.literals += cube.count();
    }
    
    // Simple cost function - can be made more sophisticated
    cost.total = cost.cubes + cost.literals;
    cost.out = cost.cubes; // Simplified output cost
    
    return cost;
}

template<size_t WIDTH>
bool EspressoContext<WIDTH>::should_timeout() const noexcept {
    if (options_.timeout.count() == 0) {
        return false; // No timeout set
    }
    
    return elapsed_time() >= options_.timeout;
}

template<size_t WIDTH>
std::chrono::milliseconds EspressoContext<WIDTH>::elapsed_time() const noexcept {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
}

template<size_t WIDTH>
template<typename... Args>
void EspressoContext<WIDTH>::trace(const std::string& format, Args&&... args) const {
    if (options_.trace) {
        // In a real implementation, you'd use a proper logging library
        // For now, just print to stdout
        printf("[TRACE] ");
        if constexpr (sizeof...(args) == 0) {
            printf("%s", format.c_str());
        } else {
            printf(format.c_str(), std::forward<Args>(args)...);
        }
        printf("\n");
    }
}

template<size_t WIDTH>
template<typename... Args>
void EspressoContext<WIDTH>::debug(const std::string& format, Args&&... args) const {
    if (options_.debug) {
        printf("[DEBUG] ");
        if constexpr (sizeof...(args) == 0) {
            printf("%s", format.c_str());
        } else {
            printf(format.c_str(), std::forward<Args>(args)...);
        }
        printf("\n");
    }
}

template<size_t WIDTH>
typename EspressoContext<WIDTH>::CoverType& EspressoContext<WIDTH>::get_temp_cover() {
    ensure_temp_covers(temp_cover_index_ + 1);
    auto& cover = *temp_covers_[temp_cover_index_++];
    cover.clear(); // Clear for reuse
    return cover;
}

template<size_t WIDTH>
typename EspressoContext<WIDTH>::CubeType& EspressoContext<WIDTH>::get_temp_cube() {
    ensure_temp_cubes(temp_cube_index_ + 1);
    auto& cube = *temp_cubes_[temp_cube_index_++];
    cube.reset(); // Clear for reuse
    return cube;
}

template<size_t WIDTH>
void EspressoContext<WIDTH>::release_temp_buffers() {
    temp_cover_index_ = 0;
    temp_cube_index_ = 0;
}

template<size_t WIDTH>
void EspressoContext<WIDTH>::ensure_temp_covers(size_t count) const {
    while (temp_covers_.size() < count) {
        temp_covers_.emplace_back(std::make_unique<CoverType>());
    }
}

template<size_t WIDTH>
void EspressoContext<WIDTH>::ensure_temp_cubes(size_t count) const {
    while (temp_cubes_.size() < count) {
        temp_cubes_.emplace_back(std::make_unique<CubeType>());
    }
}

} // namespace espresso
