
#pragma  once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>

#include <zlib.h>
#include <stdlib.h>
#include <cmath>
#include <string.h>

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

#define MAP_NAME_SIZE 24
#define MAP_VALUE_SIZE 22

struct _trace_map_entry_t
{
    // 32 bytes total
    char     map_name[MAP_NAME_SIZE];
    uint16_t id;
    char     val[MAP_VALUE_SIZE];
};
typedef struct _trace_map_entry_t trace_map_entry_t;

struct _trace_header_t {
    uint64_t  num_map_entires;
    uint64_t  num_maps;
};
typedef struct _trace_header_t trace_header_t;


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

int tline_read_header(gzFile fp, trace_header_t * val, trace_header_t **p_val, int *edx) {

    int idx;

    idx = (*edx) / sizeof(trace_entry_t);
    //first read
    if (NULL == *p_val) {
        *edx = gzread(fp, val, sizeof(trace_header_t));
        *p_val = val;
    }
    else if (*p_val == &val[idx]) {
        *edx = gzread(fp, val, sizeof(trace_header_t));
        *p_val = val;
    }

    if (0 == *edx)
        return 0;

    return 1;
}

int tline_read_maps(gzFile fp, trace_map_entry_t * val, trace_map_entry_t **p_val, int *edx) {

    int idx;

    idx = (*edx) / sizeof(trace_map_entry_t);
    //first read
    if (NULL == *p_val) {
        *edx = gzread(fp, val, sizeof(trace_map_entry_t));
        *p_val = val;
    }
    else if (*p_val == &val[idx]) {
        *edx = gzread(fp, val, sizeof(trace_map_entry_t));
        *p_val = val;
    }

    if (0 == *edx)
        return 0;

    return 1;
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

    static constexpr const char * ID_TO_OPCODE        = "ID_TO_OPCODE";
    static constexpr const char * ID_TO_OPCODE_SHORT  = "ID_TO_OPCODE_SHORT";

    static constexpr const char * NVGS_TARGET_KERNEL  = "NVGS_TARGET_KERNEL";
    static constexpr const char * NVGS_TRACE_OUT_FILE = "NVGS_TRACE_OUT_FILE";
    static constexpr const char * NVGS_PROGRAM_BINARY = "NVGS_PROGRAM_BINARY";

    MemPatternsForNV(): _metrics(GATHER, SCATTER),
                        _iinfo(GATHER, SCATTER),
                        _ofs_tmp()  { }

    virtual ~MemPatternsForNV() override ;

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

    void set_trace_file(const std::string & trace_file_name);
    const std::string & get_trace_file_name() { return _trace_file_name; }

    void set_binary_file(const std::string & binary_file_name) { _binary_file_name = binary_file_name; }
    const std::string & get_binary_file_name() { return _binary_file_name; }

    void set_file_prefix(const std::string & prefix) { _file_prefix = prefix; }
    std::string get_file_prefix();

    void set_config_file (const std::string & config_file);

    void update_metrics();

    std::string get_trace_file_prefix ();

    void process_traces();
    void update_source_lines();
    double update_source_lines_from_binary(mem_access_type);
    void process_second_pass();

    void set_trace_out_file(const std::string & trace_file_name);
     void write_trace_out_file();

    // Handle an nvbit CTA memory update
    void handle_cta_memory_access(const mem_access_t * ma);
    // Validate cta stride is within minimum
    bool valid_gs_stride(const std::vector<trace_entry_t> & te_list, const uint32_t min_stride);

    // store opcode mappings
    bool add_or_update_opcode(int opcode_id, const std::string & opcode);
    // retreive opcode mapping by opcode_id
    const std::string & get_opcode(int opcode_id);
    // store opcode_short mappings
    bool add_or_update_opcode_short(int opcode_short_id, const std::string & opcode_short);
    // retreive opcode_short mapping by opcode_short_id
    const std::string & get_opcode_short(int opcode_short_id);

    bool should_instrument(const std::string & kernel_name);

    std::vector<trace_entry_t> convert_to_trace_entry(const mem_access_t & ma, bool ignore_partial_warps);

private:

    std::pair<Metrics, Metrics>        _metrics;
    std::pair<InstrInfo, InstrInfo>    _iinfo;
    TraceInfo                          _trace_info;
    InstrWindow                        _iw;

    std::string                        _trace_file_name;
    std::string                        _binary_file_name;
    std::string                        _file_prefix;
    std::string                        _trace_out_file_name;
    std::string                        _tmp_trace_out_file_name;
    std::string                        _config_file_name;
    std::set<std::string>              _target_kernels;

    bool                               _write_trace_file = false;
    bool                               _first_access     = true;
    std::ofstream                      _ofs_tmp;
    std::ofstream                      _ofs;
    std::vector<InstrAddrAdapterForNV> _traces;

    std::map<int, std::string> id_to_opcode_map;
    std::map<int, std::string> id_to_opcode_short_map;
};

MemPatternsForNV::~MemPatternsForNV()
{
    if (_write_trace_file)
    {
        write_trace_out_file();
        /// TODO: COMPRESS trace_file on exit
    }
}

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

// store opcode mappings
bool MemPatternsForNV::add_or_update_opcode(int opcode_id, const std::string & opcode) {
    auto it = id_to_opcode_map.find(opcode_id);
    if (it == id_to_opcode_map.end()) {
        id_to_opcode_map[opcode_id] = opcode;
        //std::cout << "OPCODE: " << opcode_id << " -> " << opcode << std::endl;
        return true;
    }
    return false;
}

// retreive opcode mapping by opcode_id
const std::string & MemPatternsForNV::get_opcode(int opcode_id) {
    auto result = id_to_opcode_map.find(opcode_id);
    if (result != id_to_opcode_map.end()) {
        return result->second;
    }
    std::stringstream ss;
    ss << "Unknown opcode_id: " << opcode_id;
    throw GSDataError(ss.str());
}

// store opcode_short mappings
bool MemPatternsForNV::add_or_update_opcode_short(int opcode_short_id, const std::string & opcode_short) {
    auto it = id_to_opcode_short_map.find(opcode_short_id);
    if (it == id_to_opcode_short_map.end()) {
        id_to_opcode_short_map[opcode_short_id] = opcode_short;
        //std::cout << "OPCODE: " << opcode_id << " -> " << opcode << std::endl;
        return true;
    }
    return false;
}

// retreive opcode_short mapping by opcode_short_id
const std::string & MemPatternsForNV::get_opcode_short(int opcode_short_id) {
    auto result = id_to_opcode_short_map.find(opcode_short_id);
    if (result != id_to_opcode_short_map.end()) {
        return result->second;
    }
    std::stringstream ss;
    ss << "Unknown opcode_short_id: " << opcode_short_id;
    throw GSDataError(ss.str());
}

/*
 * Read traces from a nvbit trace file. Includes header which describes opcode mappings used in trace data.
 * Used by test runner (gsnv_test) to simulate nvbit execution.
 */
void MemPatternsForNV::process_traces()
{
    int iret = 0;
    mem_access_t * t_line;
    InstrWindow iw;

    gzFile fp_trace = open_trace_file(get_trace_file_name());

    // Read header **
    trace_header_t * p_header = NULL;
    trace_header_t  header[1];
    tline_read_header(fp_trace, header, &p_header, &iret);

    uint32_t count = 0;
    trace_map_entry_t * p_map_entry = NULL;
    trace_map_entry_t map_entry[1];
    while (count < p_header->num_map_entires && tline_read_maps(fp_trace, map_entry, &p_map_entry, &iret) )
    {
        std::cout << "MAP ENTRY: " << p_map_entry -> map_name << " " << p_map_entry->id << " -> " << p_map_entry->val << std::endl;
        if (std::string(p_map_entry->map_name) == ID_TO_OPCODE) {
            id_to_opcode_map[p_map_entry->id] = p_map_entry->val;
        }
        else if (std::string(p_map_entry->map_name) == ID_TO_OPCODE_SHORT) {
            id_to_opcode_short_map[p_map_entry->id]  = p_map_entry->val;
        }
        else {
            std::cerr << "Unsupported Map: " << p_map_entry->map_name << " found in trace, ignoring ..."
                      << p_map_entry->id << " -> " << p_map_entry->val << std::endl;
        }

        count++;
        p_map_entry++;
    }

    // Read Traces **
    mem_access_t * p_trace = NULL;
    mem_access_t trace_buff[NBUFS]; // was static (1024 bytes)
    while (tline_read(fp_trace, trace_buff, &p_trace, &iret))
    {
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

std::vector<trace_entry_t> MemPatternsForNV::convert_to_trace_entry(const mem_access_t & ma, bool ignore_partial_warps)
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
    return te_list;
}

void MemPatternsForNV::handle_cta_memory_access(const mem_access_t * ma)
{
    if (_first_access) {
        _first_access = false;
        printf("First pass to find top gather / scatter iaddresses\n");
        fflush(stdout);
    }

    if (_write_trace_file && _ofs_tmp.is_open()) {
        // Write entry to trace_output file
        _ofs_tmp.write(reinterpret_cast<const char*>(ma), sizeof *ma);
    }
#if 0
    std::stringstream ss;
    //ss << "CTX " << HEX(ctx) << " - grid_launch_id "
    ss << "GSNV_TRACE: CTX " << " - grid_launch_id "
       << ma->grid_launch_id << " - CTA " << ma->cta_id_x << "," << ma->cta_id_y << "," << ma->cta_id_z
       << " - warp " << ma->warp_id << " - " << get_opcode(ma->opcode_id)
       << " - shortOpcode: " << ma->opcode_short_id
       << " isLoad: " << ma->is_load << " isStore: " << ma->is_store
       << " size: " << ma->size << " - ";

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


void MemPatternsForNV::set_trace_file(const std::string & trace_file_name)
{
    if (trace_file_name == _trace_out_file_name) {
        throw GSError ("Cannot set trace input file to same name as trace output file [" + trace_file_name + "].");
    }

    _trace_file_name = trace_file_name;
}

void MemPatternsForNV::set_trace_out_file(const std::string & trace_out_file_name)
{
    try
    {
        if (trace_out_file_name == _trace_file_name) {
            throw GSError ("Cannot set trace output file to same name as trace input file [" + trace_out_file_name + "].");
        }

        _trace_out_file_name = trace_out_file_name;
        _tmp_trace_out_file_name = _trace_out_file_name + ".tmp";

        // Open a temp file for writing data
        _ofs_tmp.open(_tmp_trace_out_file_name, std::ios::binary | std::ios::trunc | std::ios::in);
        if (!_ofs_tmp.is_open()) {
            throw GSFileError("Unable to open " + _tmp_trace_out_file_name + " for writing");
        }

        // Open a ouput file for writing data header and appending data
        _ofs.open(_trace_out_file_name, std::ios::binary | std::ios::trunc);
        if (!_ofs.is_open()) {
            throw GSFileError("Unable to open " + _trace_out_file_name + " for writing");
        }
        _write_trace_file = true;
    }
    catch (const std::exception & ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        throw;
    }
}

void MemPatternsForNV:: write_trace_out_file()
{
    if (!_write_trace_file) return;

    try
    {
        _ofs_tmp.flush();

        // Write header
        trace_header_t header;
        header.num_maps = 2;
        header.num_map_entires = id_to_opcode_map.size() + id_to_opcode_short_map.size();
        _ofs.write(reinterpret_cast<const char *>(&header), sizeof header);

        // Write Maps
        trace_map_entry_t m_entry;
        strncpy(m_entry.map_name, ID_TO_OPCODE, MAP_NAME_SIZE);
        for (auto itr = id_to_opcode_map.begin(); itr != id_to_opcode_map.end(); itr++)
        {
            m_entry.id = itr->first;
            strncpy(m_entry.val, itr->second.c_str(), MAP_VALUE_SIZE); // write 22 chars
            _ofs.write(reinterpret_cast<const char *>(&m_entry), sizeof m_entry);
        }

        strncpy(m_entry.map_name, ID_TO_OPCODE_SHORT, MAP_NAME_SIZE);
        //uint64_t opcode_short_len = id_to_opcode_short_map.size();
        for (auto itr = id_to_opcode_short_map.begin(); itr != id_to_opcode_short_map.end(); itr++)
        {
            m_entry.id = itr->first;
            strncpy(m_entry.val, itr->second.c_str(), MAP_VALUE_SIZE); // write 22 chars
            _ofs.write(reinterpret_cast<const char *>(&m_entry), sizeof m_entry);
        }

        // Write file contents
        _ofs_tmp.close();
        std::ifstream ifs(_tmp_trace_out_file_name);
        if (!ifs.is_open()) {
            throw GSFileError("Unable to open " + _tmp_trace_out_file_name + " for reading");
        }

        _ofs.flush();
        _ofs << ifs.rdbuf();
        _ofs.flush();
        _ofs.close();
        ifs.close();

        std::remove(_tmp_trace_out_file_name.c_str());

        std::cout << "-- OPCODE_ID to OPCODE MAPPING -- " << std::endl;
        for (auto itr = id_to_opcode_map.begin(); itr != id_to_opcode_map.end(); itr++) {
            std::cout << itr->first << " -> " << itr->second << std::endl;
        }

        std::cout << "-- OPCODE_SHORT_ID to OPCODE_SHORT MAPPING -- " << std::endl;
        for (auto itr = id_to_opcode_short_map.begin(); itr != id_to_opcode_short_map.end(); itr++) {
            std::cout << itr->first << " -> " << itr->second << std::endl;
        }
    }
    catch (const std::exception & ex)
    {
        std::cerr << "ERROR: failed to write trace file: " << _trace_file_name << std::endl;
        throw;
    }
}

void MemPatternsForNV::set_config_file(const std::string & config_file)
{
    _config_file_name = config_file;
    std::ifstream ifs;
    ifs.open(_config_file_name);
    if (!ifs.is_open())
        throw GSFileError("Unable to open config file: " + _config_file_name);

    while (!ifs.eof())
    {
        std::string name;
        std::string value;
        ifs >> name >> value;
        if (name.empty() || value.empty())
            continue;

        std::cout << "CONFIG: name: " << name << " value: " << value << std::endl;

        if (NVGS_TARGET_KERNEL == name) {
            _target_kernels.insert(value);
        }
        else if (NVGS_TRACE_OUT_FILE == name)
        {
            set_trace_out_file(value);
        }
        else if (NVGS_PROGRAM_BINARY == name) {
            set_binary_file(value);
        }
        else {
            std::cerr << "Unknown setting <" << name << "> with value <" << value << "> "
                      << "specified in config file: " << _config_file_name << " ignoring ..." << std::endl;
        }
    }
}

bool MemPatternsForNV::should_instrument(const std::string & kernel_name)
{
    // Instrument all if none specified
    if (_target_kernels.size() == 0) {
        std::cout << "Instrumenting all <by default>: " << kernel_name << std::endl;
        return true;
    }

    auto itr = _target_kernels.find (kernel_name);
    if ( itr != _target_kernels.end())  // Hard code for now
    {
        std::cout << "Instrumenting: " << kernel_name << std::endl;
        return  true;
    }

    return false;
}
