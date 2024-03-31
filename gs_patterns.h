//
// Created by christopher on 3/31/24.
//

#pragma once

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

class Metrics
{
public:
    typedef enum { GATHER=0, SCATTER } metrics_type;

    Metrics(metrics_type mType) : _mType(mType)
    {
        /// TODO: Convert to new/free
        for (int j = 0; j < NTOP; j++) {
            patterns[j] = (int64_t *) calloc(PSIZE, sizeof(int64_t));
            if (patterns[j] == NULL) {
                printf("ERROR: Could not allocate gather_patterns!\n");
                throw std::runtime_error("Could not allocate patterns for " + type_as_string());  //exit(-1);
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

    metrics_type _mType;
};

/*
class Address_Instr
{
public:
};
*/
