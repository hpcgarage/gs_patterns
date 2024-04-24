//
// Created by christopher on 4/2/24.
//

#pragma once

#include <stdlib.h>
#include <string.h>
#include <string>
#include <zlib.h>

static inline int popcount(uint64_t x);

int startswith(const char *a, const char *b);

int endswith(const char *a, const char *b);

const char *str_replace(const char *orig, const char *rep, const char *with);

char *get_str(char *line, char *bparse, char *aparse);

int cnt_str(char *line, char c);

gzFile open_trace_file(const std::string & trace_file_name);

void close_trace_file (gzFile & fp);
