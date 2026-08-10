/* modaka.c — Module MicroPython natif `aka` : expose le materiel AKA (ecran,
 * boutons, audio, temps, SD) aux scripts Python.
 *
 * IMPORTANT : ce fichier est aussi PRE-TRAITE cote hote lors de la generation
 * des QSTR/moduledefs. Il ne doit donc inclure QUE des en-tetes portables :
 * les en-tetes du dossier py, aka_hal.h (sans dependance), et la libc. Toute la mecanique materielle
 * est derriere aka_hal_* (implementee dans aka_hal.cpp, cote ESP-IDF). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "py/compile.h"
#include "py/gc.h"
#include "py/runtime.h"
#include "py/stackctrl.h"

#include "aka_hal.h"
#include "aka_keys.h"

// Marge de securite de pile laissee a MicroPython (octets). Doit rester
// inferieure a la pile de la tache app_main (cf. sdkconfig.defaults).
#define AKA_MP_STACK_LIMIT (16 * 1024)

// ---------------------------------------------------------------------------
// Amorcage : init de la VM + execution d'un fichier .py depuis la SD.
// ---------------------------------------------------------------------------
void aka_boot_init(void *heap, size_t heap_size) {
    mp_stack_ctrl_init();
    mp_stack_set_limit(AKA_MP_STACK_LIMIT);
    gc_init(heap, (uint8_t *)heap + heap_size);
    mp_init();
}

// Compile puis execute un texte source ; affiche l'exception si non rattrapee.
static void aka_exec_named(const char *name, const char *src, size_t len) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(qstr_from_str(name), src, len, 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}

// Lit tout un fichier en memoire (heap C) ; renvoie NULL si echec.
static char *aka_read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t rd = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[rd] = '\0';
    if (out_len) *out_len = rd;
    return buf;
}

static void aka_exec_file(const char *path) {
    size_t len = 0;
    char *src = aka_read_file(path, &len);
    if (!src) {
        mp_printf(&mp_plat_print, "aka: fichier introuvable: %s\n", path);
        aka_hal_set_color(aka_hal_color(255, 0, 0));
        aka_hal_text(8, 8, "Fichier .py introuvable:");
        aka_hal_text(8, 24, path);
        aka_hal_display();
        return;
    }
    aka_exec_named(path, src, len);
    free(src);
}

// Appele par main.cpp apres aka_boot_init.
void aka_boot_exec(const char *path) { aka_exec_file(path); }

// ---------------------------------------------------------------------------
// Fonctions du module `aka`.
// ---------------------------------------------------------------------------
static mp_obj_t aka_color(mp_obj_t r, mp_obj_t g, mp_obj_t b) {
    return mp_obj_new_int(aka_hal_color(
        (uint8_t)mp_obj_get_int(r), (uint8_t)mp_obj_get_int(g), (uint8_t)mp_obj_get_int(b)));
}
static MP_DEFINE_CONST_FUN_OBJ_3(aka_color_obj, aka_color);

static mp_obj_t aka_set_color(mp_obj_t r, mp_obj_t g, mp_obj_t b) {
    aka_hal_set_color(aka_hal_color(
        (uint8_t)mp_obj_get_int(r), (uint8_t)mp_obj_get_int(g), (uint8_t)mp_obj_get_int(b)));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(aka_set_color_obj, aka_set_color);

static mp_obj_t aka_clear(size_t n_args, const mp_obj_t *args) {
    if (n_args == 3) {
        aka_hal_clear(aka_hal_color((uint8_t)mp_obj_get_int(args[0]),
                                    (uint8_t)mp_obj_get_int(args[1]),
                                    (uint8_t)mp_obj_get_int(args[2])));
    } else {
        aka_hal_clear(aka_hal_color(0, 0, 0));
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aka_clear_obj, 0, 3, aka_clear);

static mp_obj_t aka_pixel(mp_obj_t x, mp_obj_t y) {
    aka_hal_pixel(mp_obj_get_int(x), mp_obj_get_int(y));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(aka_pixel_obj, aka_pixel);

static mp_obj_t aka_hline(mp_obj_t x, mp_obj_t y, mp_obj_t w) {
    aka_hal_hline(mp_obj_get_int(x), mp_obj_get_int(y), mp_obj_get_int(w));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(aka_hline_obj, aka_hline);

static mp_obj_t aka_vline(mp_obj_t x, mp_obj_t y, mp_obj_t h) {
    aka_hal_vline(mp_obj_get_int(x), mp_obj_get_int(y), mp_obj_get_int(h));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(aka_vline_obj, aka_vline);

static mp_obj_t aka_line(size_t n_args, const mp_obj_t *args) {
    aka_hal_line(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]),
                 mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aka_line_obj, 4, 4, aka_line);

static mp_obj_t aka_rect(size_t n_args, const mp_obj_t *args) {
    aka_hal_rect(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]),
                 mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aka_rect_obj, 4, 4, aka_rect);

static mp_obj_t aka_fill_rect(size_t n_args, const mp_obj_t *args) {
    aka_hal_fill_rect(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]),
                      mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aka_fill_rect_obj, 4, 4, aka_fill_rect);

static mp_obj_t aka_circle(mp_obj_t x, mp_obj_t y, mp_obj_t r) {
    aka_hal_circle(mp_obj_get_int(x), mp_obj_get_int(y), mp_obj_get_int(r));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(aka_circle_obj, aka_circle);

static mp_obj_t aka_fill_circle(mp_obj_t x, mp_obj_t y, mp_obj_t r) {
    aka_hal_fill_circle(mp_obj_get_int(x), mp_obj_get_int(y), mp_obj_get_int(r));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(aka_fill_circle_obj, aka_fill_circle);

static mp_obj_t aka_triangle(size_t n_args, const mp_obj_t *args) {
    aka_hal_triangle(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]),
                     mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                     mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aka_triangle_obj, 6, 6, aka_triangle);

static mp_obj_t aka_fill_triangle(size_t n_args, const mp_obj_t *args) {
    aka_hal_fill_triangle(mp_obj_get_int(args[0]), mp_obj_get_int(args[1]),
                          mp_obj_get_int(args[2]), mp_obj_get_int(args[3]),
                          mp_obj_get_int(args[4]), mp_obj_get_int(args[5]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aka_fill_triangle_obj, 6, 6, aka_fill_triangle);

static mp_obj_t aka_text(mp_obj_t x, mp_obj_t y, mp_obj_t s) {
    aka_hal_text(mp_obj_get_int(x), mp_obj_get_int(y), mp_obj_str_get_str(s));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(aka_text_obj, aka_text);

static mp_obj_t aka_display(void) { aka_hal_display(); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_display_obj, aka_display);

static mp_obj_t aka_width(void)  { return mp_obj_new_int(aka_hal_width()); }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_width_obj, aka_width);
static mp_obj_t aka_height(void) { return mp_obj_new_int(aka_hal_height()); }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_height_obj, aka_height);

static mp_obj_t aka_update(void)   { return mp_obj_new_bool(aka_hal_update()); }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_update_obj, aka_update);
static mp_obj_t aka_buttons(void)  { return mp_obj_new_int_from_uint(aka_hal_buttons()); }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_buttons_obj, aka_buttons);
static mp_obj_t aka_pressed(void)  { return mp_obj_new_int_from_uint(aka_hal_pressed()); }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_pressed_obj, aka_pressed);
static mp_obj_t aka_released(void) { return mp_obj_new_int_from_uint(aka_hal_released()); }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_released_obj, aka_released);

static mp_obj_t aka_joystick(void) {
    int x = 0, y = 0;
    aka_hal_joystick(&x, &y);
    mp_obj_t t[2] = { mp_obj_new_int(x), mp_obj_new_int(y) };
    return mp_obj_new_tuple(2, t);
}
static MP_DEFINE_CONST_FUN_OBJ_0(aka_joystick_obj, aka_joystick);

static mp_obj_t aka_ticks_ms(void) { return mp_obj_new_int_from_uint(aka_hal_ticks_ms()); }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_ticks_ms_obj, aka_ticks_ms);

static mp_obj_t aka_sleep_ms(mp_obj_t ms) {
    aka_hal_sleep_ms((uint32_t)mp_obj_get_int(ms));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(aka_sleep_ms_obj, aka_sleep_ms);

static mp_obj_t aka_vibrate(mp_obj_t ms) {
    aka_hal_vibrate((uint32_t)mp_obj_get_int(ms));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(aka_vibrate_obj, aka_vibrate);

static mp_obj_t aka_language(void) {
    const char *l = aka_hal_language();
    return mp_obj_new_str(l, strlen(l));
}
static MP_DEFINE_CONST_FUN_OBJ_0(aka_language_obj, aka_language);

static mp_obj_t aka_tr(mp_obj_t key) {
    const char *v = aka_hal_tr(mp_obj_str_get_str(key));
    return mp_obj_new_str(v, strlen(v));
}
static MP_DEFINE_CONST_FUN_OBJ_1(aka_tr_obj, aka_tr);

static mp_obj_t aka_screenshot(void) { return mp_obj_new_bool(aka_hal_screenshot()); }
static MP_DEFINE_CONST_FUN_OBJ_0(aka_screenshot_obj, aka_screenshot);

static mp_obj_t aka_run_file(mp_obj_t path) {
    aka_exec_file(mp_obj_str_get_str(path));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(aka_run_file_obj, aka_run_file);

static mp_obj_t aka_set_controls(mp_obj_t lines) {
    size_t n = 0;
    mp_obj_t *items = NULL;
    mp_obj_get_array(lines, &n, &items);
    const char *buf[12];
    if (n > 12) n = 12;
    for (size_t i = 0; i < n; ++i) buf[i] = mp_obj_str_get_str(items[i]);
    aka_hal_set_controls(buf, (int)n);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(aka_set_controls_obj, aka_set_controls);

static mp_obj_t aka_set_credits(size_t n_args, const mp_obj_t *args) {
    aka_hal_set_credits(mp_obj_str_get_str(args[0]), mp_obj_str_get_str(args[1]),
                        mp_obj_str_get_str(args[2]), mp_obj_str_get_str(args[3]));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aka_set_credits_obj, 4, 4, aka_set_credits);

static mp_obj_t aka_list_py(mp_obj_t dir) {
    const char *d = mp_obj_str_get_str(dir);
    mp_obj_t list = mp_obj_new_list(0, NULL);
    DIR *dp = opendir(d);
    if (dp) {
        struct dirent *e;
        while ((e = readdir(dp)) != NULL) {
            size_t l = strlen(e->d_name);
            if (l > 3 && strcmp(e->d_name + l - 3, ".py") == 0) {
                mp_obj_list_append(list, mp_obj_new_str(e->d_name, l));
            }
        }
        closedir(dp);
    }
    return list;
}
static MP_DEFINE_CONST_FUN_OBJ_1(aka_list_py_obj, aka_list_py);

// ---------------------------------------------------------------------------
// Table du module.
// ---------------------------------------------------------------------------
static const mp_rom_map_elem_t aka_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),     MP_ROM_QSTR(MP_QSTR_aka) },
    // graphismes
    { MP_ROM_QSTR(MP_QSTR_color),        MP_ROM_PTR(&aka_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_color),    MP_ROM_PTR(&aka_set_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear),        MP_ROM_PTR(&aka_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel),        MP_ROM_PTR(&aka_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_hline),        MP_ROM_PTR(&aka_hline_obj) },
    { MP_ROM_QSTR(MP_QSTR_vline),        MP_ROM_PTR(&aka_vline_obj) },
    { MP_ROM_QSTR(MP_QSTR_line),         MP_ROM_PTR(&aka_line_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect),         MP_ROM_PTR(&aka_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_rect),    MP_ROM_PTR(&aka_fill_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_circle),       MP_ROM_PTR(&aka_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_circle),  MP_ROM_PTR(&aka_fill_circle_obj) },
    { MP_ROM_QSTR(MP_QSTR_triangle),     MP_ROM_PTR(&aka_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_triangle),MP_ROM_PTR(&aka_fill_triangle_obj) },
    { MP_ROM_QSTR(MP_QSTR_text),         MP_ROM_PTR(&aka_text_obj) },
    { MP_ROM_QSTR(MP_QSTR_display),      MP_ROM_PTR(&aka_display_obj) },
    { MP_ROM_QSTR(MP_QSTR_width),        MP_ROM_PTR(&aka_width_obj) },
    { MP_ROM_QSTR(MP_QSTR_height),       MP_ROM_PTR(&aka_height_obj) },
    // entrees
    { MP_ROM_QSTR(MP_QSTR_update),       MP_ROM_PTR(&aka_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_buttons),      MP_ROM_PTR(&aka_buttons_obj) },
    { MP_ROM_QSTR(MP_QSTR_pressed),      MP_ROM_PTR(&aka_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_released),     MP_ROM_PTR(&aka_released_obj) },
    { MP_ROM_QSTR(MP_QSTR_joystick),     MP_ROM_PTR(&aka_joystick_obj) },
    // temps / divers
    { MP_ROM_QSTR(MP_QSTR_ticks_ms),     MP_ROM_PTR(&aka_ticks_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_ms),     MP_ROM_PTR(&aka_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_vibrate),      MP_ROM_PTR(&aka_vibrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_language),     MP_ROM_PTR(&aka_language_obj) },
    { MP_ROM_QSTR(MP_QSTR_tr),           MP_ROM_PTR(&aka_tr_obj) },
    { MP_ROM_QSTR(MP_QSTR_screenshot),   MP_ROM_PTR(&aka_screenshot_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_file),     MP_ROM_PTR(&aka_run_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_list_py),      MP_ROM_PTR(&aka_list_py_obj) },
    // aide integree au menu systeme
    { MP_ROM_QSTR(MP_QSTR_set_controls), MP_ROM_PTR(&aka_set_controls_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_credits),  MP_ROM_PTR(&aka_set_credits_obj) },
    // constantes touches
    { MP_ROM_QSTR(MP_QSTR_UP),    MP_ROM_INT(AKA_KEY_UP) },
    { MP_ROM_QSTR(MP_QSTR_DOWN),  MP_ROM_INT(AKA_KEY_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_LEFT),  MP_ROM_INT(AKA_KEY_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_RIGHT), MP_ROM_INT(AKA_KEY_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_A),     MP_ROM_INT(AKA_KEY_A) },
    { MP_ROM_QSTR(MP_QSTR_B),     MP_ROM_INT(AKA_KEY_B) },
    { MP_ROM_QSTR(MP_QSTR_C),     MP_ROM_INT(AKA_KEY_C) },
    { MP_ROM_QSTR(MP_QSTR_D),     MP_ROM_INT(AKA_KEY_D) },
    { MP_ROM_QSTR(MP_QSTR_RUN),   MP_ROM_INT(AKA_KEY_RUN) },
    { MP_ROM_QSTR(MP_QSTR_MENU),  MP_ROM_INT(AKA_KEY_MENU) },
    { MP_ROM_QSTR(MP_QSTR_L1),    MP_ROM_INT(AKA_KEY_L1) },
    { MP_ROM_QSTR(MP_QSTR_R1),    MP_ROM_INT(AKA_KEY_R1) },
};
static MP_DEFINE_CONST_DICT(aka_module_globals, aka_module_globals_table);

const mp_obj_module_t aka_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&aka_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_aka, aka_module);
