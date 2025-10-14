from machine import Pin, PWM, ADC
from time import ticks_ms, sleep_ms

# Déclaration des différents "périphériques"
pot = ADC(27)                 
buz = PWM(Pin(28))            
led = Pin(18, Pin.OUT)         
btn = Pin(16, Pin.IN, Pin.PULL_UP)

# Variables pour les notes de la musique
C4=262; D4=294; E4=330; F4=349; G4=392; A4=440; B4=494
C5=523; D5=587; E5=659; F5=698; G5=784; A5=880; B5=988

# Variables pour définir la durée des notes musicales
E=125 
Q=250
H=500

# Notes pour recréer les deux mélodies
m1 = [
    (E5,E),(E5,E),(0,E),(E5,E),(0,E),(C5,E),(E5,E),(0,E),(G5,H),(0,H),(G4,H),
    (C5,E),(0,E),(G4,E),(0,E),(E4,E),(0,E),(A4,E),(0,E),(B4,E),(0,E),(A4,E),(G4,Q),
    (E5,E),(G5,E),(A5,E),(F5,E),(G5,E),(0,E),(E5,E),(C5,E),(D5,E),(B4,H)
]
m2 = [
    (G5,E),(F5,E),(E5,E),(0,E),(A4,E),(C5,E),(D5,Q),
    (0,E),(G5,E),(F5,E),(E5,E),(0,E),(C5,E),(D5,E),(E5,H)
]
melodies = [m1, m2]
mel = 0

# anti-rebond pour le bouton pour que le pico ne pense pas qu'on a appuyé plusieurs fois
_last = 1
_last_t = 0
def pressed():
    global _last, _last_t
    v = btn.value()
    t = ticks_ms()
    if v == 0 and _last == 1 and t - _last_t > 150:
        _last = 0; _last_t = t
        return True
    if v == 1 and _last == 0:
        _last = 1; _last_t = t
    return False

def play_note(freq, dur_ms):
    end = ticks_ms() + dur_ms
    if freq > 0:
        buz.freq(freq)
    while ticks_ms() < end:
        buz.duty_u16(int(pot.read_u16() * (0 if freq==0 else 0.5)))
        led.value(1 if freq>0 else 0)
 # Permet de changer de mélodie à n'importe quel moment.      
        if pressed():          
            return True
        sleep_ms(10)
    led.value(0)
    buz.duty_u16(0)
    return False

try:
    while True:
        for f, d in melodies[mel]:
            if play_note(f, d):
                mel = (mel + 1) % len(melodies)
                break
        else:
# Si on appuie pas sur le bouton, on recommence à jouer la mélodie.
            pass
        sleep_ms(200)
except KeyboardInterrupt:
    pass
finally:
    led.value(0)
    buz.duty_u16(0)
    buz.deinit()
