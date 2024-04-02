

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <exception>

#include "gs_patterns.h"
#include "gs_patterns_core.h"

//Terminal colors
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"

// Class Static data initialization
char    Metrics::srcline[2][NGS][MAX_LINE_LENGTH];
addr_t  InstrInfo::iaddrs[2][NGS];
int64_t InstrInfo::icnt[2][NGS];
int64_t InstrInfo::occ[2][NGS];

#if 0
int64_t InstrWindow::w_iaddrs[2][IWINDOW];
int64_t InstrWindow::w_bytes[2][IWINDOW];
int64_t InstrWindow::w_maddr[2][IWINDOW][VBYTES];
int64_t InstrWindow::w_cnt[2][IWINDOW];
#endif

double update_source_lines_from_binary(InstrInfo & target_iinfo, Metrics & target_metrics, const std::string & binary_file_name)
{
    double scatter_cnt = 0.0;

    //Check it is not a library
    for (int k = 0; k < NGS; k++) {

        if (target_iinfo.get_iaddrs()[k] == 0) {
            break;
        }
        translate_iaddr(binary_file_name, target_metrics.get_srcline()[k], target_iinfo.get_iaddrs()[k]);
        if (startswith(target_metrics.get_srcline()[k], "?"))
            target_iinfo.get_icnt()[k] = 0;

        scatter_cnt += target_iinfo.get_icnt()[k];
    }
    printf("done.\n");

    return scatter_cnt;
}


// First Pass
void process_traces(
        TraceInfo & trace_info,
        InstrInfo & gather_iinfo,
        InstrInfo & scatter_iinfo,
        Metrics & gather_metrics,
        Metrics & scatter_metrics,
        gzFile & fp_drtrace)
{
    int iret = 0;
    trace_entry_t *drline;
    InstrWindow iw;

    printf("First pass to find top gather / scatter iaddresses\n");
    fflush(stdout);

    trace_entry_t *p_drtrace = NULL;
    trace_entry_t drtrace[NBUFS];  // was static (1024 bytes)

    while (drline_read(fp_drtrace, drtrace, &p_drtrace, &iret)) {
        //decode drtrace
        drline = p_drtrace;

        handle_trace_entry(drline, trace_info, gather_iinfo, scatter_iinfo, gather_metrics, scatter_metrics, iw);

        p_drtrace++;
    }

    //metrics
    trace_info.gather_occ_avg /= gather_metrics.cnt;
    trace_info.scatter_occ_avg /= scatter_metrics.cnt;

    display_stats(trace_info, gather_metrics, scatter_metrics);

}

void update_source_lines(
        InstrInfo & gather_iinfo,
        InstrInfo & scatter_iinfo,
        Metrics & gather_metrics,
        Metrics & scatter_metrics,
        const std::string & binary)
{
    // Find source lines for gathers - Must have symbol
    printf("\nSymbol table lookup for gathers...");
    fflush(stdout);

    gather_metrics.cnt = update_source_lines_from_binary(gather_iinfo, gather_metrics, binary);

    // Find source lines for scatters
    printf("Symbol table lookup for scatters...");
    fflush(stdout);

    scatter_metrics.cnt = update_source_lines_from_binary(scatter_iinfo, scatter_metrics, binary);
}


gzFile open_trace_file(const std::string & trace_file_name)
{
    gzFile fp;

    fp = gzopen(trace_file_name.c_str(), "hrb");
    if (fp == NULL) {
        throw GSFileError("Could not open " + trace_file_name + "!");
    }
    return fp;
}

void close_trace_file (gzFile & fp)
{
    gzclose(fp);
}

int main(int argc, char **argv)
{
    try
    {
        if (argc != 3) {
            throw GSError("Invalid arguments, should be: trace.gz binary_file_name");
        }

        gzFile fp_drtrace;
        std::string trace_file_name(argv[1]);
        std::string binary_file_name(argv[2]);

        fp_drtrace = open_trace_file(trace_file_name);

        Metrics gather_metrics(GATHER);
        Metrics scatter_metrics(SCATTER);

        InstrInfo gather_iinfo(GATHER);
        InstrInfo scatter_iinfo(SCATTER);

        TraceInfo trace_info;

        // ----------------- Process Traces -----------------

        process_traces(trace_info, gather_iinfo, scatter_iinfo, gather_metrics, scatter_metrics, fp_drtrace);

        close_trace_file(fp_drtrace);

        // ----------------- Update Source Lines -----------------

        update_source_lines(gather_iinfo, scatter_iinfo, gather_metrics, scatter_metrics, binary_file_name);

        // ----------------- Update Metrics -----------------
        fp_drtrace = open_trace_file(argv[1]);

        update_metrics(gather_iinfo, scatter_iinfo, gather_metrics, scatter_metrics, fp_drtrace);

        close_trace_file(fp_drtrace);

        // ----------------- Create Spatter File -----------------

        create_spatter_file(argv[1], gather_metrics, scatter_metrics);

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
