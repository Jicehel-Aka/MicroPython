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

## 5. Temps, vibration, son, divers

| Fonction | Effet |
|----------|-------|
| `aka.ticks_ms()` | millisecondes depuis le démarrage (entier) |
| `aka.sleep_ms(ms)` | pause (laisse respirer le système) |
| `aka.vibrate(ms)` | fait vibrer la console pendant `ms` millisecondes (`ms=0` arrête immédiatement) |
| `aka.is_vibrating()` | `True` si la console est en train de vibrer |
| `aka.screenshot()` | enregistre une capture d'écran (BMP) sur la SD |
| `aka.language()` | code langue courant de la console (`"fr"`, `"en"`, …) |
| `aka.tr("CLE")` | traduit une clé selon la langue (voir aka_runtime) |
| `aka.play_pcm8(data, loop=False)` | joue un échantillon PCM 8 bits non signé (`bytes`/`bytearray`, 128=silence) |
| `aka.is_sound_playing()` | `True` si un son est en cours de lecture |

`play_pcm8` copie les données en interne (pas besoin de garder l'objet
`bytes` vivant après l'appel) et gère lui-même le rebouclage si `loop=True`.

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

## 8. Porter un jeu Python Pokitto (upygame / umachine)

De nombreux jeux Pokitto écrits en MicroPython utilisent une bibliothèque de
compatibilité `pygame`-like, **uPyGame** (`import upygame as pygame` +
`import umachine`), documentée officiellement par Pokitto. Trois fichiers
fournis dans `sdcard_files/py/` réimplémentent ce sous-ensemble par-dessus le
module `aka` :

- **`upygame.py`** — `Rect`, `display` (`set_mode`, `set_palette_16bit`,
  `flip`), `draw.text`, `surface.Surface` (+ `.blit`, `.fill`, `.get_rect`),
  `mixer.Sound` (`.play_sfx`, via `aka.play_pcm8`), `event.poll()`.
- **`umachine.py`** — `Cookie` (sauvegarde persistante sur SD, un fichier par
  nom de cookie) et `time_ms()`.
- **`urandom.py`** — `getrandbits()` en pur Python (générateur xorshift32),
  pour ne pas dépendre d'un module natif non garanti selon la version de
  MicroPython embarquée.
- **`sprite.py`** — classes `Sprite`/`Group` (dérivées du vrai module
  `sprite.py` de pygame, licence LGPL préservée), utile pour les jeux
  utilisant `sprite.Group()`.

Format des données attendu par `Surface` (identique à Pokitto, confirmé via
le code source officiel PokittoLib) : 4 bits par pixel, indice de palette
0-15, nibble haut = premier pixel de la paire (format `GS4_HMSB`).

**Statut actuel** : implémentation correcte-avant-rapide (dessin pixel par
pixel via `aka.pixel()`) — fonctionnelle mais potentiellement lente pour
beaucoup de sprites par frame. Si besoin, l'étape suivante est une fonction
native `aka.blit_indexed(...)` qui ferait tout le travail côté C, sans
changer l'API Python vue par les jeux.

Un jeu Pokitto peut ainsi souvent être copié presque tel quel dans
`/sdcard/py/` (ou son propre dossier, voir section suivante), à condition
qu'il n'utilise que ce sous-ensemble de l'API (pas de mode TAS, pas de
`Tilemap`, pas de sprites matériels `setHwSprite` — non couverts pour
l'instant).

---

## 9. Faire d'un jeu Python une app AKA autonome

Par défaut, cette app "MicroPython" exécute `/sdcard/py/main.py` et apparaît
dans le loader AKA comme **un seul jeu générique**. Pour qu'un jeu Python
précis apparaisse **comme sa propre app**, avec sa propre icône et son
propre nom dans le loader (exactement comme un jeu C++ classique) :

1. Copier ce projet entier (composants `micropython`/`aka_runtime`/
   `gamebuino` inclus, **non modifiés**) dans un nouveau dossier de projet.
2. Dans `main/main.cpp`, adapter seulement :
   - `akaRuntime.begin("<id_du_jeu>")` (au lieu de `"micropython"`)
   - `AKA_MAIN_PY "/sdcard/<id_du_jeu>/main.py"` (dossier propre au jeu)
   - `aka_hal_set_credits(...)` / `aka_hal_set_controls(...)` (crédits du jeu)
3. Placer les fichiers `.py` du jeu (plus `upygame.py`/`umachine.py`/
   `urandom.py`/`sprite.py` si besoin) dans `sdcard_files/<id_du_jeu>/`.
4. Donner au projet son propre `port_manifest.json`, `screen.bmp` et
   `meta.json` (voir le dossier `sdcard_files/micropython/` de ce projet
   pour l'exemple du lanceur générique lui-même).

Ce lanceur générique et les lanceurs dédiés coexistent sans conflit — les
fonctions `aka.list_py()`/`aka.run_file()` restent disponibles dans les deux
cas si un jeu veut proposer son propre sous-menu de scripts.

---

## 10. Que faire si… ?

- **Écran figé / rien ne s'affiche** : as-tu appelé `aka.display()` après avoir
  dessiné, et `aka.update()` au début de la frame ?
- **« Fichier .py introuvable »** à l'écran : vérifie que `/sdcard/py/main.py`
  existe bien (dossier `py` à la racine de la SD).
- **Erreur Python** : le message (avec le numéro de ligne) est envoyé sur la
  console série USB — branche l'AKA et ouvre `idf.py monitor` pour le lire.
- **Le jeu rame** : réduis le travail par frame, ou augmente `aka.sleep_ms`.
