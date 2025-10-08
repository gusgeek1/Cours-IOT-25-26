from machine import Pin
from time import sleep, ticks_ms, ticks_diff

#Déclaration des entrées/sorties
bouton = Pin(16, Pin.IN, Pin.PULL_DOWN)
led = Pin(18, Pin.OUT)

#Déclaration des variables
valeurClic = 0
dernierAppui = 0  
faireBonus = False 

#Fonction bouton Appuye pour détecter de quand le bouton est pressé
def boutonAppuye(pin):
    global valeurClic, dernierAppui, faireBonus
    maintenant = ticks_ms()
    if ticks_diff(maintenant, dernierAppui) > 200:
        valeurClic = (valeurClic + 1) % 3 
        dernierAppui = maintenant
        #une fois le cycle fait, on repasse à 0 donc faire une fois bonus
        if valeurClic == 0:
            faireBonus = True

#Faire une interruption pour que le bouton réagisse tout le temps.
bouton.irq(trigger=Pin.IRQ_RISING, handler=boutonAppuye)

#Boucle principale
while True:
    # Faire le bonus une seule fois quand on est revenu à 0
    if faireBonus:
        for i in range(3):
            led.toggle()
            sleep(0.1)
        led.value(0)
        faireBonus = False
        continue

    if valeurClic == 1:
        led.toggle()
        sleep(0.5)
    elif valeurClic == 2:
        led.toggle()
        sleep(0.25)
    else:
        led.value(0)
        sleep(0.1)
