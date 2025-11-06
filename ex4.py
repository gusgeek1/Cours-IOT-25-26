from machine import Pin, ADC
import neopixel
import utime
import urandom

# Définition des différentes Pin
# Pin PWM pour micro
PIN_MIC_ADC = 26

# Pin pour la led RGB
PIN_WS = 16
N_PIX = 1
# Valeur de la luminosité
BRIGHT = 160

# Initialisation de la LED
np = neopixel.NeoPixel(Pin(PIN_WS, Pin.OUT), N_PIX)

# Permet de faire en sorte de limiter les rgb de dépasser le BRIGHT
def show_color(r, g, b):
    if r > BRIGHT: r = BRIGHT
    if g > BRIGHT: g = BRIGHT
    if b > BRIGHT: b = BRIGHT
    for i in range(N_PIX):
        np[i] = (r, g, b)
    np.write()

# Permet de récupérer la valeur du micro
adc = ADC(Pin(PIN_MIC_ADC))

# Paramètres pour détecter les beats du micro (les pics de son)
SEUIL_FACTEUR = 2.0
LISSAGE = 0.90
REFRACT_MS = 150
NB_ECH = 30
CENTRE = 32768

enveloppe = 2000.0
dernier_beat = 0

# Quand on démarre le programme, on garde la led éteinte
show_color(0, 0, 0)

print("WS2813: détection de pics -> couleur aléatoire")
while True:
    # Permet de mesurer l'amplitude moyenne du son
    s = 0
    for _ in range(NB_ECH):
        v = adc.read_u16()
        s += abs(v - CENTRE)
    amplitude = s // NB_ECH

    # Enveloppe qui permet de déterminer les beats puisque "l'onde du son" varie trop brusquement et rapidement
    enveloppe = LISSAGE * enveloppe + (1 - LISSAGE) * amplitude

    # Permet de vérifier si un beat est détecté
    now = utime.ticks_ms()
    if amplitude > enveloppe * SEUIL_FACTEUR and utime.ticks_diff(now, dernier_beat) > REFRACT_MS:
        dernier_beat = now

        # On fait en sorte de mettre une couleur aléatoire
        r = urandom.getrandbits(8) % (BRIGHT + 1)
        g = urandom.getrandbits(8) % (BRIGHT + 1)
        b = urandom.getrandbits(8) % (BRIGHT + 1)
        show_color(r, g, b)

    # Pause pour éviter de surcharger le processeur
    utime.sleep_ms(10)
