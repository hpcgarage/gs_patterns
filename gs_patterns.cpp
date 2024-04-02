#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <zlib.h>
#include <sys/resource.h>

#include <stdexcept>
#include <iostream>
#include <exception>

#include "gs_patterns.h"

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

static inline int popcount(uint64_t x) {
    int c;

    for (c = 0; x != 0; x >>= 1)
        if (x & 1)
            c++;
    return c;
}

//string tools
int startswith(const char *a, const char *b) {
    if (strncmp(b, a, strlen(b)) == 0)
        return 1;
    return 0;
}

int endswith(const char *a, const char *b) {
    int idx = strlen(a);
    int preidx = strlen(b);

    if (preidx >= idx)
        return 0;
    if (strncmp(b, &a[idx - preidx], preidx) == 0)
        return 1;
    return 0;
}

//https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c
const char *str_replace(const char *orig, const char *rep, const char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig)
        return NULL;

    if (!rep)
        return orig;

    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = (char*)orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = (char*)malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    while (count--) {
        ins = (char*)strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

char *get_str(char *line, char *bparse, char *aparse) {

    char *sline;

    sline = (char*)str_replace(line, bparse, "");
    sline = (char*)str_replace(sline, aparse, "");

    return sline;
}

int cnt_str(char *line, char c) {

    int cnt = 0;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == c)
            cnt++;
    }

    return cnt;
}

void translate_iaddr(const char *binary, char *source_line, addr_t iaddr) {

    int i = 0;
    int ntranslated = 0;
    char path[MAX_LINE_LENGTH];
    char cmd[MAX_LINE_LENGTH];
    FILE *fp;

    sprintf(cmd, "addr2line -e %s 0x%lx", binary, iaddr);

    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        exit(1);
    }

    /* Read the output a line at a time - output it. */
    while (fgets(path, sizeof(path), fp) != NULL) {
        strcpy(source_line, path);
        source_line[strcspn(source_line, "\n")] = 0;
    }

    /* close */
    pclose(fp);

    return;
}

int drline_read(gzFile fp, trace_entry_t *val, trace_entry_t **p_val, int *edx) {

    int idx;

    idx = (*edx) / sizeof(trace_entry_t);
    //first read
    if (*p_val == NULL) {
        *edx = gzread(fp, val, sizeof(trace_entry_t) * NBUFS);
        *p_val = val;

    } else if (*p_val == &val[idx]) {
        *edx = gzread(fp, val, sizeof(trace_entry_t) * NBUFS);
        *p_val = val;
    }

    if (*edx == 0)
        return 0;

    return 1;
}

void create_metrics_file(FILE *fp, FILE *fp2, const char* trace_file_name, Metrics & target_metrics, bool & first_spatter)
{
    int i = 0;
    int j = 0;

    //Create stride histogram and create spatter
    int sidx;
    int unique_strides;
    int64_t idx, pidx;
    int64_t n_stride[1027];
    double outbounds;

    printf("\n");
    for (i = 0; i < target_metrics.ntop; i++) {
        printf("***************************************************************************************\n");

        unique_strides = 0;
        for (j = 0; j < 1027; j++)
            n_stride[j] = 0;

        for (j = 1; j < target_metrics.offset[i]; j++) {
            sidx = target_metrics.patterns[i][j] - target_metrics.patterns[i][j - 1] + 513;
            sidx = (sidx < 1) ? 0 : sidx;
            sidx = (sidx > 1025) ? 1026 : sidx;
            n_stride[sidx]++;
        }

        for (j = 0; j < 1027; j++) {
            if (n_stride[j] > 0) {
                unique_strides++;
            }
        }

        outbounds = (double) (n_stride[0] + n_stride[1026]) / (double) target_metrics.offset[i];

        //if ( ( (unique_strides > NSTRIDES) || (outbounds > OUTTHRESH) ) && (gather_offset[i] > USTRIDES ) ){
        if (1) {

            //create a binary file
            FILE *fp_bin;
            char *bin_name;
            bin_name = (char*)str_replace(trace_file_name, ".gz", ".sbin");
            if (strstr(bin_name, ".sbin") == 0) {
                strncat(bin_name, ".sbin", strlen(".sbin")+1);
            }
            printf("%s\n", bin_name);
            fp_bin = fopen(bin_name, "w");
            if (fp_bin == NULL) {
                printf("ERROR: Could not open %s!\n", bin_name);
                exit(-1);
            }

            printf("%sIADDR    -- %p\n", target_metrics.getShortName().c_str(), (void*) target_metrics.top[i]);
            printf("SRCLINE   -- %s\n", target_metrics.get_srcline()[target_metrics.top_idx[i]]);
            printf("%s %c -- %6.3f%c (512-bit chunks)\n", target_metrics.type_as_string().c_str(),
                   '%', 100.0 * (double) target_metrics.tot[i] / target_metrics.cnt, '%');
            printf("NDISTS  -- %ld\n", (long int)target_metrics.offset[i]);

            int64_t nlcnt = 0;
            for (j = 0; j < target_metrics.offset[i]; j++) {

                if (j < 39) {
                    printf("%10ld ", target_metrics.patterns[i][j]);
                    fflush(stdout);
                    if ((++nlcnt % 13) == 0)
                        printf("\n");

                } else if (j >= (target_metrics.offset[i] - 39)) {
                    printf("%10ld ", target_metrics.patterns[i][j]);
                    fflush(stdout);
                    if ((++nlcnt % 13) == 0)
                        printf("\n");

                } else if (j == 39)
                    printf("...\n");
            }
            printf("\n");
            printf("DIST HISTOGRAM --\n");

            for (j = 0; j < 1027; j++) {
                if (n_stride[j] > 0) {
                    if (j == 0)
                        printf("%6s: %ld\n", "< -512", n_stride[j]);
                    else if (j == 1026)
                        printf("%6s: %ld\n", ">  512", n_stride[j]);
                    else
                        printf("%6d: %ld\n", j - 513, n_stride[j]);
                }
            }

            if (first_spatter) {
                first_spatter = false;
                fprintf(fp, " {\"kernel\":\"%s\", \"pattern\":[", target_metrics.getName().c_str());
            } else {
                fprintf(fp, ",\n {\"kernel\":\"%s\", \"pattern\":[", target_metrics.getName().c_str());
            }

            fwrite(target_metrics.patterns[i], sizeof(uint64_t), target_metrics.offset[i], fp_bin);
            fclose(fp_bin);

            for (j = 0; j < target_metrics.offset[i] - 1; j++)
                fprintf(fp, "%ld,", target_metrics.patterns[i][j]);
            fprintf(fp, "%ld", target_metrics.patterns[i][target_metrics.offset[i] - 1]);
            fprintf(fp, "], \"count\":1}");

            fprintf(fp2, "%s,%s,%ld,%6.3f\n",
                    target_metrics.get_srcline()[target_metrics.top_idx[i]], target_metrics.getShortName().c_str(),
                    (long int)target_metrics.offset[i],
                    100.0 * (double) target_metrics.tot[i] / target_metrics.cnt);
        }
        printf("***************************************************************************************\n\n");
    }
}

void create_spatter_file(const char* trace_file_name, Metrics & gather_metrics, Metrics & scatter_metrics)
{
    //Create spatter file
    FILE *fp, *fp2;
    char *json_name, *gs_info;
    json_name = (char*)str_replace(trace_file_name, ".gz", ".json");
    if (strstr(json_name, ".json") == 0) {
        strncat(json_name, ".json", strlen(".json")+1);
    }

    fp = fopen(json_name, "w");
    if (fp == NULL) {
        printf("ERROR: Could not open %s!\n", json_name);
        exit(-1);
    }
    gs_info = (char*)str_replace(trace_file_name, ".gz", ".txt");
    if (strstr(gs_info, ".json") == 0) {
        strncat(gs_info, ".txt", strlen(".txt")+1);
    }

    fp2 = fopen(gs_info, "w");
    if (fp2 == NULL) {
        printf("ERROR: Could not open %s!\n", gs_info);
        exit(-1);
    }

    //Header
    fprintf(fp, "[ ");
    fprintf(fp2, "#sourceline, g/s, indices, percentage of g/s in trace\n");

    bool first_spatter = true;
    create_metrics_file(fp, fp2, trace_file_name, gather_metrics, first_spatter);

    create_metrics_file(fp, fp2, trace_file_name, scatter_metrics, first_spatter);

    //Footer
    fprintf(fp, " ]");
    fclose(fp);
    fclose(fp2);
}

void normalize_stats(Metrics & target_metrics)
{
    //Normalize
    int64_t smallest;
    for (int i = 0; i < target_metrics.ntop; i++) {

        //Find smallest
        smallest = 0;
        for (int j = 0; j < target_metrics.offset[i]; j++) {
            if (target_metrics.patterns[i][j] < smallest)
                smallest = target_metrics.patterns[i][j];
        }

        smallest *= -1;

        //Normalize
        for (int j = 0; j < target_metrics.offset[i]; j++) {
            target_metrics.patterns[i][j] += smallest;
        }
    }
}

double update_source_lines_from_binary(InstrInfo & target_iinfo, Metrics & target_metrics, const char* binary_file_name)
{
    double scatter_cnt = 0.0;

    fflush(stdout);
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

// Second Pass
void second_pass(gzFile fp_drtrace, Metrics & gather_metrics, Metrics & scatter_metrics)
{
    uint64_t mcnt = 0;  // used our own local mcnt while iterating over file in this method.
    int iret = 0;
    trace_entry_t* drline;
    addr_t iaddr;
    int64_t maddr;
    int i = 0;

    addr_t gather_base[NTOP] = {0};
    addr_t scatter_base[NTOP] = {0};

    bool breakout = false;
    printf("\nSecond pass to fill gather / scatter subtraces\n");
    fflush(stdout);

    trace_entry_t* p_drtrace = NULL;
    trace_entry_t drtrace[NBUFS];   // was static (1024 bytes)

    while (drline_read(fp_drtrace, drtrace, &p_drtrace, &iret) && !breakout) {

        //decode drtrace
        drline = p_drtrace;

        /*****************************/
        /** INSTR 0xa-0x10 and 0x1e **/
        /*****************************/
        if (((drline->type >= 0xa) && (drline->type <= 0x10)) || (drline->type == 0x1e)) {
            iaddr = drline->addr;

            /***********************/
            /** MEM 0x00 and 0x01 **/
            /***********************/
        }
        else if ((drline->type == 0x0) || (drline->type == 0x1)) {

            maddr = drline->addr / drline->size;

            if ((++mcnt % PERSAMPLE) == 0) {
#if SAMPLE
                break;
#endif
                printf(".");
                fflush(stdout);
            }

            // gather ?
            if (drline->type == 0x0) {

                for (i = 0; i < gather_metrics.ntop; i++) {

                    //found it
                    if (iaddr == gather_metrics.top[i]) {

                        if (gather_base[i] == 0)
                            gather_base[i] = maddr;

                        //Add index
                        if (gather_metrics.offset[i] >= PSIZE) {
                            printf("WARNING: Need to increase PSIZE. Truncating trace...\n");
                            breakout = true;
                        }
                        //printf("g -- %d % d\n", i, gather_offset[i]); fflush(stdout);
                        gather_metrics.patterns[i][gather_metrics.offset[i]++] = (int64_t) (maddr - gather_base[i]);

                        break;
                    }
                }
            }
            // scatter ?
            else {

                for (i = 0; i < scatter_metrics.ntop; i++) {

                    //found it
                    if (iaddr == scatter_metrics.top[i]) {

                        //set base
                        if (scatter_base[i] == 0)
                            scatter_base[i] = maddr;

                        //Add index
                        if (scatter_metrics.offset[i] >= PSIZE) {
                            printf("WARNING: Need to increase PSIZE. Truncating trace...\n");
                            breakout = true;
                        }
                        scatter_metrics.patterns[i][scatter_metrics.offset[i]++] = (int64_t) (maddr - scatter_base[i]);
                        break;
                    }
                }
            }
        } // MEM

        p_drtrace++;

    } //while drtrace
}

int get_top_target(InstrInfo & target_iinfo, Metrics & target_metrics)
{
    int target_ntop = 0;
    int bestcnt;

    for (int j = 0; j < NTOP; j++) {

        int bestcnt = 0;
        addr_t best_iaddr = 0;
        int bestidx = -1;

        for (int k = 0; k < NGS; k++) {

            if (target_iinfo.get_icnt()[k] == 0)
                continue;

            if (target_iinfo.get_iaddrs()[k] == 0) {
                break;
            }

            if (target_iinfo.get_icnt()[k] > bestcnt) {
                bestcnt = target_iinfo.get_icnt()[k];
                best_iaddr = target_iinfo.get_iaddrs()[k];
                bestidx = k;
            }
        }

        if (best_iaddr == 0) {
            break;
        } else {
            target_ntop++;
            target_metrics.top[j] = best_iaddr;
            target_metrics.top_idx[j] = bestidx;
            target_metrics.tot[j] = target_iinfo.get_icnt()[bestidx];
            target_iinfo.get_icnt()[bestidx] = 0;

            //printf("%sIADDR -- %016lx: %16lu -- %s\n", target_metrics.getShortName().c_str(), target_metrics.top[j], target_metrics.tot[j], target_metrics.get_srcline()[bestidx]);
        }
    }

    return target_ntop;
}

void handle_trace_entry(
        trace_entry_t *drline,
        TraceInfo &   trace_info,
        InstrInfo &   gather_iinfo,
        InstrInfo &   scatter_iinfo,
        Metrics &     gather_metrics,
        Metrics &     scatter_metrics,
        InstrWindow & iw)
{
    int i, j, k, w;
    int w_rw_idx;
    int w_idx;
    int gs;

    /*****************************/
    /** INSTR 0xa-0x10 and 0x1e **/
    /*****************************/
    if (((drline->type >= 0xa) && (drline->type <= 0x10)) || (drline->type == 0x1e)) {

        iw.iaddr = drline->addr;

        //nops
        trace_info.opcodes++;
        trace_info.did_opcode = true;

        /***********************/
        /** MEM 0x00 and 0x01 **/
        /***********************/
    } else if ((drline->type == 0x0) || (drline->type == 0x1)) {

        w_rw_idx = drline->type;

        //printf("M DRTRACE -- iaddr: %016lx addr: %016lx cl_start: %d bytes: %d\n",
        //     iaddr,  drline->addr, drline->addr % 64, drline->size);

        if ((++trace_info.mcnt % PERSAMPLE) == 0) {
#if SAMPLE
            break;
#endif
            printf(".");
            fflush(stdout);
        }

        //is iaddr in window
        w_idx = -1;
        for (i = 0; i < IWINDOW; i++) {

            //new iaddr
            if (iw.w_iaddrs[w_rw_idx][i] == -1) {
                w_idx = i;
                break;

                //iaddr exists
            } else if (iw.w_iaddrs[w_rw_idx][i] == iw.iaddr) {
                w_idx = i;
                break;
            }
        }

        //new window
        if ((w_idx == -1) || (iw.w_bytes[w_rw_idx][w_idx] >= VBYTES) ||
            (iw.w_cnt[w_rw_idx][w_idx] >= VBYTES)) {

            /***************************/
            //do analysis
            /***************************/
            //i = each window
            for (w = 0; w < 2; w++) {  // 2

                for (i = 0; i < IWINDOW; i++) {  // 1024

                    if (iw.w_iaddrs[w][i] == -1)
                        break;

                    int byte = iw.w_bytes[w][i] / iw.w_cnt[w][i];

                    //First pass
                    //Determine
                    //gather/scatter?
                    gs = -1;
                    for (j = 0; j < iw.w_cnt[w][i]; j++) {

                        //address and cl
                        iw.maddr = iw.w_maddr[w][i][j];
                        assert(iw.maddr > -1);

                        //previous addr
                        if (j == 0)
                            iw.maddr_prev = iw.maddr - 1;

                        //gather / scatter
                        if (iw.maddr != iw.maddr_prev) {
                            if ((gs == -1) && (abs(iw.maddr - iw.maddr_prev) > 1))
                                gs = w;
                        }
                        iw.maddr_prev = iw.maddr;
                    }

                    for (j = 0; j < iw.w_cnt[w][i]; j++) {

                        if (gs == -1) {
                            trace_info.other_cnt++;
                            continue;
                        }
                    }

                    if (gs == 0) {  // GATHER

                        trace_info.gather_occ_avg += iw.w_cnt[w][i];
                        gather_metrics.cnt += 1.0;

                        for (k = 0; k < NGS; k++) {
                            if (gather_iinfo.get_iaddrs()[k] == 0) {
                                gather_iinfo.get_iaddrs()[k] = iw.w_iaddrs[w][i];
                                (gather_iinfo.get_icnt()[k])++;
                                gather_iinfo.get_occ()[k] += iw.w_cnt[w][i];
                                break;
                            }

                            if (gather_iinfo.get_iaddrs()[k] == iw.w_iaddrs[w][i]) {
                                (gather_iinfo.get_icnt()[k])++;
                                gather_iinfo.get_occ()[k] += iw.w_cnt[w][i];
                                break;
                            }

                        }

                    } else if (gs == 1) { // SCATTER

                        trace_info.scatter_occ_avg += iw.w_cnt[w][i];
                        scatter_metrics.cnt += 1.0;

                        for (k = 0; k < NGS; k++) {
                            if (scatter_iinfo.get_iaddrs()[k] == 0) {
                                scatter_iinfo.get_iaddrs()[k] = iw.w_iaddrs[w][i];
                                (scatter_iinfo.get_icnt()[k])++;
                                scatter_iinfo.get_occ()[k] += iw.w_cnt[w][i];
                                break;
                            }

                            if (scatter_iinfo.get_iaddrs()[k] == iw.w_iaddrs[w][i]) {
                                (scatter_iinfo.get_icnt()[k])++;
                                scatter_iinfo.get_occ()[k] += iw.w_cnt[w][i];
                                break;
                            }
                        }
                    }
                } //WINDOW i

                w_idx = 0;

                //reset windows
                for (i = 0; i < IWINDOW; i++) {
                    iw.w_iaddrs[w][i] = -1;
                    iw.w_bytes[w][i] = 0;
                    iw.w_cnt[w][i] = 0;
                    for (j = 0; j < VBYTES; j++)
                        iw.w_maddr[w][i][j] = -1;
                }
            } // rw w
        } //analysis

        //Set window values
        iw.w_iaddrs[w_rw_idx][w_idx] = iw.iaddr;
        iw.w_maddr[w_rw_idx][w_idx][iw.w_cnt[w_rw_idx][w_idx]] = drline->addr / drline->size;
        iw.w_bytes[w_rw_idx][w_idx] += drline->size;

        //num access per iaddr in loop
        iw.w_cnt[w_rw_idx][w_idx]++;

        if (trace_info.did_opcode) {

            trace_info.opcodes_mem++;
            trace_info.addrs++;
            trace_info.did_opcode = false;

        } else {
            trace_info.addrs++;
        }

        /***********************/
        /** SOMETHING ELSE **/
        /***********************/
    } else {
        trace_info.other++;
    }
}

void display_stats(TraceInfo & trace_info, Metrics &  gather_metrics, Metrics & scatter_metrics)
{
    printf("\n RESULTS \n");

    printf("DRTRACE STATS\n");
    printf("DRTRACE LINES:        %16lu\n", trace_info.drtrace_lines);
    printf("OPCODES:              %16lu\n", trace_info.opcodes);
    printf("MEMOPCODES:           %16lu\n", trace_info.opcodes_mem);
    printf("LOAD/STORES:          %16lu\n", trace_info.addrs);
    printf("OTHER:                %16lu\n", trace_info.other);

    printf("\n");

    printf("GATHER/SCATTER STATS: \n");
    printf("LOADS per GATHER:     %16.3f\n", trace_info.gather_occ_avg);
    printf("STORES per SCATTER:   %16.3f\n", trace_info.scatter_occ_avg);
    printf("GATHER COUNT:         %16.3f (log2)\n", log(gather_metrics.cnt) / log(2.0));
    printf("SCATTER COUNT:        %16.3f (log2)\n", log(scatter_metrics.cnt) / log(2.0));
    printf("OTHER  COUNT:         %16.3f (log2)\n", log(trace_info.other_cnt) / log(2.0));
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
        trace_info.drtrace_lines++;

    } //while drtrace

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
        const char * binary)
{
    // Find source lines for gathers - Must have symbol
    printf("\nSymbol table lookup for gathers...");
    gather_metrics.cnt = update_source_lines_from_binary(gather_iinfo, gather_metrics, binary);

    // Find source lines for scatters
    printf("Symbol table lookup for scatters...");
    scatter_metrics.cnt = update_source_lines_from_binary(scatter_iinfo, scatter_metrics, binary);
}

void update_metrics(
        InstrInfo & gather_iinfo,
        InstrInfo & scatter_iinfo,
        Metrics & gather_metrics,
        Metrics & scatter_metrics,
        gzFile & fp_drtrace)
{
    // Get top gathers
    gather_metrics.ntop = get_top_target(gather_iinfo, gather_metrics);

    // Get top scatters
    scatter_metrics.ntop = get_top_target(scatter_iinfo, scatter_metrics);

    // ----------------- Second Pass -----------------

    second_pass(fp_drtrace, gather_metrics, scatter_metrics);

    // ----------------- Normalize -----------------

    normalize_stats(gather_metrics);
    normalize_stats(scatter_metrics);
}


int main(int argc, char **argv) {

    char binary[1024];
    gzFile fp_drtrace;

    if (argc == 3) {

        // 1 open dr trace
        fp_drtrace = gzopen(argv[1], "hrb");
        if (fp_drtrace == NULL) {
            printf("ERROR: Could not open %s!\n", argv[1]);
            exit(-1);
        }

        strcpy(binary, argv[2]);

    } else {
        printf("ERROR: Invalid arguments, should be: trace.gz binary\n");
        exit(-1);
    }

    try {
        Metrics gather_metrics(GATHER);
        Metrics scatter_metrics(SCATTER);

        InstrInfo gather_iinfo(GATHER);
        InstrInfo scatter_iinfo(SCATTER);

        TraceInfo trace_info;

        // ----------------- Process Traces -----------------

        process_traces(trace_info, gather_iinfo, scatter_iinfo, gather_metrics, scatter_metrics, fp_drtrace);
        //close files
        gzclose(fp_drtrace);

        fflush(stdout);

        // ----------------- Update Source Lines -----------------

        update_source_lines(gather_iinfo, scatter_iinfo, gather_metrics, scatter_metrics, binary);

        //Open trace
        fp_drtrace = gzopen(argv[1], "hrb");
        if (fp_drtrace == NULL) {
            printf("ERROR: Could not open %s!\n", argv[1]);
            exit(-1);
        }

        // ----------------- Update Metrics -----------------

        update_metrics(gather_iinfo, scatter_iinfo, gather_metrics, scatter_metrics, fp_drtrace);

        gzclose(fp_drtrace);
        printf("\n");

        // ----------------- Create Spatter File -----------------

        create_spatter_file(argv[1], gather_metrics, scatter_metrics);

    }
    catch (const std::exception & ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        exit(-1);
    }

    return 0;

}
