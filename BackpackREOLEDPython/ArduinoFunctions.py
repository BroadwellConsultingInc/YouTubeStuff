import time

millisStart = time.ticks_ms()
def millis():
    return(int((time.ticks_ms() - millisStart)))

def delay(delayMs):
    time.sleep_ms(int(delayMs))
    
def delayMicroseconds(delayUs):
    time.sleep_us(int(delayUs))