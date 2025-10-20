#include <iostream>
#include <cstring>

#include "config.h"

namespace gs_patterns
{
    void Config::parseArgs(int argc, char* argv[])
    {
        for (int i = 1; i < argc; i++)
        {
            std::string arg = argv[i];

            // Skip non-config arguments (trace files, binary files, special flags)
            if (arg[0] != '-' || arg == "-" || arg == "--" ||
                arg == "-nv" || arg == "-v" || arg == "-ow") {
                continue;
            }

            // Handle help flag (doesn't need a value)
            if (arg == "--help" || arg == "-h") {
                printHelp();
                exit(0);
            }

            // Check if we have a value after the flag
            if (i + 1 >= argc) {
                throw GSError("Missing value for argument: " + arg);
            }

            std::string value = argv[i + 1];

            try {
                if (arg == "--per-sample" || arg == "-ps") {
                    set_per_sample(std::stoull(value));
                    i++; // Skip the value we just processed
                }
                else if (arg == "--cache-line-size" || arg == "-cls") {
                    set_cache_line_size(std::stoull(value));
                    i++;
                }
                else if (arg == "--num-buffers" || arg == "-nb") {
                    set_num_buffers(std::stoll(value));
                    i++;
                }
                else if (arg == "--instruction-window" || arg == "-iw") {
                    set_instruction_window(std::stoull(value));
                    i++;
                }
                else if (arg == "--max-gather-scatter" || arg == "-mgs") {
                    set_max_gather_scatter(std::stoull(value));
                    i++;
                }
                else if (arg == "--histogram-bounds" || arg == "-hb") {
                    set_histogram_bounds(std::stoull(value));
                    i++;
                }
                else if (arg == "--unique-strides-threshold" || arg == "-ust") {
                    set_unique_strides_threshold(std::stoull(value));
                    i++;
                }
                else if (arg == "--num-unique-distances" || arg == "-nud") {
                    set_num_unique_distances(std::stoull(value));
                    i++;
                }
                else if (arg == "--out-threshold" || arg == "-ot") {
                    set_out_threshold(std::stod(value));
                    i++;
                }
                else if (arg == "--top-patterns" || arg == "-tp") {
                    set_top_patterns(std::stoull(value));
                    i++;
                }
                else if (arg == "--initial-pattern-size" || arg == "-ips") {
                    set_initial_pattern_size(std::stoull(value));
                    i++;
                }
                else if (arg == "--max-pattern-size" || arg == "-mps") {
                    set_max_pattern_size(std::stoull(value));
                    i++;
                }
                else if (arg == "--max-line-length" || arg == "-mll") {
                    set_max_line_length(std::stoull(value));
                    i++;
                }
                else {
                    // throw an error to catch typos
                    throw GSError("Unknown configuration argument: " + arg);
                }
            }
            catch (const std::invalid_argument& e) {
                throw GSError("Invalid value for " + arg + ": " + value);
            }
            catch (const std::out_of_range& e) {
                throw GSError("Value out of range for " + arg + ": " + value);
            }
        }
    }

    void Config::printHelp()
    {
        // Create a temporary instance to get default values
        Config& cfg = Config::get_instance();

        std::cout << "\nConfiguration Options:\n\n";

        std::cout << "Triggers:\n";
        std::cout << "  --per-sample, -ps <value>           Samples per trigger (default: " << cfg.get_per_sample() << ")\n";
        std::cout << "                                      Range: [" << MIN_PER_SAMPLE << ", " << MAX_PER_SAMPLE << "]\n\n";

        std::cout << "Info Parameters:\n";
        std::cout << "  --cache-line-size, -cls <value>     Cache line size in bytes (default: " << cfg.get_cache_line_size() << ")\n";
        std::cout << "                                      Range: [" << MIN_CACHE_LINE_SIZE << ", " << MAX_CACHE_LINE_SIZE << "]\n";
        std::cout << "  --num-buffers, -nb <value>          Number of trace buffers (default: " << cfg.get_num_buffers() << ")\n";
        std::cout << "                                      Range: [" << MIN_NUM_BUFFERS << ", " << MAX_NUM_BUFFERS << "]\n";
        std::cout << "  --instruction-window, -iw <value>   Instruction window size (default: " << cfg.get_instruction_window() << ")\n";
        std::cout << "                                      Range: [" << MIN_INSTRUCTION_WINDOW << ", " << MAX_INSTRUCTION_WINDOW << "]\n";
        std::cout << "  --max-gather-scatter, -mgs <value>  Max gather/scatter elements (default: " << cfg.get_max_gather_scatter() << ")\n";
        std::cout << "                                      Range: [" << MIN_MAX_GATHER_SCATTER << ", " << MAX_MAX_GATHER_SCATTER << "]\n";
        std::cout << "  --histogram-bounds, -hb <value>     Histogram bounds (default: " << cfg.get_histogram_bounds() << ")\n";
        std::cout << "                                      Range: [" << MIN_HISTOGRAM_BOUNDS << ", " << MAX_HISTOGRAM_BOUNDS << "]\n\n";

        std::cout << "Pattern Parameters:\n";
        std::cout << "  --unique-strides-threshold, -ust    Unique strides threshold (default: " << cfg.get_unique_strides_threshold() << ")\n";
        std::cout << "                                      Range: [" << MIN_UNIQUE_STRIDES_THRESHOLD << ", " << MAX_UNIQUE_STRIDES_THRESHOLD << "]\n";
        std::cout << "  --num-unique-distances, -nud        Number of unique distances (default: " << cfg.get_num_unique_distances() << ")\n";
        std::cout << "                                      Range: [" << MIN_NUM_UNIQUE_DISTANCES << ", " << MAX_NUM_UNIQUE_DISTANCES << "]\n";
        std::cout << "  --out-threshold, -ot <value>        Out threshold (default: " << cfg.get_out_threshold() << ")\n";
        std::cout << "                                      Range: [" << MIN_OUT_THRESHOLD << ", " << MAX_OUT_THRESHOLD << "]\n";
        std::cout << "  --top-patterns, -tp <value>         Number of top patterns to keep (default: " << cfg.get_top_patterns() << ")\n";
        std::cout << "                                      Range: [" << MIN_TOP_PATTERNS << ", " << MAX_TOP_PATTERNS << "]\n";
        std::cout << "  --initial-pattern-size, -ips        Initial pattern size (default: " << cfg.get_initial_pattern_size() << ")\n";
        std::cout << "                                      Range: [" << MIN_PATTERN_SIZE << ", " << MAX_PATTERN_SIZE << "]\n";
        std::cout << "  --max-pattern-size, -mps <value>    Maximum pattern size (default: " << cfg.get_max_pattern_size() << ")\n";
        std::cout << "                                      Range: [" << MIN_PATTERN_SIZE << ", " << MAX_PATTERN_SIZE << "]\n";
        std::cout << "  --max-line-length, -mll <value>     Maximum line length (default: " << cfg.get_max_line_length() << ")\n";
        std::cout << "                                      Range: [" << MIN_MAX_LINE_LENGTH << ", " << MAX_MAX_LINE_LENGTH << "]\n\n";

        std::cout << "Note: All numeric values must be powers of 2 (except out-threshold, num-unique-distances, and top-patterns).\n\n";
    }

} // namespace gs_patterns