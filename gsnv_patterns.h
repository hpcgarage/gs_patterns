
#pragma  once

#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <zlib.h>
#include <map>
#include <stdlib.h>
#include <cmath>

#include "gs_patterns.h"
#include "gs_patterns_core.h"
#include "utils.h"

#define HEX(x)                                                            \
    "0x" << std::setfill('0') << std::setw(16) << std::hex << (uint64_t)x \
         << std::dec

#include "nvbit_tracing/nvgs_trace/common.h"

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

int tline_read(gzFile fp, mem_access_t * val, mem_access_t **p_val, int *edx) {

    int idx;

    idx = (*edx) / sizeof(trace_entry_t);
    //first read
    if (NULL == *p_val) {
        *edx = gzread(fp, val, sizeof(mem_access_t) * NBUFS);
        *p_val = val;

    } else if (*p_val == &val[idx]) {
        *edx = gzread(fp, val, sizeof(mem_access_t) * NBUFS);
        *p_val = val;
    }

    if (0 == *edx)
        return 0;

    return 1;
}

// An adapter for trace_entry_t (temporaritly untl replaced with nvbit memory detail type)
class InstrAddrAdapterForNV : public InstrAddrAdapter
{
public:
    InstrAddrAdapterForNV(const trace_entry_t * te) : _te(*te) { }
    InstrAddrAdapterForNV(const trace_entry_t te) : _te(te) { }

    virtual ~InstrAddrAdapterForNV() { }

    virtual bool is_valid() const override       { return true;  }
    virtual bool is_mem_instr() const override   { return true;  }
    virtual bool is_other_instr() const override { return false; }

    virtual mem_access_type get_mem_instr_type() const override { return (_te.type == 0) ? GATHER : SCATTER; }

    virtual size_t get_size() const override         {  return _te.size;  } // in bytes
    virtual addr_t get_address() const override      {  return _te.addr;  }
    virtual unsigned short get_type() const override {  return _te.type;  } // must be 0 for GATHER, 1 for SCATTER !!

    virtual void output(std::ostream & os) const override {     os << "InstrAddrAdapterForNV: trace entry: type: [" << _te.type << "] size: [" << _te.size << "]";}

private:
    trace_entry_t _te;
    //mem_access_t _ma;
};

class MemPatternsForNV : public MemPatterns
{
public:
    static const uint8_t CTA_LENGTH = 32;

    MemPatternsForNV(): _metrics(GATHER, SCATTER),
                        _iinfo(GATHER, SCATTER),
                        _ofs()
    { }

    virtual ~MemPatternsForNV() override {
        if (_write_trace_file) {
            _ofs.flush();
            _ofs.close();
        }

        /// TODO: COMPRESS trace_file on exit
#if 1
        std::cout << "-- OPCODE_ID to OPCODE MAPPING -- " << std::endl;
        for (auto itr = id_to_opcode_map.begin(); itr != id_to_opcode_map.end(); itr++) {
            std::cout << "OPCODE: " << itr->first << " -> " << itr->second << std::endl;
        }

        std::cout << "-- OPCODE_SHORT_ID to OPCODE_SHORT MAPPING -- " << std::endl;
        for (auto itr = id_to_opcode_short_map.begin(); itr != id_to_opcode_short_map.end(); itr++) {
            std::cout << "OPCODE_SHORT: " << itr->first << " -> " << itr->second << std::endl;
        }
#endif
    }

    void handle_trace_entry(const InstrAddrAdapter & ia) override;
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

    void set_file_prefix(const std::string & prefix) { _file_prefix = prefix; }
    std::string get_file_prefix();

    void update_metrics();

    std::string get_trace_file_prefix ();

    void process_traces();
    void update_source_lines();
    double update_source_lines_from_binary(mem_access_type);
    void process_second_pass();

    void set_trace_out_file(const std::string & trace_file_name) {
        _trace_out_file_name = trace_file_name;

        try
        {
            _ofs.open(trace_file_name, std::ios::binary);
            if (_ofs.is_open()) _write_trace_file = true;
        }
        catch (...)
        {
            throw GSFileError("Unable to open " + trace_file_name + " for writing");
        }
    }

    // Handle an nvbit CTA memory update
    void handle_cta_memory_access(const mem_access_t * ma);
    // Validate cta stride is within minimum
    bool valid_gs_stride(const std::vector<trace_entry_t> & te_list, const uint32_t min_stride);

    // store opcode mappings
    bool add_or_update_opcode(int opcode_id, const std::string & opcode) {
        auto it = id_to_opcode_map.find(opcode_id);
        if (it == id_to_opcode_map.end()) {
            id_to_opcode_map[opcode_id] = opcode;
            //std::cout << "OPCODE: " << opcode_id << " -> " << opcode << std::endl;
            return true;
        }
        return false;
    }
    // retreive opcode mapping by opcode_id
    const std::string & get_opcode(int opcode_id) {
        auto result = id_to_opcode_map.find(opcode_id);
        if (result != id_to_opcode_map.end()) {
            return result->second;
        }
        std::stringstream ss;
        ss << "Unknown opcode_id: " << opcode_id;
        throw GSDataError(ss.str());
    }

    // store opcode_short mappings
    bool add_or_update_opcode_short(int opcode_short_id, const std::string & opcode_short) {
        auto it = id_to_opcode_short_map.find(opcode_short_id);
        if (it == id_to_opcode_short_map.end()) {
            id_to_opcode_short_map[opcode_short_id] = opcode_short;
            //std::cout << "OPCODE: " << opcode_id << " -> " << opcode << std::endl;
            return true;
        }
        return false;
    }
    // retreive opcode_short mapping by opcode_short_id
    const std::string & get_opcode_short(int opcode_short_id) {
        auto result = id_to_opcode_short_map.find(opcode_short_id);
        if (result != id_to_opcode_short_map.end()) {
            return result->second;
        }
        std::stringstream ss;
        ss << "Unknown opcode_short_id: " << opcode_short_id;
        throw GSDataError(ss.str());
    }

#if 0
    std::vector<trace_entry_t> convert_to_trace_entry(const mem_access_t & ma)
    {
        // opcode : forms LD.E.64, ST.E.64

        std::string mem_type;
        std::string mem_attr;
        uint16_t    mem_size = 0;
        int count = 0;
        uint16_t mem_type_code = 0;
        uint16_t mem_attr_code = 0;

        //const char * m = reinterpret_cast<const char*>(&ma.opcode);
        //const std::string opcode(m, 8);
        std::string opcode = get_opcode(ma.opcode_id);

        size_t start=0, pos = 0;
        while (std::string::npos != (pos = opcode.find(".", start)))
        {
            count++;
            std::string token = opcode.substr(start, pos-start);
            uint64_t s;
            switch (count)
            {
                case 1: mem_type = token;
                    if ("LD" == mem_type)      { mem_type_code = 0; }
                    else if ("ST" == mem_type) { mem_type_code = 1; }
                    else throw GSDataError ("Invalid mem_type must be LD(0) or ST(1)");
                    break;

                case 2: mem_attr = token;
                    if ("E" == mem_attr)      { mem_attr_code = 1; }
                    else                      { mem_attr_code = 2; }
                    break;

                default:
                    throw GSDataError("Unsupported opcode: " + opcode);
            }
            start = pos+1;
        }
        // Snag the rest as mem_size
        if (start < opcode.length()) {
            std::string token = opcode.substr(start, opcode.length());
            int s = atoi(token.c_str());
            mem_size = (uint16_t) s;
        }
        else {
            throw GSDataError("Unsupported opcode: " + opcode);
        }

        // TODO: This is a SLOW way of doing this
        std::vector<trace_entry_t> te_list;
        te_list.reserve(MemPatternsForNV::CTA_LENGTH);
        for (int i = 0; i < MemPatternsForNV::CTA_LENGTH; i++)
        {
            if (ma.addrs[i] != 0)
            {
                trace_entry_t te { mem_type_code, mem_size, ma.addrs[i] };
                te_list.push_back(te);
            }
        }
        return std::move(te_list);
    }
#endif

    std::vector<trace_entry_t> convert_to_trace_entry(const mem_access_t & ma, bool ignore_partial_warps)
    {
        // opcode : forms LD.E.64, ST.E.64
        //std::string mem_type;
        uint16_t mem_size = ma.size;
        uint16_t mem_type_code;
        //uint16_t mem_attr_code = 0;

        if (ma.is_load)
            mem_type_code = GATHER;
        else if (ma.is_store)
            mem_type_code = SCATTER;
        else
            throw GSDataError ("Invalid mem_type must be LD(0) or ST(1)");

        //const char * m = reinterpret_cast<const char*>(&ma.opcode);
        //const std::string opcode(m, 8);
        std::string opcode = get_opcode(ma.opcode_id);
        std::string opcode_short = get_opcode_short(ma.opcode_short_id);

        // TODO: This is a SLOW way of doing this
        std::vector<trace_entry_t> te_list;
        te_list.reserve(MemPatternsForNV::CTA_LENGTH);
        for (int i = 0; i < MemPatternsForNV::CTA_LENGTH; i++)
        {
            if (ma.addrs[i] != 0)
            {
                trace_entry_t te { mem_type_code, mem_size, ma.addrs[i] };
                te_list.push_back(te);
            }
            else if (ignore_partial_warps)
            {
                // Ignore memory_accesses which have less than MemPatternsForNV::CTA_LENGTH
                return std::vector<trace_entry_t>();
            }
        }
        return std::move(te_list);
    }

private:

    std::pair<Metrics, Metrics>        _metrics;
    std::pair<InstrInfo, InstrInfo>    _iinfo;
    TraceInfo                          _trace_info;
    InstrWindow                        _iw;

    std::string                        _trace_file_name;
    std::string                        _binary_file_name;
    std::string                        _file_prefix;

    std::string                        _trace_out_file_name;
    bool                               _write_trace_file = false;
    std::ofstream                      _ofs;
    std::vector<InstrAddrAdapterForNV> _traces;

    //std::map<std::string, int> opcode_to_id_map;
    std::map<int, std::string> id_to_opcode_map;
    std::map<int, std::string> id_to_opcode_short_map;
};


Metrics & MemPatternsForNV::get_metrics(mem_access_type m)
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

InstrInfo & MemPatternsForNV::get_iinfo(mem_access_type m)
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

void MemPatternsForNV::handle_trace_entry(const InstrAddrAdapter & ia)
{
    // Call libgs_patterns
    ::handle_trace_entry(*this, ia);

    const InstrAddrAdapterForNV & ianv = dynamic_cast<const InstrAddrAdapterForNV &> (ia);
    _traces.push_back(ianv);

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

std::string MemPatternsForNV::get_file_prefix()
{
    if (!_file_prefix.empty()) return _file_prefix;

    // If no file_prefix was set try extracting one from trace_file
    std::string prefix = _trace_file_name;
    size_t pos = std::string::npos;
    while (std::string::npos != (pos = prefix.find(".gz")))
    {
        prefix.replace(pos, 3, "");
    }
    return prefix;
}

// First Pass
void MemPatternsForNV::process_traces()
{
    int iret = 0;
    mem_access_t * t_line;
    InstrWindow iw;

    gzFile fp_trace = open_trace_file(get_trace_file_name());

    printf("First pass to find top gather / scatter iaddresses\n");
    fflush(stdout);

    mem_access_t * p_trace = NULL;
    mem_access_t trace_buff[NBUFS]; // was static (1024 bytes)

    while (tline_read(fp_trace, trace_buff, &p_trace, &iret)) {
        //decode drtrace
        t_line = p_trace;

        if (-1 == t_line->cta_id_x) { break; }

        try
        {
            handle_cta_memory_access(t_line);

            p_trace++;
        }
        catch (const GSError & ex) {
            std::cerr << "ERROR: " << ex.what() << std::endl;
            throw;
        }
    }

    close_trace_file(fp_trace);

    //metrics
    get_trace_info().gather_occ_avg /= get_gather_metrics().cnt;
    get_trace_info().scatter_occ_avg /= get_scatter_metrics().cnt;

    display_stats(*this);

}


// TRY
void MemPatternsForNV::update_source_lines()
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

// TRY
double MemPatternsForNV::update_source_lines_from_binary(mem_access_type mType)
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

void MemPatternsForNV::process_second_pass()
{
    uint64_t mcnt = 0;  // used our own local mcnt while iterating over file in this method.
    int iret = 0;
//    trace_entry_t *drline;

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
        InstrAddrAdapter & ia = *itr;

        breakout = ::handle_2nd_pass_trace_entry(ia, get_gather_metrics(), get_scatter_metrics(),
                                                 iaddr, maddr, mcnt, gather_base, scatter_base);
    }
}

void MemPatternsForNV::handle_cta_memory_access(const mem_access_t * ma)
{
    if (_write_trace_file && _ofs.is_open()) {
        // Write entry to trace_output file
        _ofs.write(reinterpret_cast<const char*>(ma), sizeof *ma);
    }
#if 0
    std::stringstream ss;
    //ss << "CTX " << HEX(ctx) << " - grid_launch_id "
    ss << "GSNV_TRACE: CTX " << " - grid_launch_id "
       << ma->grid_launch_id << " - CTA " << ma->cta_id_x << "," << ma->cta_id_y << "," << ma->cta_id_z
       << " - warp " << ma->warp_id << " - " << get_opcode(ma->opcode_id)
       << " - shortOpcode: " << ma->opcode_short_id
       << " isLoad: " << ma->is_load << " isStore: " << ma->is_store << " - ";

    for (int i = 0; i < MemPatternsForNV::CTA_LENGTH; i++) {
        ss << HEX(ma->addrs[i]) << " ";
    }
    std::cout << ss.str() << std::endl;
#endif

    // Convert to vector of trace_entry_t if full warp. ignore partial warps.
    std::vector<trace_entry_t> te_list = convert_to_trace_entry(*ma, true);
    uint64_t min_size = !te_list.empty() ? (te_list[0].size) + 1 : 0;
    if (min_size > 0 && valid_gs_stride(te_list, min_size))
    {
        for (auto it = te_list.begin(); it != te_list.end(); it++)
        {
            handle_trace_entry(InstrAddrAdapterForNV(*it));
        }
    }
}

bool MemPatternsForNV::valid_gs_stride(const std::vector<trace_entry_t> & te_list, const uint32_t min_stride)
{
    bool valid_stride = false;
    uint32_t min_stride_found = INT32_MAX;
    uint64_t last_addr = 0;
    bool first = true;
    for (auto it = te_list.begin(); it != te_list.end(); it++)
    {
        const trace_entry_t & te = *it;
        if (first) {
            first = false;
            last_addr = te.addr;
            continue;
        }

        uint64_t diff = std::labs (last_addr - (uint64_t)te.addr);
        if (diff < min_stride_found)
            min_stride_found = diff;

        last_addr = te.addr;
    }

    return min_stride_found >= min_stride;
}