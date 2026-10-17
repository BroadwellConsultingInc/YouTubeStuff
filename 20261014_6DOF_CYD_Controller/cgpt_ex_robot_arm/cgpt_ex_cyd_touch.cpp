#include "cgpt_ex_cyd_touch.h"

#if cgpt_ex_enableCydSoftTouch
#include "XPT2046_TouchscreenSOFTSPI.h"

static SoftSPI cgpt_ex_touchSpi(cgpt_ex_touchMosi, cgpt_ex_touchMiso, cgpt_ex_touchSck);
static XPT2046_TouchscreenSOFTSPI cgpt_ex_touch(cgpt_ex_touchCs, cgpt_ex_touchIrq);
static bool cgpt_ex_touchInitialized = false;
static lv_indev_t *cgpt_ex_touchInput = nullptr;
static lv_point_t cgpt_ex_touchLast = {};

static int32_t cgpt_ex_touchScale(int16_t cgpt_ex_raw, int16_t cgpt_ex_min,
                                int16_t cgpt_ex_max, int32_t cgpt_ex_size) {
    const int32_t cgpt_ex_value = (int32_t(cgpt_ex_raw) - cgpt_ex_min) *
        (cgpt_ex_size - 1) / (int32_t(cgpt_ex_max) - cgpt_ex_min);
    return cgpt_ex_value < 0 ? 0 : (cgpt_ex_value >= cgpt_ex_size ? cgpt_ex_size - 1 : cgpt_ex_value);
}

#if LVGL_VERSION_MAJOR >= 9
static void cgpt_ex_touchRead(lv_indev_t *cgpt_ex_input, lv_indev_data_t *cgpt_ex_data) {
    lv_display_t *cgpt_ex_display = lv_indev_get_display(cgpt_ex_input);
    int32_t cgpt_ex_width = lv_display_get_horizontal_resolution(cgpt_ex_display);
    int32_t cgpt_ex_height = lv_display_get_vertical_resolution(cgpt_ex_display);
    const lv_display_rotation_t cgpt_ex_rotation = lv_display_get_rotation(cgpt_ex_display);
    if (cgpt_ex_rotation == LV_DISPLAY_ROTATION_90 || cgpt_ex_rotation == LV_DISPLAY_ROTATION_270) {
        const int32_t cgpt_ex_swap = cgpt_ex_width;
        cgpt_ex_width = cgpt_ex_height; cgpt_ex_height = cgpt_ex_swap;
    }
#else
static void cgpt_ex_touchRead(lv_indev_drv_t *cgpt_ex_driver, lv_indev_data_t *cgpt_ex_data) {
    const int32_t cgpt_ex_width = cgpt_ex_driver->disp->driver->hor_res;
    const int32_t cgpt_ex_height = cgpt_ex_driver->disp->driver->ver_res;
#endif
    cgpt_ex_data->state = LV_INDEV_STATE_REL;
    cgpt_ex_data->continue_reading = false;
    if (cgpt_ex_touch.tirqTouched() && cgpt_ex_touch.touched(&cgpt_ex_touchSpi)) {
        const cgpt_ex_touchPoint cgpt_ex_point = cgpt_ex_touch.getPoint(&cgpt_ex_touchSpi);
        // Coordinates in the UNROTATED LVGL display space. LVGL rotates once.
        cgpt_ex_touchLast.x = cgpt_ex_touchScale(cgpt_ex_point.x, cgpt_ex_touchRawMinX,
                                               cgpt_ex_touchRawMaxX, cgpt_ex_width);
        cgpt_ex_touchLast.y = cgpt_ex_touchScale(cgpt_ex_point.y, cgpt_ex_touchRawMinY,
                                               cgpt_ex_touchRawMaxY, cgpt_ex_height);
        cgpt_ex_data->state = LV_INDEV_STATE_PR;
    }
    cgpt_ex_data->point = cgpt_ex_touchLast;
}

bool cgpt_ex_cydTouchBegin(lv_indev_t *cgpt_ex_input) {
    if (cgpt_ex_touchInput) return !cgpt_ex_input || cgpt_ex_input == cgpt_ex_touchInput;
#if LVGL_VERSION_MAJOR >= 9
    lv_display_t *cgpt_ex_display = cgpt_ex_input ? lv_indev_get_display(cgpt_ex_input) : lv_display_get_default();
    if (!cgpt_ex_display || (cgpt_ex_input && lv_indev_get_type(cgpt_ex_input) != LV_INDEV_TYPE_POINTER)) return false;
    if (!cgpt_ex_input) {
        while ((cgpt_ex_input = lv_indev_get_next(cgpt_ex_input))) {
            if (lv_indev_get_type(cgpt_ex_input) == LV_INDEV_TYPE_POINTER &&
                lv_indev_get_display(cgpt_ex_input) == cgpt_ex_display) break;
        }
    }
    int32_t cgpt_ex_width = lv_display_get_horizontal_resolution(cgpt_ex_display);
    int32_t cgpt_ex_height = lv_display_get_vertical_resolution(cgpt_ex_display);
    const lv_display_rotation_t cgpt_ex_rotation = lv_display_get_rotation(cgpt_ex_display);
    if (cgpt_ex_rotation == LV_DISPLAY_ROTATION_90 || cgpt_ex_rotation == LV_DISPLAY_ROTATION_270) {
        const int32_t cgpt_ex_swap = cgpt_ex_width;
        cgpt_ex_width = cgpt_ex_height; cgpt_ex_height = cgpt_ex_swap;
    }
    if (!cgpt_ex_input) {
        cgpt_ex_input = lv_indev_create();
        if (!cgpt_ex_input) return false;
        lv_indev_set_type(cgpt_ex_input, LV_INDEV_TYPE_POINTER);
        lv_indev_set_display(cgpt_ex_input, cgpt_ex_display);
    }
#else
    lv_disp_t *cgpt_ex_display = cgpt_ex_input ? cgpt_ex_input->driver->disp : lv_disp_get_default();
    if (!cgpt_ex_display || (cgpt_ex_input && lv_indev_get_type(cgpt_ex_input) != LV_INDEV_TYPE_POINTER)) return false;
    if (!cgpt_ex_input) {
        while ((cgpt_ex_input = lv_indev_get_next(cgpt_ex_input))) {
            if (lv_indev_get_type(cgpt_ex_input) == LV_INDEV_TYPE_POINTER &&
                cgpt_ex_input->driver->disp == cgpt_ex_display) break;
        }
    }
    const int32_t cgpt_ex_width = cgpt_ex_display->driver->hor_res;
    const int32_t cgpt_ex_height = cgpt_ex_display->driver->ver_res;
    if (!cgpt_ex_input) {
        static lv_indev_drv_t cgpt_ex_touchDriver;
        lv_indev_drv_init(&cgpt_ex_touchDriver);
        cgpt_ex_touchDriver.type = LV_INDEV_TYPE_POINTER;
        cgpt_ex_touchDriver.disp = cgpt_ex_display;
        cgpt_ex_touchDriver.read_cb = cgpt_ex_touchRead;
        cgpt_ex_input = lv_indev_drv_register(&cgpt_ex_touchDriver);
        if (!cgpt_ex_input) return false;
    }
#endif
    if (!cgpt_ex_touchInitialized) {
        cgpt_ex_touch.begin(&cgpt_ex_touchSpi);
        cgpt_ex_touch.setRotation(cgpt_ex_touchRotation < 0 ?
            (cgpt_ex_width < cgpt_ex_height ? 2 : 1) : cgpt_ex_touchRotation);
        cgpt_ex_touchInitialized = true;
    }
#if LVGL_VERSION_MAJOR >= 9
    lv_indev_set_read_cb(cgpt_ex_input, cgpt_ex_touchRead);
#else
    cgpt_ex_input->driver->read_cb = cgpt_ex_touchRead;
#endif
    cgpt_ex_touchInput = cgpt_ex_input;
    return true;
}

#else
bool cgpt_ex_cydTouchBegin(lv_indev_t *) { return true; }
#endif
