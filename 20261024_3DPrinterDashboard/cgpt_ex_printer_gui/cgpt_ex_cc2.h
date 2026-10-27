#pragma once
#include "cgpt_ex_neptune.h"

// Called only by the printer worker. Disabled/non-CC2 slots release their socket.
bool cgpt_ex_cc2Service(uint8_t cgpt_ex_slot, const cgpt_ex_NeptuneConfig &cgpt_ex_config,
    cgpt_ex_NeptuneStatus &cgpt_ex_status);
// Independent parser for full method-1002 response fixtures.
bool cgpt_ex_cc2Parse(const char *cgpt_ex_payload, size_t cgpt_ex_length,
    cgpt_ex_NeptuneStatus &cgpt_ex_status);
