import SerialWombat

class SerialWombatPin:
    _sw = 0 # will be serial wombat
    _pin = 255
    _pinMode = 255
    def __init__(self,sw):
        self._sw = sw

    def readPublicData(self):
        return self._sw.readPublicData(self._pin)

    def writePublicData(self,value):
        return self._sw.writePublicData(self._pin,value)

    def pinMode(self,mode, pullDown = False, openDrain = False):
        self._sw.pinMode(self._pin,mode, pullDown,openDrain)

    def digitalWrite(self,val):
        self._sw.digitalWrite(self._pin,val)

    def digitalRead(self):
        return self._sw.digitalRead(self._pin)

    def pin(self):
        return self._pin

    def swPinModeNumber(self):
        return (self._pinMode)

