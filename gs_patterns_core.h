//
// Created by christopher on 4/2/24.
//

#pragma once

#include <stdio.h>
#include <zlib.h>
#include <string>

#include "utils.h"
#include "gs_patterns.h"

int drline_read(gzFile           fp,
                trace_entry_t *  val,
                trace_entry_t ** p_val,
                int *            edx);

void translate_iaddr(const std::string & binary,
                     char *              source_line,
                     addr_t              iaddr);

void create_metrics_file(FILE *      fp,
                         FILE *      fp2,
                         const char* trace_file_name,
                         Metrics &   target_metrics,
                         bool &      first_spatter);

void create_spatter_file(const char* trace_file_name,
                         Metrics &   gather_metrics,
                         Metrics &   scatter_metrics);

void normalize_stats(Metrics & target_metrics);

void handle_trace_entry(trace_entry_t *drline,
                        TraceInfo &   trace_info,
                        InstrInfo &   gather_iinfo,
                        InstrInfo &   scatter_iinfo,
                        Metrics &     gather_metrics,
                        Metrics &     scatter_metrics,
                        InstrWindow & iw);

void display_stats(TraceInfo & trace_info,
                   Metrics &   gather_metrics,
                   Metrics &   scatter_metrics);

int get_top_target(InstrInfo & target_iinfo,
                   Metrics &   target_metrics);

void update_metrics(InstrInfo & gather_iinfo,
                    InstrInfo & scatter_iinfo,
                    Metrics &   gather_metrics,
                    Metrics &   scatter_metrics,
                    gzFile &    fp_drtrace);
