#pragma once
#include "cgpt_ex_config.h"
#include <lvgl.h>

// Call after LVGL display/input registration. nullptr reuses the first pointer
// on the default display, or creates one if none exists. Repeated calls are safe.
// cgpt_ex_begin() calls this automatically; an explicit device may be passed first.
// The bound device must remain alive. Do not call an older hardware touch reader.
bool cgpt_ex_cydTouchBegin(lv_indev_t *cgpt_ex_input = nullptr);

