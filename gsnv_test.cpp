
#include <stdlib.h>

#include "gs_patterns.h"
#include "gsnv_patterns.h"

#define NVGS_CONFIG_FILE "NVGS_CONFIG_FILE"

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2) {
            size_t pos = std::string(argv[0]).find_last_of("/");
            std::string prog_name = std::string(argv[0]).substr(pos+1);
            throw GSError("Invalid program arguments, should be: " + prog_name + " <trace.gz>");
        }

        MemPatternsForNV mp;

        // nvbit trace file with memory access traces
        mp.set_trace_file(argv[1]);

        const char * config_file = std::getenv(NVGS_CONFIG_FILE);
        if (config_file) {
            mp.set_config_file(config_file);
        }

        // File to save nvbit memory accessses to
        //mp.set_trace_out_file(mp.get_file_prefix() + ".nvbit.bin");

        // ----------------- Process Traces -----------------

        mp.process_traces();

        mp.write_trace_out_file();

        // ----------------- Generate Patterns -----------------

        mp.generate_patterns();
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
