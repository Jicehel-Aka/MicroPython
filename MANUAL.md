# User Manual — MicroPython on the AKA (English)

This app turns the AKA into a console you can **program in Python**. You write
your game in a `.py` file on the SD card — no need to recompile the firmware.

*Version française : [MANUEL.md](MANUEL.md).*

---

## 1. Installation

1. Flash the `micropython` app onto the console (see [README.md](README.md)).
2. Copy the `sdcard_files/py/` folder to the root of your SD card. At boot the
   firmware runs **`/sdcard/py/main.py`**.
3. Replace `main.py` with your own game (or edit the provided example).

On power-up, the AKA automatically launches `/sdcard/py/main.py`.

---

## 2. Your first game

```python
import aka

W = aka.width()      # 320
H = aka.height()     # 240
x, y = W // 2, H // 2

while True:
    if aka.update():                    # REQUIRED once per frame
        aka.clear(0, 0, 0)             # clear the screen to black

        b = aka.buttons()
        if b & aka.LEFT:  x -= 3
        if b & aka.RIGHT: x += 3
        if b & aka.UP:    y -= 3
        if b & aka.DOWN:  y += 3

        aka.set_color(0, 220, 0)
        aka.fill_circle(x, y, 12)

        aka.display()                   # show the frame
    aka.sleep_ms(16)                    # ~60 frames per second
```

### The game loop

A frame is always built like this:

1. **`aka.update()`** — call it first. Reads the buttons **and** lets the AKA
   system menu take over if the player presses MENU. Returns `True` if your game
   should run this frame, `False` while the menu is showing (draw nothing then).
2. Draw (see §4).
3. **`aka.display()`** — send the image to the screen.
4. **`aka.sleep_ms(16)`** — pacing.

---

## 3. Buttons

`aka.buttons()` returns an integer "bit mask". Test a button with `&`:

```python
b = aka.buttons()
if b & aka.A:
    shoot()
```

| Constant | Button                |
|----------|-----------------------|
| `aka.UP` `aka.DOWN` `aka.LEFT` `aka.RIGHT` | D-pad |
| `aka.A` `aka.B` `aka.C` `aka.D` | action buttons |
| `aka.L1` `aka.R1` | shoulder triggers |
| `aka.RUN` `aka.MENU` | system buttons |

- `aka.pressed()` — buttons **just pressed** this frame (rising edge).
- `aka.released()` — buttons **just released** this frame.
- `aka.joystick()` — returns a tuple `(x, y)` (analog position).

> Holding RUN + MENU always returns to the console launcher — handled
> automatically, you don't need to do anything.

---

## 4. Drawing

The screen is **320 × 240** pixels, in color. Origin `(0, 0)` is top-left.
Colors are given as RGB (0-255).

| Function | Effect |
|----------|--------|
| `aka.set_color(r, g, b)` | set the current "pen" color |
| `aka.color(r, g, b)` | return the packed color (RGB565) without changing the pen |
| `aka.clear()` / `aka.clear(r, g, b)` | clear the whole screen (black, or a color) |
| `aka.pixel(x, y)` | one pixel |
| `aka.line(x0, y0, x1, y1)` | a line |
| `aka.hline(x, y, w)` / `aka.vline(x, y, h)` | horizontal / vertical line |
| `aka.rect(x, y, w, h)` / `aka.fill_rect(...)` | rectangle (outline / filled) |
| `aka.circle(x, y, r)` / `aka.fill_circle(...)` | circle (outline / filled) |
| `aka.triangle(x0,y0,x1,y1,x2,y2)` / `aka.fill_triangle(...)` | triangle |
| `aka.text(x, y, "text")` | draw text in the current color |
| `aka.display()` | show the built frame |
| `aka.width()` / `aka.height()` | screen size |

---

## 5. Time, vibration, misc

| Function | Effect |
|----------|--------|
| `aka.ticks_ms()` | milliseconds since boot (integer) |
| `aka.sleep_ms(ms)` | pause (lets the system breathe) |
| `aka.vibrate(ms)` | vibrate the console for `ms` milliseconds |
| `aka.screenshot()` | save a screenshot (BMP) to the SD card |
| `aka.language()` | current console language code (`"fr"`, `"en"`, …) |
| `aka.tr("KEY")` | translate a key according to the language (see aka_runtime) |

---

## 6. Help built into the system menu

When the player presses **MENU**, the console shows its system menu (Resume,
Controls, Language, Volume, Credits, Quit). You can fill in your game's
**"Controls"** and **"Credits"** screens:

```python
aka.set_controls([
    "Arrows: move",
    "A: jump",
    "B: shoot",
    "MENU: pause",
])
aka.set_credits("My Game", "My Name", "MIT", "github.com/me/mygame")
```

Call once at the start of the script. Up to 12 control lines.

---

## 7. Multiple games / your own menu

Two functions let you build your own launcher in Python:

```python
games = aka.list_py("/sdcard/py")    # list the .py files in a folder
aka.run_file("/sdcard/py/pong.py")   # run another script
```

This lets you turn `main.py` into a menu that lists and launches other `.py` files.

---

## 8. Troubleshooting

- **Frozen / blank screen**: did you call `aka.display()` after drawing, and
  `aka.update()` at the start of the frame?
- **"Fichier .py introuvable" on screen**: check that `/sdcard/py/main.py`
  exists (a `py` folder at the SD root).
- **Python error**: the message (with line number) is printed to the USB serial
  console — plug in the AKA and open `idf.py monitor` to read it.
- **Game is slow**: do less work per frame, or increase `aka.sleep_ms`.
