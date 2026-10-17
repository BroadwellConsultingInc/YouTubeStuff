#pragma once
#include "cgpt_ex_config.h"
#include <stddef.h>

struct cgpt_ex_csvPoint {
    uint32_t cgpt_ex_moveMs;
    uint16_t cgpt_ex_positions[cgpt_ex_jointCount];
};
extern const char cgpt_ex_csvHeader[];
bool cgpt_ex_csvParseHeader(const char *cgpt_ex_line);
bool cgpt_ex_csvParsePoint(const char *cgpt_ex_line, cgpt_ex_csvPoint &cgpt_ex_point);
bool cgpt_ex_csvPointInLimits(const cgpt_ex_csvPoint &cgpt_ex_point);
size_t cgpt_ex_csvFormatPoint(char *cgpt_ex_buffer, size_t cgpt_ex_size, const cgpt_ex_csvPoint &cgpt_ex_point);
bool cgpt_ex_csvFileName(const char *cgpt_ex_name);
bool cgpt_ex_csvRootPath(const char *cgpt_ex_name, char *cgpt_ex_path, size_t cgpt_ex_size);
