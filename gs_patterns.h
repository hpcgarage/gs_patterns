
#pragma once

#include <exception>
#include <string>
#include <cstring>
#include <utility>
#include <vector>
#include <cstdint>
#include <memory>

#include "errors.h"
#include "config.h"

//symbol lookup options
#if !defined(SYMBOLS_ONLY)
#define SYMBOLS_ONLY 1 //Filter out instructions that have no symbol
#endif

//info
// #define CLSIZE (64) //cacheline bytes - Unused - available via Config::get_instance().get_cache_line_size()

//patterns
#define NTOP (10)       //Final gather / scatters to keep - Used as compile time constant for
// sizing static arrays - skipping for now

#define MAX_LINE_LENGTH 1024 // Used as compile time constant for
// sizing static arrays - skipping for now

#if !defined(VBITS)
# define VBITS (512L)
# define VBYTES (VBITS/8)
#endif

namespace gs_patterns
{
    typedef uintptr_t addr_t;
    typedef enum { GATHER=0, SCATTER } mem_access_type;
    typedef enum { VECTOR=0, CTA } mem_instr_type;



    class InstrAddrAdapter
    {
    public:
        InstrAddrAdapter() = default;
        virtual ~InstrAddrAdapter() = default;

        [[nodiscard]] virtual bool            is_valid() const            = 0;
        [[nodiscard]] virtual bool            is_mem_instr() const        = 0;
        [[nodiscard]] virtual bool            is_other_instr() const      = 0;
        [[nodiscard]] virtual mem_access_type get_mem_access_type() const = 0;
        [[nodiscard]] virtual mem_instr_type  get_mem_instr_type() const  = 0;

        [[nodiscard]] virtual size_t         get_size() const             = 0;
        [[nodiscard]] virtual addr_t         get_base_addr() const        = 0;
        [[nodiscard]] virtual addr_t         get_address() const          = 0;
        [[nodiscard]] virtual addr_t         get_iaddr() const            = 0;
        [[nodiscard]] virtual addr_t         get_maddr() const            = 0;
        [[nodiscard]] virtual unsigned short get_type() const             = 0; // must be 0 for GATHER, 1 for SCATTER !!
        [[nodiscard]] virtual int64_t        get_max_access_size() const  = 0;

        [[nodiscard]] virtual bool is_gather() const
        { return (is_valid() && is_mem_instr() && GATHER == get_mem_access_type()) ? true : false; }

        [[nodiscard]] virtual bool is_scatter() const
        { return (is_valid() && is_mem_instr() && SCATTER == get_mem_access_type()) ? true : false; }

        virtual void output(std::ostream & os) const      = 0;
    };

    std::ostream & operator<<(std::ostream & os, const InstrAddrAdapter & ia);


    class Metrics
    {
    private:
        const size_t _initial_size;
        const size_t _top_patterns;
        const size_t _max_gather_scatter;
        const size_t _max_line_length;

        std::unique_ptr<char[]> srcline;

        mem_access_type _mType;

    public:
        // Proxy class for 2D array access
        // This class is public so it can be the return type of get_srcline()

        class SrcLine2D {
            char* base;
            size_t max_len;
        public:
            SrcLine2D(char* ptr, size_t ml) : base(ptr), max_len(ml) {}

            char* get(size_t j) {
                return base + (j * max_len);
            }
        };
        explicit Metrics(mem_access_type mType,
                size_t initial_size = Config::get_instance().get_initial_pattern_size(),
                size_t top_patterns = Config::get_instance().get_top_patterns(),
                size_t max_gather_scatter = Config::get_instance().get_max_gather_scatter(),
                size_t max_line_length = Config::get_instance().get_max_pattern_size()
            )
        :       _initial_size{initial_size},
                _top_patterns{top_patterns},
                _max_gather_scatter{max_gather_scatter},
                _max_line_length{max_line_length},
                srcline(std::make_unique<char[]>(2 * _max_gather_scatter * _max_line_length)),
                _mType(mType),
                offset(std::make_unique<int[]>(_top_patterns)),
                size(std::make_unique<int[]>(_top_patterns)),
                tot(std::make_unique<addr_t[]>(_top_patterns)),
                top(std::make_unique<addr_t[]>(_top_patterns)),
                top_idx(std::make_unique<addr_t[]>(_top_patterns)),
                patterns(_top_patterns)
        {
            try
            {
                for (int j = 0; j < _top_patterns; j++)
                {
                    patterns[j].resize(_initial_size);
                }
            }
            catch (const std::exception & ex)
            {
                throw GSAllocError("Could not allocate patterns for " + type_as_string() + "! due to: " + ex.what());
            }
        }

        ~Metrics() = default;

        [[nodiscard]] size_t get_pattern_size(int pattern_index) const
        {
            return patterns[pattern_index].size();
        }

        bool grow(int pattern_index) {
            try {
                size_t old_size = patterns[pattern_index].size();
                size_t new_size = old_size * 2;
                if (new_size > Config::get_instance().get_max_pattern_size()) {
                    return false;
                }

                patterns[pattern_index].resize(new_size);

                return true;
            }
            catch (...) {
                return false;
            }
        }

        Metrics(const Metrics &) = delete;
        Metrics & operator=(const Metrics & right) = delete;

        [[nodiscard]] std::string type_as_string() const { return !_mType ? "GATHER" : "SCATTER"; }
        [[nodiscard]] std::string getName() const { return !_mType ? "Gather" : "Scatter"; }
        [[nodiscard]] std::string getShortName() const { return !_mType ? "G" : "S"; }
        [[nodiscard]] std::string getShortNameLower() const { return !_mType ? "g" : "s"; }

        SrcLine2D get_srcline() {
            return SrcLine2D(srcline.get() + (_mType * _max_gather_scatter * _max_line_length), _max_line_length);
        }

        int      ntop = 0;
        int64_t  iaddrs_nosym = 0;
        int64_t  indices_nosym = 0;
        int64_t  iaddrs_sym = 0;
        int64_t  indices_sym = 0;
        double   cnt = 0.0;

        std::unique_ptr<int[]> offset;
        std::unique_ptr<int[]> size;

        std::unique_ptr<addr_t[]> tot;
        std::unique_ptr<addr_t[]> top;
        std::unique_ptr<addr_t[]> top_idx;
        std::vector<std::vector<int64_t>> patterns;
    };


    class InstrInfo
    {
    public:
        explicit InstrInfo(mem_access_type mType,
                size_t max_gather_scatter = Config::get_instance().get_max_gather_scatter())
        :
            _mType(mType),
            _max_gather_scatter(max_gather_scatter),
            _iaddrs(std::make_unique<addr_t[]>(2 * _max_gather_scatter)),
            _icnt(std::make_unique<int64_t[]>(2 * _max_gather_scatter)),
            _occ(std::make_unique<int64_t[]>(2 * _max_gather_scatter))

        { }
        ~InstrInfo() = default;

        InstrInfo(const InstrInfo &) = delete;
        InstrInfo & operator=(const InstrInfo & right) = delete;

        addr_t*  get_iaddrs() { return &_iaddrs[_mType * _max_gather_scatter]; }
        int64_t* get_icnt()   { return &_icnt[_mType * _max_gather_scatter]; }
        int64_t* get_occ()    { return &_occ[_mType * _max_gather_scatter]; }

    private:
        const size_t _max_gather_scatter;
        std::unique_ptr<addr_t[]> _iaddrs;
        std::unique_ptr<int64_t[]> _icnt; //vector instances
        std::unique_ptr<int64_t[]> _occ; //load/store instances

        mem_access_type _mType;
    };

    class TraceInfo  // Stats
    {
    public:
        /// TODO: need a reset method to zero out counters

        uint64_t opcodes     = 0;
        uint64_t opcodes_mem = 0;
        uint64_t addrs       = 0;
        uint64_t other       = 0;
        int64_t  ngs         = 0;
        int64_t trace_lines  = 0;

        bool    did_opcode      = false; // revist this ---------------
        double  other_cnt       = 0.0;
        double  gather_score    = 0.0;
        double  gather_occ_avg  = 0.0;
        double  scatter_occ_avg = 0.0;

        uint64_t     mcnt  = 0;
    };

    template <std::size_t MAX_ACCESS_SIZE>
    class InstrWindow
    {
    public:
        explicit InstrWindow(size_t window_size = Config::get_instance().get_instruction_window())
        : _window_size(window_size),
          _w_iaddrs{std::make_unique<int64_t[]>(2 * _window_size)},
          _w_bytes {std::make_unique<int64_t[]>(2 * _window_size)},
          _w_maddr {std::make_unique<int64_t[]>(2 * _window_size * MAX_ACCESS_SIZE)},
          _w_cnt   {std::make_unique<int64_t[]>(2 * _window_size)}
        {
            // First dimension is 0=GATHER/1=SCATTER
            init();
        }

        virtual ~InstrWindow() = default;

        void init() {
            for (int w = 0; w < 2; w++) {
                for (int i = 0; i < _window_size; i++) {
                    w_iaddrs(w, i) = -1;
                    w_bytes(w, i) = 0;
                    w_cnt(w, i) = 0;
                    for (uint64_t j = 0; j < MAX_ACCESS_SIZE; j++)
                        w_maddr(w, i, j) = -1;
                }
            }
        }

        void reset(int w) {
            for (int i = 0; i < _window_size; i++) {
                w_iaddrs(w, i) = -1;
                w_bytes(w, i) = 0;
                w_cnt(w, i) = 0;
                for (uint64_t j = 0; j < MAX_ACCESS_SIZE; j++)
                    w_maddr(w, i, j) = -1;
            }
        }

        void reset() {
            for (int w = 0; w < 2; w++) {
                reset(w);
            }
        }

        InstrWindow(const InstrWindow &) = delete;
        InstrWindow & operator=(const InstrWindow & right) = delete;

        int64_t & w_iaddrs(int32_t i, int32_t j)
        {
            return _w_iaddrs[i * _window_size + j];
        }
        int64_t & w_bytes(int32_t i, int32_t j)
        {
            return _w_bytes[i * _window_size + j];
        }
        int64_t & w_maddr(int32_t i, int32_t j, int32_t k)
        {
            return _w_maddr[i * _window_size * MAX_ACCESS_SIZE + j * MAX_ACCESS_SIZE + k];
        }
        int64_t & w_cnt(int32_t i, int32_t j)
        {
            return _w_cnt[i * _window_size + j];
        }

        [[nodiscard]] size_t get_window_size() const { return _window_size; }
        addr_t &  get_iaddr()       { return iaddr;      }
        int64_t & get_maddr_prev()  { return maddr_prev; }
        int64_t & get_maddr()       { return maddr;      }

    private:
        const size_t _window_size;
        // First dimension is 0=GATHER/1=SCATTER
        std::unique_ptr<int64_t[]> _w_iaddrs;
        std::unique_ptr<int64_t[]> _w_bytes;
        std::unique_ptr<int64_t[]> _w_maddr;
        std::unique_ptr<int64_t[]> _w_cnt;

        // State which must be carried with each call to handle a trace
        addr_t   iaddr = -1;
        int64_t  maddr_prev = -1;
        int64_t  maddr = -1;
    };

    template <std::size_t MAX_ACCESS_SIZE>
    class MemPatterns
    {
    public:
        MemPatterns() = default;
        virtual ~MemPatterns() = default;

        MemPatterns(const MemPatterns &) = delete;
        MemPatterns & operator=(const MemPatterns &) = delete;

        virtual void handle_trace_entry(const InstrAddrAdapter & ia) = 0;
        virtual void generate_patterns() = 0;

        virtual Metrics &     get_metrics(mem_access_type) = 0;
        virtual InstrInfo &   get_iinfo(mem_access_type)   = 0;

        virtual Metrics &     get_gather_metrics()      = 0;
        virtual Metrics &     get_scatter_metrics()     = 0;
        virtual InstrInfo &   get_gather_iinfo()        = 0;
        virtual InstrInfo &   get_scatter_iinfo()       = 0;
        virtual TraceInfo &   get_trace_info()          = 0;
        virtual InstrWindow<MAX_ACCESS_SIZE> &
                              get_instr_window()        = 0;
        virtual void          set_log_level(int8_t ll)  = 0;
        virtual int8_t        get_log_level()           = 0;
    };

} // namespace gs_patterns
