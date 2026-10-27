#pragma once
#include <stdint.h>
#include "cgpt_ex_neptune.h"

// Call after cgpt_ex_wombatBegin(). These functions run in the Arduino loop task.
void cgpt_ex_tm1637Begin();
void cgpt_ex_tm1637Service();

// Six display characters plus a terminating null, independent of hardware/LVGL.
// The network flag is retained for API compatibility; sample age determines validity.
void cgpt_ex_tm1637Format(const cgpt_ex_NeptuneConfig &cgpt_ex_config,
    const cgpt_ex_NeptuneStatus &cgpt_ex_status, bool cgpt_ex_networkConnected,
    uint32_t cgpt_ex_now, char cgpt_ex_text[7]);
