import SerialWombat
import time
import machine



class SerialWombatChip_mp_i2c(SerialWombat.SerialWombatChip):
    i2c  = 0
    def __init__(self,i2c_port,address):
        super().__init__()
        self.i2c = i2c_port
        self.address = address



    def sendReceivePacketHardware (self,tx):
        try:
            if (isinstance(tx,list)):
                tx = bytearray(tx);
            
            self.i2c.writeto(self.address,tx)
            rx = self.i2c.readfrom(self.address,8)
            if (len(rx) < 8 ):
                return (-len(rx))
            return 8,rx  
        except OSError:
            return -48,bytes("E00048UU",'utf-8')

    def sendPacketToHardware (self,tx):
        try:
            if (isinstance(tx,list)):
                tx = bytearray(tx);
            
            self.i2c.writeto(self.address,tx)
            return 8,bytes("E00048UU",'utf-8')  
        except OSError:
            return -48,bytes("E00048UU",'utf-8')

