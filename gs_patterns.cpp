
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
#include "utils.h"

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

//FROM DR SOURCE
//DR trace
struct _trace_entry_t {
    unsigned short type; // 2 bytes: trace_type_t
    unsigned short size;
    union {
        addr_t addr;
        unsigned char length[sizeof(addr_t)];
    };
}  __attribute__((packed));
typedef struct _trace_entry_t trace_entry_t;

gzFile open_trace_file(const std::string & trace_file_name)
{
    gzFile fp;

    fp = gzopen(trace_file_name.c_str(), "hrb");
    if (NULL == fp) {
        throw GSFileError("Could not open " + trace_file_name + "!");
    }
    return fp;
}

void close_trace_file (gzFile & fp)
{
    gzclose(fp);
}

int drline_read(gzFile fp, trace_entry_t *val, trace_entry_t **p_val, int *edx) {

    int idx;

    idx = (*edx) / sizeof(trace_entry_t);
    //first read
    if (NULL == *p_val) {
        *edx = gzread(fp, val, sizeof(trace_entry_t) * NBUFS);
        *p_val = val;

    } else if (*p_val == &val[idx]) {
        *edx = gzread(fp, val, sizeof(trace_entry_t) * NBUFS);
        *p_val = val;
    }

    if (0 == *edx)
        return 0;

    return 1;
}

// An adapter for trace_entry_t
class InstrAddressInfoForPin : public InstrAddressInfo
{
public:
    InstrAddressInfoForPin(const trace_entry_t * te)
    {
        /// TODO: do we need to copy this, will we outlive trace_entry_t which is passed in ?
        _te.type = te->type;
        _te.size = te->size;
        _te.addr = te->addr;
    }
    InstrAddressInfoForPin(const trace_entry_t te) : _te(te) { }

    virtual ~InstrAddressInfoForPin() { }

    virtual bool is_valid() const override       { return !(0 == _te.type && 0 == _te.size);        }
    virtual bool is_mem_instr() const override   { return ((_te.type == 0x0) || (_te.type == 0x1)); }
    virtual bool is_other_instr() const override { return ((_te.type >= 0xa) && (_te.type <= 0x10)) || (_te.type == 0x1e); }

    virtual mem_access_type get_mem_instr_type() const override {
        if (!is_mem_instr()) throw GSDataError("Not a Memory Instruction - unable to determine Instruction");
        // Must be 0x0 or 0x1
        if (_te.type == 0x0) return GATHER;
        else return SCATTER;
    }

    virtual size_t get_size() const override         { return _te.size; } // TODO: FIX conversion <----------------------------------------
    virtual addr_t get_address() const override      { return _te.addr; };
    virtual unsigned short get_type() const override { return _te.type; }

    virtual void output(std::ostream & os) const override {
        os << "InstrAddressInfoForPin: trace entry: type: [" << _te.type << "] size: [" << _te.size << "]";
    }

private:
    trace_entry_t _te;
};

class MemPatternsForPin : public MemPatterns
{
public:
    MemPatternsForPin() : _metrics(GATHER, SCATTER),
                          _iinfo(GATHER, SCATTER) { }
    virtual ~MemPatternsForPin() override { }

    void handle_trace_entry(const InstrAddressInfo & ia) override;
    void generate_patterns() override;

    Metrics &     get_metrics(mem_access_type) override;
    InstrInfo &   get_iinfo(mem_access_type) override;

    Metrics &     get_gather_metrics() override  { return _metrics.first;  }
    Metrics &     get_scatter_metrics() override { return _metrics.second; }
    InstrInfo &   get_gather_iinfo () override   { return _iinfo.first;    }
    InstrInfo &   get_scatter_iinfo () override  { return _iinfo.second;   }
    TraceInfo &   get_trace_info() override      { return _trace_info;     }
    InstrWindow & get_instr_window() override    { return _iw;             }

    void set_trace_file(const std::string & trace_file_name) { _trace_file_name = trace_file_name; }
    const std::string & get_trace_file_name() { return _trace_file_name; }

    void set_binary_file(const std::string & binary_file_name) { _binary_file_name = binary_file_name; }
    const std::string & get_binary_file_name() { return _binary_file_name; }

    void update_metrics();

    std::string get_trace_file_prefix ();

    void process_traces();
    void update_source_lines();
    double update_source_lines_from_binary(mem_access_type);
    void process_second_pass(gzFile & fp_drtrace);

private:
    std::pair<Metrics, Metrics>     _metrics;
    std::pair<InstrInfo, InstrInfo> _iinfo;
    TraceInfo                       _trace_info;
    InstrWindow                     _iw;

    std::string                     _trace_file_name;
    std::string                     _binary_file_name;
};

Metrics & MemPatternsForPin::get_metrics(mem_access_type m)
{
    switch (m)
    {
        case GATHER : return _metrics.first;
            break;
        case SCATTER : return _metrics.second;
            break;
        default:
            throw GSError("Unable to get Metrics - Invalid Metrics Type: " + m);
    }
}

InstrInfo & MemPatternsForPin::get_iinfo(mem_access_type m)
{
    switch (m)
    {
        case GATHER : return _iinfo.first;
            break;
        case SCATTER : return _iinfo.second;
            break;
        default:
            throw GSError("Unable to get InstrInfo - Invalid Metrics Type: " + m);
    }
}

void MemPatternsForPin::handle_trace_entry(const InstrAddressInfo & ia)
{
    // Call libgs_patterns
    ::handle_trace_entry(*this, ia);
}

void MemPatternsForPin::generate_patterns()
{
    // ----------------- Update Source Lines -----------------

    update_source_lines();

    // ----------------- Update Metrics -----------------

    update_metrics();

    // ----------------- Create Spatter File -----------------

    ::create_spatter_file(*this, get_trace_file_prefix());

}

void MemPatternsForPin::update_metrics()
{
    gzFile fp_drtrace = ::open_trace_file(get_trace_file_name());

    // Get top gathers
    get_gather_metrics().ntop = get_top_target(get_gather_iinfo(), get_gather_metrics());

    // Get top scatters
    get_scatter_metrics().ntop = get_top_target(get_scatter_iinfo(), get_scatter_metrics());

    // ----------------- Second Pass -----------------

    process_second_pass(fp_drtrace);

    // ----------------- Normalize -----------------

    ::normalize_stats(get_gather_metrics());
    ::normalize_stats(get_scatter_metrics());

    close_trace_file(fp_drtrace);
}

std::string MemPatternsForPin::get_trace_file_prefix()
{
    std::string prefix = _trace_file_name;
    size_t pos = std::string::npos;
    while (std::string::npos != (pos = prefix.find(".gz")))
    {
        prefix.replace(pos, 3, "");
    }
    return prefix;
}

double MemPatternsForPin::update_source_lines_from_binary(mem_access_type mType)
{
    double scatter_cnt = 0.0;

    InstrInfo & target_iinfo   = get_iinfo(mType);
    Metrics &   target_metrics = get_metrics(mType);

    //Check it is not a library
    for (int k = 0; k < NGS; k++) {

        if (0 == target_iinfo.get_iaddrs()[k]) {
            break;
        }
        translate_iaddr(get_binary_file_name(), target_metrics.get_srcline()[k], target_iinfo.get_iaddrs()[k]);
        if (startswith(target_metrics.get_srcline()[k], "?"))
            target_iinfo.get_icnt()[k] = 0;

        scatter_cnt += target_iinfo.get_icnt()[k];
    }
    printf("done.\n");

    return scatter_cnt;
}

// First Pass
void MemPatternsForPin::process_traces()
{
    int iret = 0;
    trace_entry_t *drline;
    InstrWindow iw;

    gzFile fp_drtrace = open_trace_file(get_trace_file_name());

    printf("First pass to find top gather / scatter iaddresses\n");
    fflush(stdout);

    trace_entry_t *p_drtrace = NULL;
    trace_entry_t drtrace[NBUFS];  // was static (1024 bytes)

    while (drline_read(fp_drtrace, drtrace, &p_drtrace, &iret)) {
        //decode drtrace
        drline = p_drtrace;

        handle_trace_entry(InstrAddressInfoForPin(drline));

        p_drtrace++;
    }

    close_trace_file(fp_drtrace);

    //metrics
    get_trace_info().gather_occ_avg /= get_gather_metrics().cnt;
    get_trace_info().scatter_occ_avg /= get_scatter_metrics().cnt;

    display_stats(*this);

}

void MemPatternsForPin::process_second_pass(gzFile & fp_drtrace)
{
    uint64_t mcnt = 0;  // used our own local mcnt while iterating over file in this method.
    int iret = 0;
    trace_entry_t *drline;

    // State carried thru
    addr_t iaddr;
    int64_t maddr;
    addr_t gather_base[NTOP] = {0};
    addr_t scatter_base[NTOP] = {0};

    bool breakout = false;
    printf("\nSecond pass to fill gather / scatter subtraces\n");
    fflush(stdout);

    trace_entry_t *p_drtrace = NULL;
    trace_entry_t drtrace[NBUFS];   // was static (1024 bytes)

    while (drline_read(fp_drtrace, drtrace, &p_drtrace, &iret) && !breakout) {
        //decode drtrace
        drline = p_drtrace;

        breakout = ::handle_2nd_pass_trace_entry(InstrAddressInfoForPin(drline), get_gather_metrics(), get_scatter_metrics(),
                                                  iaddr, maddr, mcnt, gather_base, scatter_base);

        p_drtrace++;
    }
}

void MemPatternsForPin::update_source_lines()
{
    // Find source lines for gathers - Must have symbol
    printf("\nSymbol table lookup for gathers...");
    fflush(stdout);

    get_gather_metrics().cnt = update_source_lines_from_binary(GATHER);

    // Find source lines for scatters
    printf("Symbol table lookup for scatters...");
    fflush(stdout);

    get_scatter_metrics().cnt = update_source_lines_from_binary(SCATTER);
}

int main(int argc, char **argv)
{
    try
    {
        if (argc != 3) {
            throw GSError("Invalid arguments, should be: trace.gz binary_file_name");
        }

        MemPatternsForPin mp;

        mp.set_trace_file(argv[1]);
        mp.set_binary_file(argv[2]);

        // ----------------- Process Traces -----------------

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
