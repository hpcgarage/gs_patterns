
#pragma  once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <unordered_map>
#include <set>
#include <filesystem>

#include <zlib.h>
#include <stdlib.h>
#include <cmath>
#include <string.h>
#include <algorithm>

#include "gs_patterns.h"
#include "gs_patterns_core.h"
#include "utils.h"

// Enable to use a vector for storing trace data for use by second pass (if not defined data is stored to a temp file
//#define USE_VECTOR_FOR_SECOND_PASS 1

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
    addr_t         base_addr;
    char           padding[4];
}  __attribute__((packed));
typedef struct _trace_entry_t trace_entry_t;

#define MAP_NAME_SIZE 24
#define MAP_VALUE_SIZE 22
#define MAP_VALUE_LONG_SIZE 94
#define NUM_MAPS 3
// Setting this to fit within a 4k page e.g 170 * 24 bytes <= 4k
#define TRACE_BUFFER_LENGTH 170

struct _trace_map_entry_t
{
    // 32 bytes total
    char     map_name[MAP_NAME_SIZE];
    uint16_t id;
    char     val[MAP_VALUE_LONG_SIZE];
};
typedef struct _trace_map_entry_t trace_map_entry_t;

struct _trace_header_t {
    uint64_t  num_maps;
    uint64_t  num_map_entires;
    uint64_t  total_traces;
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

    idx = (*edx) / sizeof(trace_header_t);
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

    idx = (*edx) / sizeof(mem_access_t);
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
    InstrAddrAdapterForNV(const trace_entry_t & te) : _te(te) { }

    virtual ~InstrAddrAdapterForNV() { }

    virtual inline bool            is_valid() const override       { return true;          }
    virtual inline bool            is_mem_instr() const override   { return true;          }
    virtual inline bool            is_other_instr() const override { return false;         }
    virtual inline mem_access_type get_mem_access_type() const override { return (_te.type == 0) ? GATHER : SCATTER; }
    virtual inline mem_instr_type  get_mem_instr_type() const override  { return CTA;      }

    virtual inline size_t          get_size() const override       { return _te.size;      } // in bytes
    virtual inline addr_t          get_address() const override    { return _te.addr;      }
    virtual inline unsigned short  get_type() const override       { return _te.type;      } // must be 0 for GATHER, 1 for SCATTER !!
    virtual inline addr_t          get_iaddr () const override     { return _te.base_addr; }
    virtual inline int64_t         min_size() const override       { return  256;          }

    virtual void output(std::ostream & os) const override   {  os << "InstrAddrAdapterForNV: trace entry: type: ["
                                                                  << _te.type << "] size: [" << _te.size << "]";  }

    const trace_entry_t & get_trace_entry() const                  { return _te; }

private:
    const trace_entry_t  _te;
};

class MemPatternsForNV : public MemPatterns
{
public:
    static const uint8_t CTA_LENGTH = 32;

    static constexpr const char * ID_TO_OPCODE         = "ID_TO_OPCODE";
    static constexpr const char * ID_TO_OPCODE_SHORT   = "ID_TO_OPCODE_SHORT";
    static constexpr const char * ID_TO_LINE           = "ID_TO_LINE";

    static constexpr const char * NVGS_TARGET_KERNEL   = "NVGS_TARGET_KERNEL";
    static constexpr const char * NVGS_TRACE_OUT_FILE  = "NVGS_TRACE_OUT_FILE";
    static constexpr const char * NVGS_PROGRAM_BINARY  = "NVGS_PROGRAM_BINARY";
    static constexpr const char * NVGS_FILE_PREFIX     = "NVGS_FILE_PREFIX";
    static constexpr const char * NVGS_MAX_TRACE_COUNT = "NVGS_MAX_TRACE_COUNT";


    MemPatternsForNV(): _metrics(GATHER, SCATTER),
                        _iinfo(GATHER, SCATTER),
                        _target_opcodes { "LD", "ST", "LDS", "STS", "LDG", "STG" }
                        { }

    virtual ~MemPatternsForNV() override {  }

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
    inline const std::string & get_trace_file_name() { return _trace_file_name; }

    inline void set_file_prefix(const std::string & prefix) { _file_prefix = prefix; }
    std::string get_file_prefix();

    void set_max_trace_count(const std::string & max_trace_count_str);
    inline bool exceed_max_count() const {
        if (_limit_trace_count && (_trace_info.trace_lines >= _max_trace_count)) {
            return true;
        }
        return false;
    }

    // Mainly Called by nvbit kernel
    void set_config_file (const std::string & config_file);


    void update_metrics();

    void process_traces();
    void update_source_lines();
    double update_source_lines_from_binary(mem_access_type);
    void process_second_pass();

    std::string addr_to_line(addr_t addr)
    {
        auto itr = _addr_to_line_id.find(addr);
        if (itr != _addr_to_line_id.end()) {
            auto it2 = _id_to_line_map.find(itr->second);
            if (it2 != _id_to_line_map.end()) {
                return it2->second;
            }
        }
        return std::string();
    }

    void set_trace_out_file(const std::string & trace_file_name);
     void write_trace_out_file();

    // Handle an nvbit CTA memory update
    void handle_cta_memory_access(const mem_access_t * ma);
    // Validate cta stride is within minimum
    bool valid_gs_stride(const std::vector<trace_entry_t> & te_list, const uint32_t min_stride);

    // TODO: Migrate these to template functions !
    // -----------------------------------------------------------------

    // Store opcode mappings
    bool add_or_update_opcode(int opcode_id, const std::string & opcode);
    // Retrieve opcode mapping by opcode_id
    const std::string & get_opcode(int opcode_id);

    // Store opcode_short mappings
    bool add_or_update_opcode_short(int opcode_short_id, const std::string & opcode_short);
    // Retrieve opcode_short mapping by opcode_short_id
    const std::string & get_opcode_short(int opcode_short_id);

    // Store line mappings
    bool add_or_update_line(int line_id, const std::string & line);
    // Retrieve line number mapping by line_id
    const std::string & get_line(int line_id);

    // -----------------------------------------------------------------

    bool should_instrument(const std::string & kernel_name);

    bool convert_to_trace_entry(const mem_access_t & ma, bool ignore_partial_warps, std::vector<trace_entry_t> & te_list);

private:

    std::pair<Metrics, Metrics>        _metrics;
    std::pair<InstrInfo, InstrInfo>    _iinfo;
    TraceInfo                          _trace_info;
    InstrWindow                        _iw;

    std::string                        _trace_file_name;            // Input compressed nvbit trace file
    std::string                        _file_prefix;                // Used by gs_patterns_core to write out pattern files
    std::string                        _trace_out_file_name;        // Ouput file containing nvbit traces encounterd if requested
    std::string                        _tmp_trace_out_file_name;    // Temp file used to store traces before re-writing to _trace_out_filename

    std::string                        _config_file_name;
    std::set<std::string>              _target_kernels;
    bool                               _limit_trace_count = false;
    int64_t                            _max_trace_count   = 0;
    uint64_t                           _traces_written    = 0;
    uint64_t                           _traces_handled    = 0;

    bool                               _write_trace_file = false;
    bool                               _first_access     = true;

    /* The output stream used to temporarily hold raw trace warp data (mem_access_t) before being writen to _trace_out_file_name */
    std::fstream                       _ofs_tmp;
    /* The output stream cooresponding to _trace_out_file_name */
    std::ofstream                      _ofs;

#ifdef USE_VECTOR_FOR_SECOND_PASS
    /* A vector used to store intermediate trace records (trace_entry_t) exclusively for use by second pass
       (instead of _tmp_dump_file if USE_VECTOR_FOR_SECOND_PASS is defined) */
    std::vector<InstrAddrAdapterForNV> _traces;
#else
    /* A temp file used to store intermediate trace records (trace_entry_t) exclusively for use by second pass */
    std::FILE *                        _tmp_dump_file;
#endif

    std::map<int, std::string>       _id_to_opcode_map;
    std::map<int, std::string>       _id_to_opcode_short_map;
    std::map<int, std::string>       _id_to_line_map;
    std::unordered_map<addr_t, int>  _addr_to_line_id;
    const std::set<std::string>      _target_opcodes;
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

    const InstrAddrAdapterForNV &ianv = dynamic_cast<const InstrAddrAdapterForNV &> (ia);
#ifdef USE_VECTOR_FOR_SECOND_PASS
    _traces.push_back(ianv);
#else
    if (std::fwrite(reinterpret_cast<const char *>(&ianv.get_trace_entry()), sizeof(trace_entry_t), 1, _tmp_dump_file) != 1)
    {
        throw GSFileError("Write of trace to temp file failed");
    }
#endif
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

// Store opcode mappings
bool MemPatternsForNV::add_or_update_opcode(int opcode_id, const std::string & opcode) {
    auto it = _id_to_opcode_map.find(opcode_id);
    if (it == _id_to_opcode_map.end()) {
        _id_to_opcode_map[opcode_id] = opcode;
        //std::cout << "OPCODE: " << opcode_id << " -> " << opcode << std::endl;
        return true;
    }
    return false;
}

// Retrieve opcode mapping by opcode_id
const std::string & MemPatternsForNV::get_opcode(int opcode_id) {
    auto result = _id_to_opcode_map.find(opcode_id);
    if (result != _id_to_opcode_map.end()) {
        return result->second;
    }
    std::stringstream ss;
    ss << "Unknown opcode_id: " << opcode_id;
    throw GSDataError(ss.str());
}

// Store opcode_short mappings
bool MemPatternsForNV::add_or_update_opcode_short(int opcode_short_id, const std::string & opcode_short) {
    auto it = _id_to_opcode_short_map.find(opcode_short_id);
    if (it == _id_to_opcode_short_map.end()) {
        _id_to_opcode_short_map[opcode_short_id] = opcode_short;
        //std::cout << "OPCODE: " << opcode_id << " -> " << opcode << std::endl;
        return true;
    }
    return false;
}

// Retrieve opcode_short mapping by opcode_short_id
const std::string & MemPatternsForNV::get_opcode_short(int opcode_short_id) {
    auto result = _id_to_opcode_short_map.find(opcode_short_id);
    if (result != _id_to_opcode_short_map.end()) {
        return result->second;
    }
    std::stringstream ss;
    ss << "Unknown opcode_short_id: " << opcode_short_id;
    throw GSDataError(ss.str());
}

// Store line  mappings
bool MemPatternsForNV::add_or_update_line(int line_id, const std::string & line) {
    auto it = _id_to_line_map.find(line_id);
    if (it == _id_to_line_map.end()) {
        _id_to_line_map[line_id] = line;
        //std::cout << "LINE: " << line_id << " -> " << line << std::endl;
        return true;
    }
    return false;
}

// Retrieve line number mapping by line_id
const std::string & MemPatternsForNV::get_line(int line_id) {
    auto result = _id_to_line_map.find(line_id);
    if (result != _id_to_line_map.end()) {
        return result->second;
    }
    std::stringstream ss;
    ss << "Unknown line_id: " << line_id;
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
        std::cout << "MAP: " << p_map_entry -> map_name << " entry [" << p_map_entry->id << "] -> [" << p_map_entry->val << "]" << std::endl;

        if (std::string(p_map_entry->map_name) == ID_TO_OPCODE) {
            _id_to_opcode_map[p_map_entry->id] = p_map_entry->val;
        }
        else if (std::string(p_map_entry->map_name) == ID_TO_OPCODE_SHORT) {
            _id_to_opcode_short_map[p_map_entry->id]  = p_map_entry->val;
        }
        else if (std::string(p_map_entry->map_name) == ID_TO_LINE) {
            _id_to_line_map[p_map_entry->id]  = p_map_entry->val;
        }
        else {
            std::cerr << "Unsupported Map: " << p_map_entry->map_name << " found in trace, ignoring ..."
                      << p_map_entry->id << " -> " << p_map_entry->val << std::endl;
        }

        count++;
        p_map_entry++;
    }

    // Read Traces **
    iret = 0;
    uint64_t lines_read = 0;
    uint64_t pos = 0;
    mem_access_t * p_trace = NULL;
    mem_access_t trace_buff[NBUFS]; // was static (1024 bytes)
    while (tline_read(fp_trace, trace_buff, &p_trace, &iret))
    {
        // Decode trace
        t_line = p_trace;

        if (-1 == t_line->cta_id_x) { continue; }

        try
        {
            // Progress bar
            if (lines_read == 0) {
                for (int i = 0; i < 100; i++) { std::cout << "-"; }
                std::cout << std::endl;
            }
            if (lines_read % ((uint64_t) std::max((p_header->total_traces * .01), 1.0)) == 0) {
                if ((pos % 20) == 0) { std::cout << "|"; }
                else { std::cout << "+"; }
                std::flush(std::cout);
                pos++;
            }

            handle_cta_memory_access(t_line);

            p_trace++;
            lines_read++;
        }
        catch (const GSError & ex) {
            std::cerr << "ERROR: " << ex.what() << std::endl;
            close_trace_file(fp_trace);
            throw;
        }
    }

    std::cout << "\nLines Read: " << lines_read << " of Total: " << p_header->total_traces << std::endl;

    close_trace_file(fp_trace);

    //metrics
    get_trace_info().gather_occ_avg /= get_gather_metrics().cnt;
    get_trace_info().scatter_occ_avg /= get_scatter_metrics().cnt;

    display_stats(*this);

}

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

double MemPatternsForNV::update_source_lines_from_binary(mem_access_type mType)
{
    double target_cnt = 0.0;

    InstrInfo & target_iinfo   = get_iinfo(mType);
    Metrics &   target_metrics = get_metrics(mType);

    for (int k = 0; k < NGS; k++) {

        if (0 == target_iinfo.get_iaddrs()[k]) {
            break;
        }

        std::string line;
        line = addr_to_line(target_iinfo.get_iaddrs()[k]);
        strncpy(target_metrics.get_srcline()[k], line.c_str(), MAX_LINE_LENGTH-1);

        if (std::string(target_metrics.get_srcline()[k]).empty())
            target_iinfo.get_icnt()[k] = 0;

        target_cnt += target_iinfo.get_icnt()[k];
    }
    printf("done.\n");

    return target_cnt;

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

#if USE_VECTOR_FOR_SECOND_PASS
    for (auto itr = _traces.begin(); itr != _traces.end(); ++itr)
    {
        InstrAddrAdapter & ia = *itr;

        breakout = ::handle_2nd_pass_trace_entry(ia, get_gather_metrics(), get_scatter_metrics(),
                                                 iaddr, maddr, mcnt, gather_base, scatter_base);
        if (breakout) {
            break;
        }
    }
#else
    std::fflush(_tmp_dump_file);
    std::rewind(_tmp_dump_file); // Back to the future, ... sort of
    try
    {
        trace_entry_t ta[TRACE_BUFFER_LENGTH];
        size_t count_read = 0;
        size_t read;
        while ( !breakout && (read = std::fread(&ta, sizeof (ta[0]), TRACE_BUFFER_LENGTH, _tmp_dump_file)) )
        {
            for (int i = 0; i < read; i++)
            {
                InstrAddrAdapterForNV ia(const_cast<const trace_entry_t &>(ta[i]));
                breakout = ::handle_2nd_pass_trace_entry(ia, get_gather_metrics(), get_scatter_metrics(),
                                                         iaddr, maddr, mcnt, gather_base, scatter_base);
                count_read++;

                if (breakout) break;
            }
        }
        std::cout << "Reread: " << count_read << " for second_pass " << std::endl;

        if (!breakout && !std::feof(_tmp_dump_file)) {
            if (std::ferror(_tmp_dump_file)) {
                throw GSFileError("Unexpected error occurred while reading temp file");
            }
        }
        std::fclose(_tmp_dump_file);
    }
    catch (const GSError & ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        std::fclose(_tmp_dump_file);
        throw;
    }
#endif
}

bool MemPatternsForNV::convert_to_trace_entry(const mem_access_t & ma,
                                              bool ignore_partial_warps,
                                              std::vector<trace_entry_t> & te_list)
{
    uint16_t mem_size = ma.size;
    uint16_t mem_type_code;

    if (ma.is_load)
        mem_type_code = GATHER;
    else if (ma.is_store)
        mem_type_code = SCATTER;
    else
        throw GSDataError ("Invalid mem_type must be LD(0) or ST(1)");

    if (_id_to_opcode_short_map.find(ma.opcode_short_id) == _id_to_opcode_short_map.end())
        return false;
    std::string opcode_short = _id_to_opcode_short_map[ma.opcode_short_id];

    if (_target_opcodes.find(opcode_short) == _target_opcodes.end())
        return false;

    // TODO: This is a SLOW way of doing this
    const addr_t & base_addr = ma.addrs[0];
    te_list.reserve(MemPatternsForNV::CTA_LENGTH);
    for (int i = 0; i < MemPatternsForNV::CTA_LENGTH; i++)
    {
        if (ma.addrs[i] != 0)
        {
            trace_entry_t te { mem_type_code, mem_size, ma.addrs[i], base_addr };
            te_list.push_back(te);

            if (_addr_to_line_id.find(base_addr) == _addr_to_line_id.end()) {
                _addr_to_line_id[base_addr] = ma.line_id;
            }
        }
        else if (ignore_partial_warps)
        {
            // Ignore memory_accesses which have less than MemPatternsForNV::CTA_LENGTH
            return false;
        }
    }
    return true;
}

void MemPatternsForNV::handle_cta_memory_access(const mem_access_t * ma)
{
    if (exceed_max_count()) { return; }

    if (_first_access) {
        _first_access = false;
        printf("First pass to find top gather / scatter iaddresses\n");
        fflush(stdout);
    }

    if (_write_trace_file && _ofs_tmp.is_open()) {
        // Write entry to trace_output file
        _ofs_tmp.write(reinterpret_cast<const char*>(ma), sizeof *ma);
        _traces_written++;
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
    std::vector<trace_entry_t> te_list;
    te_list.reserve(MemPatternsForNV::CTA_LENGTH);

    bool status = convert_to_trace_entry(*ma, true, te_list);
    if (!status) return;

    uint64_t min_size = !te_list.empty() ? (te_list[0].size) + 1 : 0;
    if (min_size > 0 && valid_gs_stride(te_list, min_size))
    {
        for (auto it = te_list.begin(); it != te_list.end(); it++)
        {
            handle_trace_entry(InstrAddrAdapterForNV(*it));
        }
        _traces_handled++;
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
        if (diff < min_stride)
            return false;

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
        if (trace_out_file_name.empty()) {
            throw GSError ("Cannot set trace output file to empty filename [" + trace_out_file_name + "].");
        }

        if (trace_out_file_name == _trace_file_name) {
            throw GSError ("Cannot set trace output file to same name as trace input file [" + trace_out_file_name + "].");
        }

        _trace_out_file_name     = trace_out_file_name;
        _tmp_trace_out_file_name = _trace_out_file_name + ".tmp";

        // Open a temp file for writing data
        _ofs_tmp.open(_tmp_trace_out_file_name, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
        if (!_ofs_tmp.is_open()) {
            throw GSFileError("Unable to open " + _tmp_trace_out_file_name + " for writing");
        }
        std::remove(_tmp_trace_out_file_name.c_str());  // Force auto cleanup

        // Open a ouput file for writing data header and appending data
        _ofs.open(_trace_out_file_name, std::ios::binary | std::ios::trunc);
        if (!_ofs.is_open()) {
            throw GSFileError("Unable to open " + _trace_out_file_name + " for writing");
        }

#ifndef USE_VECTOR_FOR_SECOND_PASS
        // Open an output file for dumping temp data used exclusively by second_pass
        _tmp_dump_file = std::tmpfile();
        if (!_tmp_dump_file) {
            throw GSFileError("Unable to create a temp file for second pass");
        }
#endif
        _write_trace_file = true;
    }
    catch (const std::exception & ex)
    {
        std::cerr << "ERROR: " << ex.what() << std::endl;
        throw;
    }
}

void MemPatternsForNV::write_trace_out_file()
{
    if (!_write_trace_file || _first_access) return;

    /// TODO: COMPRESS trace_file
    try
    {
        std::cout << "\nSaving trace file - traces_written: " << _traces_written
                  << " traced_handled: " << _traces_handled << "\n" << std::endl;

        _ofs_tmp.flush();

        // Write header
        trace_header_t header;
        header.num_maps        = NUM_MAPS;
        header.num_map_entires = _id_to_opcode_map.size() +
                                 _id_to_opcode_short_map.size() +
                                 _id_to_line_map.size();
        header.total_traces    = _traces_written;

        _ofs.write(reinterpret_cast<const char *>(&header), sizeof header);

        // Write Maps
        trace_map_entry_t m_entry;
        strncpy(m_entry.map_name, ID_TO_OPCODE, MAP_NAME_SIZE-1);
        for (auto itr = _id_to_opcode_map.begin(); itr != _id_to_opcode_map.end(); itr++)
        {
            m_entry.id = itr->first;
            strncpy(m_entry.val, itr->second.c_str(), MAP_VALUE_LONG_SIZE-1);
            _ofs.write(reinterpret_cast<const char *>(&m_entry), sizeof m_entry);
        }

        strncpy(m_entry.map_name, ID_TO_OPCODE_SHORT, MAP_NAME_SIZE-1);
        //uint64_t opcode_short_map_len = _id_to_opcode_short_map.size();
        for (auto itr = _id_to_opcode_short_map.begin(); itr != _id_to_opcode_short_map.end(); itr++)
        {
            m_entry.id = itr->first;
            strncpy(m_entry.val, itr->second.c_str(), MAP_VALUE_LONG_SIZE-1);
            _ofs.write(reinterpret_cast<const char *>(&m_entry), sizeof m_entry);
        }

        strncpy(m_entry.map_name, ID_TO_LINE, MAP_NAME_SIZE-1);
        //uint64_t line_map_len = _id_to_line_map.size();
        for (auto itr = _id_to_line_map.begin(); itr != _id_to_line_map.end(); itr++)
        {
            m_entry.id = itr->first;
            strncpy(m_entry.val, itr->second.c_str(), MAP_VALUE_LONG_SIZE-1);
            _ofs.write(reinterpret_cast<const char *>(&m_entry), sizeof m_entry);
        }
        _ofs.flush();

        // Write file contents
        _ofs_tmp.seekp(0);
        _ofs << _ofs_tmp.rdbuf();
        _ofs.flush();
        _ofs.close();
        _ofs_tmp.close();

        std::remove(_tmp_trace_out_file_name.c_str());

        std::cout << "Mappings found" << std::endl;

        std::cout << "-- OPCODE_ID to OPCODE MAPPING -- " << std::endl;
        for (auto itr = _id_to_opcode_map.begin(); itr != _id_to_opcode_map.end(); itr++) {
            std::cout << itr->first << " -> " << itr->second << std::endl;
        }

        std::cout << "-- OPCODE_SHORT_ID to OPCODE_SHORT MAPPING -- " << std::endl;
        for (auto itr = _id_to_opcode_short_map.begin(); itr != _id_to_opcode_short_map.end(); itr++) {
            std::cout << itr->first << " -> " << itr->second << std::endl;
        }

        std::cout << "-- LINE_ID to LINE MAPPING -- " << std::endl;
        for (auto itr = _id_to_line_map.begin(); itr != _id_to_line_map.end(); itr++) {
            std::cout << itr->first << " -> " << itr->second << std::endl;
        }
    }
    catch (const std::exception & ex)
    {
        std::remove(_tmp_trace_out_file_name.c_str());
        std::cerr << "ERROR: failed to write trace file: " << _trace_file_name << std::endl;
        throw;
    }
}

void MemPatternsForNV::set_max_trace_count(const std::string & max_trace_count_str)
{
    try {
        _max_trace_count = (int64_t) std::stoi(max_trace_count_str);
        if (_max_trace_count < 0) {
            throw GSError("Max Trace count must be greater than 0");
        }
        _limit_trace_count = true;
        std::cout << "Max Trace Count set to: " << _max_trace_count << std::endl;
    }
    catch (const std::exception & ex) {
        std::cerr << "Failed to set Max Trace Count from value: " << max_trace_count_str
                  << " with error: " << ex.what() << std::endl;
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
        if (name.empty() || value.empty() || name[0] == '#')
            continue;

        std::cout << "CONFIG: name: " << name << " value: " << value << std::endl;

        if (NVGS_TARGET_KERNEL == name) {
            _target_kernels.insert(value);
        }
        else if (NVGS_TRACE_OUT_FILE == name) {
            set_trace_out_file(value);
        }
        else if (NVGS_FILE_PREFIX == name) {
            set_file_prefix(value);
        }
        else if (NVGS_MAX_TRACE_COUNT == name) {
            set_max_trace_count(value);
        }
        else {
            std::cerr << "Unknown setting <" << name << "> with value <" << value << "> "
                      << "specified in config file: " << _config_file_name << " ignoring ..." << std::endl;
        }
    }
}

bool MemPatternsForNV::should_instrument(const std::string & kernel_name)
{
    if (exceed_max_count()) { return false; }

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
