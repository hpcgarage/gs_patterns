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

#define NVGS_CONFIG_FILE "NVGS_CONFIG_FILE"

using namespace gs_patterns;
using namespace gs_patterns::gs_patterns_core;
using namespace gs_patterns::gsnv_patterns;
using namespace gs_patterns::gspin_patterns;

void usage (const std::string & prog_name)
{
    std::cerr << "Usage: " << prog_name << " <trace.gz> | " + prog_name + " <trace.gz> [-nv]" << std::endl;
}

int main(int argc, char **argv)
{
    try
    {
        bool use_gs_nv = false;
        for (int i = 0; i < argc; i++) {
            if (std::string(argv[i]) == "-nv") {
                use_gs_nv = true;
            }
        }

        size_t pos = std::string(argv[0]).find_last_of("/");
        std::string prog_name = std::string(argv[0]).substr(pos+1);

        std::unique_ptr<MemPatterns> mp (use_gs_nv ? (MemPatterns *) new MemPatternsForNV : (MemPatterns *) new MemPatternsForPin);

        if (argc != 3) {
            usage(prog_name);
            throw GSError("Invalid program arguments");
        }

        if (use_gs_nv)
        {
            MemPatternsForNV mp;

            mp.set_trace_file(argv[1]);

            const char * config_file = std::getenv(NVGS_CONFIG_FILE);
            if (config_file) {
                mp.set_config_file(config_file);
            }

            // ----------------- Process Traces -----------------

            mp.process_traces();

            // ----------------- Generate Patterns -----------------

            mp.generate_patterns();
        }
        else
        {
            MemPatternsForPin mp;

            mp.set_trace_file(argv[1]);
            mp.set_binary_file(argv[2]);

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
