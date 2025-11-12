from machine import Pin, PWM
import network, ntptime, time

# Définition des différentes Pin
# Pin pour le servo moteur en A0
PIN_SERVO = 22
# Fréquence du servo
FREQ = 50
# Valeurs minimum et maximum du servo
US_MIN = 500
US_MAX = 2500

# Identifiant et mot de passe du Wi-Fi
SSID = "iPhone 15 Pro de Gus"
PASSWORD = "motdepasse123"

# Connexion au Wi-Fi
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
if not wlan.isconnected():
    print("Connexion Wi-Fi...")
    wlan.connect(SSID, PASSWORD)
    while not wlan.isconnected():
        time.sleep(0.2)
print("Wi-Fi connecté :", wlan.ifconfig())

# Récupère l'heure via Internet
ntptime.settime()

# Initialisation du servo
servo = PWM(Pin(PIN_SERVO))
servo.freq(FREQ)

# Permet de convertir un angle en valeur utilisable par le servo
def angle_vers_duty(angle_deg):
    if angle_deg < 0: angle_deg = 0
    if angle_deg > 180: angle_deg = 180
    us = US_MIN + (US_MAX - US_MIN) * (angle_deg / 180.0)
    periode_us = 1000000 // FREQ
    duty = int(us * 65535 // periode_us)
    return duty

print("Horloge servo - Mode 12h : 12h = 0°, 6h = 90°, 12h = 180°")

while True:
    # Récupération de l'heure locale (UTC par défaut)
    t = time.localtime()
    h = t[3]
    m = t[4]
    s = t[5]

    # Calcul de l'angle selon l'heure (12h -> 180°)
    h12 = (h % 12) + m / 60 + s / 3600
    angle = h12 * 15.0

    # Envoie la position au servo
    servo.duty_u16(angle_vers_duty(angle))

    # Pause avant la prochaine mise à jour
    time.sleep(0.5)
