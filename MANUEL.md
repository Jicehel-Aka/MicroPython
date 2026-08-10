# Mode d'emploi — MicroPython sur l'AKA (français)

Cette app transforme l'AKA en console **programmable en Python**. Tu écris ton
jeu dans un fichier `.py` sur la carte SD, sans recompiler le firmware.

*English version: [MANUAL.md](MANUAL.md).*

---

## 1. Installation

1. Flashe l'app `micropython` sur la console (voir [README.md](README.md)).
2. Copie le dossier `sdcard_files/py/` à la racine de ta carte SD. Le firmware
   exécute au démarrage le fichier **`/sdcard/py/main.py`**.
3. Remplace `main.py` par ton propre jeu (ou modifie l'exemple fourni).

Au démarrage, l'AKA lance automatiquement `/sdcard/py/main.py`.

---

## 2. Ton premier jeu

```python
import aka

W = aka.width()      # 320
H = aka.height()     # 240
x, y = W // 2, H // 2

while True:
    if aka.update():                    # OBLIGATOIRE une fois par frame
        aka.clear(0, 0, 0)              # efface l'écran en noir

        b = aka.buttons()
        if b & aka.LEFT:  x -= 3
        if b & aka.RIGHT: x += 3
        if b & aka.UP:    y -= 3
        if b & aka.DOWN:  y += 3

        aka.set_color(0, 220, 0)
        aka.fill_circle(x, y, 12)

        aka.display()                   # affiche la frame
    aka.sleep_ms(16)                    # ~60 images/seconde
```

### La boucle de jeu

Une frame se construit toujours ainsi :

1. **`aka.update()`** — à appeler en premier. Lit les boutons **et** laisse le
   menu système AKA prendre la main si le joueur appuie sur MENU. Renvoie `True`
   si ton jeu doit tourner cette frame, `False` pendant que le menu est affiché
   (dans ce cas, ne dessine rien).
2. Dessine (voir §4).
3. **`aka.display()`** — envoie l'image à l'écran.
4. **`aka.sleep_ms(16)`** — cadence.

---

## 3. Les boutons

`aka.buttons()` renvoie un entier : un « masque de bits ». Teste un bouton avec
l'opérateur `&` :

```python
b = aka.buttons()
if b & aka.A:
    tirer()
```

| Constante | Bouton                |
|-----------|-----------------------|
| `aka.UP` `aka.DOWN` `aka.LEFT` `aka.RIGHT` | croix directionnelle |
| `aka.A` `aka.B` `aka.C` `aka.D` | boutons d'action |
| `aka.L1` `aka.R1` | gâchettes |
| `aka.RUN` `aka.MENU` | boutons système |

- `aka.pressed()` — boutons **tout juste enfoncés** cette frame (front montant).
- `aka.released()` — boutons **tout juste relâchés** cette frame.
- `aka.joystick()` — renvoie un tuple `(x, y)` (position analogique).

> RUN + MENU maintenus reviennent toujours au menu de la console (loader) — c'est
> géré automatiquement, tu n'as rien à faire.

---

## 4. Dessiner

L'écran fait **320 × 240** pixels, en couleur. L'origine `(0, 0)` est en haut à
gauche. Les couleurs se donnent en RVB (0-255).

| Fonction | Effet |
|----------|-------|
| `aka.set_color(r, g, b)` | choisit la couleur du « pinceau » courant |
| `aka.color(r, g, b)` | renvoie l'entier couleur (RGB565) sans changer le pinceau |
| `aka.clear()` / `aka.clear(r, g, b)` | efface tout l'écran (noir, ou couleur) |
| `aka.pixel(x, y)` | un pixel |
| `aka.line(x0, y0, x1, y1)` | une ligne |
| `aka.hline(x, y, w)` / `aka.vline(x, y, h)` | ligne horizontale / verticale |
| `aka.rect(x, y, w, h)` / `aka.fill_rect(...)` | rectangle (contour / plein) |
| `aka.circle(x, y, r)` / `aka.fill_circle(...)` | cercle (contour / plein) |
| `aka.triangle(x0,y0,x1,y1,x2,y2)` / `aka.fill_triangle(...)` | triangle |
| `aka.text(x, y, "texte")` | écrit du texte à la couleur courante |
| `aka.display()` | affiche la frame construite |
| `aka.width()` / `aka.height()` | dimensions de l'écran |

---

## 5. Temps, vibration, divers

| Fonction | Effet |
|----------|-------|
| `aka.ticks_ms()` | millisecondes depuis le démarrage (entier) |
| `aka.sleep_ms(ms)` | pause (laisse respirer le système) |
| `aka.vibrate(ms)` | fait vibrer la console pendant `ms` millisecondes |
| `aka.screenshot()` | enregistre une capture d'écran (BMP) sur la SD |
| `aka.language()` | code langue courant de la console (`"fr"`, `"en"`, …) |
| `aka.tr("CLE")` | traduit une clé selon la langue (voir aka_runtime) |

---

## 6. Aide intégrée au menu système

Quand le joueur appuie sur **MENU**, la console affiche son menu système
(Reprendre, Commandes, Langue, Volume, Crédits, Quitter). Tu peux renseigner
l'écran **« Commandes »** et **« Crédits »** de ton jeu :

```python
aka.set_controls([
    "Fleches : deplacer",
    "A : sauter",
    "B : tirer",
    "MENU : pause",
])
aka.set_credits("Mon Jeu", "Mon Nom", "MIT", "github.com/moi/monjeu")
```

À appeler une fois au début du script. Jusqu'à 12 lignes de commandes.

---

## 7. Plusieurs jeux / un menu maison

Deux fonctions permettent de construire ton propre lanceur en Python :

```python
jeux = aka.list_py("/sdcard/py")   # liste les fichiers .py d'un dossier
aka.run_file("/sdcard/py/pong.py") # exécute un autre script
```

Tu peux ainsi faire de `main.py` un menu qui liste et lance les autres `.py`.

---

## 8. Que faire si… ?

- **Écran figé / rien ne s'affiche** : as-tu appelé `aka.display()` après avoir
  dessiné, et `aka.update()` au début de la frame ?
- **« Fichier .py introuvable »** à l'écran : vérifie que `/sdcard/py/main.py`
  existe bien (dossier `py` à la racine de la SD).
- **Erreur Python** : le message (avec le numéro de ligne) est envoyé sur la
  console série USB — branche l'AKA et ouvre `idf.py monitor` pour le lire.
- **Le jeu rame** : réduis le travail par frame, ou augmente `aka.sleep_ms`.
