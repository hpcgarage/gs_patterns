#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <exception>

#include "gs_patterns.h"
#include "gs_patterns_core.h"
#include "gspin_patterns.h"
#include "gsnv_patterns.h"
#include "utils.h"

#define GSNV_CONFIG_FILE "GSNV_CONFIG_FILE"

using namespace gs_patterns;
using namespace gs_patterns::gs_patterns_core;
using namespace gs_patterns::gsnv_patterns;
using namespace gs_patterns::gspin_patterns;

// explained in helper function in config.cpp
// void usage (const std::string & prog_name)
// {
//     std::cerr << "Usage: " << prog_name << " <pin_trace.gz> <prog_bin> \n"
//               << "       " << prog_name << " <nvbit_trace.gz> -nv [-ow] [-v]" << std::endl;
// }

int main(int argc, char ** argv)
{
    try
    {
        // Parse configuration arguments first
        Config& config = Config::get_instance();
        config.parseArgs(argc, argv);
        bool use_gs_nv = false;
        bool verbose = false;
        bool one_warp = false;
        for (int i = 0; i < argc; i++) {
            if (std::string(argv[i]) == "-nv") {
                use_gs_nv = true;
            }
            else if (std::string(argv[i]) == "-v") {
                verbose = true;
            }
            else if (std::string(argv[i]) == "-ow") {
                one_warp = true;
            }
        }

        size_t pos = std::string(argv[0]).find_last_of("/");
        std::string prog_name = std::string(argv[0]).substr(pos+1);

        // --- Find positional arguments ---
        // This loop collects arguments that are NOT options or values for options.
        std::vector<std::string> positional_args;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg.empty()) continue;

            // Stop processing options at "--" and add the rest
            if (arg == "--") {
                for (int j = i + 1; j < argc; ++j) {
                    positional_args.push_back(argv[j]);
                }
                break;
            }

            // A lone "-" is a positional arg (stdin/stdout), which config.cpp skips
            if (arg == "-") {
                continue;
            }

            // If it doesn't start with "-", it's positional
            if (arg[0] != '-') {
                positional_args.push_back(arg);
                continue;
            }

            // It *is* an option. We need to skip it and its value (if it has one).

            // Check for valueless flags (handled by main or config.cpp help)
            if (arg == "-nv" || arg == "-v" || arg == "-ow" || arg == "-h" || arg == "--help") {
                continue;
            }

            // Check for --opt=value format
            if (arg.find('=') != std::string::npos) {
                continue; // config.parseArgs handled this
            }

            // If we're here, it's an option like "-ps" or "--per-sample"
            // that takes a value as the *next* argument.
            // We must skip that value so it isn't counted as positional.
            if (i + 1 < argc) {
                i++; // Skip the value argument
            }
        }

        // parser in config.cpp deals with it
        // if (argc < 3) {
        //     usage(prog_name);
        //     throw GSError("Invalid program arguments");
        // }

        if (use_gs_nv)
        {
            if (positional_args.empty()) {
                config.printHelp(prog_name.c_str());
                throw GSError("Missing required <nvbit_trace.gz> argument.");
            }

            MemPatternsForNV mp;

            mp.set_trace_file(positional_args[0]);

            const char * config_file = std::getenv(GSNV_CONFIG_FILE);
            if (config_file) {
                mp.set_config_file(config_file);
            }
            if (verbose) mp.set_log_level(1);
            if (one_warp) mp.set_one_warp_mode(one_warp);

            // ----------------- Process Traces -----------------

            mp.process_traces();

            // ----------------- Generate Patterns -----------------

            mp.generate_patterns();
        }
        else
        {
            if (positional_args.size() < 2) {
                config.printHelp(prog_name.c_str());
                throw GSError("Missing required <pin_trace.gz> and <prog_bin> arguments.");
            }

            MemPatternsForPin mp;

            mp.set_trace_file(positional_args[0]);
            mp.set_binary_file(positional_args[1]);
            if (verbose) mp.set_log_level(1);

            // ----------------- Process Traces -----------------

            mp.process_traces();

            // ----------------- Generate Patterns -----------------

            mp.generate_patterns();
        }
    }
    catch (const GSFileError & ex)
    {
        std::cerr << "ERROR: <GSFileError> " << ex.what() << std::endl;
        exit(-1);
    }
    catch (const GSAllocError & ex)
    {
        std::cerr << "ERROR: <GSAllocError> " << ex.what() << std::endl;
        exit(-1);
    }
    catch (const GSDataError & ex)
    {
        std::cerr << "ERROR: <GSDataError> " << ex.what() << std::endl;
        exit(1);
    }
    catch (const GSError & ex)
    {
        std::cerr << "ERROR: <GSError> " << ex.what() << std::endl;
        exit(1);
    }
    catch (const std::exception & ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        exit(-1);
    }

    return 0;
}
