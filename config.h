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
        [[nodiscard]] size_t get_instruction_window() const { return _instruction_window; }
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
        void set_instruction_window(size_t instruction_window);
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
        size_t _instruction_window = 1024; //number of iaddrs per window
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
        static constexpr size_t MIN_PER_SAMPLE = 1000;
        static constexpr size_t MAX_PER_SAMPLE = 1LL << 40;    // ~1 trillion

        // Info
        static constexpr size_t MIN_CACHE_LINE_SIZE = 16;
        static constexpr size_t MAX_CACHE_LINE_SIZE = 512;

        static constexpr size_t MIN_TRACE_BUFFER_SIZE = 1;
        static constexpr size_t MAX_TRACE_BUFFER_SIZE = 1LL << 20;   // Over 1 million buffers

        static constexpr size_t MIN_INSTRUCTION_WINDOW = 16;
        static constexpr size_t MAX_INSTRUCTION_WINDOW = 1LL << 17; // 131,072 instructions

        static constexpr size_t MIN_MAX_GATHER_SCATTER = 2;
        static constexpr size_t MAX_MAX_GATHER_SCATTER = 1LL << 14; // 16,384 elements

        static constexpr size_t MIN_HISTOGRAM_BOUNDS = 16;
        static constexpr size_t MAX_HISTOGRAM_BOUNDS = 1LL << 16; // 65,536 bins

        // Patterns
        static constexpr size_t MIN_UNIQUE_STRIDES_THRESHOLD = 16;
        static constexpr size_t MAX_UNIQUE_STRIDES_THRESHOLD = 1LL << 16; // 65,536

        static constexpr size_t MIN_UNIQUE_DISTANCES_THRESHOLD = 1;
        static constexpr size_t MAX_UNIQUE_DISTANCES_THRESHOLD = 128;

        static constexpr double MIN_OUT_THRESHOLD = 0.0;
        static constexpr double MAX_OUT_THRESHOLD = 1.0;

        static constexpr size_t MIN_TOP_PATTERNS = 1;
        static constexpr size_t MAX_TOP_PATTERNS = 100;

        static constexpr size_t MIN_PATTERN_SIZE = 1LL << 10; // 1 KB
        static constexpr size_t MAX_PATTERN_SIZE = 1LL << 33; // 8 GB

        static constexpr size_t MIN_MAX_LINE_LENGTH = 80;
        static constexpr size_t MAX_MAX_LINE_LENGTH = 1LL << 13;
    };
}