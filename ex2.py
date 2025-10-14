from machine import Pin, PWM, ADC
from time import sleep_ms, ticks_ms, ticks_diff

# Déclaration des différents "périphériques"
pot = ADC(27)         
buzzer = PWM(Pin(28))

# Variables pour les notes de la musique
C4=262; D4=294; E4=330; F4=349; G4=392; A4=440; B4=494
C5=523; D5=587; E5=659; F5=698; G5=784; A5=880; B5=988
C6=1047

# Variables pour définir la durée des notes musicales
E = 125
Q = 250  
H = 500  
R = 0    

# Musique de Mario
melody = [
    (E5,E), (E5,E), (R,E), (E5,E), (R,E), (C5,E), (E5,E),
    (R,E), (G5,H), (R,H), (G4,H),

    (C5,E), (R,E), (G4,E), (R,E), (E4,E), (R,E),
    (A4,E), (R,E), (B4,E), (R,E), (A4,E), (G4,Q),

    (E5,E), (G5,E), (A5,E), (F5,E), (G5,E),
    (R,E), (E5,E), (C5,E), (D5,E), (B4,H)
]

# Lecture
def read_volume():
    raw = pot.read_u16()
    return int(raw * 0.5)  
# Limite du volume à 50% car sinon ça sature


def play(freq, dur_ms):
    if freq == 0:
        buzzer.duty_u16(0)
        sleep_ms(dur_ms)
        return

    buzzer.freq(freq)
    start = ticks_ms()
    while ticks_diff(ticks_ms(), start) < dur_ms:
        buzzer.duty_u16(read_volume())
        sleep_ms(10)
    buzzer.duty_u16(0)

# Boucle principale
try:
    while True:
        for freq, dur in melody:
            play(freq, dur)
        sleep_ms(300)
except KeyboardInterrupt:
    pass
finally:
    buzzer.duty_u16(0)
    buzzer.deinit()
