//
// Created by christopher on 3/31/24.
//

#pragma once

#include <exception>
#include <string>

#define MAX(X, Y) (((X) < (Y)) ? Y : X)
#define MIN(X, Y) (((X) > (Y)) ? Y : X)
#define ABS(X) (((X) < 0) ? (-1) * (X) : X)

//triggers
#define SAMPLE 0
#define PERSAMPLE 10000000
//#define PERSAMPLE 1000

//info
#define CLSIZE (64)
#define VBITS (512)
#define NBUFS (1LL<<10)
#define IWINDOW (1024)
#define NGS (8096)

//patterns
#define USTRIDES 1024   //Threshold for number of accesses
#define NSTRIDES 15     //Threshold for number of unique distances
#define OUTTHRESH (0.5) //Threshold for percentage of distances at boundaries of histogram
#define NTOP (10)
#define PSIZE (1<<23)
//#define PSIZE (1<<18)

//DONT CHANGE
#define VBYTES (VBITS/8)

//address status
#define ADDREND   (0xFFFFFFFFFFFFFFFFUL)
#define ADDRUSYNC (0xFFFFFFFFFFFFFFFEUL)

#define MAX_LINE_LENGTH 1024

typedef uintptr_t addr_t;

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

typedef enum { GATHER=0, SCATTER } mem_access_type;

class GSError : public std::exception
{
public:
    GSError (const std::string & reason) : _reason(reason) { }
    ~GSError() {}

    const char * what() const noexcept override { return _reason.c_str(); }
private:
    std::string _reason;
};

class GSFileError : public GSError
{
public:
    GSFileError (const std::string & reason) : GSError(reason) { }
    ~GSFileError() {}
};

class GSDataError : public GSError
{
public:
    GSDataError (const std::string & reason) : GSError(reason) { }
    ~GSDataError() {}
};

class GSAllocError : public GSError
{
public:
    GSAllocError (const std::string & reason) : GSError(reason) { }
    ~GSAllocError() {}
};

class InstrAddressInfo
{
public:
    InstrAddressInfo() { }
    virtual ~InstrAddressInfo() { }

    virtual bool is_valid() const                      = 0;
    virtual bool is_mem_instr() const                  = 0;
    virtual bool is_other_instr() const                = 0;
    virtual mem_access_type get_mem_instr_type() const = 0;

    virtual size_t get_size() const                   = 0;
    virtual addr_t get_address() const                = 0;
    virtual unsigned short get_type() const           = 0;
    // multiple?

    virtual bool is_gather() const
    { return (is_valid() && is_mem_instr() && GATHER == get_mem_instr_type()) ? true : false; }

    virtual bool is_scatter() const
    { return (is_valid() && is_mem_instr() && SCATTER == get_mem_instr_type()) ? true : false; }

    virtual void output(std::ostream & os) const      = 0;
};

std::ostream & operator<<(std::ostream & os, const InstrAddressInfo & ia);


class Metrics
{
public:
    Metrics(mem_access_type mType) : _mType(mType)
    {
        /// TODO: Convert to new/free
        for (int j = 0; j < NTOP; j++) {
            patterns[j] = (int64_t *) calloc(PSIZE, sizeof(int64_t));
            if (patterns[j] == NULL) {
                throw GSAllocError("Could not allocate patterns for " + type_as_string() + "!");
            }
        }
    }

    ~Metrics()
    {
        /// TODO: Convert to new/free
        for (int i = 0; i < NTOP; i++) {
            free(patterns[i]);
        }
    }

    Metrics(const Metrics &) = delete;
    Metrics & operator=(const Metrics & right) = delete;

    std::string type_as_string() { return !_mType ? "GATHER" : "SCATTER"; }
    std::string getName()        { return !_mType ? "Gather" : "Scatter"; }
    std::string getShortName()   { return !_mType ? "G" : "S"; }

    auto get_srcline() { return srcline[_mType]; }

//private:
    int      ntop = 0;
    double   cnt = 0.0;
    int      offset[NTOP]  = {0};

    addr_t   tot[NTOP]     = {0};
    addr_t   top[NTOP]     = {0};
    addr_t   top_idx[NTOP] = {0};

    int64_t* patterns[NTOP] = {0};

private:
    static char srcline[2][NGS][MAX_LINE_LENGTH]; // was static (may move out and have 1 per type)

    mem_access_type _mType;
};


class InstrInfo
{
public:
    InstrInfo(mem_access_type mType) : _mType(mType) { }
    ~InstrInfo() { }

    InstrInfo(const InstrInfo &) = delete;
    InstrInfo & operator=(const InstrInfo & right) = delete;

    addr_t*  get_iaddrs() { return iaddrs[_mType]; }
    int64_t* get_icnt()   { return icnt[_mType]; }
    int64_t* get_occ()    { return occ[_mType]; }

private:
    static addr_t iaddrs[2][NGS];
    static int64_t icnt[2][NGS];
    static int64_t occ[2][NGS];

    mem_access_type _mType;
};

class TraceInfo  // Stats
{
public:
    /// TODO: need a reset method to zero out counters

    uint64_t opcodes      = 0;
    uint64_t opcodes_mem  = 0;
    uint64_t addrs        = 0;
    uint64_t other        = 0;
    int64_t  ngs          = 0;
    int64_t drtrace_lines = 0;

    bool    did_opcode  = false; // revist this ---------------
    double  other_cnt       = 0.0;
    double  gather_score    = 0.0;
    double  gather_occ_avg  = 0.0;
    double  scatter_occ_avg = 0.0;

    uint64_t     mcnt  = 0;

};

class InstrWindow
{
public:
    InstrWindow() {
        //init window arrays
        for (int w = 0; w < 2; w++) {
            for (int i = 0; i < IWINDOW; i++) {
                w_iaddrs[w][i] = -1;
                w_bytes[w][i] = 0;
                w_cnt[w][i] = 0;
                for (int j = 0; j < VBYTES; j++)
                    w_maddr[w][i][j] = -1;
            }
        }
    }

    ~InstrWindow() { }

    InstrWindow(const InstrWindow &) = delete;
    InstrWindow & operator=(const InstrWindow & right) = delete;

#if 0
    static int64_t w_iaddrs[2][IWINDOW];
    static int64_t w_bytes[2][IWINDOW];
    static int64_t w_maddr[2][IWINDOW][VBYTES];
    static int64_t w_cnt[2][IWINDOW];
#else
    // moved from static storage to instance variables (watch out for stack overflow)
    // Revisit and move to heap if an issue - estimate of 2k*3 + 128k
    int64_t w_iaddrs[2][IWINDOW];
    int64_t w_bytes[2][IWINDOW];
    int64_t w_maddr[2][IWINDOW][VBYTES];
    int64_t w_cnt[2][IWINDOW];
#endif

    // State which must be carried with each call to handle a trace
    addr_t   iaddr;
    int64_t  maddr_prev;
    int64_t  maddr;
};

class MemPatterns
{
public:
    MemPatterns() { }
    virtual ~MemPatterns() { };

    MemPatterns(const MemPatterns &) = delete;
    MemPatterns & operator=(const MemPatterns &) = delete;

    virtual void handle_trace_entry(const trace_entry_t * te) = 0;
    virtual void generate_patterns() = 0;

    virtual Metrics &     get_metrics(mem_access_type) = 0;
    virtual InstrInfo &   get_iinfo(mem_access_type)   = 0;

    virtual Metrics &     get_gather_metrics()      = 0;
    virtual Metrics &     get_scatter_metrics()     = 0;
    virtual InstrInfo &   get_gather_iinfo()        = 0;
    virtual InstrInfo &   get_scatter_iinfo()       = 0;
    virtual TraceInfo &   get_trace_info()          = 0;
    virtual InstrWindow & get_instr_window()        = 0;
};