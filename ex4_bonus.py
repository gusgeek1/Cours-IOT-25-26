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
BRIGHT = 180

# Initialisation de la LED
np = neopixel.NeoPixel(Pin(PIN_WS, Pin.OUT), N_PIX)

# Permet de faire en sorte de limiter les rgb de dépasser le BRIGHT
def show_color(r, g, b):
    if r > BRIGHT: r = BRIGHT
    if g > BRIGHT: g = BRIGHT
    if b > BRIGHT: b = BRIGHT
    for i in range(NPIX):
        np[i] = (r, g, b)
    np.write()


NPIX = N_PIX 

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

# Variables qui permettent le calcul du BPM
instant_prec = None
liste_bpm = []
debut_minute = utime.ticks_ms()
FICHIER_LOG = "bpm_log.txt"

# Quand on démarre le programme, on garde la led éteinte
show_color(0, 0, 0)
print("WS2813 BONUS: couleur + BPM + moyenne/min -> bpm_log.txt")

while True:
    # amplitude moyenne
    s = 0
    for _ in range(NB_ECH):
        v = adc.read_u16()
        s += abs(v - CENTRE)
    amplitude = s // NB_ECH

    # Enveloppe qui permet de déterminer les beats puisque "l'onde du son" varie trop brusquement et rapidement.
    enveloppe = LISSAGE * enveloppe + (1 - LISSAGE) * amplitude


    t = utime.ticks_ms()
    if amplitude > enveloppe * SEUIL_FACTEUR and utime.ticks_diff(t, dernier_beat) > REFRACT_MS:
        dernier_beat = t

        # On fait en sorte de mettre une couleur aléatoire
        r = urandom.getrandbits(8) % (BRIGHT + 1)
        g = urandom.getrandbits(8) % (BRIGHT + 1)
        b = urandom.getrandbits(8) % (BRIGHT + 1)
        show_color(r, g, b)

        # BPM instantané si on a un précédent
        if instant_prec is not None:
            dt = utime.ticks_diff(t, instant_prec)
            if 300 <= dt <= 2000:
                bpm = 60000.0 / dt
                liste_bpm.append(bpm)
        instant_prec = t

    # Chaque minute, on log la moyenne
    if utime.ticks_diff(t, debut_minute) >= 60000:
        if len(liste_bpm) == 0:
            moyenne = 0.0
        else:
            moyenne = sum(liste_bpm) / len(liste_bpm)

        try:
            tt = utime.localtime()
            stamp = "%04d-%02d-%02d %02d:%02d:%02d" % (tt[0], tt[1], tt[2], tt[3], tt[4], tt[5])
            with open(FICHIER_LOG, "a") as f:
                f.write("%s ; moyenne_BPM=%.1f ; n=%d\n" % (stamp, moyenne, len(liste_bpm)))
            print("Moyenne/min:", moyenne)
        except Exception as e:
            print("Erreur fichier:", e)

        liste_bpm = []
        debut_minute = t

    utime.sleep_ms(10)
