from machine import Pin, ADC, I2C, PWM   # Il faut ajouter le PWM pour faire varier la led
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
DIMMER_RANGE = 5.0        

# Déclaration des variables
# LED en PWM pour pouvoir faire varier la luminosité
led_pwm = PWM(Pin(PIN_LED, Pin.OUT))
led_pwm.freq(1000)              
def led_set(duty_u16):         
    led_pwm.duty_u16(max(0, min(65535, int(duty_u16))))
def led_off():
    led_set(0)

buz = Pin(PIN_BUZ, Pin.OUT, value=0)
pot = ADC(Pin(ADC_POT))
sensor = dht.DHT11(Pin(PIN_DHT))  

i2c = I2C(I2C_ID, sda=Pin(SDA), scl=Pin(SCL), freq=400000)
lcd = I2cLcd(i2c, LCD_ADDR, 2, 16)

# Mise à l'échelle du signal du potentiomètre pour qu'il soit dans la plage 15°C..35°C
def adc_to_temp(v):
    return TMIN + (v / 65535.0) * (TMAX - TMIN)

def due(last_ms, period_ms):
    return time.ticks_diff(time.ticks_ms(), last_ms) >= period_ms

# État des variables
ambient = None
last_dht = -READ_EVERY_MS
last_toggle = 0
blink_on = False

while True:
    # Consigne lue sur le potentiomètre
    setpoint = adc_to_temp(pot.read_u16())

    # Mesure capteur toutes les 2s
    if due(last_dht, READ_EVERY_MS):
        last_dht = time.ticks_ms()
        try:
            sensor.measure()
            ambient = sensor.temperature()
        except Exception:
            ambient = None

    # Affichage & contrôle
    lcd.move_to(0, 0)
    lcd.putstr("T souhaitée: %5.1fC   " % setpoint)

    if ambient is None:
        led_off(); buz.off()
        lcd.move_to(0, 1); lcd.putstr("T Ambiante: --.- ")
    else:
        if ambient >= setpoint + ALARM_DELTA:
            # Quand alarme, clignotement rapide + buzzer on
            if due(last_toggle, FAST_MS):
                last_toggle = time.ticks_ms()
                blink_on = not blink_on
                # Lorsque on est en alarme on met la led a 100´%
                led_set(65535 if blink_on else 0)
            buz.on()
            lcd.move_to(0, 1); lcd.putstr("ALARME          ")

        elif ambient > setpoint:
            # Quand on est au dessus de la consigne le clignotement est lent
            if due(last_toggle, SLOW_MS):
                last_toggle = time.ticks_ms()
                blink_on = not blink_on
                if blink_on:
                    # BONUS: luminosité proportionnelle au dépassement (0..+5°C)
                    overshoot = max(0.0, ambient - setpoint)
                    overshoot = min(DIMMER_RANGE, overshoot)
                    duty = int((overshoot / DIMMER_RANGE) * 65535)
                    led_set(duty)
                else:
                    led_off()
            buz.off()
            lcd.move_to(0, 1); lcd.putstr("T Ambiante:%5.1fC" % ambient)

        else:
            # En-dessous ou égal à la consigne
            led_off(); buz.off()
            lcd.move_to(0, 1); lcd.putstr("T Ambiante:%5.1fC" % ambient)

    time.sleep_ms(20)
