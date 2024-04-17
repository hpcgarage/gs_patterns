
#include <assert.h> /// TODO: use cassert instead
#include <math.h>

#include <string>
#include <sstream>

#include "utils.h"
#include "gs_patterns.h"

void translate_iaddr(const std::string & binary, char *source_line, addr_t iaddr) {

    int i = 0;
    int ntranslated = 0;
    char path[MAX_LINE_LENGTH];
    char cmd[MAX_LINE_LENGTH];
    FILE *fp;

    sprintf(cmd, "addr2line -e %s 0x%lx", binary.c_str(), iaddr);

    /* Open the command for reading. */
    fp = popen(cmd, "r");
    if (NULL == fp) {
        throw GSError("Failed to run command");
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


static void create_metrics_file(FILE *fp, FILE *fp2, const std::string & file_prefix, Metrics & target_metrics, bool & first_spatter)
{
    int i = 0;
    int j = 0;

    //Create stride histogram and create spatter
    int sidx;
    int unique_strides;
    int64_t idx, pidx;
    int64_t n_stride[1027];
    double outbounds;

    if (file_prefix.empty()) throw GSFileError ("Empty file prefix provided.");

    if (first_spatter) printf("\n");

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
            std::string bin_name = file_prefix + ".sbin";
            printf("%s\n", bin_name.c_str());
            fp_bin = fopen(bin_name.c_str(), "w");
            if (NULL == fp_bin) {
                throw GSFileError("Could not open " + std::string(bin_name) + "!");
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
                    if (0 == (++nlcnt % 13))
                        printf("\n");

                } else if (j >= (target_metrics.offset[i] - 39)) {
                    printf("%10ld ", target_metrics.patterns[i][j]);
                    fflush(stdout);
                    if (0 == (++nlcnt % 13))
                        printf("\n");

                } else if (39 == j)
                    printf("...\n");
            }
            printf("\n");
            printf("DIST HISTOGRAM --\n");

            for (j = 0; j < 1027; j++) {
                if (n_stride[j] > 0) {
                    if (0 == j)
                        printf("%6s: %ld\n", "< -512", n_stride[j]);
                    else if (1026 == j)
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

void create_spatter_file(MemPatterns & mp, const std::string & file_prefix)
{
    // Create spatter file
    FILE *fp, *fp2;

    if (file_prefix.empty()) throw GSFileError ("Empty file prefix provided.");

    std::string json_name = file_prefix + ".json";
    fp = fopen(json_name.c_str(), "w");
    if (NULL == fp) {
        throw GSFileError("Could not open " + json_name + "!");
    }

    std::string gs_info = file_prefix + ".txt";
    fp2 = fopen(gs_info.c_str(), "w");
    if (NULL == fp2) {
        throw GSFileError("Could not open " + gs_info + "!");
    }

    //Header
    fprintf(fp, "[ ");
    fprintf(fp2, "#sourceline, g/s, indices, percentage of g/s in trace\n");

    bool first_spatter = true;
    create_metrics_file(fp, fp2, file_prefix, mp.get_gather_metrics(), first_spatter);

    create_metrics_file(fp, fp2, file_prefix, mp.get_scatter_metrics(), first_spatter);

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

void handle_trace_entry(MemPatterns & mp, const InstrAddrAdapter & ia)
{
    int i, j, k, w = 0;
    int w_rw_idx;   // Index into instruction window first dimension (RW: 0=Gather(R) or 1=Scatter(W))
    int w_idx;
    int gs;

    auto & trace_info = mp.get_trace_info();
    auto & gather_iinfo = mp.get_gather_iinfo();
    auto & scatter_iinfo = mp.get_scatter_iinfo();
    auto & gather_metrics = mp.get_gather_metrics();
    auto & scatter_metrics = mp.get_scatter_metrics();
    auto & iw = mp.get_instr_window();

    if (!ia.is_valid()) {
        std::ostringstream os;
        os << "Invalid " << ia;
        throw GSDataError(os.str());
    }

    /*****************************/
    /** INSTR 0xa-0x10 and 0x1e **/
    /*****************************/
    if (ia.is_other_instr()) {

        iw.iaddr = ia.get_iaddr();

        //nops
        trace_info.opcodes++;
        trace_info.did_opcode = true;

        /***********************/
        /** MEM 0x00 and 0x01 **/
        /***********************/
    } else if (ia.is_mem_instr()) {

        if ( ia.get_iaddr() != ia.get_address()) {
            iw.iaddr = ia.get_iaddr();
            trace_info.opcodes++;
            trace_info.did_opcode = true;
        }

        w_rw_idx = ia.get_type();

        //printf("M DRTRACE -- iaddr: %016lx addr: %016lx cl_start: %d bytes: %d\n",
        //     iw.iaddr,  ia.get_address(), ia.get_address() % 64, ia.get_size());

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
        if ((w_idx == -1) || (iw.w_bytes[w_rw_idx][w_idx] >= ia.min_size()) ||   // was >= VBYTES
            (iw.w_cnt[w_rw_idx][w_idx] >= ia.min_size())) {                      // was >= VBYTES

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
                            if ((gs == -1) && (abs(iw.maddr - iw.maddr_prev) > 1))  // ? > 1 stride (non-contiguous)   <--------------------
                                gs = w;
                        }
                        iw.maddr_prev = iw.maddr;
                    }

                    // Update other_cnt
                    if (gs == -1) trace_info.other_cnt += iw.w_cnt[w][i];

                    // GATHER or SCATTER handling
                    if (gs == 0 || gs == 1) {
                        InstrInfo & target_iinfo = (gs == 0) ? gather_iinfo : scatter_iinfo;

                        if (gs == 0) {
                            trace_info.gather_occ_avg += iw.w_cnt[w][i];
                            gather_metrics.cnt += 1.0;
                        }
                        else {
                            trace_info.scatter_occ_avg += iw.w_cnt[w][i];
                            scatter_metrics.cnt += 1.0;
                        }

                        for (k = 0; k < NGS; k++) {
                            if (target_iinfo.get_iaddrs()[k] == 0) {
                                target_iinfo.get_iaddrs()[k] = iw.w_iaddrs[w][i];
                                (target_iinfo.get_icnt()[k])++;
                                target_iinfo.get_occ()[k] += iw.w_cnt[w][i];
                                break;
                            }

                            if (target_iinfo.get_iaddrs()[k] == iw.w_iaddrs[w][i]) {
                                (target_iinfo.get_icnt()[k])++;
                                target_iinfo.get_occ()[k] += iw.w_cnt[w][i];
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
        iw.w_maddr[w_rw_idx][w_idx][iw.w_cnt[w_rw_idx][w_idx]] = ia.get_address() / ia.get_size();
        iw.w_bytes[w_rw_idx][w_idx] += ia.get_size();

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

    trace_info.trace_lines++;
}

void display_stats(MemPatterns & mp)
{
    printf("\n RESULTS \n");

    printf("DRTRACE STATS\n");
    printf("DRTRACE LINES:        %16lu\n", mp.get_trace_info().trace_lines);
    printf("OPCODES:              %16lu\n", mp.get_trace_info().opcodes);
    printf("MEMOPCODES:           %16lu\n", mp.get_trace_info().opcodes_mem);
    printf("LOAD/STORES:          %16lu\n", mp.get_trace_info().addrs);
    printf("OTHER:                %16lu\n", mp.get_trace_info().other);

    printf("\n");

    printf("GATHER/SCATTER STATS: \n");
    printf("LOADS per GATHER:     %16.3f\n", mp.get_trace_info().gather_occ_avg);
    printf("STORES per SCATTER:   %16.3f\n", mp.get_trace_info().scatter_occ_avg);
    printf("GATHER COUNT:         %16.3f (log2)\n", log(mp.get_gather_metrics().cnt) / log(2.0));
    printf("SCATTER COUNT:        %16.3f (log2)\n", log(mp.get_scatter_metrics().cnt) / log(2.0));
    printf("OTHER  COUNT:         %16.3f (log2)\n", log(mp.get_trace_info().other_cnt) / log(2.0));
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

bool handle_2nd_pass_trace_entry(const InstrAddrAdapter & ia,
                                 Metrics & gather_metrics, Metrics & scatter_metrics,
                                 addr_t & iaddr, int64_t & maddr, uint64_t & mcnt,
                                 addr_t * gather_base, addr_t * scatter_base)
{
    int iret = 0;
    int i = 0;

    bool breakout = false;

    /*****************************/
    /** INSTR 0xa-0x10 and 0x1e **/
    /*****************************/
    if (!ia.is_valid()) {
        std::ostringstream os;
        os << "Invalid " << ia;
        throw GSDataError(os.str());
    }

    if (ia.is_other_instr()) {
        iaddr = ia.get_address();

        /***********************/
        /** MEM 0x00 and 0x01 **/
        /***********************/
    }
    else if (ia.is_mem_instr()) {

        maddr = ia.get_address() / ia.get_size();

        iaddr = ia.get_iaddr() != ia.get_address() ? ia.get_iaddr() : iaddr;

        if ((++mcnt % PERSAMPLE) == 0) {
#if SAMPLE
            break;
#endif
            printf(".");
            fflush(stdout);
        }

        // gather ?
        if (ia.get_mem_instr_type() == GATHER) {

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
        else if (ia.get_mem_instr_type() == SCATTER) {

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
        else { // belt and suspenders, yep = but helps to validate correct logic in children of InstrAddresInfo
            throw GSDataError("Unknown Memory Instruction Type: " + ia.get_mem_instr_type());
        }
    } // MEM

    return breakout;
}

std::ostream & operator<<(std::ostream & os, const InstrAddrAdapter & ia)
{
    ia.output(os);
    return os;
}
