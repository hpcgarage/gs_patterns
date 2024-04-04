//
// Created by christopher on 4/2/24.
//

#pragma once

#include <stdio.h>
#include <zlib.h>
#include <string>

#include "gs_patterns.h"

int drline_read(gzFile           fp,
                trace_entry_t *  val,
                trace_entry_t ** p_val,
                int *            edx);

void translate_iaddr(const std::string & binary,
                     char *              source_line,
                     addr_t              iaddr);

void create_metrics_file(FILE *              fp,
                         FILE *              fp2,
                         const std::string & file_prefix,
                         Metrics &           target_metrics,
                         bool &              first_spatter);

void create_spatter_file(MemPatterns & mp, const std::string & file_prefix);

void handle_trace_entry(MemPatterns & mp, const trace_entry_t *drline);

void display_stats(MemPatterns & mp);

void update_metrics(MemPatterns & mp, gzFile & fp_drtrace);

int get_top_target(InstrInfo & target_iinfo, Metrics & target_metrics);

void normalize_stats(Metrics & target_metrics);

bool handle_2nd_pass_trace_entry(trace_entry_t * drline,
                                 Metrics & gather_metrics, Metrics & scatter_metrics,
                                 addr_t & iaddr, int64_t & maddr, uint64_t & mcnt,
                                 addr_t * gather_base, addr_t * scatter_base);
