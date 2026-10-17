#pragma once
#include "cgpt_ex_csv.h"

enum cgpt_ex_csvState { cgpt_ex_csvIdle, cgpt_ex_csvChecking, cgpt_ex_csvApproach, cgpt_ex_csvMotion };
bool cgpt_ex_sdBegin();
bool cgpt_ex_sdRemount(); // Only when no capture/playback file is open.
bool cgpt_ex_sdReady();
const char *cgpt_ex_sdError();
bool cgpt_ex_sdCaptureStart(); // Creates header only; does NOT capture a pose.
bool cgpt_ex_sdCapturePoint(const uint16_t cgpt_ex_positions[cgpt_ex_jointCount], uint32_t cgpt_ex_moveMs);
bool cgpt_ex_sdCaptureStop(); // Flushes/closes only; does NOT capture a pose.
bool cgpt_ex_sdCapturing();
const char *cgpt_ex_sdCaptureName();
bool cgpt_ex_sdListPage(uint32_t cgpt_ex_page, char cgpt_ex_names[cgpt_ex_filesPerPage][cgpt_ex_fileNameSize], uint8_t &cgpt_ex_count, bool &cgpt_ex_more);
bool cgpt_ex_sdPlayStart(const char *cgpt_ex_path, uint32_t cgpt_ex_now, const uint16_t cgpt_ex_positions[cgpt_ex_jointCount]);
// Advances validation/playback. Returns true if the output pose changed.
bool cgpt_ex_sdPlayService(uint32_t cgpt_ex_now, uint16_t cgpt_ex_positions[cgpt_ex_jointCount], bool cgpt_ex_loop);
void cgpt_ex_sdPlayStop();
cgpt_ex_csvState cgpt_ex_sdPlayState();
const char *cgpt_ex_sdPlayName();

// CSV editor uses streaming scans/rewrites; no RAM pose-capacity limit.
// Caller must stop playback and close capture before these operations.
uint32_t cgpt_ex_sdPlayPointIndex();
uint32_t cgpt_ex_sdPlayPointCount(); // Full validated count once playback begins.
// Read validates/counts the entire file. Fills point only when index < count;
// UINT32_MAX requests counting only. Empty (header-only) files are valid.
bool cgpt_ex_sdEditRead(const char *cgpt_ex_path, uint32_t cgpt_ex_index,
                       cgpt_ex_csvPoint &cgpt_ex_point, uint32_t &cgpt_ex_count);
bool cgpt_ex_sdEditInsert(const char *cgpt_ex_path, uint32_t cgpt_ex_index,
                         const cgpt_ex_csvPoint &cgpt_ex_point);
bool cgpt_ex_sdEditDelete(const char *cgpt_ex_path, uint32_t cgpt_ex_index);
