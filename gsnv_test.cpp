
#include "gs_patterns.h"
#include "gsnv_patterns.h"

int main(int argc, char **argv)
{
    try
    {
        if (argc != 3) {
            throw GSError("Invalid arguments, should be: trace.gz binary_file_name");
        }

        MemPatternsForNV mp;

        // nvbit trace file with memory access traces
        mp.set_trace_file(argv[1]);
        // Executable file built with debug symbols
        mp.set_binary_file(argv[2]);

        // File to save nvbit memory accessses to
        //mp.set_trace_out_file(mp.get_file_prefix() + ".nvbit.bin");

        // ----------------- Process Traces -----------------

        mp.add_or_update_opcode(0, "LD.E.64");
        mp.add_or_update_opcode(1, "ST.E.64");

        mp.add_or_update_opcode_short(0, "LD");
        mp.add_or_update_opcode_short(1, "ST");

        mp.process_traces();

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



