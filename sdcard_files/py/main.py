# main.py — Script principal execute au demarrage par l'app AKA MicroPython.
# Demo : une balle rebondissante + affichage des boutons. Sert de squelette de
# jeu. Copie ce dossier "py/" a la racine de la carte SD (-> /sdcard/py/main.py).
#
# API disponible : module natif `aka` (voir README / MANUEL).

import aka

# Aide affichee dans le menu systeme (touche MENU -> "Commandes").
aka.set_controls([
    "Fleches : (rien ici)",
    "A : flash rouge",
    "B : vibration",
    "MENU : menu systeme",
    "RUN + MENU : quitter",
])
aka.set_credits("Demo Balle", "AKA Port Studio", "MIT", "micropython.org")

W = aka.width()
H = aka.height()

x = W // 2
y = H // 2
vx = 3
vy = 2
r = 12

WHITE = (255, 255, 255)
GREEN = (0, 220, 0)
RED = (220, 40, 40)

while True:
    # aka.update() lit les boutons et laisse le menu systeme AKA prendre la
    # main si besoin ; renvoie False la frame ou le menu est affiche.
    if aka.update():
        x += vx
        y += vy
        if x < r or x > W - r:
            vx = -vx
        if y < r or y > H - r:
            vy = -vy

        aka.clear(10, 10, 30)

        aka.set_color(*GREEN)
        aka.fill_circle(x, y, r)

        aka.set_color(*WHITE)
        aka.text(8, 8, "MicroPython sur AKA")
        aka.text(8, 24, "Langue: " + aka.language())

        b = aka.buttons()
        if b & aka.A:
            aka.set_color(*RED)
            aka.fill_rect(0, 0, W, 6)
        if b & aka.B:
            aka.vibrate(60)

        aka.display()

    # Cadence ~60 fps.
    aka.sleep_ms(16)
