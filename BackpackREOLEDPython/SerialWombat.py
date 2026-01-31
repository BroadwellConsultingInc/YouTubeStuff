"""
Copyright 2020-2023 Broadwell Consulting Inc.

"Serial Wombat" is a registered trademark of Broadwell Consulting Inc. in
the United States.  See SerialWombat.com for usage guidance.

Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
"""

"""! \file SerialWombat.py
"""
import time
#from enum import IntEnum
from ArduinoFunctions import delayMicroseconds
from ArduinoFunctions import millis
from ArduinoFunctions import delay



def SW_LE16(i):
    return (bytearray([i & 0xFF, int(i/256) & 0xFF]))
def SW_LE32(i):
    return (bytearray([i & 0xFF, int(i>>8) & 0xFF, int(i>>16) & 0xFF, int(i>>24) & 0xFF]))


class SerialWombatPinState_t ():
	SW_LOW = 0,
	SW_HIGH = 1,
	SW_INPUT = 2,

class ArduinoInputOutput ():
    INPUT = 0,
    OUTPUT = 1,
    PULLUP = 2



#! \brief A list of Serial Wombat public data sources
class SerialWombatDataSource():
    SW_DATA_SOURCE_PIN_0 = 0 #!< (0) 16 bit public data provided by Pin 0
    SW_DATA_SOURCE_PIN_1 = 1 #!< (1) 16 bit public data provided by Pin 1
    SW_DATA_SOURCE_PIN_2 = 2 #!< (2) 16 bit public data provided by Pin 2
    SW_DATA_SOURCE_PIN_3 = 3 #!< (3) 16 bit public data provided by Pin 3
    SW_DATA_SOURCE_PIN_4 = 4 #!< (4) 16 bit public data provided by Pin 4
    SW_DATA_SOURCE_PIN_5 = 5 #!< (5) 16 bit public data provided by Pin 5
    SW_DATA_SOURCE_PIN_6 = 6 #!< (6) 16 bit public data provided by Pin 6
    SW_DATA_SOURCE_PIN_7 = 7 #!< (7) 16 bit public data provided by Pin 7
    SW_DATA_SOURCE_PIN_8 = 8 #!< (8) 16 bit public data provided by Pin 8
    SW_DATA_SOURCE_PIN_9 = 9 #!< (9) 16 bit public data provided by Pin 9
    SW_DATA_SOURCE_PIN_10 = 10 #!< (10) 16 bit public data provided by Pin 10
    SW_DATA_SOURCE_PIN_11 = 11 #!< (11) 16 bit public data provided by Pin 11
    SW_DATA_SOURCE_PIN_12 = 12 #!< (12) 16 bit public data provided by Pin 12
    SW_DATA_SOURCE_PIN_13 = 13 #!< (13) 16 bit public data provided by Pin 13
    SW_DATA_SOURCE_PIN_14 = 14 #!< (14) 16 bit public data provided by Pin 14
    SW_DATA_SOURCE_PIN_15 = 15 #!< (15) 16 bit public data provided by Pin 15
    SW_DATA_SOURCE_PIN_16 = 16 #!< (16) 16 bit public data provided by Pin 16
    SW_DATA_SOURCE_PIN_17 = 17 #!< (17) 16 bit public data provided by Pin 17
    SW_DATA_SOURCE_PIN_18 = 18 #!< (18) 16 bit public data provided by Pin 18
    SW_DATA_SOURCE_PIN_19 = 19 #!< (19) 16 bit public data provided by Pin 19
    #	SW_DATA_SOURCE_PIN_20 = 20
    #	SW_DATA_SOURCE_PIN_21 = 21
    #	SW_DATA_SOURCE_PIN_22 = 22
    #	SW_DATA_SOURCE_PIN_23 = 23
    #	SW_DATA_SOURCE_PIN_24 = 24
    #	SW_DATA_SOURCE_PIN_25 = 25
    #	SW_DATA_SOURCE_PIN_26 = 26
    #	SW_DATA_SOURCE_PIN_27 = 27
    #	SW_DATA_SOURCE_PIN_28 = 28
    #	SW_DATA_SOURCE_PIN_29 = 29
    #	SW_DATA_SOURCE_PIN_30 = 30
    #	SW_DATA_SOURCE_PIN_31 = 31
    #	SW_DATA_SOURCE_PIN_32 = 32
    #	SW_DATA_SOURCE_PIN_33 = 33
    #	SW_DATA_SOURCE_PIN_34 = 34
    #	SW_DATA_SOURCE_PIN_35 = 35
    #	SW_DATA_SOURCE_PIN_36 = 36
    #	SW_DATA_SOURCE_PIN_37 = 37
    #	SW_DATA_SOURCE_PIN_38 = 38
    #	SW_DATA_SOURCE_PIN_39 = 39
    #	SW_DATA_SOURCE_PIN_40 = 40
    #	SW_DATA_SOURCE_PIN_41 = 41
    #	SW_DATA_SOURCE_PIN_42 = 42
    #	SW_DATA_SOURCE_PIN_43 = 43
    #	SW_DATA_SOURCE_PIN_44 = 44
    #	SW_DATA_SOURCE_PIN_45 = 45
    #	SW_DATA_SOURCE_PIN_46 = 46
    #	SW_DATA_SOURCE_PIN_47 = 47
    #	SW_DATA_SOURCE_PIN_48 = 48
    #	SW_DATA_SOURCE_PIN_49 = 49
    #	SW_DATA_SOURCE_PIN_50 = 50
    #	SW_DATA_SOURCE_PIN_51 = 51
    #	SW_DATA_SOURCE_PIN_52 = 52
    #	SW_DATA_SOURCE_PIN_53 = 53
    #	SW_DATA_SOURCE_PIN_54 = 54
    #	SW_DATA_SOURCE_PIN_55 = 55
    #	SW_DATA_SOURCE_PIN_56 = 56
    #	SW_DATA_SOURCE_PIN_57 = 57
    #	SW_DATA_SOURCE_PIN_58 = 58
    #	SW_DATA_SOURCE_PIN_59 = 59
    #	SW_DATA_SOURCE_PIN_60 = 60
    #	SW_DATA_SOURCE_PIN_61 = 61
    #	SW_DATA_SOURCE_PIN_62 = 62
    #	SW_DATA_SOURCE_PIN_63 = 63
    SW_DATA_SOURCE_INCREMENTING_NUMBER = 65 #!< (65) An number that increments each time it is accessed.
    SW_DATA_SOURCE_1024mvCounts = 66  #!< (66) The number of ADC counts that result from a 1.024V reading
    SW_DATA_SOURCE_FRAMES_RUN_LSW = 67 #!< (67) The number of frames run since reset, least significant 16 bits
    SW_DATA_SOURCE_FRAMES_RUN_MSW = 68 #!< (68) The number of frames run since reset, most significant 16 bits
    SW_DATA_SOURCE_OVERRUN_FRAMES = 69 #!< (69) The number of frames that ran more than 1mS
    SW_DATA_SOURCE_TEMPERATURE = 70 #!< (70)The internal core temperature expressed in 100ths deg C
    SW_DATA_SOURCE_PACKETS_RECEIVED = 71 #!< (71) The nubmer of incoming command packets that have been processed since reset (rolls over at 65535)
    SW_DATA_SOURCE_ERRORS = 72 #!< (72)The number of incoming packets that have caused errors since reset (rolls over at 65535)  
    SW_DATA_SOURCE_DROPPED_FRAMES = 73 #!< (73) The number of times since reset that a frame ran so far behind that it crossed two subsequent 1ms boundaries, causing a permanent lost frame
    SW_DATA_SOURCE_SYSTEM_UTILIZATION = 74 #!< (74) A number between 0 and 65535 that scales to the average length of pin processing frames between 0 and 1000mS
    SW_DATA_SOURCE_VCC_mVOLTS = 75 #!< (75) The system source voltage in mV
    SW_DATA_SOURCE_VBG_COUNTS_VS_VREF = 76 #!< (76) A/D conversion of VBG against VRef .  Used for mfg calibration
    SW_DATA_SOURCE_LFSR = 78 #!< (78) A  Linear Feedback Shift Register that produces a Pseudo random sequence of 16 bit values
    SW_DATA_SOURCE_0x55 = 85 #!< (85) 0x55 is a reserved value for resyncing.  Returns 0x55 0x55 
    SW_DATA_SOURCE_PIN_0_MV = 100 #!< (100) Pin 0 public output expressed in mV (for analog modes only)
    SW_DATA_SOURCE_PIN_1_MV = 101 #!< (101) Pin 1 public output expressed in mV (for analog modes only)
    SW_DATA_SOURCE_PIN_2_MV = 102 #!< (102) Pin 2 public output expressed in mV (for analog modes only)
    SW_DATA_SOURCE_PIN_3_MV = 103 #!< (103) Pin 3 public output expressed in mV (for analog modes only)
    SW_DATA_SOURCE_PIN_4_MV = 104 #!< (104) Pin 4 public output expressed in mV (for analog modes only)
    #NOT ANALOG            SW_DATA_SOURCE_PIN_5_MV = 105
    #NOT ANALOG            SW_DATA_SOURCE_PIN_6_MV = 106
    #NOT ANALOG            SW_DATA_SOURCE_PIN_7_MV = 107
    #NOT ANALOG            SW_DATA_SOURCE_PIN_8_MV = 108
    #NOT ANALOG            SW_DATA_SOURCE_PIN_9_MV = 109
    #NOT ANALOG            SW_DATA_SOURCE_PIN_10_MV = 110
    #NOT ANALOG            SW_DATA_SOURCE_PIN_11_MV = 111
    #NOT ANALOG            SW_DATA_SOURCE_PIN_12_MV = 112
    #NOT ANALOG            SW_DATA_SOURCE_PIN_13_MV = 113
    #NOT ANALOG            SW_DATA_SOURCE_PIN_14_MV = 114
    #NOT ANALOG            SW_DATA_SOURCE_PIN_15_MV = 115
    SW_DATA_SOURCE_PIN_16_MV = 116 #!< (116) Pin 16 public output expressed in mV (for analog modes only)
    SW_DATA_SOURCE_PIN_17_MV = 117 #!< (117) Pin 17 public output expressed in mV (for analog modes only)
    SW_DATA_SOURCE_PIN_18_MV = 118 #!< (118) Pin 18 public output expressed in mV (for analog modes only)
    SW_DATA_SOURCE_PIN_19_MV = 119 #!< (119) Pin 19 public output expressed in mV (for analog modes only)
    SW_DATA_SOURCE_2HZ_SQUARE = 164 #!< (164) Square wave that alternates between 0 and 65535 every 256 frames
    SW_DATA_SOURCE_2HZ_SAW = 165#!< (165) Sawtooth wave that goes from  0 to 65535 to 0 every  512 frames
    #            SW_DATA_SOURCE_2HZ_SIN = 166
    SW_DATA_SOURCE_1HZ_SQUARE = 167#!< (167) Square wave that alternates between 0 and 65535 every 512 frames
    SW_DATA_SOURCE_1HZ_SAW = 168#!<  (168) Sawtooth wave that goes from  0 to 65535 to 0 every  1024 frames
    #           SW_DATA_SOURCE_1HZ_SIN = 169
    SW_DATA_SOURCE_2SEC_SQUARE = 170#!< (170)Square wave that alternates between 0 and 65535 every 1024 frames
    SW_DATA_SOURCE_2SEC_SAW = 171#!< (171)Sawtooth wave that goes from  0 to 65535 to 0 every  2048 frames
    #          SW_DATA_SOURCE_2SEC_SIN = 172
    SW_DATA_SOURCE_8SEC_SQUARE = 173#!< (173)Square wave that alternates between 0 and 65535 every 4096 frames
    SW_DATA_SOURCE_8SEC_SAW = 174#!< (174)Sawtooth wave that goes from  0 to 65535 to 0 every  8192 frames
    #         SW_DATA_SOURCE_8SEC_SIN = 175
    SW_DATA_SOURCE_65SEC_SQUARE = 176#!< (176) Square wave that alternates between 0 and 65535 every 32768 frames
    SW_DATA_SOURCE_65SEC_SAW = 177#!< (177 )Sawtooth wave that goes from  0 to 65535 to 0 every  65536 frames
    #        SW_DATA_SOURCE_65SEC_SIN = 178
    SW_DATA_SOURCE_NONE = 255#!< (255 ) Used to mean "No Source Selected"


class SerialWombatCommands():
    CMD_ECHO =ord('!') #!< ('!')
    CMD_READ_BUFFER_ASCII = ord('G')#!< ('G')
    CMD_ASCII_SET_PIN =ord('P') #!< ('P')
    CMD_RESET = ord('R') #!< ('R')
    CMD_SET_BUFFER_ASCII = ord('S')#!< ('S')
    CMD_RESYNC = ord('U')#!< ('U')
    CMD_VERSION = ord('V')#!< ('V')
    CMD_SUPPLYVOLTAGE = ord('v')#!< ('v')
    COMMAND_BINARY_READ_PIN_BUFFFER = 0x81 #!< (0x81)
    COMMAND_BINARY_SET_PIN_BUFFFER = 0x82 #!< (0x82)
    COMMAND_BINARY_READ_USER_BUFFER = 0x83 #!< (0x83)
    COMMAND_BINARY_WRITE_USER_BUFFER = 0x84 #!< (0x84)
    COMMAND_BINARY_WRITE_USER_BUFFER_CONTINUE = 0x85 #!< (0x85)
    COMMAND_BINARY_QUEUE_INITIALIZE = 0x90 #!< (0x90)
    COMMAND_BINARY_QUEUE_ADD_BYTES = 0x91 #!< (0x91)
    COMMAND_BINARY_QUEUE_ADD_7BYTES = 0x92 #!< (0x92)
    COMMAND_BINARY_QUEUE_READ_BYTES = 0x93 #!< (0x93)
    COMMAND_BINARY_QUEUE_INFORMATION = 0x94 #!< (0x94)
    COMMAND_BINARY_CONFIGURE = 0x9F #!< (0x9F)
    COMMAND_BINARY_READ_RAM = 0xA0 #!< (0xA0)
    COMMAND_BINARY_READ_FLASH = 0xA1 #!< (0xA1)
    COMMAND_BINARY_READ_EEPROM = 0xA2 #!< (0xA2)
    COMMAND_BINARY_WRITE_RAM = 0xA3 #!< (0xA3)
    COMMAND_BINARY_WRITE_FLASH = 0xA4 #!< (0xA4)
    COMMAND_CALIBRATE_ANALOG = 0xA5 #!< (0xA5)
    COMMAND_ENABLE_2ND_UART = 0xA6 #!< (0xA6)
    COMMAND_READ_LAST_ERROR_PACKET = 0xA7 #!< (0xA7)
    COMMAND_UART0_TX_7BYTES = 0xB0 #!< (0xB0)
    COMMAND_UART0_RX_7BYTES = 0xB1 #!< (0xB1)
    COMMAND_UART1_TX_7BYTES = 0xB2 #!< (0xB2)
    COMMAND_UART1_RX_7BYTES = 0xB3 #!< (0xB3)
    COMMAND_BINARY_TEST_SEQUENCE = 0xB4 #!< (0xB4)
    COMMAND_BINARY_RW_PIN_MEMORY = 0xB5 #!< (0xB5)
    COMMAND_CAPTURE_STARTUP_SEQUENCE = 0xB6 #!< (0xB6)
    COMMAND_ADJUST_FREQUENCY = 0xB7 #!< (0xB7)
    CONFIGURE_PIN_MODE0 = 200 #!< (200)
    CONFIGURE_PIN_MODE1 = 201 #!< (201)
    CONFIGURE_PIN_MODE2 = 202 #!< (202)
    CONFIGURE_PIN_MODE3 = 203 #!< (203)
    CONFIGURE_PIN_MODE4 = 204 #!< (204)
    CONFIGURE_PIN_MODE5 = 205 #!< (205)
    CONFIGURE_PIN_MODE6 = 206 #!< (206)
    CONFIGURE_PIN_MODE7 = 207 #!< (207)
    CONFIGURE_PIN_MODE8 = 208 #!< (208)
    CONFIGURE_PIN_MODE9 = 209 #!< (209)
    CONFIGURE_PIN_MODE10 = 210 #!< (210)
    CONFIGURE_PIN_OUTPUTSCALE = 210 #!< (210)
    CONFIGURE_PIN_MODE_DISABLE = 219 #!< (219)
    CONFIGURE_PIN_INPUTPROCESS = 211 #!< (211)
    CONFIGURE_PIN_MODE_HW_0 = 220 #!< (220)
    CONFIGURE_CHANNEL_MODE_HW_1 = 221 #!< (221)
    CONFIGURE_CHANNEL_MODE_HW_2 = 222 #!< (222)
    CONFIGURE_CHANNEL_MODE_HW_3 = 223 #!< (223)

class SerialWombatPinMode_t():
    PIN_MODE_DIGITALIO = 0 #!< (0)
    PIN_MODE_CONTROLLED = 1 #!< (1)
    PIN_MODE_ANALOGINPUT = 2 #!< (2)
    PIN_MODE_SERVO = 3 #!< (3)
    PIN_MODE_THROUGHPUT_CONSUMER = 4 #!< (4)
    PIN_MODE_QUADRATUREENCODER = 5 #!< (5)
    PIN_MODE_HBRIDGE = 6 #!< (6)
    PIN_MODE_WATCHDOG = 7 #!< (7)
    PIN_MODE_PROTECTED_OUTPUT = 8 #!< (8)
    PIN_MODE_DEBOUNCE = 10 #!< (10)
    PIN_MODE_TM1637 = 11 #!< (11)
    PIN_MODE_WS2812 = 12 #!< (12)
    PIN_MODE_SW_UART = 13 #!< (13)
    PIN_MODE_INPUT_PROCESSOR = 14 #!< (14)
    PIN_MODE_MATRIX_KEYPAD = 15 #!< (15)
    PIN_MODE_PWM = 16 #!< (16)
    PIN_MODE_UART_RX_TX = 17 #!< (17)  
    PIN_MODE_PULSETIMER = 18 #!< (18)
    PIN_MODE_FRAME_TIMER = 21 #!< (21)
    PIN_MODE_SW18AB_CAPTOUCH = 22 #!< (22)
    PIN_MODE_UART1_RX_TX = 23 #!< (23)
    PIN_MODE_RESISTANCEINPUT = 24 #!< (24)
    PIN_MODE_PULSE_ON_CHANGE = 25 #!< (25)
    PIN_MODE_HS_SERVO = 26 #!< (26)
    PIN_MODE_ULTRASONIC_DISTANCE = 27 #!< (27)
    PIN_MODE_LIQUIDCRYSTAL = 28 #!< (28)
    PIN_MODE_HS_CLOCK = 29 #! < (29)
    PIN_MODE_HS_COUNTER = 30 #!< (30)
    PIN_MODE_VGA = 31 #!<(31)
    PIN_MODE_PS2KEYBOARD = 32 #!<(32)
    PIN_MODE_QUEUED_OUTPUT = 34 #!< (34)
    PIN_MODE_UNKNOWN = 255 #!< (0xFF)


"""! \brief Class for a Serial Wombat chip.  Each Serial Wombat chip on a project should have its own instance.

This class describes the capabilties of a Serial Wombat Chip that are not Pin Mode functionalities

"""
class SerialWombatChip:
#private:
    def __init__(self):
        self.WOMBAT_MAXIMUM_PINS = 20
        self.version = [0,0,0,0,0,0,0,0]
        #HardwareSerial * Serial = NULL
        #TwoWire* i2cInterface = NULL;

        self._pinmode = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0] # includes pullup 0 = input, 1= output, 2 = input w/ pullup
        self._pullDown = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
        self._openDrain = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
        self._highLow = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]
        self._asleep = False
        """!
        Stores the last value retreived by readSupplyVoltage_mV().  Used by SerialWombatAnalogInput 
        class to calculate mV outputs from retreived A/D counts.
        Don't access this member, as it may become private and SerialWombatAnalog input be made
        a friend of SerialWombat in the future.  Call readSupplyVoltage_mV instead.
            """
        self._supplyVoltagemV = 3300
        self.deviceRevision = 0
        #Incremented every time a communication or command error is detected.  
        self.errorCount = 0
        self.inBoot = False
        self.lastErrorCode = 0
        self.model = [0,0,0,0]
        self.fwVersion = [0,0,0,0]
        #! @brief The I2C address of the SerialWombatChip instance
        self.address = 0
        self.sendReadyTime = 0
        self.uniqueIdentifier = bytearray(16)

    def configureDigitalPin(self,pin, highLow):
        tx = [200,pin,0,0,0,0,0,0x55]
        if (self._pinmode[pin] == 0): #input
            tx[3] = 2 #input
        elif (self._pinmode[pin] == 1): #output
            if (highLow == 0):  #LOW
                tx[3] = 0  #low
            elif (highLow == 1):
                tx[3] = 1
            else:
                return
        elif (self._pinmode[pin] == 2 ): #pullup
            tx[3] = 2 #input
            tx[4] = 1 #Pullup on 
        else:
            return
        tx[6] = self._openDrain[pin]
        tx[5] = self._pullDown[pin]
        self.sendPacket(tx)
        
        self.sendReadyTime = 0

    def initialize(self):
        lastErrorCode = 0
        self.readVersion()
        self.readSupplyVoltage_mV()
        self.readUniqueIdentifier()
        self.readDeviceIdentifier()
        return(lastErrorCode)

    def readUniqueIdentifier(self):
        uniqueIdentifierLength = 0
        if (self.version[0] == 'S' and self.version[1] == '0' and self.version[2] == '4'):
                    #16F15214
            for address in range(0x8100,0x8109,1):
        
                data = self.readFlashAddress(address)
                self.uniqueIdentifier[uniqueIdentifierLength] = data & 0xFF
                uniqueIdentifierLength += 1
                """ Always zero... leave out
                uniqueIdentifier[uniqueIdentifierLength] = (uint8_t)(data>>8);
                ++uniqueIdentifierLength;
                """
        elif (self.isSW18()):
            for address  in range (0x801600, 0x80160A, 2):
                data = self.readFlashAddress(address)
                self.uniqueIdentifier[uniqueIdentifierLength] = data & 0xFF
                uniqueIdentifierLength += 1
                self.uniqueIdentifier[uniqueIdentifierLength] = (data >> 8) & 0xFF
                uniqueIdentifierLength += 1
                self.uniqueIdentifier[uniqueIdentifierLength] = (data >> 16) & 0xFF
                uniqueIdentifierLength += 1



    def readDeviceIdentifier(self):
        if (self.version[0] == 'S' and self.version[1] == '0' and self.version[2] == '4'):
             #16F15214
                    
            data = self.readFlashAddress(0x8006)
            self.deviceIdentifier = data & 0xFFFF
            data = self.readFlashAddress(0x8005)
            self.deviceRevision = data & 0xFFFF
        elif (self.isSW18()):
            data = self.readFlashAddress(0xFF0000)
            self.deviceIdentifier = data & 0xFFFF
            data = self.readFlashAddress(0xFF0002)
            self.deviceRevision = data & 0xF

    def returnErrorCode(self,rx):
        out = rx[1] - ord('0')
        out *=10;
        out += rx[2] - ord('0')
        out *=10;
        out += rx[3] - ord('0')
        out *=10;
        out += rx[4] - ord('0')
        out *=10;
        out += rx[5] - ord('0')
        return (out)


        return 8

    def sendPacketToHardware (self,tx):
        return (self.hardwareSend(tx))
    
    def sendPacket(self, tx,  retryIfEchoDoesntMatch = False, startBytesToMatch = 1,  endBytesToMatch = 0):
        retry = 4  #TODO self.communicationErrorRetries
        if (self._asleep):
            self._asleep = False;    
            txw = [ 0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21 ] 
            self.sendPacketToHardware(txw)
            delayMicroseconds(200)
            txu = [ 0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55, ] 
            self.sendPacketToHardware(txu)
        if (self.sendReadyTime != 0):
            currentTime = millis()
            if (currentTime < self.sendReadyTime):
                delay(self.sendReadyTime - currentTime)
			
            self.sendReadyTime = 0
            self.initialize()
           

        if (not retryIfEchoDoesntMatch):
            retry = 1
        
        result = 0
        while (retry > 0):
            result,rx = self.sendReceivePacketHardware(tx)
            if (rx[0] == 'E'):
                return (-1 * self.returnErrorCode(rx),rx)

            success = True
            for i in range(0,startBytesToMatch):
                if ( tx[i] != rx[i]):
                    success = False

            for i in range(8- endBytesToMatch, 8):
                if (tx[i] != rx[i]):
                    success = False
            if (success):
                return (8,rx)
            retry -= 1
            delayMicroseconds(100)
            
        return(result, rx)





    
    def sendReceivePacketHardware(self,tx):
                return 8,[0x55,0x55,0x55,0x55,0x55,0x55,0x55]



    """!
            \brief initialize a Serial Wombat chip to use a Serial Interface.
            
            The reset parameter determines if the Serial Wombat chip is reset
            prior to other initialization operations.  If false,
            then any prior pin modes and configurations may still be in
            place.
            The Serial Wombat chips's source 
            voltage is then read as well as its version.
            
            \param reset Whether or not to reset the Serial Wombat chip via command as the first initialization operation
    """
    def begin(self,reset = True):
        if (reset):
            self.hardwareReset();
            self._sendReadyTime = millis() + 1000
            delay(1000)
            self.initialize()
            return 1
        else:
            self._sendReadyTime = 0
            return self.initialize()


    """!
	\brief Request version string (combined model and firmware) and return pointer to it
	
	This queries the Serial Wombat chip for the 7 characters:   product line (1 character)
	Model (3 characters) and firmware version (3 characters)
	This is stored in a string in the Serial Wombat object.  A pointer to this string
	is returned.
    """
    def readVersion(self):
        count,rx=self.sendPacket( (bytearray("VUUUUUUU",'utf8')))
        if (count >= 0):
            self.version = rx[1:8]
            self.model = rx[1:4]
            self.fwVersion = rx[5:8]

        """!
	@brief Request version as a uint32
	
	This queries the Serial Wombat chip for its version information, and returns the
	firmware version as a uint32 0x0XYZ where X,Y,and Z represent firmwre version X.Y.Z
        """
    def readVersion_uint32(self) :
        self.readVersion()
        return (((self.fwVersion[0]) << 16) |((self.fwVersion[1]) << 8) |	self.fwVersion[2])

    """!
	\brief Read the 16 Bit public data associated with a Serial Wombat Pin Mode 
	
	Reads and returns the 16 bit value associated with a Serial Wombat Pin Mode.
	Additionally, values of 65 and higher have special meanings.  See
	Serial Wombat firmware documentation for details.
	\return 16 bit public data for pin specified
	\param pin The pin (or special meaning value) for which to retreive data
    """
    def readPublicData(self,pin):
        tx = [0x81,pin,255,255,0x55,0x55,0x55,0x55]
        count,rx = self.sendPacket(tx)
        return (rx[2]+ rx[3] * 256)

    """!
	@brief Write a 16 bit value to a Serial Wombat pin Mode
	@param pin The pin number to which to write
	@param value The 16 bit value to write
    """
    def writePublicData(self,pin, value):
        tx = [0x82, pin, value & 0xFF, value // 256, 255, 0x55,0x55,0x55]
        count,rx = self.sendPacket(tx)
        return (rx[2] + rx[3] * 256)

    """!
	\brief Measure the Serial Wombat chip's Supply voltage
	
	Causes the Serial Wombat chip to measure the counts for the 
	internal reference voltage.  The Arduino library
	then converts these counts to a Source votlage in mV
	
	\return The Serial Wombat chip's source voltage in mV
    """
    def readSupplyVoltage_mV(self):
        #TODO add support for SW18AB
        if (self.isSW18()):
            self._supplyVoltagemV = self.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_VCC_mVOLTS);
            return(self._supplyVoltagemV)
        counts = self.readPublicData(66)
        if (counts > 0):
            mv = 1024 * 65536 // counts
            self._supplyVoltagemV = mv
        else:
            self._supplyVoltagemV = 0

        return (self._supplyVoltagemV)

    """!
	\brief Measure the Serial Wombat chip's internal temperature
	
	This command is only supported by the SerialWombat 18 Series.
	The Arduino library will return 25 deg. C for other models
	
	This value is low accuracy unless a calibration has been performed
	
	\return The Serial Wombat chip's temperature in 100ths deg C
    """
    def readTemperature_100thsDegC(self):
        if (self.isSW18()):
		
            result = self.readPublicData(70)
            if (result >= 32768):
			
                result = result - 65536
			
            return (result)
		
        else:
		
            return 2500
		
    """!
	\brief Send a reset command to the Serial Wombat chip
	
	Sends a reset command to the Serial Wombat chip.  The calling function
	should wait 500mS before sending additional commands.
    """
    def hardwareReset(self):
       self.sendPacketToHardware((bytearray("ReSeT!#*",'utf8')))#, encoding = 'utf8')))

    """!
	\brief Set a pin to INPUT or OUTPUT, with options for pull Ups and open Drain settings
	
	\param pin The Serial Wombat pin to set
	\param mode Valid values are INPUT, OUTPUT or INPUT_PULLUP as defined by arduino.  Do not use SW_INPUT, SW_HIGH or SW_LOW here, as these have different meanings
	\param pullDown  If True, a weak pull down will be enabled on this pin (No effect on SW4A/SW4B)
	\param openDrain If True, output becomes openDrain output rather than push / pull
    """
    def pinMode(self,pin,mode,pullDown=False,openDrain=False):
        if (pin >= self.WOMBAT_MAXIMUM_PINS):
            return -32767
        self._pullDown[pin] = pullDown
        self._openDrain[pin] = openDrain
        self._pinmode[pin] = mode
        self.configureDigitalPin(pin,mode)

    """!
    @brief Set an output pin High or Low
    
    Before calling this function, the pin should be configured as an input or output with pinMode()
    @param pin The Serial Wombat pin to set.  Valid values for SW4A: 0-3  SW4B: 1-3
    @param val  Valid values are HIGH or LOW not use SW_INPUT, SW_HIGH or SW_LOW here, as these have different meanings
    """
    def digitalWrite(self, pin,  val):
        self.configureDigitalPin(pin, val)
	
    """
    @brief Reads the state of a Pin
	
    @return Returns LOW if pin is low or public data is 0.  Returns HIGH if pin is high or public data is > 0
    """
    def digitalRead(self, pin) :
            if (self.readPublicData(pin) > 0):
                    return (1)
            else:
                    return (0)
    
    """
    @brief Configures pin as analog input and does an immediate A/D conversion.  
    
    This function is compatible with the Arduino Uno analogRead function.  
    It does not make use of advanced Serial Wombat chip's functionality such as averaging and
    filtering.  Consider declaring a SerialWombatAnalogInput instead.
    @param pin The Serial Wombat pin to set.  Valid values for SW4A: 0-3  SW4B: 1-3
    @return An Analog to Digital conversion ranging from 0 to 1023 (10-bit)
    """
    def analogRead(self,pin):
        tx = [ 200,pin,SerialWombatPinMode_t.PIN_MODE_ANALOGINPUT,0,0,0,0,0 ]
        self.sendPacket(tx)
        return (self.readPublicData(pin) >> 6) # Scale from 16 bit value to 10 bit value.

    """
    @brief Set a pin to PWM output
    
    This function is compatible with the Arduino Uno analogWrite function, but will
    output a PWM with a different frequency.
    Consider declaring a SerialWombatPWM instead.  It has higher resolution and
    the ability to choose frequency.
    
    @param pin The Serial Wombat pin to set.  Valid values for SW4A: 0-3  SW4B: 1-3
    @param val A value from 0 (always low) to 255(always high) for the PWM duty cycle
    """
    def analogWrite(self, pin,  val):
            dutyCycleLow = 0
            if (val == 255):
                    dutyCycleLow = 255;
            tx = [SerialWombatCommands.CONFIGURE_PIN_MODE0,pin,SerialWombatPinMode_t.PIN_MODE_PWM,pin,dutyCycleLow,val,0,0x55 ]
            self.sendPacket(tx)


    """!
    @brief Send a version request to the Serial Wombat chip
    
    This function queries the Serial Wombat chip for its model and version
    and stores the result in the public members model and fwVersion
    as zero terminated strings.  Returns true if the response is
    likely a proper version response and false otherwise.
    
    @return TRUE if response was likely a valid version, FALSE otherwise
    """
    def queryVersion(self):

        tx = [ord('V'),0x55,0x55,0x55,0x55,0x55,0x55,0x55]
        
        count,rx = self.sendPacket(tx)
        if (rx[0] == ord('V') and rx[1] == ord('S') and rx[1] ==  ord('B')):

            self.model[0] = rx[1]
            self.model[1] = rx[2]
            self.model[2] = rx[3]
            self.model[3] = 0
            self.fwVersion[0] = rx[5]
            self.fwVersion[1] = rx[6]
            self.fwVersion[2] = rx[7]
            self.fwVersion[3] = 0

            self.inBoot = (rx[1] == ord('B'))
            return (True)
        
        return (False)


    """!
    !brief Get the number of 1mS frames that have been executed since Serial Wombat chip reset
    
    This value should be roughly equal to the mS since reset.  It will vary based on the Serial Wombat chip's
    internal oscillator variation, and may run slow if Overflow frames are occuring.
    """
    def readFramesExecuted(self):
        tx = [ 0x81,67,68,0x55,0x55,0x55,0x55,0x55 ]
        result, rx = self.sendPacket(tx)
        returnval = rx[2] + ((rx[3]) << 8) + ((rx[4]) << 16) + ((rx[5]) << 24)
        return (returnval)

    """!
    @brief Get the number of times an overflow Frame has occured.
    
    This value increments each time the Serial Wombat firmware determines it is time to start a new 1 mS frame,
    but the previous frame is still executing.  Indicates processor loading over 100% of real-time.  Overflows
    back to 0 when incremented from 65535.
    """

    def readOverflowFrames(self):
        return self.readPublicData(69)



    """!
    @brief Jump to Bootloader and wait for a UART download of new firmware
    
    This function causes a reset of the Serial Wombat chip and causes it to remain
    in the bootloader until a power-cycle occurs.  This allows loading new
    firmware via a UART connection to the bottom two pins (DIP pins 4 (RX) and 5(TX))
    on the SW4A/SW4B.  When jumping to boot the TX pin will go high.  All other 
    communication or functional pins will become inputs (i.e. PWMS, etc will stop).
    """
    def jumpToBoot(self):
        tx = bytes("BoOtLoAd",'utf-8')
        self.sendPacket(tx)

    """!
    @brief Read Address from RAM based on 16 bit address
    
    Most Arduino users should not need this command.
    
    This command can be used to read variables and registers within the Serial Wombat Chip
    Note that reading registers may have unintended side effects.  See the microcontroller datasheet
    for details.
    
    Note that Note that the PIC16F15214 used in the SW4A and SW4B chips
    is a Microchip Enhanced Mid-Range chip with both a banked RAM area and a Linear RAM area at an offset address.
    See the datasheet for details.  It's wierd to people who are unfamilliar with it.  The same location
    can have two different addresses.
    
    Addresses are not validated to be available in a given chip's address range.
    
    @param address  A 16-bit address pointing to a location in the Serial Wombat Chip's memory map
    
    @return An 8 bit value returned from the Serial Wombat chip.
    """
    def readRamAddress(self,address):
        tx = bytearray([ 0xA0]) + SW_LE16(address) + bytearray([0x55,0x55,0x55,0x55,0x55])
        result,rx = self.sendPacket(tx)
        return(rx[3])

    """!
    @brief Write byte to Address in RAM based on 16 bit address
    
    Most Arduino users should not need this command.
    
    This command can be used to write variables and registers within the Serial Wombat Chip
    Note that write registers may have unintended side effects.  See the microcontroller datasheet
    for details.
    
    Note that Note that the PIC16F15214 used in the SW4A and SW4B chips
    is a Microchip Enhanced Mid-Range chip with both a banked RAM area and a Linear RAM area at an offset address.
    See the datasheet for details.  It's wierd to people who are unfamilliar with it.  The same location
    can have two different addresses.
    
    Addresses are not validated to be available in a given chip's address range.
    
    @param address  A 16-bit address pointing to a location in the Serial Wombat Chip's memory map
    @param value An 8 bit value to be written to RAM
    """

    def writeRamAddress(self, address, value):
        tx = bytearray([ 0xA3]) + SW_LE16(address) + bytearray([0,0,value,0x55,0x55])
        result,rx = self.sendPacket(tx);
        return result

    """!
    @brief Read Address from Flash based on 32 bit address
    
    Most Arduino users should not need this command.
    
    This command can be used to read flash locations within the Serial Wombat Chip
    
    
    Addresses are not validated to be available in a given chip's address range.
    
    @param address  A 32-bit address pointing to a location in the Serial Wombat Chip's memory map
    
    @return An 32 bit value returned from the Serial Wombat chip.  32 bits are used to accomodate different chips.  The SW18 series has a 24 bit flash word, whereas the SW4A and SW4B have a 14 bit word.
    """

    def readFlashAddress(self,address):
        tx = bytearray([ 0xA1]) + SW_LE32(address) + bytearray([0x55,0x55,0x55])
        result,rx = self.sendPacket(tx)
        if (result <= 0):
            return (0)
        return((rx[4]) + ((rx[5]) <<8) + ((rx[6]) <<16) + ((rx[7]) <<24))

    """!
    @brief Shuts down most functions of the Serial Wombat chip reducing power consumption
    
    This command stops the Serial Wombat chip's internal clock, greatly reducing power consumption.
    The host is responsible for configuring outputs to a safe state prior to calling sleep.
    
    @warning This command does not cause any sort of shutdown routine to run.  The chip just stops.
    Outputs, including PWM, Servo and Protected Outputs, may retain their logic levels 
    at the moment the sleep command is processed.  In other words, they may stay high or low as long as the chip is in sleep.
    """
    def sleep(self):
        tx = bytes('SlEeP!#*','utf-8')
        self.sendPacket(tx)
        self._asleep = True

    #! \brief Called to send a dummy packet to the Serial Wombat chip to wake it from sleep and ready it for other commands
    def wake(self):
        tx = bytes('!!!!!!!!','utf-8')
        self.sendPacket(tx)

    #! \brief Returns true if the instance received a model number corresponding to the Serial Wombat 18 series of chips at begin
    def isSW18(self):
        return ( self.model[1] == 0x31 and self.model[2] == 0x38)

    #! \brief Erases a page in flash.  Intended for use with the Bootloader, not by end users outside of bootloading sketch
    def eraseFlashPage(self, address):
        tx = bytearray([ SerialWombatCommands.COMMAND_BINARY_WRITE_FLASH, 0] ) + SW_LE32(address) + bytearray([ 0x55,0x55])
        result,rx = self.sendPacket(tx)
        return result


    #! \brief Writes a row in flash.  Intended for use with the Bootloader, not by end users outside of bootloading sketc
    def writeFlashRow(self, address):
        tx = bytearray([ SerialWombatCommands.COMMAND_BINARY_WRITE_FLASH, 1] ) + SW_LE32(address) + bytearray([ 0x55,0x55])
        result,rx = self.sendPacket(tx)
        return result

    """!
    @brief Set a pin to be a throughput monitoring pin. 
    
    This pin goes high when pin processing begins in each 1mS frame, and goes low
    after pin processing is complete.  This allows the CPU utilization of the Serial
    Wombat chip to be measured using a logic analyzer.  This function can only be applied
    to one pin, and is only disabled by resetting the chip.  This function is supported on
    the SW18AB chip.  It is not supported on the SW4 series of chips.
    """
    def setThroughputPin(self, pin):
        tx = [ SerialWombatCommands.CONFIGURE_PIN_MODE0,pin,SerialWombatPinMode_t.PIN_MODE_FRAME_TIMER,0x55,0x55,0x55,0x55,0x55 ]
        result,rx = self.sendPacket(tx)
        return result

    """!
	@brief Write bytes to the User Memory Buffer in the Serial Wombat chip
	@param index The index into the User Buffer array of bytes where the data should be loaded
	@param buffer a pointer to an array of bytes to be loaded into the User Buffer array
	@param number of bytes to load
	@return Number of bytes written or error code.
    """
    def writeUserBuffer(self,address,buf,count):
        bytesToSend = 0
        bytesSent = 0
        if (count == 0):
            return (0)
        #send first packet of up to 4 bytes
        if (count < 4):
            bytesToSend = count
            count = 0
        else:
            bytesToSend = 4
            count -= 4

        tx = [0x84,address & 0xFF, int(address / 256), bytesToSend, 0x55,0x55,0x55,0x55]
        for i in range (bytesToSend):
            tx[4+i] = buf[i]
        result,rx = self.sendPacket(tx)
        if (result < 0):
            return (count)
        bytesSent = bytesToSend

        while (count >= 7):
            tx = [0x85,0x55,0x55,0x55,0x55,0x55,0x55,0x55]
            for i in range(7):
                tx[i+1] = buf[bytesSent + i]
            result,rx = self.sendPacket(tx)           
            if (result < 0):
                return count
            count -= 7
            bytesSent += 7

        while (count > 0):
            bytesToSend = 4
            if (count < 4):
                bytesToSend = count
                count = 0
            else:
                count -=4
            a = address + bytesSent
            tx = [0x84,a & 0xFF, a//256,bytesToSend,0x55,0x55,0x55,0x55]
            for i in range(bytesToSend):
                tx[4+i] = buf[i + bytesSent]
            result,rx = self.sendPacket(tx)
            if (result < 0):
                return(count)
            bytesSent += bytesToSend

        return bytesSent

    """!
    @brief Read bytes from the User Memory Buffer in the Serial Wombat chip
    @param index The index into the User Buffer array of bytes from which data should be read 
    @param number of bytes to read
    @return A bytearray containing the read bytes
    """
    def readUserBuffer(self, index, count):
            buffer = bytearray()
            while (len(buffer) < count):
            
                    tx = bytearray([ SerialWombatCommands.COMMAND_BINARY_READ_USER_BUFFER])+ SW_LE16(index)+ bytearray([0x55,0x55,0x55,0x55,0x55])
                    result, rx = self.sendPacket(tx)
                    if (result >= 0):
                        for i in range(1,8):
                                    buffer+= bytearray(rx[i])
                                    if (len(buffer) >= count):
                                            break
                    else:
                            return (buffer)
            
            return (buffer)
    """!
    @brief Enable UART command interface in addition to I2C (SW18AB Only)
    @param 2nd communication interface is enabled
    
    @return 0 or positive for success or negative error code
    """
    def enable2ndCommandInterface(self, enabled = True):
            tx = [ 0xA6,0,0xB2, 0xA5, 0x61, 0x73, 0xF8 ,0xA2 ]
            if (enabled):
                    tx[1] = 1
            result,rx = self.sendPacket(tx)
            return (result)

    """!
    @brief Start capture of startup commands (SW18AB Only)
    @return 0 or positive for success or negative error code
    
    This command begins startup command capture.  On the SW18AB up to 256 commands can be captured
    This command is followed by a stopStartupCommandCapture command which stops command capture
    and a writeStartupCommandCapture command which writes the captured commands to flash.
    Calling this command discards any prior unwritten capture commands.
    """
    def startStartupCommandCapture(self):
        tx = { 0xB3,0,'C', 'A', 'P','T','U','R' };
        result,rx =  self.sendPacket(tx)
        return result

    """!
    @brief Stop capture of startup commands (SW18AB Only)
    @return 0 or positive for success or negative error code
    """
    def stopStartupCommandCapture(self):
        tx = [ 0xB3,1,'C', 'A', 'P','T','U','R' ]
        result,rx = self.sendPacket(tx)
        return result


    """!
    @brief Write captured startup commands to flash (SW18AB Only)
    @return 0 or positive for success or negative error code
    
    This command writes the commands captured between startStartupCommandCapture and stopStartupCommandCapture 
    to flash.  These commands will be executed each time the chip is reset in the future.
    The command will return success but do nothing if the data to be stored matches the data already stored in
    flash.  This allows the  startStartupCommandCapture / stopStartupCommandCapture / writeStartupCommandCapture
    sequence to be placed around the initialization code of a sketch without concern that flash write endurance
    will be an issue (assuming the exact same initalization sequence occurs each time).  
    """
    def writeStartupCommandCapture(self):
        tx = [ 0xB3,2,'C', 'A', 'P','T','U','R' ]
        result,rx = self.sendPacket(tx);
        return result

    """!
    @brief Set a pin to be a frame timer for system utilization (SW18AB Only)
    @return 0 or positive for success or negative error code
    
    This command configures a Serial Wombat 18AB Pin to be a frame timer.  This frame goes high at
    the beginning of pin processing, and low after all pins have been serviced.  The duty cycle of 
    this pin is an indicator of the Serial Wombat Chip's CPU utilization.  This pin has a frequency
    of 1kHz corresponding to the 1000 frames per second executive.  Most multimeters will filter this
    pin to a voltage, so the CPU utilization can be seen as a fraction of the system voltage.
    Only one pin can be the Frame Timer pin at a time.
    """
    def writeFrameTimerPin(self, pin):
        tx = [ 0xC8 ,pin,
               SerialWombatPinMode_t.PIN_MODE_FRAME_TIMER,
               0x55,0x55,0x55,0x55,0x55 ]
        result,rx = self.sendPacket(tx)
        return result

    """
/*!
	\brief Search the I2C Bus addresses 0x68 to 0x6F for I2C devices, and test to see if they respond to Serial Wombat version commands.  Returns first address that responds properly or 0 if none found
	
	\param keepTrying if True, go into a loop and do not exit until a Serial Wombat Chip is found
	
	\return I2C address of first found Seirla Wombat chip or 0 if none found
*/
	static uint8_t find(bool keepTrying = false)
	{
		do
		{
			for (int i2cAddress = 0x68; i2cAddress <= 0x6F; ++i2cAddress)
			{
				Wire.beginTransmission(i2cAddress);
				int error = Wire.endTransmission();


				if (error == 0)
				{
					uint8_t tx[8] = { 'V',0x55,0x55,0x55,0x55,0x55,0x55,0x55 };
					uint8_t rx[8];
					Wire.beginTransmission(i2cAddress);
					Wire.write(tx, 8);
					Wire.endTransmission();
					Wire.requestFrom((uint8_t)i2cAddress, (uint8_t)8);

					int count = 0;
					while (Wire.available() && count < 8)
					{
						rx[count] = Wire.read();
						++count;
					}
					if (count == 8)
					{
						if (rx[0] == 'V' && rx[1] == 'S')
						{
							return(i2cAddress); // Found one.
						}
					}
				}
			}
			delay(0);
		}while (keepTrying);
				return(0);  // Didn't find one.
	}
"""
    """!
    @brief Returns the last Serial Wombat command that produced a protocol error
    
    @return Returns the last error code and rronious command
    @param cmd pointer to a uint8_t [8] array into which the error command will be copied
    """
    def readLastErrorCommand(self):
        tx = [ SerialWombatCommands.COMMAND_READ_LAST_ERROR_PACKET, 0,0x55,0x55,0x55,0x55,0x55,0x55]
        result,rx = self.sendPacket(tx)
        cmd = bytearray()
        if (result >= 0):
            for  i in range(1,8):
                cmd.add(rx[i])
        else:
            return (self.lastErrorCode,cmd)
        tx[1] = 7
        result,rx = self.sendPacket(tx)
        if (result >= 0):
            cmd.add( rx[1])
        return(self.lastErrorCode.cmd)


    def registerErrorHandler(self, handler):
        self.errorHandler = handler
	


	#  @brief How many times to retry a packet if communcation bus (such as I2C) error
    communicationErrorRetries = 5

    def echo(self, data,  count = 7):
        tx = bytes("!UUUUUUU",'utf-8')
        for i in range(count):
            tx[i + 1] = data[i]
        result, rx = self.sendPacket(tx)
        return result



	

    def readBirthday(self):
        if (self.isSW18()):
            birthday = (self.readFlashAddress(0x2A00C) >> 8) & 0xFF
            birthday *= 100
            birthday += (self.readFlashAddress(0x2A00C)) & 0xFF
            birthday *= 100
            birthday += self.readFlashAddress(0x2A00E) & 0xFF
            birthday *= 100
            birthday += self.readFlashAddress(0x2A010) & 0xFF
            return (birthday)
        return 0

    def readBrand(self):
        data = bytearray()
        if (self.isSW18()):
            for i in range(32):
                val = self.readFlashAddress(0x2A020 + i * 2) ;
                if ((val & 0xFF) != 0xFF):
                    data.add(val & 0xFF)
                else:
                    return (data)
        return data 


"""!
	@brief A class which tunes the oscillator on a Serial Wombat 18AB chip
	
	This class is designed to be called periodically in the program main loop.  It compares
	the 1mS execution frame count to the millis() funciton provided by the host.  When
	at least 10 seconds of execution have occured the class compares the counts and
	issues a command to tune the Serial Wombat Chip's oscillator slightly slower or faster.
	This can reduce the error in the Serial Wombat's 32MHz nominal clock to less than +/- 0.1%
	vs. the +/- 1.5% limit in the datasheet.   Simply call update() periodically and the class
	will take care of the rest.  Allow up to 10 calls at least 10 seconds apart each to reach
	optimal timing.
	See the example sketch for an example.
"""
class SerialWombat18ABOscillatorTuner:
    #{
    #private:
    """!
    @brief Class constructor for SerialWombat18OscillatorTuner
    @param serialWombat The Serial Wombat chip on which the Oscillator will be tuned;
    """
    def __init__(self,serial_wombat):
        self._sw = serial_wombat
        self._lastMillis = 0
        self._lastFrames = 0
    #public:
    """!
    @brief   Call periodically to tune the SW18AB oscillator to reported millis
    """
    def update(self):   
        m = millis()
        if (self._lastMillis == 0):
            self._lastMillis = m
            frames = self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_MSW)
            frameslsb = self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_LSW)
            if (frames != self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_MSW)):
                frameslsb = self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_LSW)
                frames = self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_MSW)
            frames <<= 16
            frames += frameslsb
            self._lastFrames = frames
        elif ((m - self._lastMillis) < 10000):
            pass
        elif (m < self._lastMillis):
            #Has it been 47 days already?
            self._lastMillis = 0
        else:
            diff = m - self._lastMillis

            frames = self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_MSW)
            frameslsb =self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_LSW)
            
            if (frames != self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_MSW)):
                frameslsb = self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_LSW)
                frames = self._sw.readPublicData(SerialWombatDataSource.SW_DATA_SOURCE_FRAMES_RUN_MSW)
            frames <<= 16
            frames += frameslsb
            framesDif = frames - self._lastFrames

            if (diff > framesDif ):
                # Running  slow
                tx = bytearray([ SerialWombatCommands.COMMAND_ADJUST_FREQUENCY]) + SW_LE16(1) +SW_LE16(0) + bytearray([0x55,0x55,0x55])
                result,rx = self._sw.sendPacket(tx)         

            elif (diff < framesDif):
                 # Running  sFast
                tx = bytearray([ SerialWombatCommands.COMMAND_ADJUST_FREQUENCY]) + SW_LE16(0) +SW_LE16(1) + bytearray([0x55,0x55,0x55])
                result,rx = self._sw.sendPacket(tx)         

            self._lastMillis = m
            self._lastFrames = frames

"""
End of cross platform code synchronization.  Random string to help the compare tool sync lines:
asdkj38vjn1nasdnvuwlamafdjiivnowalskive
"""

