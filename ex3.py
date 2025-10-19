from machine import Pin, ADC, I2C
import time
import dht
from pico_i2c_lcd import I2cLcd

# Déclaration des broches et paramètres
PIN_DHT, PIN_LED, PIN_BUZ, ADC_POT = 16, 15, 14, 26
I2C_ID, SDA, SCL, LCD_ADDR = 0, 0, 1, 0x27
TMIN, TMAX = 15.0, 35.0
ALARM_DELTA = 3.0
READ_EVERY_MS = 2000
SLOW_MS, FAST_MS = 1000, 150   

# Déclaration des variables
led = Pin(PIN_LED, Pin.OUT, value=0)
buz = Pin(PIN_BUZ, Pin.OUT, value=0)
pot = ADC(Pin(ADC_POT))
sensor = dht.DHT11(Pin(PIN_DHT))  

i2c = I2C(I2C_ID, sda=Pin(SDA), scl=Pin(SCL), freq=400000)
lcd = I2cLcd(i2c, LCD_ADDR, 2, 16)

# Mise à l'échelle du signal du potentiomètre pour qu'il soit dans la plage 15°C 35°C
def adc_to_temp(v): 
    return TMIN + (v / 65535.0) * (TMAX - TMIN)

def due(last_ms, period_ms):
    return time.ticks_diff(time.ticks_ms(), last_ms) >= period_ms

# Mise des variables pour fixer leurs états avant exec programme
ambient = None
last_dht = -READ_EVERY_MS
last_toggle = 0
blink_on = False

# Programme
while True:
    # 1) Mettre la valeur lue du potentoimètre dans la variable setpoint
    setpoint = adc_to_temp(pot.read_u16())

    # 2) Mesure de la valeur du capteur toute les deux secondes
    if due(last_dht, READ_EVERY_MS):
        last_dht = time.ticks_ms()
        try:
            sensor.measure()
            ambient = sensor.temperature()
        except Exception:
            ambient = None

    # 3) Affichage & contrôle
    lcd.move_to(0, 0)
    lcd.putstr("T voulue: %5.1fC   " % setpoint)

    if ambient is None:
        led.off(); buz.off()
        lcd.move_to(0, 1); lcd.putstr("T Ambiante: --.- ")
    else:
        if ambient >= setpoint + ALARM_DELTA:
            # Alarme
            if due(last_toggle, FAST_MS):
                last_toggle = time.ticks_ms()
                blink_on = not blink_on
                led.value(1 if blink_on else 0)
            buz.on()
            lcd.move_to(0, 1); lcd.putstr("ALARME          ")
        elif ambient > setpoint:
            # Lorsque la température est au dessus de la consigne
            if due(last_toggle, SLOW_MS):
                last_toggle = time.ticks_ms()
                blink_on = not blink_on
                led.value(1 if blink_on else 0)
            buz.off()
            lcd.move_to(0, 1); lcd.putstr("T Ambiante:%5.1fC" % ambient)
        else:
            # Lorsque la température est plus petite ou égale à la consigne
            led.off(); buz.off()
            lcd.move_to(0, 1); lcd.putstr("T Ambiante:%5.1fC" % ambient)

    time.sleep_ms(20)
