from machine import Pin, PWM
import network, ntptime, time

# Définition des différentes Pin
# Pin pour le servo moteur
PIN_SERVO = 22
# Pin pour le bouton
PIN_BOUTON = 16
# Fréquence du servo
FREQ = 50
# Valeurs minimum et maximum du servo
US_MIN = 500
US_MAX = 2500

# Identifiant et mot de passe du Wi-Fi
SSID = "iPhone 15 Pro de Gus"
PASSWORD = "motdepasse123"

# Liste des fuseaux horaires (heures de décalage)
FUSEAUX = [0, 1, -5, 2]
indice_fuseau = 0

# Variable pour savoir si on est en 24h ou 12h
mode_24h = False

# Connexion au Wi-Fi
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
if not wlan.isconnected():
    print("Connexion Wi-Fi...")
    wlan.connect(SSID, PASSWORD)
    while not wlan.isconnected():
        time.sleep(0.2)
print("Wi-Fi connecté :", wlan.ifconfig())

# Récupération de l'heure via Internet
ntptime.settime()

# Initialisation du servo
servo = PWM(Pin(PIN_SERVO))
servo.freq(FREQ)

# Initialisation du bouton en PULL_UP
bouton = Pin(PIN_BOUTON, Pin.IN, Pin.PULL_UP)

# Fonction pour convertir un angle en valeur utilisable par le servo
def angle_vers_duty(angle_deg):
    if angle_deg < 0: angle_deg = 0
    if angle_deg > 180: angle_deg = 180
    us = US_MIN + (US_MAX - US_MIN) * (angle_deg / 180.0)
    periode_us = 1000000 // FREQ
    duty = int(us * 65535 // periode_us)
    return duty

print("Horloge servo: simple clic = changer fuseau / double clic = mode 24h")

while True:
    # Vérifie si le bouton est appuyé
    if bouton.value() == 0:
        t0 = time.ticks_ms()
        # Attend que le bouton soit relâché
        while bouton.value() == 0:
            time.sleep_ms(10)
        t1 = time.ticks_ms()

        # Attend pour vérifier un éventuel double clic
        double = False
        attente = 0
        while attente < 400:
            if bouton.value() == 0:
                while bouton.value() == 0:
                    time.sleep_ms(10)
                double = True
                break
            time.sleep_ms(10)
            attente += 10

        # Double clic = changer entre 12h et 24h
        if double:
            mode_24h = not mode_24h
            print("Mode :", "24h" if mode_24h else "12h")
        # Simple clic = changer de fuseau horaire
        else:
            indice_fuseau = (indice_fuseau + 1) % len(FUSEAUX)
            print("Fuseau horaire -> UTC%+d" % FUSEAUX[indice_fuseau])

    # Récupération de l'heure locale avec fuseau
    t_utc = time.localtime()
    offset_s = FUSEAUX[indice_fuseau] * 3600
    secondes = time.mktime(t_utc) + offset_s
    t_loc = time.localtime(secondes)
    h = t_loc[3]
    m = t_loc[4]
    s = t_loc[5]

    # Calcul de l'angle selon le mode (12h ou 24h)
    if not mode_24h:
        # Mode 12h
        h12 = (h % 12) + m / 60 + s / 3600
        angle = h12 * 15.0
    else:
        # Mode 24h
        h24 = h + m / 60 + s / 3600
        angle = h24 * 7.5

    # Envoie la position au servo
    servo.duty_u16(angle_vers_duty(angle))

    # Petite pause pour la stabilité
    time.sleep_ms(50)
