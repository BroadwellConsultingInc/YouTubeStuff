#include "cgpt_ex_csv.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

const char cgpt_ex_csvHeader[] = "move_ms,base,shoulder,elbow,wrist,wrist_roll,gripper";
static const char *const cgpt_ex_csvColumns[] = {"move_ms","base","shoulder","elbow","wrist","wrist_roll","gripper"};

// Accept plain or quoted fields, spaces around fields, and Excel UTF-8 BOM.
// Quoted commas/escaped quotes are not needed for these numeric fields/names.
static bool cgpt_ex_csvField(const char *&cgpt_ex_cursor, char *cgpt_ex_field,
                             size_t cgpt_ex_size, bool cgpt_ex_last) {
    while (*cgpt_ex_cursor==' ' || *cgpt_ex_cursor=='\t') ++cgpt_ex_cursor;
    const bool cgpt_ex_quoted = *cgpt_ex_cursor=='"';
    if (cgpt_ex_quoted) ++cgpt_ex_cursor;
    size_t cgpt_ex_length=0;
    while (*cgpt_ex_cursor && *cgpt_ex_cursor!='\r' && *cgpt_ex_cursor!='\n' &&
           (cgpt_ex_quoted ? *cgpt_ex_cursor!='"' : *cgpt_ex_cursor!=',')) {
        if (cgpt_ex_length+1>=cgpt_ex_size) return false;
        cgpt_ex_field[cgpt_ex_length++]=*cgpt_ex_cursor++;
    }
    if (cgpt_ex_quoted) {
        if (*cgpt_ex_cursor!='"') return false;
        ++cgpt_ex_cursor;
    }
    while (cgpt_ex_length && (cgpt_ex_field[cgpt_ex_length-1]==' ' || cgpt_ex_field[cgpt_ex_length-1]=='\t')) --cgpt_ex_length;
    cgpt_ex_field[cgpt_ex_length]='\0';
    while (*cgpt_ex_cursor==' ' || *cgpt_ex_cursor=='\t') ++cgpt_ex_cursor;
    if (!cgpt_ex_last) { if (*cgpt_ex_cursor!=',') return false; ++cgpt_ex_cursor; return cgpt_ex_length>0; }
    while (*cgpt_ex_cursor=='\r' || *cgpt_ex_cursor=='\n') ++cgpt_ex_cursor;
    return cgpt_ex_length>0 && !*cgpt_ex_cursor;
}
bool cgpt_ex_csvParseHeader(const char *cgpt_ex_line) {
    if (!cgpt_ex_line) return false;
    if (strlen(cgpt_ex_line)>=3 && static_cast<unsigned char>(cgpt_ex_line[0])==0xEF &&
        static_cast<unsigned char>(cgpt_ex_line[1])==0xBB && static_cast<unsigned char>(cgpt_ex_line[2])==0xBF) cgpt_ex_line+=3;
    for (uint8_t cgpt_ex_i=0;cgpt_ex_i<7;++cgpt_ex_i) {
        char cgpt_ex_field[24];
        if (!cgpt_ex_csvField(cgpt_ex_line,cgpt_ex_field,sizeof(cgpt_ex_field),cgpt_ex_i==6) ||
            strcmp(cgpt_ex_field,cgpt_ex_csvColumns[cgpt_ex_i])) return false;
    }
    return true;
}
bool cgpt_ex_csvParsePoint(const char *cgpt_ex_line, cgpt_ex_csvPoint &cgpt_ex_point) {
    if (!cgpt_ex_line) return false;
    cgpt_ex_csvPoint cgpt_ex_parsed = {};
    for (uint8_t cgpt_ex_i=0;cgpt_ex_i<7;++cgpt_ex_i) {
        char cgpt_ex_field[24];
        if (!cgpt_ex_csvField(cgpt_ex_line,cgpt_ex_field,sizeof(cgpt_ex_field),cgpt_ex_i==6)) return false;
        uint32_t cgpt_ex_value=0;
        const uint32_t cgpt_ex_max=cgpt_ex_i ? 65535 : cgpt_ex_csvMaxMoveMs;
        for (const char *cgpt_ex_digit=cgpt_ex_field;*cgpt_ex_digit;++cgpt_ex_digit) {
            if (*cgpt_ex_digit<'0' || *cgpt_ex_digit>'9') return false;
            const uint32_t cgpt_ex_n=*cgpt_ex_digit-'0';
            if (cgpt_ex_value>cgpt_ex_max/10 || (cgpt_ex_value==cgpt_ex_max/10 && cgpt_ex_n>cgpt_ex_max%10)) return false;
            cgpt_ex_value=cgpt_ex_value*10+cgpt_ex_n;
        }
        if (!cgpt_ex_i) {if (cgpt_ex_value<20) return false;cgpt_ex_parsed.cgpt_ex_moveMs=cgpt_ex_value;}
        else cgpt_ex_parsed.cgpt_ex_positions[cgpt_ex_i-1]=static_cast<uint16_t>(cgpt_ex_value);
    }
    cgpt_ex_point=cgpt_ex_parsed; // Invalid rows never partially update a point.
    return true;
}
bool cgpt_ex_csvPointInLimits(const cgpt_ex_csvPoint &cgpt_ex_point) {
    for (uint8_t cgpt_ex_i=0;cgpt_ex_i<cgpt_ex_jointCount;++cgpt_ex_i)
        if (cgpt_ex_point.cgpt_ex_positions[cgpt_ex_i]<cgpt_ex_jointSettings[cgpt_ex_i].cgpt_ex_minPosition ||
            cgpt_ex_point.cgpt_ex_positions[cgpt_ex_i]>cgpt_ex_jointSettings[cgpt_ex_i].cgpt_ex_maxPosition) return false;
    return true;
}
size_t cgpt_ex_csvFormatPoint(char *cgpt_ex_buffer, size_t cgpt_ex_size, const cgpt_ex_csvPoint &cgpt_ex_point) {
    const int cgpt_ex_length=snprintf(cgpt_ex_buffer,cgpt_ex_size,"%lu,%u,%u,%u,%u,%u,%u\n",
        static_cast<unsigned long>(cgpt_ex_point.cgpt_ex_moveMs),
        static_cast<unsigned>(cgpt_ex_point.cgpt_ex_positions[0]),static_cast<unsigned>(cgpt_ex_point.cgpt_ex_positions[1]),
        static_cast<unsigned>(cgpt_ex_point.cgpt_ex_positions[2]),static_cast<unsigned>(cgpt_ex_point.cgpt_ex_positions[3]),
        static_cast<unsigned>(cgpt_ex_point.cgpt_ex_positions[4]),static_cast<unsigned>(cgpt_ex_point.cgpt_ex_positions[5]));
    return cgpt_ex_length>0 && static_cast<size_t>(cgpt_ex_length)<cgpt_ex_size ? static_cast<size_t>(cgpt_ex_length) : 0;
}
bool cgpt_ex_csvFileName(const char *cgpt_ex_name) {
    const size_t cgpt_ex_len=cgpt_ex_name ? strlen(cgpt_ex_name) : 0;
    return cgpt_ex_len>4 && cgpt_ex_name[cgpt_ex_len-4]=='.' &&
        tolower(static_cast<unsigned char>(cgpt_ex_name[cgpt_ex_len-3]))=='c' &&
        tolower(static_cast<unsigned char>(cgpt_ex_name[cgpt_ex_len-2]))=='s' &&
        tolower(static_cast<unsigned char>(cgpt_ex_name[cgpt_ex_len-1]))=='v';
}
bool cgpt_ex_csvRootPath(const char *cgpt_ex_name, char *cgpt_ex_path, size_t cgpt_ex_size) {
    if (!cgpt_ex_name) return false;
    if (*cgpt_ex_name=='/') ++cgpt_ex_name;
    if (!cgpt_ex_csvFileName(cgpt_ex_name) || strchr(cgpt_ex_name,'/') || strchr(cgpt_ex_name,'\\') ||
        strlen(cgpt_ex_name)+2>cgpt_ex_size) return false;
    for (const char *cgpt_ex_c=cgpt_ex_name;*cgpt_ex_c;++cgpt_ex_c) if (static_cast<unsigned char>(*cgpt_ex_c)<32) return false;
    snprintf(cgpt_ex_path,cgpt_ex_size,"/%s",cgpt_ex_name);
    return true;
}
