#include <iostream>
#include <string>
#include <iomanip>

#include "config.h"
#include "errors.h"

namespace gs_patterns
{
    // setters
    void Config::set_per_sample(size_t per_sample)
    {
        if (per_sample < MIN_PER_SAMPLE || per_sample > MAX_PER_SAMPLE ) {
            throw GSError("Invalid per_sample");
        }
        _per_sample = per_sample;
    }

    void Config::set_cache_line_size(size_t cache_line_size)
    {
        if (cache_line_size < MIN_CACHE_LINE_SIZE || cache_line_size > MAX_CACHE_LINE_SIZE ) {
            throw gs_patterns::GSError("Invalid cache_line_size");
        }
        if (!isPowerOf2(cache_line_size)) {
            throw GSError("cache_line_size must be power of 2");
        }
        _cache_line_size = cache_line_size;
    }

    void Config::set_num_buffers(int64_t num_buffers)
    {
        if (num_buffers < MIN_NUM_BUFFERS || num_buffers > MAX_NUM_BUFFERS ) {
            throw GSError("Invalid num_buffers");
        }
        if (!isPowerOf2(num_buffers)) {
            throw GSError("num_buffers must be power of 2");
        }
        _num_buffers = num_buffers;
    }

    void Config::set_instruction_window(size_t instruction_window)
    {
        if (instruction_window < MIN_INSTRUCTION_WINDOW || instruction_window > MAX_INSTRUCTION_WINDOW ) {
            throw GSError("Invalid instruction_window");
        }
        if (!isPowerOf2(instruction_window)) {
            throw GSError("instruction_window must be power of 2");
        }
        _instruction_window = instruction_window;
    }

    void Config::set_max_gather_scatter(size_t max_gather_scatter)
    {
        if (max_gather_scatter < MIN_MAX_GATHER_SCATTER || max_gather_scatter > MAX_MAX_GATHER_SCATTER ) {
            throw GSError("Invalid max_gather_scatter");
        }
        _max_gather_scatter = max_gather_scatter;
    }

    void Config::set_histogram_bounds(size_t histogram_bounds)
    {
        if (histogram_bounds < MIN_HISTOGRAM_BOUNDS || histogram_bounds > MAX_HISTOGRAM_BOUNDS ) {
            throw GSError("Invalid histogram_bounds");
        }
        if (!isPowerOf2(histogram_bounds)) {
            throw GSError("histogram_bounds must be power of 2");
        }
        _histogram_bounds = histogram_bounds;
    }

    void Config::set_unique_strides_threshold(size_t unique_strides_threshold)
    {
        if (unique_strides_threshold < MIN_UNIQUE_STRIDES_THRESHOLD || unique_strides_threshold > MAX_UNIQUE_STRIDES_THRESHOLD ) {
            throw GSError("Invalid unique_strides_threshold");
        }
        if (!isPowerOf2(unique_strides_threshold)) {
            throw GSError("unique_strides_threshold must be power of 2");
        }
        _unique_strides_threshold = unique_strides_threshold;
    }

    void Config::set_num_unique_distances(size_t num_unique_distances)
    {
        if (num_unique_distances < MIN_NUM_UNIQUE_DISTANCES || num_unique_distances > MAX_NUM_UNIQUE_DISTANCES ) {
            throw GSError("Invalid num_unique_distances");
        }
        _num_unique_distances = num_unique_distances;
    }

    void Config::set_out_threshold(double out_threshold)
    {
        if (out_threshold < MIN_OUT_THRESHOLD || out_threshold > MAX_OUT_THRESHOLD ) {
            throw GSError("Invalid out_threshold");
        }
        _out_threshold = out_threshold;
    }

    void Config::set_top_patterns(size_t top_patterns)
    {
        if (top_patterns < MIN_TOP_PATTERNS || top_patterns > MAX_TOP_PATTERNS ) {
            throw GSError("Invalid top_patterns");
        }
        _top_patterns = top_patterns;
    }

    void Config::set_initial_pattern_size(size_t initial_pattern_size)
    {
        if (initial_pattern_size < MIN_PATTERN_SIZE || initial_pattern_size > MAX_PATTERN_SIZE ) {
            throw GSError("Invalid initial_pattern_size");
        }
        if (!isPowerOf2(initial_pattern_size)) {
            throw GSError("initial_pattern_size must be power of 2");
        }
        _initial_pattern_size = initial_pattern_size;
    }

    void Config::set_max_pattern_size(size_t max_pattern_size)
    {
        if (max_pattern_size < MIN_PATTERN_SIZE || max_pattern_size > MAX_PATTERN_SIZE ) {
            throw GSError("Invalid max_pattern_size");
        }
        if (!isPowerOf2(max_pattern_size)) {
            throw GSError("max_pattern_size must be power of 2");
        }
        _max_pattern_size = max_pattern_size;
    }

    void Config::set_max_line_length(size_t max_line_length)
    {
        if (max_line_length < MIN_MAX_LINE_LENGTH || max_line_length > MAX_MAX_LINE_LENGTH ) {
            throw GSError("Invalid max_pattern_size");
        }
        if (!isPowerOf2(max_line_length)) {
            throw GSError("max_line_length must be power of 2");
        }
        _max_line_length = max_line_length;
    }
    void Config::parseArgs(int argc, char* argv[])
    {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            // Ignore empty tokens or non-options
            if (arg.empty() || arg[0] != '-') {
                continue;
            }

            // End-of-options marker
            if (arg == "--") {
                break;
            }

            // Treat lone "-" as a positional stdin/stdout placeholder
            if (arg == "-") {
                continue;
            }

            // Known non-config flags to ignore
            if (arg == "-nv" || arg == "-v" || arg == "-ow") {
                continue;
            }

            // Help
            if (arg == "--help" || arg == "-h") {
                printHelp(argv[0]);      // keep your existing help text
                std::exit(0);     // or return if you don’t want to exit here
            }

            // Accept both "--opt=value" and "--opt value"
            std::string value;
            std::size_t eq = arg.find('=');
            if (eq != std::string::npos) {
                // '=' was found
                value = arg.substr(eq + 1);
                arg.erase(eq); // keep only the option and value for the ladder comparison
                if (value.empty()) {
                    throw GSError("Missing value for " + arg);
                }
            } else {
                // '=' wasn't found
                if (i + 1 >= argc) {
                    // end of string
                    throw GSError("Missing value for " + arg);
                }
                // using space instead of '='
                value = argv[++i];
            }

            try {
                if (arg == "--per-sample" || arg == "-ps") {
                    set_per_sample(std::stoull(value));
                }
                else if (arg == "--cache-line-size" || arg == "-cls") {
                    set_cache_line_size(std::stoull(value));
                }
                else if (arg == "--num-buffers" || arg == "-nb") {
                    set_num_buffers(std::stoll(value));
                }
                else if (arg == "--instruction-window" || arg == "-iw") {
                    set_instruction_window(std::stoull(value));
                }
                else if (arg == "--max-gather-scatter" || arg == "-mgs") {
                    set_max_gather_scatter(std::stoull(value));
                }
                else if (arg == "--histogram-bounds" || arg == "-hb") {
                    set_histogram_bounds(std::stoull(value));
                }
                else if (arg == "--unique-strides-threshold" || arg == "-ust") {
                    set_unique_strides_threshold(std::stoull(value));
                }
                else if (arg == "--num-unique-distances" || arg == "-nud") {
                    set_num_unique_distances(std::stoull(value));
                }
                else if (arg == "--out-threshold" || arg == "-ot") {
                    set_out_threshold(std::stod(value));
                }
                else if (arg == "--top-patterns" || arg == "-tp") {
                    set_top_patterns(std::stoull(value));
                }
                else if (arg == "--initial-pattern-size" || arg == "-ips") {
                    set_initial_pattern_size(std::stoull(value));
                }
                else if (arg == "--max-pattern-size" || arg == "-mps") {
                    set_max_pattern_size(std::stoull(value));
                }
                else if (arg == "--max-line-length" || arg == "-mll") {
                    set_max_line_length(std::stoull(value));
                }
                else {
                    throw GSError("Unknown configuration argument: " + arg);
                }
            } catch (const std::invalid_argument&) {
                throw GSError("Invalid value for " + arg + ": " + value);
            } catch (const std::out_of_range&) {
                throw GSError("Value out of range for " + arg + ": " + value);
            }
        }
    }

    void Config::printHelp(const char* program_name /*= "program"*/)
    {
        const Config& cfg = get_instance();
        constexpr int option_width = 45;

        // --- Usage header (generic) ---
        std::cout << "\nUsage:\n"
                  << "  " << program_name << " [options] <trace.gz> [<binary>|-nv]\n\n"
                  << "  Options accept both '--opt value' and '--opt=value' formats.\n"
                  << "  Argument parsing stops at '--'. A lone '-' is treated as stdin/stdout.\n\n";

        // (from README) ---
        std::cout << "Invocation:\n"
                  << "  For Pin/DynamoRIO traces:\n"
                  << "    " << program_name << " <pin_trace.gz> <binary>\n"
                  << "  For NVBit (CUDA kernels):\n"
                  << "    " << program_name << " <nvbit_trace.gz> -nv\n\n";

        // --- Examples ---
        std::cout << "Examples:\n"
                  << "  " << program_name << " app.pin.trace.gz ./app_with_symbols\n"
                  << "  " << program_name << " kernel.nvbit.trace.gz -nv\n\n";

        // --- Notes / prerequisites ---
        std::cout << "Notes:\n"
                  << "  • Trace file must be gzipped ('.gz') — not 'tar.gz'.\n"
                  << "  • For Pin/DynamoRIO, the <binary> should be compiled with symbols (e.g., -g).\n"
                  << "  • For NVBit, compile CUDA kernels with line info (--generate-line-info).\n"
                  << "  • See nvbit_tracing/README.md for extracting compatible CUDA traces.\n\n";

        // --- Configuration Options ---
        std::cout << "Configuration Options:\n\n";
        std::cout << "Triggers:\n";
        std::cout << "  " << std::left << std::setw(option_width) << "--per-sample, -ps <value>"
                  << "Samples per trigger (default: " << cfg.get_per_sample() << ")\n";
        std::cout << "  " << std::left << std::setw(option_width) << ""
                  << "Range: [" << MIN_PER_SAMPLE << ", " << MAX_PER_SAMPLE << "]\n\n";

        std::cout << "Info Parameters:\n";
        // std::cout << "  " << std::left << std::setw(option_width) << "--cache-line-size, -cls <value>"
        //           << "Cache line size in bytes (default: " << cfg.get_cache_line_size() << ")\n";
        // std::cout << "  " << std::left << std::setw(option_width) << ""
        //           << "Range: [" << MIN_CACHE_LINE_SIZE << ", " << MAX_CACHE_LINE_SIZE << "]\n";
        std::cout << "  " << std::left << std::setw(option_width) << "--num-buffers, -nb <value>"
                  << "Number of trace buffers (default: " << cfg.get_num_buffers() << ")\n";
        // std::cout << "  " << std::left << std::setw(option_width) << ""
        //           << "Range: [" << MIN_NUM_BUFFERS << ", " << MAX_NUM_BUFFERS << "]\n";
        // std::cout << "  " << std::left << std::setw(option_width) << "--instruction-window, -iw <value>"
        //           << "Instruction window size (default: " << cfg.get_instruction_window() << ")\n";
        // std::cout << "  " << std::left << std::setw(option_width) << ""
        //           << "Range: [" << MIN_INSTRUCTION_WINDOW << ", " << MAX_INSTRUCTION_WINDOW << "]\n";
        // std::cout << "  " << std::left << std::setw(option_width) << "--max-gather-scatter, -mgs <value>"
        //           << "Max gather/scatter elements (default: " << cfg.get_max_gather_scatter() << ")\n";
        // std::cout << "  " << std::left << std::setw(option_width) << ""
        //           << "Range: [" << MIN_MAX_GATHER_SCATTER << ", " << MAX_MAX_GATHER_SCATTER << "]\n";
        std::cout << "  " << std::left << std::setw(option_width) << "--histogram-bounds, -hb <value>"
                  << "Histogram bounds (default: " << cfg.get_histogram_bounds() << ")\n";
        std::cout << "  " << std::left << std::setw(option_width) << ""
                  << "Range: [" << MIN_HISTOGRAM_BOUNDS << ", " << MAX_HISTOGRAM_BOUNDS << "]\n\n";

        std::cout << "Pattern Parameters:\n";
        std::cout << "  " << std::left << std::setw(option_width) << "--unique-strides-threshold, -ust <value>"
                  << "Unique strides threshold (default: " << cfg.get_unique_strides_threshold() << ")\n";
        std::cout << "  " << std::left << std::setw(option_width) << ""
                  << "Range: [" << MIN_UNIQUE_STRIDES_THRESHOLD << ", " << MAX_UNIQUE_STRIDES_THRESHOLD << "]\n";
        std::cout << "  " << std::left << std::setw(option_width) << "--num-unique-distances, -nud <value>"
                  << "Number of unique distances (default: " << cfg.get_num_unique_distances() << ")\n";
        std::cout << "  " << std::left << std::setw(option_width) << ""
                  << "Range: [" << MIN_NUM_UNIQUE_DISTANCES << ", " << MAX_NUM_UNIQUE_DISTANCES << "]\n";
        std::cout << "  " << std::left << std::setw(option_width) << "--out-threshold, -ot <value>"
                  << "Out threshold (default: " << cfg.get_out_threshold() << ")\n";
        std::cout << "  " << std::left << std::setw(option_width) << ""
                  << "Range: [" << MIN_OUT_THRESHOLD << ", " << MAX_OUT_THRESHOLD << "]\n";
        // std::cout << "  " << std::left << std::setw(option_width) << "--top-patterns, -tp <value>"
        //           << "Number of top patterns to keep (default: " << cfg.get_top_patterns() << ")\n";
        // std::cout << "  " << std::left << std::setw(option_width) << ""
        //           << "Range: [" << MIN_TOP_PATTERNS << ", " << MAX_TOP_PATTERNS << "]\n";
        std::cout << "  " << std::left << std::setw(option_width) << "--initial-pattern-size, -ips <value>"
                  << "Initial pattern size (default: " << cfg.get_initial_pattern_size() << ")\n";
        std::cout << "  " << std::left << std::setw(option_width) << ""
                  << "Range: [" << MIN_PATTERN_SIZE << ", " << MAX_PATTERN_SIZE << "]\n";
        std::cout << "  " << std::left << std::setw(option_width) << "--max-pattern-size, -mps <value>"
                  << "Maximum pattern size (default: " << cfg.get_max_pattern_size() << ")\n";
        std::cout << "  " << std::left << std::setw(option_width) << ""
                  << "Range: [" << MIN_PATTERN_SIZE << ", " << MAX_PATTERN_SIZE << "]\n";
        // std::cout << "  " << std::left << std::setw(option_width) << "--max-line-length, -mll <value>"
        //           << "Maximum line length (default: " << cfg.get_max_line_length() << ")\n";
        // std::cout << "  " << std::left << std::setw(option_width) << ""
        //           << "Range: [" << MIN_MAX_LINE_LENGTH << ", " << MAX_MAX_LINE_LENGTH << "]\n\n";

        std::cout << "Note: Most numeric values must be powers of 2.\n"
                  << "      Exceptions: out-threshold, num-unique-distances, and top-patterns.\n\n";

        // --- How it works ---
        std::cout << "How gs_patterns works:\n"
                  << "  • Detects gather/scatter (g/s) by finding repeated instruction addresses (loops)\n"
                  << "    that correspond to memory instructions (scalar or vector).\n"
                  << "  • Pass 1: ranks top g/s instructions and filters out trivial access patterns.\n"
                  << "  • Pass 2: focuses on those top g/s; records normalized address array indices\n"
                  << "    to a binary file and a spatter YAML file.\n\n";

        // --- Quick flags reminder (non-config) ---
        std::cout << "Other flags:\n"
                  << "  -nv          Interpret trace as NVBit (CUDA) trace.\n"
                  << "  -v           Verbose logging.\n"
                  << "  -ow          Overwrite outputs if present.\n\n";
    }


} // namespace gs_patterns