# umachine.py — sous-ensemble minimal du module `umachine` de Pokitto,
# implemente en pur Python par-dessus le module natif `aka` (SD deja montee,
# E/S fichier standard MicroPython).
import aka

_COOKIE_DIR = "/sdcard/cookies"

def time_ms():
    """Equivalent de umachine.time_ms() (Pokitto) -- simple alias."""
    return aka.ticks_ms()


class Cookie:
    """Sauvegarde persistante simple, un fichier par nom de cookie.

    Usage (identique a l'API Pokitto d'origine) :
        data = bytearray(1)
        c = umachine.Cookie("MONJEU", data)
        c.load()    # remplit data depuis la sauvegarde (si elle existe)
        data[0] = 42
        c.save()    # ecrit data sur la sauvegarde
    """

    def __init__(self, name, buffer):
        self._name = name
        self._buffer = buffer   # le jeu garde sa propre reference vers ce bytearray

    def _path(self):
        return "%s/%s.dat" % (_COOKIE_DIR, self._name)

    def _ensure_dir(self):
        try:
            import os
            try:
                os.stat(_COOKIE_DIR)
            except OSError:
                os.mkdir(_COOKIE_DIR)
        except Exception:
            pass   # SD non disponible ou deja present -- on continue quand meme

    def load(self):
        """Remplit le buffer depuis la sauvegarde. Ne fait rien (buffer
        inchange) si le fichier n'existe pas encore (premiere partie)."""
        try:
            with open(self._path(), "rb") as f:
                data = f.read(len(self._buffer))
                n = min(len(data), len(self._buffer))
                for i in range(n):
                    self._buffer[i] = data[i]
        except OSError:
            pass   # pas de sauvegarde existante -- comportement normal au premier lancement

    def save(self):
        """Ecrit le buffer courant sur la sauvegarde."""
        self._ensure_dir()
        try:
            with open(self._path(), "wb") as f:
                f.write(self._buffer)
        except OSError:
            pass   # echec silencieux (carte pleine/retiree) -- coherent avec le style aka_runtime
