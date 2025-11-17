# pragma once

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
        void printHelp(const char* program_name);


        // expose compile-time choices for run time inspection
        static constexpr size_t vector_bits = VBITS;
        static constexpr size_t vector_bytes = VBYTES;
        static constexpr bool symbols_only = (SYMBOLS_ONLY != 0);


        // getters implemented in line for performance
        [[nodiscard]] size_t get_per_sample() const { return _per_sample; }
        [[nodiscard]] size_t get_cache_line_size() const { return _cache_line_size; }
        [[nodiscard]] size_t get_trace_buffer_size() const { return _trace_buffer_size; }
        [[nodiscard]] size_t get_iaddr_per_window() const { return _iaddr_per_window; }
        [[nodiscard]] size_t get_max_gather_scatter() const { return _max_gather_scatter; }
        [[nodiscard]] size_t get_histogram_bounds() const { return _histogram_bounds; }
        [[nodiscard]] size_t get_histogram_bounds_alloc() const { return 2 * _histogram_bounds + 3; }
        [[nodiscard]] size_t get_unique_strides_threshold() const { return _unique_strides_threshold; }
        [[nodiscard]] size_t get_unique_distances_threshold() const { return _unique_distances_threshold; }
        [[nodiscard]] double get_out_threshold() const { return _out_threshold; }
        [[nodiscard]] size_t get_top_patterns() const { return _top_patterns; }
        [[nodiscard]] size_t get_initial_pattern_size() const { return _initial_pattern_size; }
        [[nodiscard]] size_t get_max_pattern_size() const { return _max_pattern_size; }
        [[nodiscard]] size_t get_max_line_length() const { return _max_line_length; }

        // setters
        void set_per_sample(size_t per_sample);
        void set_cache_line_size(size_t cache_line_size);
        void set_trace_buffer_size(size_t trace_buffer_size);
        void set_iaddr_per_window(size_t iaddr_per_window);
        void set_max_gather_scatter(size_t max_gather_scatter);
        void set_histogram_bounds(size_t histogram_bounds);
        void set_unique_strides_threshold(size_t unique_strides_threshold);
        void set_unique_distances_threshold(size_t unique_distances_threshold);
        void set_out_threshold(double out_threshold);
        void set_top_patterns(size_t top_patterns);
        void set_initial_pattern_size(size_t initial_pattern_size);
        void set_max_pattern_size(size_t max_pattern_size);
        void set_max_line_length(size_t max_line_length);

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
        size_t _trace_buffer_size = 1LL << 10; //trace reading buffer size
        size_t _iaddr_per_window = 1024; //number of iaddrs per window
        size_t _max_gather_scatter = 8096; //max number for gathers and scatters
        size_t _histogram_bounds = 512; //histogram positive max

        // patterns
        size_t _unique_strides_threshold = 1024; //Threshold for number of accesses
        size_t _unique_distances_threshold = 15;  //Threshold for number of unique distances
        double _out_threshold = 0.5; //Threshold for percentage of distances at boundaries of histogram
        size_t _top_patterns = 10;  //Final gather / scatters to keep
        size_t _initial_pattern_size = 1 << 15;
        size_t _max_pattern_size = 1 << 30; //Max number of indices recorded per gather/scatter

        size_t _max_line_length = 1024;


        // -- Validation boundaries --
        // Triggers
        static constexpr size_t MIN_PER_SAMPLE = 1LL << 10; // Minimum number of memory operations before printing a progress dot.
        static constexpr size_t MAX_PER_SAMPLE = 1LL << 30; // Maximum number of memory operations before printing a progress dot (avoids impractically large values).

        // Info
        static constexpr size_t MIN_CACHE_LINE_SIZE = 16; // Smallest cache line size (in bytes) to consider.
        static constexpr size_t MAX_CACHE_LINE_SIZE = 512; // Largest cache line size (in bytes) to consider (e.g., for specialized hardware).

        static constexpr size_t MIN_TRACE_BUFFER_SIZE = 1; // Smallest number of trace entries to read at once (at least 1).
        static constexpr size_t MAX_TRACE_BUFFER_SIZE = 1LL << 20;   // Largest number of trace entries to read at once (~1M), limits memory for the read buffer.

        static constexpr size_t MIN_IADDR_PER_WINDOW = 16; // Smallest "window" of unique instruction addresses to analyze for 1st pass.
        static constexpr size_t MAX_IADDR_PER_WINDOW = 1LL << 12; // Largest instruction window (4,096), prevents extreme 1st pass slowdown.

        static constexpr size_t MIN_MAX_GATHER_SCATTER = 2; // Minimum number of unique gather/scatter iaddrs to track (at least 2).
        static constexpr size_t MAX_MAX_GATHER_SCATTER = 1LL << 14; // Maximum number of unique gather/scatter iaddrs (16,384) to track across the whole trace.

        static constexpr size_t MIN_HISTOGRAM_BOUNDS = 16; // Smallest positive/negative bound for the stride histogram.
        static constexpr size_t MAX_HISTOGRAM_BOUNDS = 1LL << 12; // Largest bound (4,096), controls memory/size of the stride histogram (total bins = 2*bounds+3).

        // Patterns
        static constexpr size_t MIN_UNIQUE_STRIDES_THRESHOLD = 16; // Minimum number of accesses a pattern must have to be considered.
        static constexpr size_t MAX_UNIQUE_STRIDES_THRESHOLD = 1LL << 14; // Maximum number of accesses (16,384) to require for a pattern.

        static constexpr size_t MIN_UNIQUE_DISTANCES_THRESHOLD = 1; // Minimum number of unique strides (distances) to trigger filtering (at least 1).
        static constexpr size_t MAX_UNIQUE_DISTANCES_THRESHOLD = 128; // Maximum number of unique strides, used to identify complex patterns.

        static constexpr double MIN_OUT_THRESHOLD = 0.0; // Minimum percentage (0%) of accesses allowed "out of bounds" of the histogram.
        static constexpr double MAX_OUT_THRESHOLD = 1.0; // Maximum percentage (100%) of accesses allowed "out of bounds".

        static constexpr size_t MIN_TOP_PATTERNS = 1; // Minimum number of top gather/scatter patterns to save (at least 1).
        static constexpr size_t MAX_TOP_PATTERNS = 100; // Maximum number of top gather/scatter patterns to save.

        static constexpr size_t MIN_PATTERN_SIZE = 1LL << 10; // Minimum *initial* size (1,024 indices) to allocate for storing a pattern.
        static constexpr size_t MAX_PATTERN_SIZE = 1LL << 31; // Absolute *maximum* size (2B indices) a pattern can grow to, prevents OOM errors.

        static constexpr size_t MIN_MAX_LINE_LENGTH = 1LL << 10; // Minimum buffer size (in chars) for reading source code lines (addr2line).
        static constexpr size_t MAX_MAX_LINE_LENGTH = 1LL << 13; // Maximum buffer size (8,192 chars) for reading source code lines.
    };
}