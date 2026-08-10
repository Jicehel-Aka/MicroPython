# urandom.py — implementation MINIMALE pure Python de getrandbits(), pour ne
# pas dependre d'un module `random`/`urandom` natif dont le nom exact du
# drapeau de configuration (MICROPY_PY_RANDOM vs MICROPY_PY_URANDOM selon la
# version de MicroPython) n'est pas garanti pour ce build precis -- evite un
# cycle de recompilation complet juste pour verifier un nom de macro.
#
# Generateur xorshift32 (Marsaglia) : rapide, tres largement suffisant pour
# du gameplay (pas un usage cryptographique). Graine initialisee depuis
# aka.ticks_ms() au premier appel, pour varier d'une partie a l'autre.
import aka

_state = 0

def _next32():
    global _state
    if _state == 0:
        _state = aka.ticks_ms() | 1   # jamais zero (xorshift bloquerait sinon)
    x = _state
    x ^= (x << 13) & 0xFFFFFFFF
    x ^= (x >> 17)
    x ^= (x << 5) & 0xFFFFFFFF
    x &= 0xFFFFFFFF
    _state = x
    return x

def getrandbits(n):
    """Renvoie un entier de n bits aleatoires (0 <= n <= 32)."""
    if n <= 0:
        return 0
    if n >= 32:
        return _next32()
    return _next32() >> (32 - n)

def seed(n=None):
    """Reinitialise la graine (optionnel -- appele par certains jeux)."""
    global _state
    _state = (int(n) & 0xFFFFFFFF) or 1 if n is not None else (aka.ticks_ms() | 1)

def randint(a, b):
    """Entier aleatoire dans [a, b] inclus (pas garanti par tous les jeux,
    fourni par completude)."""
    span = b - a + 1
    if span <= 0:
        return a
    return a + (_next32() % span)

def random():
    """Flottant aleatoire dans [0.0, 1.0) (fourni par completude)."""
    return _next32() / 4294967296.0
