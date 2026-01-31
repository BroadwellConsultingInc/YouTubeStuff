#pragma once
"""
Copyright 2020-2021 Broadwell Consulting Inc.

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

#include "SerialWombat.h"
#include "limits.h"
import SerialWombat
from SerialWombatPin import SerialWombatPin
from SerialWombat import SW_LE32
from SerialWombat import SW_LE16
#from enum import IntEnum


"""! @file SerialWombatDebouncedInput.h
"""

"""! @brief A pin mode class that debounces inputs

The SerialWombatDebouncedInput class is used to debounce inputs such as switches.

See the example sw4b_ard_Debounce1 distributed with the Serial Wombat Arduino Library for an example.

Video Tutorial

@htmlonly
<iframe width="560" height="315" src="https://www.youtube.com/embed/R1KM0J2Ug-M" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
@endhtmlonly

https://youtu.be/R1KM0J2Ug-M

One SerialWombatDebouncedInput instance should be declared per debounced input.

The class sends commands and reads data from the Serial Wombat.  The actual switch
debouncing algorithm runs on the Serial Wombat.

The Serial Wombat's firmware algorithm polls the input every 1mS.  After it has 
been different from the reported value for X consecutive mS, the reported value 
changes to the new polled value.  The number of mS required for a change
can be configured.  

The Debounced Input mode keeps track of how long the debounced result has been
in the current state (up to 65535 mS) and can report this to the host.  This
is useful for implementing user interfaces that react based on how long a 
button has been held.

The Debounced Input mode keeps track of how many transitions have occured.
This can be used to poll the Debounced Input infrequently for status, but
still process all button presses/releases that occured since the last poll.

The pin mode has weak pull-up circuitry availble, and the ability to report 
inverted values.  These are both enabled when the simpliest begin() call is
used, allowing buttons that switch the Serial Wombat pin to ground to be used without
additional hardware.

See also the SerialWombatButtonCounter class which can run on top of this one.

"""
class SerialWombatDebouncedInput( SerialWombatPin):
	def __init__(self,serial_wombat):
		self._sw = serial_wombat
		self.transitions = 0

	"""!
	@brief Initialize a debounced input 

	@param pin  The Serial Wombat pin used for the debounced input
	@param debounce_mS number of mS the pin must be stable to cause a transition
	@param invert FALSE: pin reading is returned  TRUE: inverted pin reading is returned
	@param usePullUp Whether the pin's weak pull up is enabled
	"""
	def begin(self,pin, debounce_mS = 30, invert = True, usePullUp = True):
		self._pin = pin
		tx = bytearray([
                    200,
                    self._pin,
                    10])
		tx += SW_LE16(debounce_mS)
		tx += bytearray([invert,0,usePullUp])
		result, rx = self._sw.sendPacket(tx)
		return result

	"""!
	@brief Returns the debounced state of the input
	
	This function reads from the public data of the pin which 
	indicates the debounced and invert adjusted state of the
	input
	@return TRUE or FALSE.  Meaning depends on inversion setting
	"""
	def digitalRead(self):
		return (self._sw.readPublicData(self._pin) > 0)

	"""
	@brief return the number of mS that the debounced input has been in true state
	
	Note that this value starts incrementing after the debounce period, not after the physical pin transition.
	
	@return returns a value in mS which saturates at 65535.  Returns 0 if currently false.
	"""
	def readDurationInTrueState_mS(self):
		tx = [ 201,self._pin,10,1,0x55,0x55,0x55,0x55 ]
		result,rx = self._sw.sendPacket(tx)

		self.transitions +=  (256 * rx[5] + rx[4])
		if (rx[3] == 0):
			return (0)
		else:
			return(256 * rx[7] + rx[6])
	"""
	@brief return the number of mS that the debounced input has been in false state
	
	Note that this value starts incrementing after the debounce period, not after the physical pin transition.
	
	@return returns a value in mS which saturates at 65535.  Returns 0 if currently true.
	"""
	def readDurationInFalseState_mS(self):
		tx = [ 201,self._pin,10,1,0x55,0x55,0x55,0x55 ]
		result, rx = self._sw.sendPacket(tx, rx)

		self.transitions += (256 * rx[5] + rx[4])

		if (rx[3] == 1):
			return (0)
		else:
			return(256 * rx[7] + rx[6])

	"""
	@brief Queries the number of transistions that have occured on the debounced input
	
	This function queries the debounced input for current state and transitions since last call.
	transition count is put in the global member transitions.  The debounced input in the Serial
	Wombat resets its count to zero after this call.
	
	@return TRUE or FALSE, current status of debounced input
	"""
	def readTransitionsState(self):
		tx = [ 201,self._pin,10,1,0x55,0x55,0x55,0x55 ]
		result, rx = self._sw.sendPacket(tx)
		self.transitions = (256 * rx[5] + rx[4])
		return (rx[3] > 0)

	


"""! @brief A class that runs on top of SerialWombaAbstractButton to increment or decrement a variable based on a button

This class runs on top of a SerialWombaAbstractButton input.  It is passed a variable reference in its begin call.
The update() method is then called periodically.  This method will look at how many times the debounced input has
transitioned since the last call, and also if the input is currently pressed and for how long.

A high limit and low limit can be set which keeps the variable from exceeing those bounds.

Times can be specified for button hold that allows the variable to be incremented at varying rates for short, medium,
and long holds.

See the example sw4b_ard_Debounce2 distributed with the Serial Wombat Arduino Library and in the video 

https://youtu.be/_EKlrEVaEhg

for an example.


"""
class SerialWombatButtonCounter:

	def __init__(self, serialWombatDebouncedInput):
		self._debouncedInput = serialWombatDebouncedInput
	#! @brief The variable will not increment above this limit.
		self.highLimit = 32767
	#!  @brief The variable will not decrement below this limit.
		self.lowLimit = -32767
	"""!
	Initializes the SerialWombatButtonCounter

	@param serialWombatDebouncedInput The debounced input used for the counter
	@param variableToIncrement  A pointer to a signed long integer
	@param slowIncrement the amount that the variable should increment (or decrement if negative) per increment
	@param slow_mS_betweenIncrements how often an increment should happen in slow mode
	@param slowToMediumTransition_mS how long to stay in slow mode before switching to medium mode
	@param mediumIncrement the amount that the variable should increment (or decrement if negative) per increment
	@param medium_mS_betweenIncrements how often an increment should happen in medium mode
	@param mediumToFastTransition_mS how long after the initail button press start until switching to Fast mode
	@param fastIncrement the amount that the variable should increment (or decrement if negative) per increment
	@param fast_mS_betweenIncrements how often an increment should happen in fast mode
	"""
	def begin(self,slowIncrement = 1, slow_mS_betweenIncrements = 250,
			slowToMediumTransition_mS = 1000, 
			mediumIncrement = 1,	 medium_mS_betweenIncrements = 100, 
			mediumToFastTransition_mS = 1000 , 
			fastIncrement = 1, fast_mS_betweenIncrements = 50):
		
		self._slowIncrement = slowIncrement
		self._slow_mS_betweenIncrements = slow_mS_betweenIncrements

		self._slowToMediumTransition_mS = slowToMediumTransition_mS
		
		self._mediumIncrement = mediumIncrement
		self._medium_mS_betweenIncrements = medium_mS_betweenIncrements

		self._mediumToFastTransistion_mS = mediumToFastTransition_mS

		self._fastIncrement = fastIncrement
		self._fast_mS_betweenIncrements = fast_mS_betweenIncrements

		self._lastPressDuration = 0

	# @brief  Called periodically to query the SerialWombatDebouncedInput and update the variable
	def update(self,variableToIncrement):
		pressDuration = self._debouncedInput.readDurationInTrueState_mS()
		increments = 0
		incremented = False
		pressed = False
		if (pressDuration > 0):
			if (self._lastPressDuration >= pressDuration):
				self._lastPressDuration = 0

			if (pressDuration > self._mediumToFastTransistion_mS):
				# Increment fast
				increments = (pressDuration - self._lastPressDuration)// self._fast_mS_betweenIncrements
				variableToIncrement += self._fastIncrement * increments
				self._lastPressDuration += self._fast_mS_betweenIncrements * increments
			
			elif (pressDuration > self._slowToMediumTransition_mS):
				# Increment medium
				increments = (pressDuration - self._lastPressDuration)// self._medium_mS_betweenIncrements
				variableToIncrement += self._mediumIncrement * increments
				self._lastPressDuration += self._medium_mS_betweenIncrements * increments
			else:
				#Increment slow
				increments = (pressDuration - self._lastPressDuration)// self._slow_mS_betweenIncrements
				variableToIncrement += self._slowIncrement * increments
				self._lastPressDuration += self._slow_mS_betweenIncrements * increments
				incremented = increments > 0;  # An increment happened
				if (incremented):
					self._debouncedInput.transitions = 0;  # Get rid of false->true transition so that final release doesn't cause and increment
			pressed = True
		else :
			# Button isn't currently pressed.  if there were other transitions, add them
			self._lastPressDuration = 0
			presses = self._debouncedInput.transitions // 2
			variableToIncrement += self._slowIncrement * presses
			self._debouncedInput.transitions -= presses * 2

		if (variableToIncrement > self.highLimit):
				variableToIncrement = self.highLimit
		if (variableToIncrement < self.lowLimit):
				variableToIncrement = self.lowLimit

		return (pressed, variableToIncrement)




