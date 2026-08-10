/* mpconfigport.h — Configuration MicroPython pour l'app AKA (port `embed`).
 *
 * Ce fichier est lu DEUX fois :
 *   1) a la generation du paquet (make -f embed.mk) pour produire les QSTR ;
 *   2) a la compilation ESP-IDF des sources generees.
 * Il DOIT donc rester identique entre les deux etapes (cf. README).
 *
 * On part de la configuration minimale du port embed puis on active ce qui est
 * utile pour ecrire des jeux en Python, SANS tirer de module ayant besoin d'une
 * couche materielle supplementaire (pas de sys/io/time cote MicroPython : le
 * temps et les E/S passent par le module natif `aka`).
 */
#include <port/mpconfigport_common.h>

// Niveau de base : minimal (desactive toutes les options facultatives), on
// reactive explicitement ci-dessous.
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

// Coeur indispensable.
#define MICROPY_ENABLE_COMPILER                 (1)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_PY_GC                            (1)
#define MICROPY_PY_SYS                           (0)

// xtensa (ESP32-S3) : pas de capture de registres native dans le port embed,
// on utilise le repli portable base sur setjmp pour le ramasse-miettes.
#define MICROPY_GCREGS_SETJMP                    (1)

// NLR via setjmp plutot que l'assembleur xtensa (nlrxtensa.c) : ce dernier fait
// un saut direct vers nlr_push_tail hors de portee d'encodage sous ESP-IDF
// ("dangerous relocation: j: cannot encode"). setjmp est portable et robuste.
#define MICROPY_NLR_SETJMP                       (1)

// Confort pour le dev de jeux (fonctionnalites purement VM, aucune dependance
// materielle -> pas de HAL supplementaire a fournir).
#define MICROPY_FLOAT_IMPL                       (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_ERROR_REPORTING                  (MICROPY_ERROR_REPORTING_NORMAL)
#define MICROPY_ENABLE_SOURCE_LINE               (1)
#define MICROPY_PY_BUILTINS_SLICE                (1)
#define MICROPY_PY_BUILTINS_ENUMERATE            (1)
#define MICROPY_PY_BUILTINS_REVERSED             (1)
#define MICROPY_PY_BUILTINS_MIN_MAX              (1)
#define MICROPY_PY_BUILTINS_SET                  (1)
#define MICROPY_PY_BUILTINS_ROUND_INT            (1)
#define MICROPY_PY_BUILTINS_RANGE_ATTRS          (1)
