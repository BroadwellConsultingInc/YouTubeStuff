#include "cgpt_ex_robot_arm.h"
#include <Wire.h>
#include <SerialWombat.h>
#include <stdio.h>
#include <string.h>

static SerialWombatChip cgpt_ex_servoChip;
static SerialWombatChip cgpt_ex_inputChip;
static bool cgpt_ex_previousButtonState[cgpt_ex_buttonCount] = {};
static cgpt_ex_chipInfo cgpt_ex_chipSnapshots[2] = {};

const cgpt_ex_chipInfo *cgpt_ex_getChipInfo(uint8_t cgpt_ex_chipIndex) {
    return cgpt_ex_chipIndex < 2 ? &cgpt_ex_chipSnapshots[cgpt_ex_chipIndex] : nullptr;
}

// Initialize the first servo packet with the startup position. The base class
// normally initializes its position to zero before attach(). Its existing
// protected member _position is library-owned, not an example-created name.
class cgpt_ex_startupServo : public SerialWombatServo {
public:
    explicit cgpt_ex_startupServo(SerialWombatChip &cgpt_ex_chip)
        : SerialWombatServo(cgpt_ex_chip) {}
    void cgpt_ex_attachAtPosition(const cgpt_ex_jointConfig &cgpt_ex_cfg) {
        _position = cgpt_ex_cfg.cgpt_ex_initialPosition;
        attach(cgpt_ex_cfg.cgpt_ex_servoPin, cgpt_ex_cfg.cgpt_ex_minPulseUs,
               cgpt_ex_cfg.cgpt_ex_maxPulseUs, cgpt_ex_cfg.cgpt_ex_reverseServo);
    }
};

// Checked form of readPublicData, also used for startup system information.
static bool cgpt_ex_readPublicChecked(SerialWombatChip &cgpt_ex_chip,
                                      uint8_t cgpt_ex_pin, uint16_t &cgpt_ex_value) {
    uint8_t cgpt_ex_tx[8] = {0x81, cgpt_ex_pin, 0xFF, 0xFF, 0x55, 0x55, 0x55, 0x55};
    uint8_t cgpt_ex_rx[8] = {};
    if (cgpt_ex_chip.sendPacket(cgpt_ex_tx, cgpt_ex_rx) < 0) return false;
    cgpt_ex_value = static_cast<uint16_t>(cgpt_ex_rx[2]) |
                    (static_cast<uint16_t>(cgpt_ex_rx[3]) << 8);
    return true;
}

static bool cgpt_ex_openChip(SerialWombatChip &cgpt_ex_chip, cgpt_ex_chipInfo &cgpt_ex_info,
                             uint8_t cgpt_ex_address) {
    cgpt_ex_info = {};
    cgpt_ex_info.cgpt_ex_address = cgpt_ex_address;
    const uint16_t cgpt_ex_errorsBefore = cgpt_ex_chip.errorCount;
    const int16_t cgpt_ex_beginResult = cgpt_ex_chip.begin(Wire, cgpt_ex_address, false);
    const bool cgpt_ex_beginOk = cgpt_ex_beginResult >= 0 && cgpt_ex_chip.errorCount == cgpt_ex_errorsBefore;
    uint8_t cgpt_ex_tx[8] = {'V', 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    uint8_t cgpt_ex_rx[8] = {};
    const int cgpt_ex_versionResult = cgpt_ex_chip.sendPacket(cgpt_ex_tx, cgpt_ex_rx);
    if (cgpt_ex_versionResult < 0 || cgpt_ex_rx[0] != 'V' ||
        (cgpt_ex_rx[1] != 'S' && cgpt_ex_rx[1] != 'B')) {
        cgpt_ex_info.cgpt_ex_errorCode = cgpt_ex_versionResult < 0 ? cgpt_ex_versionResult : -1;
        return false;
    }
    cgpt_ex_info.cgpt_ex_connected = true;
    cgpt_ex_info.cgpt_ex_inBoot = cgpt_ex_rx[1] == 'B';
    memcpy(cgpt_ex_info.cgpt_ex_model, &cgpt_ex_rx[1], 4);
    memcpy(cgpt_ex_info.cgpt_ex_firmware, &cgpt_ex_rx[5], 3);
    // Model and firmware arrays remain NUL-terminated from value initialization.
    if (cgpt_ex_info.cgpt_ex_inBoot || !cgpt_ex_beginOk) {
        cgpt_ex_info.cgpt_ex_errorCode = cgpt_ex_beginResult < 0 ? cgpt_ex_beginResult : -1;
        return false;
    }
    // Same information sources as ChipFinder; checked packets avoid using data
    // from a failed read. VDD is the chip's logic supply, not the servo supply.
    if (!cgpt_ex_readPublicChecked(cgpt_ex_chip,
            static_cast<uint8_t>(SerialWombatDataSource::SW_DATA_SOURCE_VCC_mVOLTS), cgpt_ex_info.cgpt_ex_supplyMv) ||
        !cgpt_ex_readPublicChecked(cgpt_ex_chip,
            static_cast<uint8_t>(SerialWombatDataSource::SW_DATA_SOURCE_OVERRUN_FRAMES), cgpt_ex_info.cgpt_ex_overflowFrames)) {
        cgpt_ex_info.cgpt_ex_errorCode = -1;
        return false;
    }
    uint8_t cgpt_ex_framesTx[8] = {0x81, 67, 68, 0x55, 0x55, 0x55, 0x55, 0x55};
    uint8_t cgpt_ex_framesRx[8] = {};
    const int cgpt_ex_framesResult = cgpt_ex_chip.sendPacket(cgpt_ex_framesTx, cgpt_ex_framesRx);
    if (cgpt_ex_framesResult < 0) { cgpt_ex_info.cgpt_ex_errorCode = cgpt_ex_framesResult; return false; }
    cgpt_ex_info.cgpt_ex_frames = static_cast<uint32_t>(cgpt_ex_framesRx[2]) |
        (static_cast<uint32_t>(cgpt_ex_framesRx[3]) << 8) |
        (static_cast<uint32_t>(cgpt_ex_framesRx[4]) << 16) |
        (static_cast<uint32_t>(cgpt_ex_framesRx[5]) << 24);
    cgpt_ex_info.cgpt_ex_systemInfoValid = true;
    const uint8_t cgpt_ex_uidLength = cgpt_ex_chip.uniqueIdentifierLength > 16 ? 16 : cgpt_ex_chip.uniqueIdentifierLength;
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_uidLength; ++cgpt_ex_i) {
        const size_t cgpt_ex_offset = strlen(cgpt_ex_info.cgpt_ex_uniqueId);
        snprintf(cgpt_ex_info.cgpt_ex_uniqueId + cgpt_ex_offset,
                 sizeof(cgpt_ex_info.cgpt_ex_uniqueId) - cgpt_ex_offset, "%02X%s",
                 static_cast<unsigned>(cgpt_ex_chip.uniqueIdentifier[cgpt_ex_i]),
                 cgpt_ex_i + 1 < cgpt_ex_uidLength ? ((cgpt_ex_i + 1) % 4 ? " " : "\n") : "");
    }
    // The library does not populate deviceIdentifier/deviceRevision for its
    // CH32-based models. Display N/A instead of uninitialized or invented data.
    cgpt_ex_info.cgpt_ex_deviceInfoValid = cgpt_ex_chip.isSW04() || cgpt_ex_chip.isSW18();
    if (cgpt_ex_info.cgpt_ex_deviceInfoValid) {
        cgpt_ex_info.cgpt_ex_deviceId = cgpt_ex_chip.deviceIdentifier;
        cgpt_ex_info.cgpt_ex_deviceRevision = cgpt_ex_chip.deviceRevision;
    }
    return true;
}

static bool cgpt_ex_initError(cgpt_ex_chipInfo &cgpt_ex_info, int cgpt_ex_result) {
    cgpt_ex_info.cgpt_ex_initialized = false;
    cgpt_ex_info.cgpt_ex_errorCode = cgpt_ex_result < 0 ? cgpt_ex_result : -1;
    return false;
}

// Checked equivalent of SerialWombatDebouncedInput::readTransitionsState(true).
static SerialWombatChip &cgpt_ex_buttonChip(uint8_t cgpt_ex_button) {
    return cgpt_ex_buttons[cgpt_ex_button].cgpt_ex_chip ? cgpt_ex_servoChip : cgpt_ex_inputChip;
}
static bool cgpt_ex_readButtonChecked(uint8_t cgpt_ex_button, uint16_t &cgpt_ex_presses) {
    uint8_t cgpt_ex_tx[8] = {0xC9, cgpt_ex_buttons[cgpt_ex_button].cgpt_ex_pin, PIN_MODE_DEBOUNCE, 1,
                            0x55, 0x55, 0x55, 0x55};
    uint8_t cgpt_ex_rx[8] = {};
    if (cgpt_ex_buttonChip(cgpt_ex_button).sendPacket(cgpt_ex_tx, cgpt_ex_rx) < 0) return false;
    const bool cgpt_ex_state = cgpt_ex_rx[3] != 0;
    const uint16_t cgpt_ex_transitions = static_cast<uint16_t>(cgpt_ex_rx[4]) |
                                        (static_cast<uint16_t>(cgpt_ex_rx[5]) << 8);
    // Starting released: transitions 1,2 => one press; 3,4 => two.
    // Starting pressed: transitions 1 => release only; 2,3 => one new press.
    cgpt_ex_presses = static_cast<uint16_t>(
        (static_cast<uint32_t>(cgpt_ex_transitions) + (cgpt_ex_previousButtonState[cgpt_ex_button] ? 0 : 1)) / 2);
    cgpt_ex_previousButtonState[cgpt_ex_button] = cgpt_ex_state;
    return true;
}

bool cgpt_ex_hardwareBegin() {
    // Wire must already be initialized. No reset of either chip is requested.
    // The library's begin(TwoWire&,...) currently probes the global Wire object;
    // this default adapter therefore deliberately uses Wire for both chips.
    // Probe both independently so a failure on one still reports the other.
    const bool cgpt_ex_servoOk = cgpt_ex_openChip(cgpt_ex_servoChip, cgpt_ex_chipSnapshots[0], cgpt_ex_servoAddress);
    const bool cgpt_ex_inputOk = cgpt_ex_openChip(cgpt_ex_inputChip, cgpt_ex_chipSnapshots[1], cgpt_ex_inputAddress);
    if (!cgpt_ex_servoOk || !cgpt_ex_inputOk) return false;
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
        const cgpt_ex_jointConfig &cgpt_ex_cfg = cgpt_ex_jointSettings[cgpt_ex_i];
        const uint16_t cgpt_ex_errors = cgpt_ex_servoChip.errorCount;
        cgpt_ex_startupServo cgpt_ex_servo(cgpt_ex_servoChip);
        cgpt_ex_servo.cgpt_ex_attachAtPosition(cgpt_ex_cfg);
        if (cgpt_ex_servoChip.errorCount != cgpt_ex_errors) return cgpt_ex_initError(cgpt_ex_chipSnapshots[0], cgpt_ex_servoChip.lastErrorCode);
    }
    cgpt_ex_chipSnapshots[0].cgpt_ex_initialized = true;
#if cgpt_ex_enableJoysticks
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
        const cgpt_ex_jointConfig &cgpt_ex_cfg = cgpt_ex_jointSettings[cgpt_ex_i];
        SerialWombatAnalogInput cgpt_ex_analog(cgpt_ex_inputChip);
        const int16_t cgpt_ex_analogResult = cgpt_ex_analog.begin(cgpt_ex_cfg.cgpt_ex_analogPin, 16, 0xFF80,
            AnalogInputPublicDataOutput::AnalogInputPublicDataOutput_Averaged);
        if (cgpt_ex_analogResult < 0) return cgpt_ex_initError(cgpt_ex_chipSnapshots[1], cgpt_ex_analogResult);
    }
#endif
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_buttonCount; ++cgpt_ex_i) {
        SerialWombatDebouncedInput cgpt_ex_button(cgpt_ex_buttonChip(cgpt_ex_i));
        const int16_t cgpt_ex_result = cgpt_ex_button.begin(cgpt_ex_buttons[cgpt_ex_i].cgpt_ex_pin,
            cgpt_ex_buttonDebounceMs, true, true);
        if (cgpt_ex_result < 0) return cgpt_ex_initError(
            cgpt_ex_chipSnapshots[cgpt_ex_buttons[cgpt_ex_i].cgpt_ex_chip ? 0 : 1], cgpt_ex_result);
    }
    // Let 16-sample averaging and debouncing settle before accepting input.
    // This delay is initialization-only; playback/service uses no delay().
    delay(cgpt_ex_buttonDebounceMs + 20);
    uint16_t cgpt_ex_ignoredPresses = 0;
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_buttonCount; ++cgpt_ex_i) {
        if (!cgpt_ex_readButtonChecked(cgpt_ex_i, cgpt_ex_ignoredPresses)) return cgpt_ex_initError(
            cgpt_ex_chipSnapshots[cgpt_ex_buttons[cgpt_ex_i].cgpt_ex_chip ? 0 : 1], -1);
    }
    // Flush transitions and use the sampled state. A button already held at
    // startup does not record until it is released and pressed again.
    cgpt_ex_chipSnapshots[1].cgpt_ex_initialized = true;
    return true;
}

bool cgpt_ex_hardwareRead(uint16_t cgpt_ex_samples[cgpt_ex_jointCount],
                          uint16_t &cgpt_ex_presses) {
#if cgpt_ex_enableJoysticks
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i) {
        if (!cgpt_ex_readPublicChecked(cgpt_ex_inputChip,
            cgpt_ex_jointSettings[cgpt_ex_i].cgpt_ex_analogPin, cgpt_ex_samples[cgpt_ex_i])) return false;
    }
#else
    // No reads of floating analog inputs; neutral placeholders are deterministic.
    for (uint8_t cgpt_ex_i = 0; cgpt_ex_i < cgpt_ex_jointCount; ++cgpt_ex_i)
        cgpt_ex_samples[cgpt_ex_i] = cgpt_ex_jointSettings[cgpt_ex_i].cgpt_ex_joystickCenter;
#endif
    return cgpt_ex_readButtonChecked(0, cgpt_ex_presses);
}

bool cgpt_ex_hardwareWrite(uint8_t cgpt_ex_joint, uint16_t cgpt_ex_position) {
    if (cgpt_ex_joint >= cgpt_ex_jointCount) return false;
    // Checked equivalent of Servo::write16bit()/Chip::writePublicData().
    uint8_t cgpt_ex_tx[8] = {0x82, cgpt_ex_jointSettings[cgpt_ex_joint].cgpt_ex_servoPin,
                            static_cast<uint8_t>(cgpt_ex_position),
                            static_cast<uint8_t>(cgpt_ex_position >> 8),
                            0xFF, 0x55, 0x55, 0x55};
    uint8_t cgpt_ex_rx[8] = {};
    return cgpt_ex_servoChip.sendPacket(cgpt_ex_tx, cgpt_ex_rx) >= 0;
}

bool cgpt_ex_hardwareReadEditButtons(uint16_t &cgpt_ex_previous, uint16_t &cgpt_ex_next, uint16_t &cgpt_ex_delete) {
    return cgpt_ex_readButtonChecked(1, cgpt_ex_previous) &&
        cgpt_ex_readButtonChecked(2, cgpt_ex_next) && cgpt_ex_readButtonChecked(3, cgpt_ex_delete);
}
