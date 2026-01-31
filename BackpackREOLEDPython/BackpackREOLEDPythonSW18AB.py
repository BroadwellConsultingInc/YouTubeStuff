"""
This example shows how to initialize a 16 key, 8 pin 4x4 matrix keypad using the 
Serial Wombat 18AB chip'sSerialWombatMatrixKeypad class.

This example shows how to treat the matrix keypad as a stream input 
so that it can be treated as if keypresses are Serial Input

Note that firmware versions prior to 2.0.7 have a bug that may cause slow recognition of
button presses.

This example assumes a 4x4 keypad attached with rows connected to pins 10,11,12,13 
and columns attached to pins 16,17,18,19 .  This can be changed in the keypad.begin 
statement to fit your circuit.

This example uses default modes for the SerialWombatMatrixKeypad.  The default values 
send ASCII to the queue assuming a standard 

123A
456B
789C
*0#D

keypad format.   See the pin mode documentation (link below) for more information on the 
possible buffer and queue modes It is assumed that the Serial Wombat chip is at I2C 
address 0x6B.


A video demonstrating the use of the SerialWombatMatrixKeypad class on the Serial Wombat 18AB chip is available at:
https://youtu.be/hxLda6lBWNg

Documentation for the SerialWombatTM1637 Arduino class is available at:
https://broadwellconsultinginc.github.io/SerialWombatArdLib/class_serial_wombat_w_s2812.html#details

"""

#Comment these lines in if you're connecting directly to a Serial Wombat Chip's UART through cPython serial Module
#Change the paramter of SerialWombatChip_cpy_serial to match the name of your Serial port
#import SerialWombat_cpy_serial
#sw = SerialWombat_cpy_serial.SerialWombatChip_cpy_serial("COM9")


#Comment these lines in if you're connecting to a Serial Wombat Chip's I2C port using Micropython's I2C interface
#Change the values for sclPin, sdaPin, and swI2Caddress to match your configuration
import machine
import SerialWombat_mp_i2c
sclPin = 17
sdaPin = 16
swI2Caddress = 0x64
i2c = machine.I2C(0,
            scl=machine.Pin(sclPin),
            sda=machine.Pin(sdaPin),
            freq=100000,timeout = 50000)
sw = SerialWombat_mp_i2c.SerialWombatChip_mp_i2c(i2c,swI2Caddress)
sw.address = 0x64

#Comment these lines in if you're connecting to a Serial Wombat Chip's UART port using Micropython's UART interface
#Change the values for UARTnum, txPin, and rxPin to match your configuration
#import machine
#import SerialWombat_mp_UART
#txPin = 12
#rxPin = 14
#UARTnum = 2
#uart = machine.UART(UARTnum, baudrate=115200, tx=txPin, rx=rxPin)
#sw = SerialWombat_mp_UART.SerialWombatChipUART(uart)



#Interface independent code starts here:

import SerialWombatQuadEnc
import SerialWombatPulseOnChange
import SerialWombatDebouncedInput

qe = SerialWombatQuadEnc.SerialWombatQuadEnc(sw)
poc = SerialWombatPulseOnChange.SerialWombatPulseOnChange(sw)
knobButton = SerialWombatDebouncedInput.SerialWombatDebouncedInput(sw)
confButton = SerialWombatDebouncedInput.SerialWombatDebouncedInput(sw)
backButton = SerialWombatDebouncedInput.SerialWombatDebouncedInput(sw)

def setup():
    sw.begin()
    qe.begin(5, #1st Pin
                6, #2nd Pin
                1, #DebouceTime in mS
                True, #pull ups
                SerialWombatQuadEnc.QE_READ_MODE_t.QE_ONHIGH_POLL);

    
    knobButton.begin(4);
    backButton.begin(7);
    confButton.begin(1);
    
    poc.begin(2, 
        0,
        2,
        20,
        20,
        True,
        0,
        32768);


    poc.setEntryOnChange(0, 5);
    poc.setEntryOnIncrease(1, 4);
    poc.setEntryOnIncrease(2, 7);
    poc.setEntryOnIncrease(3, 1);


def loop():

    qeval = qe.readPublicData()
    kbval = knobButton.readPublicData()
    backval = backButton.readPublicData()
    confval = confButton.readPublicData()
    
    print(f"{qeval} {kbval} {backval} {confval}") 
    

setup()
while(True):
    loop()
