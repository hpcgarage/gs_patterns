//
// Created by christopher on 4/3/24.
//

#include <vector>

#include "gs_patterns.h"
#include "gs_patterns_core.h"

class MemPatternsForNV : public MemPatterns
{
public:
    MemPatternsForNV(): _metrics(GATHER, SCATTER),
                        _iinfo(GATHER, SCATTER)
    { }

    virtual ~MemPatternsForNV() override { }

    void handle_trace_entry(const trace_entry_t * tentry) override;
    void generate_patterns() override;

    Metrics &     get_metrics(metrics_type) override;
    InstrInfo &   get_iinfo(metrics_type) override;

    Metrics &     get_gather_metrics() override  { return _metrics.first;  }
    Metrics &     get_scatter_metrics() override { return _metrics.second; }
    InstrInfo &   get_gather_iinfo () override   { return _iinfo.first;    }
    InstrInfo &   get_scatter_iinfo () override  { return _iinfo.second;   }
    TraceInfo &   get_trace_info() override      { return _trace_info;     }
    InstrWindow & get_instr_window() override    { return _iw;             }

    //void set_binary_file(const std::string & binary_file_name) { _binary_file_name = binary_file_name; }
    //const std::string & get_binary_file_name() { return _binary_file_name; }

    void set_file_prefix(const std::string & prefix) { _file_prefix = prefix; }
    const std::string & get_file_prefix() { return _file_prefix; }

    void update_metrics();

    //void process_traces();
    void update_source_lines();
    double update_source_lines_from_binary(metrics_type);
    void process_second_pass();

private:
    std::pair<Metrics, Metrics>     _metrics;
    std::pair<InstrInfo, InstrInfo> _iinfo;
    TraceInfo                       _trace_info;
    InstrWindow                     _iw;

    std::string                     _binary_file_name;
    std::string                     _file_prefix;

    std::vector<trace_entry_t>      _traces;
};


Metrics & MemPatternsForNV::get_metrics(metrics_type m)
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

InstrInfo & MemPatternsForNV::get_iinfo(metrics_type m)
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

void MemPatternsForNV::handle_trace_entry(const trace_entry_t *tentry)
{
    // Call libgs_patterns
    ::handle_trace_entry(*this, tentry);

    _traces.push_back(*tentry);

    // TODO: Determine how to get source lines
}

void MemPatternsForNV::generate_patterns()
{
    // ----------------- Update Source Lines -----------------

     update_source_lines();

    // ----------------- Update Metrics -----------------

    update_metrics();

    // ----------------- Create Spatter File -----------------

    ::create_spatter_file(*this, get_file_prefix());

}

void MemPatternsForNV::update_metrics()
{
    // Get top gathers
    get_gather_metrics().ntop = get_top_target(get_gather_iinfo(), get_gather_metrics());

    // Get top scatters
    get_scatter_metrics().ntop = get_top_target(get_scatter_iinfo(), get_scatter_metrics());

    // ----------------- Second Pass -----------------

    process_second_pass();

    // ----------------- Normalize -----------------

    ::normalize_stats(get_gather_metrics());
    ::normalize_stats(get_scatter_metrics());
}


void MemPatternsForNV::process_second_pass()
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

    for (auto itr = _traces.begin(); itr != _traces.end(); ++itr)
    {
        trace_entry_t & drline = *itr;

        breakout = ::handle_2nd_pass_trace_entry(&drline, get_gather_metrics(), get_scatter_metrics(),
                                                 iaddr, maddr, mcnt, gather_base, scatter_base);
    }
}

