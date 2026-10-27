#pragma once
#include <stdint.h>

void cgpt_ex_wifiBegin();
void cgpt_ex_wifiService();
bool cgpt_ex_wifiScan();
bool cgpt_ex_wifiConnect(const char *cgpt_ex_ssid, const char *cgpt_ex_password);
bool cgpt_ex_wifiForget();
bool cgpt_ex_wifiConnected();
bool cgpt_ex_wifiConnecting();
bool cgpt_ex_wifiScanning();
const char *cgpt_ex_wifiMessage();
const char *cgpt_ex_wifiSavedSsid();
const char *cgpt_ex_wifiSavedPassword();
uint32_t cgpt_ex_wifiScanRevision();
uint8_t cgpt_ex_wifiNetworkCount();
const char *cgpt_ex_wifiNetworkName(uint8_t cgpt_ex_index);
int32_t cgpt_ex_wifiNetworkRssi(uint8_t cgpt_ex_index);
