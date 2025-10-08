from machine import Pin
from time import sleep, ticks_ms, ticks_diff

#Déclaration des entrées/sorties
bouton = Pin(16, Pin.IN, Pin.PULL_DOWN)
led = Pin(18, Pin.OUT)

#Déclaration des variables
valeurClic = 0
dernierAppui = 0 

#Fonction bouton Appuye pour détecter de quand le bouton est pressé
def boutonAppuye(pin):
    global valeurClic, dernierAppui
    maintenant = ticks_ms()
    if ticks_diff(maintenant, dernierAppui) > 200:
        valeurClic = (valeurClic + 1) % 3
        dernierAppui = maintenant

#Faire une interruption pour que le bouton réagisse tout le temps.
bouton.irq(trigger=Pin.IRQ_RISING, handler=boutonAppuye)

#Boucle principale
while True:
    if valeurClic == 1:
        led.toggle()
        sleep(0.5)
    elif valeurClic == 2:
        led.toggle()
        sleep(0.25)
    else:
        led.value(0)
        sleep(0.1)
