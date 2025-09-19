
# pragma once

#include <cstddef>
#include <string>
#include "gs_patterns.h"

// symbol lookup options
#if !defined(SYMBOLS_ONLY)
#define SYMBOLS_ONLY 1 //Filter out instructions that have no symbol
#endif

#if !defined(VBITS)
# define VBITS (512L)
# define VBYTES (VBITS/8)
#endif

namespace gs_patterns
{
    class Config
    {
    public:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        static Config& get_instance()
        {
            static Config instance;
            return instance;
        }

        void parseArgs(int argc, char* argv[]);


        // expose compile-time choices for run time inspection
        static constexpr size_t vector_bits = VBITS;
        static constexpr size_t vector_bytes = VBYTES;
        static constexpr bool symbols_only = (SYMBOLS_ONLY != 0);


        // runtime arguments
        size_t get_per_sample() const { return _per_sample; }
        size_t get_cache_line_size() const { return _cache_line_size; }
        int64_t get_num_buffers() const { return _num_buffers; }
        size_t get_instruction_window() const { return _instruction_window; }
        size_t get_max_gather_scatter() const { return _max_gather_scatter; }
        size_t get_histogram_bounds() const { return _histogram_bounds; }
        size_t get_histogram_bounds_alloc() const { return 2 * _histogram_bounds + 3; }
        size_t get_unique_strides_threshold() const { return _unique_strides_threshold; }
        size_t get_num_unique_distances() const { return _num_unique_distances; }
        double get_out_threshold() const { return _out_threshold; }
        size_t get_top_patterns() const { return _top_patterns; }
        size_t get_initial_pattern_size() const { return _initial_pattern_size; }
        size_t get_max_pattern_size() const { return _max_pattern_size; }
        size_t get_max_line_length() const { return _max_line_length; }

        void set_per_sample(size_t per_sample) {
            if (per_sample < MIN_PER_SAMPLE || per_sample > MAX_PER_SAMPLE ) {
                throw GSError("Invalid per_sample");
            }
            if (!isPowerOf2(per_sample)) {
                throw GSError("Must be power of 2");
            }
            _per_sample = per_sample;
        }

        void set_cache_line_size(size_t cache_line_size)
        {
            if (cache_line_size < MIN_CACHE_LINE_SIZE || cache_line_size > MAX_CACHE_LINE_SIZE ) {
                throw gs_patterns::GSError("Invalid cache_line_size");
            }
            if (!isPowerOf2(cache_line_size)) {
                throw GSError("Must be power of 2");
            }
            _cache_line_size = cache_line_size;
        }

        void set_num_buffers(int64_t num_buffers)
        {
            if (num_buffers < MIN_NUM_BUFFERS || num_buffers > MAX_NUM_BUFFERS ) {
                throw GSError("Invalid num_buffers");
            }
            if (!isPowerOf2(num_buffers)) {
                throw GSError("Must be power of 2");
            }
            _num_buffers = num_buffers;
        }

        void set_instruction_window(size_t instruction_window)
        {
            if (instruction_window < MIN_INSTRUCTION_WINDOW || instruction_window > MAX_INSTRUCTION_WINDOW ) {
                throw GSError("Invalid instruction_window");
            }
            if (!isPowerOf2(instruction_window)) {
                throw GSError("Must be power of 2");
            }
            _instruction_window = instruction_window;
        }

        void set_max_gather_scatter(size_t max_gather_scatter)
        {
            if (max_gather_scatter < MIN_MAX_GATHER_SCATTER || max_gather_scatter > MAX_MAX_GATHER_SCATTER ) {
                throw GSError("Invalid max_gather_scatter");
            }
            if (!isPowerOf2(max_gather_scatter)) {
                throw GSError("Must be power of 2");
            }
            _max_gather_scatter = max_gather_scatter;
        }

        void set_histogram_bounds(size_t histogram_bounds)
        {
            if (histogram_bounds < MIN_HISTOGRAM_BOUNDS || histogram_bounds > MAX_HISTOGRAM_BOUNDS ) {
                throw GSError("Invalid histogram_bounds");
            }
            if (!isPowerOf2(histogram_bounds)) {
                throw GSError("Must be power of 2");
            }
            _histogram_bounds = histogram_bounds;
        }

        void set_unique_strides_threshold(size_t unique_strides_threshold)
        {
            if (unique_strides_threshold < MIN_UNIQUE_STRIDES_THRESHOLD || unique_strides_threshold > MAX_UNIQUE_STRIDES_THRESHOLD ) {
                throw GSError("Invalid unique_strides_threshold");
            }
            if (!isPowerOf2(unique_strides_threshold)) {
                throw GSError("Must be power of 2");
            }
            _unique_strides_threshold = unique_strides_threshold;
        }

        void set_num_unique_distances(size_t num_unique_distances)
        {
            if (num_unique_distances < MIN_NUM_UNIQUE_DISTANCES || num_unique_distances > MAX_NUM_UNIQUE_DISTANCES ) {
                throw GSError("Invalid num_unique_distances");
            }
            if (!isPowerOf2(num_unique_distances)) {
                throw GSError("Must be power of 2");
            }
            _num_unique_distances = num_unique_distances;
        }

        void set_out_threshold(double out_threshold)
        {
            if (out_threshold < MIN_OUT_THRESHOLD || out_threshold > MAX_OUT_THRESHOLD ) {
                throw GSError("Invalid out_threshold");
            }
            if (!isPowerOf2(out_threshold)) {
                throw GSError("Must be power of 2");
            }
            _out_threshold = out_threshold;
        }

        void set_top_patterns(size_t top_patterns)
        {
            if (top_patterns < MIN_TOP_PATTERNS || top_patterns > MAX_TOP_PATTERNS ) {
                throw GSError("Invalid top_patterns");
            }
            if (!isPowerOf2(top_patterns)) {
                throw GSError("Must be power of 2");
            }
            _top_patterns = top_patterns;
        }

        void set_initial_pattern_size(size_t initial_pattern_size)
        {
            if (initial_pattern_size < MIN_PATTERN_SIZE || initial_pattern_size > MAX_PATTERN_SIZE ) {
                throw GSError("Invalid initial_pattern_size");
            }
            if (!isPowerOf2(initial_pattern_size)) {
                throw GSError("Must be power of 2");
            }
            _initial_pattern_size = initial_pattern_size;
        }

        void set_max_pattern_size(size_t max_pattern_size)
        {
            if (max_pattern_size < MIN_PATTERN_SIZE || max_pattern_size > MAX_PATTERN_SIZE ) {
                throw GSError("Invalid max_pattern_size");
            }
            if (!isPowerOf2(max_pattern_size)) {
                throw GSError("Must be power of 2");
            }
            _max_pattern_size = max_pattern_size;
        }

        void set_max_line_length(size_t max_line_length)
        {
            if (max_line_length < MIN_MAX_LINE_LENGTH || max_line_length > MAX_MAX_LINE_LENGTH ) {
                throw GSError("Invalid max_pattern_size");
            }
            if (!isPowerOf2(max_line_length)) {
                throw GSError("Must be power of 2");
            }
            _max_line_length = max_line_length;
        }

    private:
        Config() = default;

        static bool isPowerOf2(const size_t n)
        {
            // positive powers of two are always like: 1000...
            // if n = 1000...; (n - 1) = 0111...; AND of both is zero
            return n > 0 && (n & (n - 1)) == 0;
        }

        // triggers
        size_t _per_sample = 10000000;

        // info
        size_t _cache_line_size = 64;
        int64_t _num_buffers = 1LL << 10;
        size_t _instruction_window = 1024;
        size_t _max_gather_scatter = 8096;
        size_t _histogram_bounds = 512;

        // patterns
        size_t _unique_strides_threshold = 1024;
        size_t _num_unique_distances = 15;
        double _out_threshold = 0.5;
        size_t _top_patterns = 10;
        size_t _initial_pattern_size = 1 << 15;
        size_t _max_pattern_size = 1 << 30;

        size_t _max_line_length = 1024;


        // -- Validation boundaries --
        // to do: check values
        // Triggers
        static constexpr size_t MIN_PER_SAMPLE = 1000;
        static constexpr size_t MAX_PER_SAMPLE = 1LL << 40;    // ~1 trillion

        // Info
        static constexpr size_t MIN_CACHE_LINE_SIZE = 16;
        static constexpr size_t MAX_CACHE_LINE_SIZE = 512;

        static constexpr int64_t MIN_NUM_BUFFERS = 1;
        static constexpr int64_t MAX_NUM_BUFFERS = 1LL << 20;   // Over 1 million buffers

        static constexpr size_t MIN_INSTRUCTION_WINDOW = 16;
        static constexpr size_t MAX_INSTRUCTION_WINDOW = 1LL << 17; // 131,072 instructions

        static constexpr size_t MIN_MAX_GATHER_SCATTER = 2;
        static constexpr size_t MAX_MAX_GATHER_SCATTER = 1LL << 14; // 16,384 elements

        static constexpr size_t MIN_HISTOGRAM_BOUNDS = 16;
        static constexpr size_t MAX_HISTOGRAM_BOUNDS = 1LL << 16; // 65,536 bins

        // Patterns
        static constexpr size_t MIN_UNIQUE_STRIDES_THRESHOLD = 16;
        static constexpr size_t MAX_UNIQUE_STRIDES_THRESHOLD = 1LL << 16; // 65,536

        static constexpr size_t MIN_NUM_UNIQUE_DISTANCES = 1;
        static constexpr size_t MAX_NUM_UNIQUE_DISTANCES = 128;

        static constexpr double MIN_OUT_THRESHOLD = 0.0;
        static constexpr double MAX_OUT_THRESHOLD = 1.0;

        static constexpr size_t MIN_TOP_PATTERNS = 1;
        static constexpr size_t MAX_TOP_PATTERNS = 100;

        static constexpr size_t MIN_PATTERN_SIZE = 1LL << 10; // 1 KB
        static constexpr size_t MAX_PATTERN_SIZE = 1LL << 30; // 1 GB

        static constexpr size_t MIN_MAX_LINE_LENGTH = 80;
        static constexpr size_t MAX_MAX_LINE_LENGTH = 1LL << 13;

    };
}