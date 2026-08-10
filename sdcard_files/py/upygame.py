# upygame.py — sous-ensemble compatible upygame (API "micro-pygame" de
# Pokitto) implemente en pur Python par-dessus le module natif `aka`.
#
# STATUT : v1, correction avant vitesse. Surface.blit() dessine pixel par
# pixel via aka.pixel()/aka.set_color() (natif, mais appele une fois par
# pixel) -- fonctionnellement correct, mais potentiellement lent pour de
# GRANDS sprites ou BEAUCOUP de sprites par frame. Si les mesures sur
# materiel reel montrent un probleme de fluidite, la prochaine etape est
# d'ajouter une fonction native aka.blit_indexed(x,y,w,h,pixels,palette)
# qui fait tout le travail cote C -- l'API Python ci-dessous n'aurait pas
# a changer, seule l'implementation de Surface.blit() serait remplacee par
# un unique appel natif.
#
# Formats de donnees (verifies sur le code source reel de MrRobot/Pandemic/
# Mars Attack, Hannu Viitala, Pokitto Punk Gamejam + PokittoLib) :
#   - Palette : liste de 16 entiers RGB565 (meme format que
#     display.set_palette_16bit() sur Pokitto).
#   - Pixels de Surface/Tilemap : 4 bits par pixel (indice de palette
#     0-15), 2 pixels par octet, nibble HAUT = premier pixel de la paire
#     -- CONFIRME (pas suppose) : perf_test.py de PokittoLib reference
#     explicitement framebuf.GS4_HMSB (le format standard MicroPython
#     "grayscale 4 bits, nibble haut = bit de poids fort"), meme format
#     que celui utilise ici. Chaque ligne occupe ceil(largeur/2) octets
#     (pas de bourrage si largeur paire).
import aka

# --- Constantes d'evenements (memes noms que l'API Pokitto d'origine) -----

NOEVENT = 0
KEYDOWN = 1
KEYUP   = 2

K_UP    = aka.UP
K_DOWN  = aka.DOWN
K_LEFT  = aka.LEFT
K_RIGHT = aka.RIGHT
BUT_A   = aka.A
BUT_B   = aka.B
BUT_C   = aka.C
BUT_D   = aka.D

_ALL_KEYS = (K_UP, K_DOWN, K_LEFT, K_RIGHT, BUT_A, BUT_B, BUT_C, BUT_D)


# --- Rect ------------------------------------------------------------------

class Rect:
    """Rectangle simple, aucune dependance materielle."""

    def __init__(self, x, y=0, w=0, h=0):
        if hasattr(x, "x"):   # Rect(autre_rect)
            other = x
            x, y, w, h = other.x, other.y, other.w, other.h
        elif isinstance(x, (tuple, list)):   # Rect((x,y,w,h))
            x, y, w, h = x[0], x[1], x[2], x[3]
        self.x = x
        self.y = y
        self.w = w
        self.h = h

    @property
    def width(self):  return self.w
    @property
    def height(self): return self.h
    @property
    def centerx(self): return self.x + self.w // 2
    @property
    def centery(self): return self.y + self.h // 2
    @property
    def left(self):   return self.x
    @property
    def right(self):  return self.x + self.w
    @property
    def top(self):    return self.y
    @property
    def bottom(self): return self.y + self.h

    def colliderect(self, other):
        return (self.x < other.x + other.w and other.x < self.x + self.w and
                self.y < other.y + other.h and other.y < self.y + self.h)

    def __repr__(self):
        return "Rect(%d,%d,%d,%d)" % (self.x, self.y, self.w, self.h)


# --- Palette globale (16 couleurs, format identique a Pokitto) -------------

_palette_rgb565 = [0] * 16
_palette_rgb8 = [(0, 0, 0)] * 16          # (r,g,b) 0-255, deduit du RGB565
_transparent_index = 0                     # index traite comme transparent par Surface.blit()
_background_index = 0

def _rgb565_to_rgb8(v):
    r5 = (v >> 11) & 0x1F
    g6 = (v >> 5) & 0x3F
    b5 = v & 0x1F
    r8 = (r5 * 255) // 31
    g8 = (g6 * 255) // 63
    b8 = (b5 * 255) // 31
    return (r8, g8, b8)


# --- module display ---------------------------------------------------------

class _Display:
    def init(self, *_args, **_kwargs):
        pass   # aka_hal deja initialise par main.cpp, rien a faire ici

    def set_mode(self, *_args, **_kwargs):
        return screen   # objet ecran unique (voir plus bas)

    def set_palette_16bit(self, values):
        global _palette_rgb565, _palette_rgb8
        for i in range(min(16, len(values))):
            _palette_rgb565[i] = values[i]
            _palette_rgb8[i] = _rgb565_to_rgb8(values[i])

    def flip(self):
        aka.display()

display = _Display()


# --- module draw -------------------------------------------------------------

class _Draw:
    def text(self, x, y, s, color_index=3):
        r, g, b = _palette_rgb8[color_index & 0xF]
        aka.set_color(r, g, b)
        aka.text(x, y, s)

    def set_background_color(self, idx):
        global _background_index
        _background_index = idx & 0xF

    def set_transparent_color(self, idx):
        global _transparent_index
        _transparent_index = idx & 0xF

draw = _Draw()


# --- module surface : Surface + blit ----------------------------------------

def _bytes_per_row(width):
    return (width + 1) // 2

def _get_pixel_index(pixels, width, x, y):
    row_bytes = _bytes_per_row(width)
    byte_index = y * row_bytes + (x >> 1)
    b = pixels[byte_index]
    return (b >> 4) & 0xF if (x & 1) == 0 else b & 0xF


class Surface:
    """Sprite/image en couleur indexee 4 bits (meme format que Pokitto).

    Surface(largeur, hauteur, pixels) -- pixels : bytes/bytearray, 4 bits
    par pixel, nibble haut = premier pixel de la paire.
    """

    def __init__(self, width, height, pixels):
        self.width = width
        self.height = height
        self._pixels = pixels

    def get_rect(self):
        return Rect(0, 0, self.width, self.height)

    def fill(self, color_index):
        """Remplit toute la surface avec un indice de palette (methode
        confirmee via perf_test.py de PokittoLib, meme si peu utilisee)."""
        row_bytes = _bytes_per_row(self.width)
        packed = ((color_index & 0xF) << 4) | (color_index & 0xF)
        self._pixels = bytearray([packed]) * (row_bytes * self.height)

    def blit(self, target_x, target_y, transparent=True):
        """Dessine cette surface a l'ecran, coin haut-gauche a
        (target_x, target_y). Les pixels dont l'indice de palette vaut
        _transparent_index sont sautes si transparent=True (comportement
        par defaut, coherent avec les sprites Pokitto qui utilisent
        toujours un fond transparent)."""
        w, h = self.width, self.height
        pixels = self._pixels
        last_idx = -1
        for y in range(h):
            py = target_y + y
            if py < 0 or py >= aka.height():
                continue
            for x in range(w):
                idx = _get_pixel_index(pixels, w, x, y)
                if transparent and idx == _transparent_index:
                    continue
                px = target_x + x
                if px < 0 or px >= aka.width():
                    continue
                if idx != last_idx:
                    r, g, b = _palette_rgb8[idx]
                    aka.set_color(r, g, b)
                    last_idx = idx
                aka.pixel(px, py)


class _SurfaceModule:
    Surface = Surface

surface = _SurfaceModule()


# --- ecran (objet renvoye par display.set_mode(), supporte .blit()) -------

class _Screen:
    def get_rect(self):
        return Rect(0, 0, aka.width(), aka.height())

    def blit(self, source_surface, x, y, transparent=True):
        source_surface.blit(x, y, transparent)

screen = _Screen()


# --- module mixer : Sound ----------------------------------------------------

class Sound:
    def __init__(self, *_args, **_kwargs):
        pass

    def play_sfx(self, data, length=None, loop=False):
        """data : bytes/bytearray PCM 8 bits non signe (0-255, 128=silence).
        length est accepte pour compatibilite avec l'appel Pokitto d'origine
        (sound.play_sfx(data, len(data), True)) mais ignore -- la longueur
        reelle du buffer Python est utilisee directement."""
        aka.play_pcm8(data, loop)

    def is_playing(self):
        return aka.is_sound_playing()

    def stop(self):
        aka.play_pcm8(b"\x80", False)   # relance un tampon silencieux d'1 echantillon = arret de fait


class _Mixer:
    Sound = Sound

mixer = _Mixer()


# --- module event -------------------------------------------------------------

class _Event:
    def __init__(self, etype, key):
        self.type = etype
        self.key = key


class _EventModule:
    def __init__(self):
        self._prev_mask = 0

    def poll(self):
        """Renvoie UN evenement par appel (KEYDOWN/KEYUP), ou NOEVENT s'il
        n'y en a pas en attente. Contrairement a une vraie file d'evenements,
        cette implementation se base sur aka.pressed()/aka.released() (etat
        de la frame courante) -- suffisant pour les jeux Pokitto qui pollent
        une touche a la fois par appel dans leur boucle principale."""
        pressed = aka.pressed()
        released = aka.released()
        for k in _ALL_KEYS:
            if pressed & k:
                return _Event(KEYDOWN, k)
        for k in _ALL_KEYS:
            if released & k:
                return _Event(KEYUP, k)
        return NOEVENT

event = _EventModule()
