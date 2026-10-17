#include "cgpt_ex_sd.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if cgpt_ex_enableSd
#include <FS.h>
#include <SD.h>
#include <SPI.h>

static SPIClass cgpt_ex_sdSpi(VSPI);
static bool cgpt_ex_sdMounted=false;
static char cgpt_ex_sdMessage[96]={};
static File cgpt_ex_captureFile;
static File cgpt_ex_playFile;
static bool cgpt_ex_captureActive=false;
static char cgpt_ex_capturePath[cgpt_ex_fileNameSize]={};
static char cgpt_ex_playPath[cgpt_ex_fileNameSize]={};
static cgpt_ex_csvState cgpt_ex_csvPhase=cgpt_ex_csvIdle;
static uint32_t cgpt_ex_csvLine=0;
static size_t cgpt_ex_csvBytesRead=0;
static size_t cgpt_ex_csvFileSize=0;
static uint32_t cgpt_ex_checkedRows=0;
static uint32_t cgpt_ex_streamPointIndex=0;
uint32_t cgpt_ex_sdPlayPointIndex() {return cgpt_ex_streamPointIndex;}
uint32_t cgpt_ex_sdPlayPointCount() {return cgpt_ex_checkedRows;}
static uint32_t cgpt_ex_csvClockStart=0;
static uint16_t cgpt_ex_csvApproachFrom[cgpt_ex_jointCount]={};
static cgpt_ex_csvPoint cgpt_ex_csvPrevious={};

const char *cgpt_ex_sdError() { return cgpt_ex_sdMessage; }
bool cgpt_ex_sdReady() { return cgpt_ex_sdMounted; }
bool cgpt_ex_sdCapturing() { return cgpt_ex_captureActive; }
const char *cgpt_ex_sdCaptureName() { return cgpt_ex_capturePath; }
const char *cgpt_ex_sdPlayName() { return cgpt_ex_playPath; }
cgpt_ex_csvState cgpt_ex_sdPlayState() { return cgpt_ex_csvPhase; }
static bool cgpt_ex_sdFail(const char *cgpt_ex_message) {
    snprintf(cgpt_ex_sdMessage,sizeof(cgpt_ex_sdMessage),"%s",cgpt_ex_message);
    return false;
}
bool cgpt_ex_sdBegin() {
    if (cgpt_ex_sdMounted) return true;
#if cgpt_ex_mountSdInternally
    // Reserve VSPI for SD. The bundled CYD touch adapter uses GPIO software
    // SPI; keep TFT on HSPI. See CYD_TOUCH_GUIDE.md.
    cgpt_ex_sdSpi.begin(cgpt_ex_sdSck,cgpt_ex_sdMiso,cgpt_ex_sdMosi,cgpt_ex_sdCs);
    if (!SD.begin(cgpt_ex_sdCs,cgpt_ex_sdSpi,cgpt_ex_sdFrequency)) return cgpt_ex_sdFail("SD mount failed");
#endif
    if (SD.cardType()==CARD_NONE) return cgpt_ex_sdFail("SD card not mounted");
    cgpt_ex_sdMounted=true;
    cgpt_ex_sdMessage[0]='\0';
    return true;
}
bool cgpt_ex_sdRemount() {
    if (cgpt_ex_captureActive || cgpt_ex_csvPhase!=cgpt_ex_csvIdle) return cgpt_ex_sdFail("Stop SD activity first");
#if cgpt_ex_mountSdInternally
    SD.end();
#endif
    cgpt_ex_sdMounted=false;
    return cgpt_ex_sdBegin();
}
static const char *cgpt_ex_basename(const char *cgpt_ex_name) {
    if (!cgpt_ex_name) return "";
    const char *cgpt_ex_slash=strrchr(cgpt_ex_name,'/');
    return cgpt_ex_slash ? cgpt_ex_slash+1 : cgpt_ex_name;
}
bool cgpt_ex_sdListPage(uint32_t cgpt_ex_page, char cgpt_ex_names[cgpt_ex_filesPerPage][cgpt_ex_fileNameSize], uint8_t &cgpt_ex_count, bool &cgpt_ex_more) {
    cgpt_ex_count=0; cgpt_ex_more=false;
    memset(cgpt_ex_names,0,cgpt_ex_filesPerPage*cgpt_ex_fileNameSize);
    if (!cgpt_ex_sdBegin()) return false;
    File cgpt_ex_root=SD.open("/",FILE_READ);
    if (!cgpt_ex_root || !cgpt_ex_root.isDirectory()) return cgpt_ex_sdFail("Cannot read SD root");
    const uint64_t cgpt_ex_skip=static_cast<uint64_t>(cgpt_ex_page)*cgpt_ex_filesPerPage;
    uint64_t cgpt_ex_index=0;
    while (true) {
        File cgpt_ex_entry=cgpt_ex_root.openNextFile(FILE_READ);
        if (!cgpt_ex_entry) break;
        const char *cgpt_ex_name=cgpt_ex_basename(cgpt_ex_entry.name());
        const bool cgpt_ex_match=!cgpt_ex_entry.isDirectory() && cgpt_ex_csvFileName(cgpt_ex_name) &&
                                 strlen(cgpt_ex_name)<cgpt_ex_fileNameSize-1;
        if (cgpt_ex_match && cgpt_ex_index++>=cgpt_ex_skip) {
            if (cgpt_ex_count==cgpt_ex_filesPerPage) {cgpt_ex_more=true;cgpt_ex_entry.close();break;}
            snprintf(cgpt_ex_names[cgpt_ex_count++],cgpt_ex_fileNameSize,"%s",cgpt_ex_name);
        }
        cgpt_ex_entry.close();
    }
    cgpt_ex_root.close();
    cgpt_ex_sdMessage[0]='\0';
    return true;
}
static uint32_t cgpt_ex_captureNumber(const char *cgpt_ex_name) {
    if (strlen(cgpt_ex_name)!=12 || !cgpt_ex_csvFileName(cgpt_ex_name) ||
        toupper(static_cast<unsigned char>(cgpt_ex_name[0]))!='A' ||
        toupper(static_cast<unsigned char>(cgpt_ex_name[1]))!='R' ||
        toupper(static_cast<unsigned char>(cgpt_ex_name[2]))!='M') return 0;
    uint32_t cgpt_ex_number=0;
    for (uint8_t cgpt_ex_i=3;cgpt_ex_i<8;++cgpt_ex_i) {
        if (cgpt_ex_name[cgpt_ex_i]<'0' || cgpt_ex_name[cgpt_ex_i]>'9') return 0;
        cgpt_ex_number=cgpt_ex_number*10+cgpt_ex_name[cgpt_ex_i]-'0';
    }
    return cgpt_ex_number;
}
static bool cgpt_ex_nextCapturePath() {
    File cgpt_ex_root=SD.open("/",FILE_READ);
    if (!cgpt_ex_root || !cgpt_ex_root.isDirectory()) return cgpt_ex_sdFail("Cannot scan SD root");
    uint32_t cgpt_ex_highest=0;
    while (true) {
        File cgpt_ex_entry=cgpt_ex_root.openNextFile(FILE_READ);
        if (!cgpt_ex_entry) break;
        if (!cgpt_ex_entry.isDirectory()) {
            const uint32_t cgpt_ex_number=cgpt_ex_captureNumber(cgpt_ex_basename(cgpt_ex_entry.name()));
            if (cgpt_ex_number>cgpt_ex_highest) cgpt_ex_highest=cgpt_ex_number;
        }
        cgpt_ex_entry.close();
    }
    cgpt_ex_root.close();
    for (uint32_t cgpt_ex_number=cgpt_ex_highest+1;cgpt_ex_number<=99999;++cgpt_ex_number) {
        snprintf(cgpt_ex_capturePath,sizeof(cgpt_ex_capturePath),"/ARM%05lu.csv",static_cast<unsigned long>(cgpt_ex_number));
        if (!SD.exists(cgpt_ex_capturePath)) return true;
    }
    return cgpt_ex_sdFail("Capture numbers exhausted");
}
static bool cgpt_ex_captureWrite(const char *cgpt_ex_data, size_t cgpt_ex_length) {
    const size_t cgpt_ex_written=cgpt_ex_captureFile.write(reinterpret_cast<const uint8_t *>(cgpt_ex_data),cgpt_ex_length);
    cgpt_ex_captureFile.flush();
    if (cgpt_ex_written!=cgpt_ex_length || cgpt_ex_captureFile.getWriteError()) {
        cgpt_ex_captureActive=false;cgpt_ex_captureFile.close();
        return cgpt_ex_sdFail("SD write failed / card full");
    }
    return true;
}
bool cgpt_ex_sdCaptureStart() {
    if (cgpt_ex_captureActive || cgpt_ex_csvPhase!=cgpt_ex_csvIdle) return cgpt_ex_sdFail("Stop SD activity first");
    cgpt_ex_sdMessage[0]='\0';
    if (!cgpt_ex_sdBegin() || !cgpt_ex_nextCapturePath()) return false;
    // Root scan/exists check precedes FILE_WRITE. Never overwrite an existing file.
    cgpt_ex_captureFile=SD.open(cgpt_ex_capturePath,FILE_WRITE);
    if (!cgpt_ex_captureFile) return cgpt_ex_sdFail("Cannot create capture");
    cgpt_ex_captureFile.clearWriteError();
    cgpt_ex_captureActive=true;
    char cgpt_ex_header[96];
    const int cgpt_ex_length=snprintf(cgpt_ex_header,sizeof(cgpt_ex_header),"%s\n",cgpt_ex_csvHeader);
    return cgpt_ex_length>0 && static_cast<size_t>(cgpt_ex_length)<sizeof(cgpt_ex_header) &&
        cgpt_ex_captureWrite(cgpt_ex_header,static_cast<size_t>(cgpt_ex_length));
}
bool cgpt_ex_sdCapturePoint(const uint16_t cgpt_ex_positions[cgpt_ex_jointCount], uint32_t cgpt_ex_moveMs) {
    if (!cgpt_ex_captureActive) return cgpt_ex_sdFail("Open New capture first");
    cgpt_ex_csvPoint cgpt_ex_point={};
    cgpt_ex_point.cgpt_ex_moveMs=cgpt_ex_moveMs;
    memcpy(cgpt_ex_point.cgpt_ex_positions,cgpt_ex_positions,sizeof(cgpt_ex_point.cgpt_ex_positions));
    if (cgpt_ex_moveMs<20 || cgpt_ex_moveMs>cgpt_ex_csvMaxMoveMs || !cgpt_ex_csvPointInLimits(cgpt_ex_point)) return cgpt_ex_sdFail("Capture pose/time invalid");
    char cgpt_ex_line[96];
    const size_t cgpt_ex_length=cgpt_ex_csvFormatPoint(cgpt_ex_line,sizeof(cgpt_ex_line),cgpt_ex_point);
    return cgpt_ex_length && cgpt_ex_captureWrite(cgpt_ex_line,cgpt_ex_length);
}
bool cgpt_ex_sdCaptureStop() {
    if (!cgpt_ex_captureActive) return true;
    cgpt_ex_captureFile.flush();
    const bool cgpt_ex_ok=!cgpt_ex_captureFile.getWriteError();
    cgpt_ex_captureFile.close();cgpt_ex_captureActive=false;
    return cgpt_ex_ok || cgpt_ex_sdFail("SD close failed");
}
// Read a bounded physical line; use position/size to distinguish EOF from a
// failed read. A final row without a newline is accepted.
static int cgpt_ex_readLine(char *cgpt_ex_buffer, size_t cgpt_ex_size) {
    size_t cgpt_ex_used=0;
    while (cgpt_ex_csvBytesRead<cgpt_ex_csvFileSize) {
        const int cgpt_ex_byte=cgpt_ex_playFile.read();
        if (cgpt_ex_byte<0) {cgpt_ex_sdFail("SD read failed");return -1;}
        ++cgpt_ex_csvBytesRead;
        if (cgpt_ex_byte=='\n') {++cgpt_ex_csvLine;cgpt_ex_buffer[cgpt_ex_used]='\0';return 1;}
        if (cgpt_ex_used+1>=cgpt_ex_size) {cgpt_ex_sdFail("CSV line too long");return -1;}
        cgpt_ex_buffer[cgpt_ex_used++]=static_cast<char>(cgpt_ex_byte);
    }
    cgpt_ex_buffer[cgpt_ex_used]='\0';
    if (cgpt_ex_used) {++cgpt_ex_csvLine;return 1;}
    return 0;
}
static int cgpt_ex_readPoint(cgpt_ex_csvPoint &cgpt_ex_point) {
    char cgpt_ex_line[192];
    for (uint8_t cgpt_ex_blank=0;cgpt_ex_blank<64;++cgpt_ex_blank) {
        const int cgpt_ex_result=cgpt_ex_readLine(cgpt_ex_line,sizeof(cgpt_ex_line));
        if (cgpt_ex_result<=0) return cgpt_ex_result;
        bool cgpt_ex_empty=true;
        for (const char *cgpt_ex_c=cgpt_ex_line;*cgpt_ex_c;++cgpt_ex_c) if (!isspace(static_cast<unsigned char>(*cgpt_ex_c))) cgpt_ex_empty=false;
        if (cgpt_ex_empty) continue;
        if (!cgpt_ex_csvParsePoint(cgpt_ex_line,cgpt_ex_point) || !cgpt_ex_csvPointInLimits(cgpt_ex_point)) {
            snprintf(cgpt_ex_sdMessage,sizeof(cgpt_ex_sdMessage),"Invalid CSV row / limits: line %lu",static_cast<unsigned long>(cgpt_ex_csvLine));return -1;
        }
        return 1;
    }
    cgpt_ex_sdFail("Too many blank CSV lines");return -1;
}
static bool cgpt_ex_openPlayback() {
    cgpt_ex_playFile.close();
    cgpt_ex_playFile=SD.open(cgpt_ex_playPath,FILE_READ);
    if (!cgpt_ex_playFile || cgpt_ex_playFile.isDirectory()) return cgpt_ex_sdFail("Cannot open CSV file");
    cgpt_ex_csvLine=0;cgpt_ex_csvBytesRead=0;cgpt_ex_csvFileSize=cgpt_ex_playFile.size();
    char cgpt_ex_header[192];
    if (cgpt_ex_readLine(cgpt_ex_header,sizeof(cgpt_ex_header))!=1 || !cgpt_ex_csvParseHeader(cgpt_ex_header)) {
        cgpt_ex_playFile.close();return cgpt_ex_sdFail("CSV header does not match");
    }
    return true;
}
void cgpt_ex_sdPlayStop() {cgpt_ex_playFile.close();cgpt_ex_csvPhase=cgpt_ex_csvIdle;}
bool cgpt_ex_sdPlayStart(const char *cgpt_ex_path, uint32_t cgpt_ex_now, const uint16_t cgpt_ex_positions[cgpt_ex_jointCount]) {
    if (cgpt_ex_captureActive) return cgpt_ex_sdFail("Finish capture first");
    cgpt_ex_sdPlayStop();cgpt_ex_sdMessage[0]='\0';
    if (!cgpt_ex_csvRootPath(cgpt_ex_path,cgpt_ex_playPath,sizeof(cgpt_ex_playPath))) return cgpt_ex_sdFail("Select a root CSV file");
    if (!cgpt_ex_sdBegin() || !cgpt_ex_openPlayback()) return false;
    memcpy(cgpt_ex_csvApproachFrom,cgpt_ex_positions,sizeof(cgpt_ex_csvApproachFrom));
    cgpt_ex_csvClockStart=cgpt_ex_now;cgpt_ex_checkedRows=0;cgpt_ex_streamPointIndex=0;
    cgpt_ex_csvPhase=cgpt_ex_csvChecking;
    return true;
}
static bool cgpt_ex_startStream(uint32_t cgpt_ex_now) {
    if (!cgpt_ex_openPlayback()) return false;
    if (cgpt_ex_readPoint(cgpt_ex_csvPrevious)!=1) {
        if (!cgpt_ex_sdMessage[0]) cgpt_ex_sdFail("CSV changed or empty");
        return false;
    }
    cgpt_ex_csvClockStart=cgpt_ex_now;cgpt_ex_csvPhase=cgpt_ex_csvMotion;cgpt_ex_streamPointIndex=0;
    return true;
}
static void cgpt_ex_interpolate(const uint16_t cgpt_ex_from[cgpt_ex_jointCount], const uint16_t cgpt_ex_to[cgpt_ex_jointCount],
                                 uint32_t cgpt_ex_elapsed, uint32_t cgpt_ex_duration, uint16_t cgpt_ex_out[cgpt_ex_jointCount]) {
    if (cgpt_ex_elapsed>cgpt_ex_duration) cgpt_ex_elapsed=cgpt_ex_duration;
    for (uint8_t cgpt_ex_i=0;cgpt_ex_i<cgpt_ex_jointCount;++cgpt_ex_i) {
        const int32_t cgpt_ex_delta=static_cast<int32_t>(cgpt_ex_to[cgpt_ex_i])-cgpt_ex_from[cgpt_ex_i];
        cgpt_ex_out[cgpt_ex_i]=static_cast<uint16_t>(cgpt_ex_from[cgpt_ex_i]+static_cast<int64_t>(cgpt_ex_delta)*cgpt_ex_elapsed/cgpt_ex_duration);
    }
}
bool cgpt_ex_sdPlayService(uint32_t cgpt_ex_now, uint16_t cgpt_ex_positions[cgpt_ex_jointCount], bool cgpt_ex_loop) {
    if (cgpt_ex_csvPhase==cgpt_ex_csvIdle) return false;
    if (cgpt_ex_csvPhase==cgpt_ex_csvChecking) {
        // Validate the entire file in short batches before any commanded motion.
        for (uint8_t cgpt_ex_i=0;cgpt_ex_i<cgpt_ex_csvRowsPerPass;++cgpt_ex_i) {
            cgpt_ex_csvPoint cgpt_ex_point={};
            const int cgpt_ex_result=cgpt_ex_readPoint(cgpt_ex_point);
            if (cgpt_ex_result<0) {cgpt_ex_sdPlayStop();return false;}
            if (!cgpt_ex_result) {
                if (!cgpt_ex_checkedRows) {cgpt_ex_sdFail("CSV has no positions");cgpt_ex_sdPlayStop();return false;}
                if (!cgpt_ex_startStream(cgpt_ex_now)) cgpt_ex_sdPlayStop();
                return false;
            }
            ++cgpt_ex_checkedRows;
        }
        return false;
    }
    const uint32_t cgpt_ex_elapsed=cgpt_ex_now-cgpt_ex_csvClockStart;
    cgpt_ex_interpolate(cgpt_ex_csvApproachFrom,cgpt_ex_csvPrevious.cgpt_ex_positions,
        cgpt_ex_elapsed,cgpt_ex_csvPrevious.cgpt_ex_moveMs,cgpt_ex_positions);
    if (cgpt_ex_elapsed>=cgpt_ex_csvPrevious.cgpt_ex_moveMs) {
        // Reach each endpoint before advancing. Do not skip poses after a stall.
        memcpy(cgpt_ex_csvApproachFrom,cgpt_ex_positions,sizeof(cgpt_ex_csvApproachFrom));
        const int cgpt_ex_result=cgpt_ex_readPoint(cgpt_ex_csvPrevious);
        if (cgpt_ex_result>0) ++cgpt_ex_streamPointIndex;
        if (cgpt_ex_result<0) cgpt_ex_sdPlayStop();
        else if (!cgpt_ex_result) {
            if (cgpt_ex_loop) {if (!cgpt_ex_startStream(cgpt_ex_now)) cgpt_ex_sdPlayStop();}
            else cgpt_ex_sdPlayStop();
        }
        cgpt_ex_csvClockStart=cgpt_ex_now;
    }
    return true;
}

// Bounded streaming editor. Existing move_ms values survive insert/delete.
struct cgpt_ex_editReader {
    File cgpt_ex_file;
    size_t cgpt_ex_offset = 0;
    size_t cgpt_ex_size = 0;
    uint32_t cgpt_ex_line = 0;
    int cgpt_ex_lineRead(char *cgpt_ex_buffer, size_t cgpt_ex_capacity) {
        size_t cgpt_ex_used = 0;
        while (cgpt_ex_offset < cgpt_ex_size) {
            const int cgpt_ex_byte = cgpt_ex_file.read();
            if (cgpt_ex_byte < 0) { cgpt_ex_sdFail("SD edit read failed"); return -1; }
            ++cgpt_ex_offset;
            if (cgpt_ex_byte == '\n') { ++cgpt_ex_line; cgpt_ex_buffer[cgpt_ex_used] = 0; return 1; }
            if (cgpt_ex_used + 1 >= cgpt_ex_capacity) { cgpt_ex_sdFail("CSV line too long"); return -1; }
            cgpt_ex_buffer[cgpt_ex_used++] = static_cast<char>(cgpt_ex_byte);
        }
        cgpt_ex_buffer[cgpt_ex_used] = 0;
        if (cgpt_ex_used) { ++cgpt_ex_line; return 1; }
        return 0;
    }
    bool cgpt_ex_open(const char *cgpt_ex_path) {
        cgpt_ex_file = SD.open(cgpt_ex_path, FILE_READ);
        if (!cgpt_ex_file || cgpt_ex_file.isDirectory()) return cgpt_ex_sdFail("Cannot open edit CSV");
        cgpt_ex_offset = 0; cgpt_ex_line = 0; cgpt_ex_size = cgpt_ex_file.size();
        char cgpt_ex_header[192];
        return (cgpt_ex_lineRead(cgpt_ex_header, sizeof(cgpt_ex_header)) == 1 &&
                cgpt_ex_csvParseHeader(cgpt_ex_header)) || cgpt_ex_sdFail("CSV header does not match");
    }
    int cgpt_ex_next(cgpt_ex_csvPoint &cgpt_ex_point) {
        char cgpt_ex_buffer[192];
        for (uint8_t cgpt_ex_blank = 0; cgpt_ex_blank < 64; ++cgpt_ex_blank) {
            const int cgpt_ex_result = cgpt_ex_lineRead(cgpt_ex_buffer, sizeof(cgpt_ex_buffer));
            if (cgpt_ex_result <= 0) return cgpt_ex_result;
            bool cgpt_ex_empty = true;
            for (const char *cgpt_ex_c = cgpt_ex_buffer; *cgpt_ex_c; ++cgpt_ex_c)
                if (!isspace(static_cast<unsigned char>(*cgpt_ex_c))) cgpt_ex_empty = false;
            if (cgpt_ex_empty) continue;
            if (cgpt_ex_csvParsePoint(cgpt_ex_buffer, cgpt_ex_point) && cgpt_ex_csvPointInLimits(cgpt_ex_point)) return 1;
            snprintf(cgpt_ex_sdMessage, sizeof(cgpt_ex_sdMessage), "Invalid edit CSV: line %lu", static_cast<unsigned long>(cgpt_ex_line));
            return -1;
        }
        cgpt_ex_sdFail("Too many blank CSV lines"); return -1;
    }
};
static bool cgpt_ex_editScan(const char *cgpt_ex_path, uint32_t cgpt_ex_index,
                             cgpt_ex_csvPoint &cgpt_ex_selected, uint32_t &cgpt_ex_count) {
    cgpt_ex_count = 0;
    cgpt_ex_editReader cgpt_ex_reader;
    if (!cgpt_ex_reader.cgpt_ex_open(cgpt_ex_path)) return false;
    while (true) {
        cgpt_ex_csvPoint cgpt_ex_point = {};
        const int cgpt_ex_result = cgpt_ex_reader.cgpt_ex_next(cgpt_ex_point);
        if (cgpt_ex_result < 0) return false;
        if (!cgpt_ex_result) return true;
        if (cgpt_ex_count == UINT32_MAX) return cgpt_ex_sdFail("Too many CSV points");
        if (cgpt_ex_count == cgpt_ex_index) cgpt_ex_selected = cgpt_ex_point;
        ++cgpt_ex_count;
        if (cgpt_ex_count % cgpt_ex_csvRowsPerPass == 0) yield();
    }
}
bool cgpt_ex_sdEditRead(const char *cgpt_ex_path, uint32_t cgpt_ex_index,
                        cgpt_ex_csvPoint &cgpt_ex_point, uint32_t &cgpt_ex_count) {
    cgpt_ex_sdMessage[0] = 0;
    if (cgpt_ex_captureActive || cgpt_ex_csvPhase != cgpt_ex_csvIdle) return cgpt_ex_sdFail("Stop SD activity before editing");
    char cgpt_ex_rootPath[cgpt_ex_fileNameSize];
    if (!cgpt_ex_csvRootPath(cgpt_ex_path, cgpt_ex_rootPath, sizeof(cgpt_ex_rootPath))) return cgpt_ex_sdFail("Choose a root CSV file");
    return cgpt_ex_sdBegin() && cgpt_ex_editScan(cgpt_ex_rootPath, cgpt_ex_index, cgpt_ex_point, cgpt_ex_count);
}
static bool cgpt_ex_editWrite(File &cgpt_ex_file, const char *cgpt_ex_data, size_t cgpt_ex_size) {
    return cgpt_ex_file.write(reinterpret_cast<const uint8_t *>(cgpt_ex_data), cgpt_ex_size) == cgpt_ex_size && !cgpt_ex_file.getWriteError();
}
static bool cgpt_ex_editWritePoint(File &cgpt_ex_file, const cgpt_ex_csvPoint &cgpt_ex_point) {
    char cgpt_ex_buffer[96];
    const size_t cgpt_ex_length = cgpt_ex_csvFormatPoint(cgpt_ex_buffer, sizeof(cgpt_ex_buffer), cgpt_ex_point);
    return cgpt_ex_length && cgpt_ex_editWrite(cgpt_ex_file, cgpt_ex_buffer, cgpt_ex_length);
}
static bool cgpt_ex_editRewrite(const char *cgpt_ex_path, uint32_t cgpt_ex_index, const cgpt_ex_csvPoint *cgpt_ex_insert) {
    cgpt_ex_csvPoint cgpt_ex_point = {};
    uint32_t cgpt_ex_count = 0;
    if (!cgpt_ex_sdEditRead(cgpt_ex_path, cgpt_ex_index, cgpt_ex_point, cgpt_ex_count)) return false;
    if (cgpt_ex_index > cgpt_ex_count || (!cgpt_ex_insert && cgpt_ex_index == cgpt_ex_count)) return cgpt_ex_sdFail("Edit point out of range");
    if (cgpt_ex_insert && (cgpt_ex_count == UINT32_MAX || !cgpt_ex_csvPointInLimits(*cgpt_ex_insert) ||
        cgpt_ex_insert->cgpt_ex_moveMs < 20 || cgpt_ex_insert->cgpt_ex_moveMs > cgpt_ex_csvMaxMoveMs)) return cgpt_ex_sdFail("Invalid inserted point");
    char cgpt_ex_rootPath[cgpt_ex_fileNameSize];
    cgpt_ex_csvRootPath(cgpt_ex_path, cgpt_ex_rootPath, sizeof(cgpt_ex_rootPath));
    char cgpt_ex_temp[cgpt_ex_fileNameSize + 16], cgpt_ex_backup[cgpt_ex_fileNameSize + 16];
    snprintf(cgpt_ex_temp, sizeof(cgpt_ex_temp), "%s.cgpt.tmp", cgpt_ex_rootPath);
    snprintf(cgpt_ex_backup, sizeof(cgpt_ex_backup), "%s.cgpt.bak", cgpt_ex_rootPath);
    if (SD.exists(cgpt_ex_temp) || SD.exists(cgpt_ex_backup)) return cgpt_ex_sdFail("Edit recovery files exist; check card on PC");
    cgpt_ex_editReader cgpt_ex_reader;
    if (!cgpt_ex_reader.cgpt_ex_open(cgpt_ex_rootPath)) return false;
    File cgpt_ex_output = SD.open(cgpt_ex_temp, FILE_WRITE);
    if (!cgpt_ex_output) return cgpt_ex_sdFail("Cannot create edit temporary file");
    cgpt_ex_output.clearWriteError();
    bool cgpt_ex_ok = cgpt_ex_editWrite(cgpt_ex_output, cgpt_ex_csvHeader, strlen(cgpt_ex_csvHeader)) &&
        cgpt_ex_editWrite(cgpt_ex_output, "\n", 1);
    uint32_t cgpt_ex_row = 0;
    while (cgpt_ex_ok) {
        const int cgpt_ex_result = cgpt_ex_reader.cgpt_ex_next(cgpt_ex_point);
        if (cgpt_ex_result < 0) { cgpt_ex_ok = false; break; }
        if (!cgpt_ex_result) {
            if (cgpt_ex_insert && cgpt_ex_index == cgpt_ex_row) cgpt_ex_ok = cgpt_ex_editWritePoint(cgpt_ex_output, *cgpt_ex_insert);
            break;
        }
        if (cgpt_ex_insert && cgpt_ex_index == cgpt_ex_row) cgpt_ex_ok = cgpt_ex_editWritePoint(cgpt_ex_output, *cgpt_ex_insert);
        if (cgpt_ex_ok && (cgpt_ex_insert || cgpt_ex_index != cgpt_ex_row)) cgpt_ex_ok = cgpt_ex_editWritePoint(cgpt_ex_output, cgpt_ex_point);
        ++cgpt_ex_row;
        if (cgpt_ex_row % cgpt_ex_csvRowsPerPass == 0) yield();
    }
    cgpt_ex_output.flush();
    cgpt_ex_ok = cgpt_ex_ok && !cgpt_ex_output.getWriteError() && cgpt_ex_row == cgpt_ex_count;
    cgpt_ex_output.close(); cgpt_ex_reader.cgpt_ex_file.close();
    uint32_t cgpt_ex_verified = 0;
    if (cgpt_ex_ok) cgpt_ex_ok = cgpt_ex_editScan(cgpt_ex_temp, UINT32_MAX, cgpt_ex_point, cgpt_ex_verified) &&
        cgpt_ex_verified == (cgpt_ex_insert ? cgpt_ex_count + 1 : cgpt_ex_count - 1);
    if (!cgpt_ex_ok) { SD.remove(cgpt_ex_temp); return cgpt_ex_sdFail("CSV edit failed; original kept"); }
    // Keep original bytes until the complete replacement is written/validated.
    if (!SD.rename(cgpt_ex_rootPath, cgpt_ex_backup)) { SD.remove(cgpt_ex_temp); return cgpt_ex_sdFail("Cannot back up CSV; original kept"); }
    if (!SD.rename(cgpt_ex_temp, cgpt_ex_rootPath)) {
        if (!SD.rename(cgpt_ex_backup, cgpt_ex_rootPath)) return cgpt_ex_sdFail("Restore .cgpt.bak on PC; original is there");
        SD.remove(cgpt_ex_temp); return cgpt_ex_sdFail("CSV replace failed; original restored");
    }
    if (!SD.remove(cgpt_ex_backup)) { cgpt_ex_sdFail("Edit saved; remove .cgpt.bak on PC"); return true; }
    cgpt_ex_sdMessage[0] = 0;
    return true;
}
bool cgpt_ex_sdEditInsert(const char *cgpt_ex_path, uint32_t cgpt_ex_index, const cgpt_ex_csvPoint &cgpt_ex_point) {
    return cgpt_ex_editRewrite(cgpt_ex_path, cgpt_ex_index, &cgpt_ex_point);
}
bool cgpt_ex_sdEditDelete(const char *cgpt_ex_path, uint32_t cgpt_ex_index) {
    return cgpt_ex_editRewrite(cgpt_ex_path, cgpt_ex_index, nullptr);
}

#else
uint32_t cgpt_ex_sdPlayPointIndex() {return 0;}
uint32_t cgpt_ex_sdPlayPointCount() {return 0;}
bool cgpt_ex_sdEditRead(const char *,uint32_t,cgpt_ex_csvPoint &,uint32_t &) {return false;}
bool cgpt_ex_sdEditInsert(const char *,uint32_t,const cgpt_ex_csvPoint &) {return false;}
bool cgpt_ex_sdEditDelete(const char *,uint32_t) {return false;}
bool cgpt_ex_sdBegin() {return false;}
bool cgpt_ex_sdRemount() {return false;}
bool cgpt_ex_sdReady() {return false;}
const char *cgpt_ex_sdError() {return "SD support disabled";}
bool cgpt_ex_sdCaptureStart() {return false;}
bool cgpt_ex_sdCapturePoint(const uint16_t *,uint32_t) {return false;}
bool cgpt_ex_sdCaptureStop() {return true;}
bool cgpt_ex_sdCapturing() {return false;}
const char *cgpt_ex_sdCaptureName() {return "";}
bool cgpt_ex_sdListPage(uint32_t,char [cgpt_ex_filesPerPage][cgpt_ex_fileNameSize],uint8_t &cgpt_ex_count,bool &cgpt_ex_more) {cgpt_ex_count=0;cgpt_ex_more=false;return false;}
bool cgpt_ex_sdPlayStart(const char *,uint32_t,const uint16_t *) {return false;}
bool cgpt_ex_sdPlayService(uint32_t,uint16_t *,bool) {return false;}
void cgpt_ex_sdPlayStop() {}
cgpt_ex_csvState cgpt_ex_sdPlayState() {return cgpt_ex_csvIdle;}
const char *cgpt_ex_sdPlayName() {return "";}
#endif
