
#pragma once

#include <stdio.h>
#include <string>

#include "gs_patterns.h"

namespace gs_patterns
{
namespace gs_patterns_core
{
    void translate_iaddr(const std::string &binary, char *source_line, addr_t iaddr);

    void handle_trace_entry(MemPatterns &mp, const InstrAddrAdapter &ia);

    void display_stats(MemPatterns &mp);

    int get_top_target(InstrInfo &target_iinfo, Metrics &target_metrics);

    void normalize_stats(Metrics &target_metrics);

    bool handle_2nd_pass_trace_entry(const InstrAddrAdapter &ia,
                                     Metrics &gather_metrics, Metrics &scatter_metrics,
                                     addr_t &iaddr, int64_t &maddr, uint64_t &mcnt,
                                     addr_t *gather_base, addr_t *scatter_base);

    void create_metrics_file(FILE *fp,
                             FILE *fp2,
                             const std::string &file_prefix,
                             Metrics &target_metrics,
                             bool &first_spatter);

    void create_spatter_file(MemPatterns &mp, const std::string &file_prefix);

} // gs_patterns_core

} // gs_patterns
