# NCURSES – syntaksguide (med pygame-sammenligninger)

> Opsummeret og omskrevet ud fra *NCURSES Programming HOWTO* af Pradeep Padala (v1.7.1, 2002).
> Alle C-eksempler herunder er testet og kompilerer med ncurses 6.4.

## Indhold

1. [Hvad er ncurses – set med pygame-øjne](#1-hvad-er-ncurses--set-med-pygame-øjne)
2. [Lidt C-syntaks du skal kende først](#2-lidt-c-syntaks-du-skal-kende-først)
3. [Kompilering](#3-kompilering)
4. [Hello World – skelettet i alle programmer](#4-hello-world--skelettet-i-alle-programmer)
5. [Navngivningsreglen (den vigtigste side i guiden)](#5-navngivningsreglen-den-vigtigste-side-i-guiden)
6. [Initialisering](#6-initialisering)
7. [Output: at skrive til skærmen](#7-output-at-skrive-til-skærmen)
8. [Input: at læse fra brugeren](#8-input-at-læse-fra-brugeren)
9. [Attributter (fed, understreget osv.)](#9-attributter-fed-understreget-osv)
10. [Vinduer](#10-vinduer)
11. [Farver](#11-farver)
12. [Taster](#12-taster)
13. [Mus](#13-mus)
14. [Skærm-manipulation](#14-skærm-manipulation)
15. [Diverse: markør, pause fra curses, ACS-tegn](#15-diverse-markør-pause-fra-curses-acs-tegn)
16. [Panel-biblioteket](#16-panel-biblioteket)
17. [Menu-biblioteket](#17-menu-biblioteket)
18. [Form-biblioteket](#18-form-biblioteket)
19. [Komplet mini-spil: C vs. pygame vs. Python-curses](#19-komplet-mini-spil-c-vs-pygame-vs-python-curses)
20. [Typiske fejl](#20-typiske-fejl)
21. [Hurtig-reference](#21-hurtig-reference)

---

## 1. Hvad er ncurses – set med pygame-øjne

ncurses er et C-bibliotek til at lave "grafiske" programmer **inde i terminalen**: menuer, bokse, farver, tastatur- og musestyring. Tænk på programmer som `htop`, `nano` eller `vim`.

Den største forskel til pygame er, at skærmen ikke består af pixels, men af **tegn-celler** i et gitter (typisk 80×24 eller hvor stor din terminal er).

| Idé | pygame | ncurses |
|---|---|---|
| Start | `pygame.init()` | `initscr()` |
| Hovedvinduet | `screen = pygame.display.set_mode(...)` | `stdscr` (oprettes automatisk) |
| Mindre flade/område | `pygame.Surface((w, h))` | `newwin(h, w, y, x)` |
| Tegn noget | `screen.blit(tekst, (x, y))` | `mvprintw(y, x, "tekst")` |
| Vis ændringerne | `pygame.display.flip()` | `refresh()` |
| Ryd skærmen | `screen.fill((0,0,0))` | `erase()` / `clear()` |
| Læs input | `pygame.event.get()` | `getch()` |
| Afslut | `pygame.quit()` | `endwin()` |
| Koordinater | **(x, y)** i pixels | **(y, x)** i tegn – rækken først! |

---

## 2. Lidt C-syntaks du skal kende først

HOWTO'en er skrevet i C. Her er de ting, der ser mærkeligst ud, hvis man kommer fra Python:

**Pointere (`*`)** – `WINDOW *win` betyder "en pointer til et WINDOW". Tænk på det som en reference til et objekt. I Python er alle variabler i praksis referencer, så du bruger det allerede – C skriver det bare ud.

```c
WINDOW *win = newwin(10, 30, 2, 4);   /* win "peger på" vinduet */
```

**Adresse-operatoren (`&`)** – I C kan en funktion ikke returnere flere værdier, så i stedet giver man den *adressen* på sine variabler, og så skriver den resultatet direkte ind:

```c
MEVENT event;
getmouse(&event);        /* getmouse fylder "event" ud for dig */
```
I Python ville det i stedet se sådan ud: `event = getmouse()`.

**Makroer (ingen `&`)** – Nogle ncurses-"funktioner" er faktisk makroer, der bliver skrevet om før kompilering. De kan ændre variabler direkte, så du skal **ikke** bruge `&`:

```c
int rows, cols;
getmaxyx(stdscr, rows, cols);   /* rigtigt – ingen & */
```
Python-versionen: `rows, cols = stdscr.getmaxyx()`.

**Bitvis OR (`|`)** – bruges til at kombinere flag, præcis som i pygame:

```c
attron(A_BOLD | A_UNDERLINE);                        /* ncurses */
```
```python
pygame.display.set_mode((640, 480), pygame.RESIZABLE | pygame.SCALED)  # pygame
```

**Tildeling inde i en betingelse** – meget almindeligt i ncurses-kode:

```c
while ((ch = getch()) != KEY_F(1)) { ... }
```
Svarer til Pythons "walrus"-operator: `while (ch := stdscr.getch()) != curses.KEY_F1:`

**`switch` / `case`** – svarer til Pythons `match`/`case`. Husk `break`, ellers "falder" koden videre ned i næste case.

**Ingen garbage collector** – alt du opretter med `new...` skal du selv frigøre med `del...`/`free...`:

| Opret | Frigør |
|---|---|
| `newwin()` | `delwin()` |
| `new_panel()` | `del_panel()` |
| `new_item()` / `new_menu()` | `free_item()` / `free_menu()` |
| `new_field()` / `new_form()` | `free_field()` / `free_form()` |
| `calloc()` / `malloc()` | `free()` |

**`TRUE` / `FALSE` / `OK` / `ERR`** – konstanter som ncurses definerer for dig. De fleste funktioner returnerer `OK` eller `ERR`.

---

## 3. Kompilering

```bash
gcc mitprogram.c -o mitprogram -lncurses
./mitprogram
```

`-lncurses` fortæller compileren, at den skal linke til ncurses-biblioteket. `ncurses.h` inkluderer allerede `stdio.h`, så den behøver du ikke selv.

Til de ekstra biblioteker skal de stå **før** `-lncurses`:

```bash
gcc prog.c -o prog -lpanel -lncurses
gcc prog.c -o prog -lmenu  -lncurses
gcc prog.c -o prog -lform  -lncurses
```

> **På Windows:** ncurses findes ikke direkte i Windows' egen konsol. Den nemmeste vej er **WSL** (Ubuntu i Windows):
> ```bash
> sudo apt install build-essential libncurses-dev
> ```
> Alternativt findes **PDCurses**, som har næsten samme syntaks. Vil du prøve idéerne i Python, kan du bruge det indbyggede `curses`-modul – på Windows skal du først køre `pip install windows-curses`.

---

## 4. Hello World – skelettet i alle programmer

```c
#include <ncurses.h>

int main(void)
{
    initscr();                  /* start curses-mode        */
    printw("Hej verden!");      /* skriv til den virtuelle skærm */
    refresh();                  /* vis den på den rigtige skærm  */
    getch();                    /* vent på en tast          */
    endwin();                   /* afslut curses-mode       */
    return 0;
}
```

Samme struktur i pygame:

```python
pygame.init()                               # initscr()
screen.blit(font.render("Hej verden!", True, (255,255,255)), (0, 0))  # printw()
pygame.display.flip()                       # refresh()
# ... vent på et event ...                  # getch()
pygame.quit()                               # endwin()
```

### De tre vigtige funktioner

**`initscr()`** – starter curses. Skal kaldes før alt andet. Opretter standardvinduet `stdscr` og sætter `LINES` og `COLS` (terminalens størrelse).

**`refresh()`** – den der forvirrer alle i starten. `printw()` skriver **ikke** direkte på skærmen, men i en usynlig buffer. Først når du kalder `refresh()`, bliver ændringerne vist. Det er præcis samme idé som i pygame, hvor du tegner på `screen` og så kalder `pygame.display.flip()`.

Faktisk er ncurses lidt smartere: `refresh()` sammenligner bufferen med det der allerede står på skærmen og sender **kun de ændrede tegn** – mere som `pygame.display.update(rects)` end `flip()`.

> Den klassiske begynderfejl: at glemme `refresh()` og undre sig over, at intet vises.

**`endwin()`** – sætter terminalen tilbage til normal. Glemmer du den, opfører terminalen sig mærkeligt efter programmet er lukket (skriv `reset` for at redde den).

---

## 5. Navngivningsreglen (den vigtigste side i guiden)

Næsten alle output- og input-funktioner findes i **fire varianter**. Lærer du dette mønster, kan du gætte navnet på hundredvis af funktioner:

| Funktion | Hvilket vindue | Hvor |
|---|---|---|
| `printw(fmt, ...)` | `stdscr` | ved markøren |
| `mvprintw(y, x, fmt, ...)` | `stdscr` | flyt til (y, x) først |
| `wprintw(win, fmt, ...)` | `win` | ved markøren i `win` |
| `mvwprintw(win, y, x, fmt, ...)` | `win` | flyt til (y, x) i `win` |

Reglen:
- **`mv`**-præfiks → tager `y, x` som ekstra argumenter ("move").
- **`w`**-præfiks → tager et `WINDOW *` som første argument.
- **`mvw`** → begge dele, i rækkefølgen `(win, y, x, ...)`.

Samme mønster gælder fx:

```
addch    mvaddch    waddch    mvwaddch
addstr   mvaddstr   waddstr   mvwaddstr
getch    mvgetch    wgetch    mvwgetch
chgat    mvchgat    wchgat    mvwchgat
```

De `w`-løse versioner er normalt bare makroer for `w`-versionen med `stdscr`: `printw(...)` er i virkeligheden `wprintw(stdscr, ...)`.

> ⚠️ **Y FØR X!** pygame bruger `(x, y)`, ncurses bruger `(y, x)` – altså `(række, kolonne)`. Det er den fejl, du kommer til at lave oftest.

---

## 6. Initialisering

Disse kaldes typisk lige efter `initscr()`:

```c
initscr();
cbreak();              /* eller raw()                             */
noecho();              /* vis ikke det brugeren taster            */
keypad(stdscr, TRUE);  /* aktiver piletaster, F1-F12 osv.          */
curs_set(0);           /* skjul markøren (valgfrit)                */
```

| Funktion | Hvad den gør |
|---|---|
| `raw()` | Taster sendes med det samme – også Ctrl+C og Ctrl+Z (de lukker **ikke** programmet). |
| `cbreak()` | Taster sendes med det samme, men Ctrl+C/Ctrl+Z virker som normalt. |
| `echo()` / `noecho()` | Slår automatisk visning af tastede tegn til/fra. Næsten alle programmer bruger `noecho()`. |
| `keypad(win, TRUE)` | Gør at piletaster returneres som `KEY_UP` osv. i stedet for mærkelige escape-koder. |
| `halfdelay(n)` | Som `cbreak()`, men `getch()` venter kun `n` tiendedele sekund og returnerer så `ERR`. |

Uden `cbreak()`/`raw()` venter terminalen på Enter, før dit program ser tasterne – ubrugeligt til spil.

### Ekstra (ikke fra HOWTO'en, men vigtigt til spil)

I pygame kører din game loop hele tiden, uanset om brugeren trykker på noget. I ncurses **blokerer** `getch()` som standard – programmet står stille, til der kommer en tast. Det løser du sådan:

```c
nodelay(stdscr, TRUE);  /* getch() returnerer ERR med det samme hvis ingen tast */
timeout(16);            /* getch() venter max 16 ms (~60 FPS) – svarer lidt til clock.tick(60) */
```

---

## 7. Output: at skrive til skærmen

Der er tre familier af output-funktioner:

### 7.1 `addch()` – ét tegn

```c
addch('@');
mvaddch(5, 10, '@');                   /* række 5, kolonne 10 */
mvaddch(5, 10, '@' | A_BOLD | A_UNDERLINE);   /* tegn + attributter i ét */
```

Det specielle ved `addch` er, at du kan OR'e attributter direkte ind i tegnet. `move(y, x); addch(ch);` er det samme som `mvaddch(y, x, ch);`.

### 7.2 `printw()` – formateret tekst (som `printf`)

```c
int point = 42;
printw("Point: %d", point);
mvprintw(0, 0, "Navn: %s, liv: %d", navn, liv);
```

Formatkoderne er de samme som i C's `printf` – og som Pythons gamle `%`-formatering:

| C | Python |
|---|---|
| `printw("%d point", p)` | `f"{p} point"` eller `"%d point" % p` |
| `%s` | streng |
| `%d` | heltal |
| `%f` / `%.2f` | decimaltal |
| `%c` | ét tegn |

**Centrér tekst på skærmen:**

```c
char besked[] = "Game Over";
int rows, cols;
getmaxyx(stdscr, rows, cols);
mvprintw(rows / 2, (cols - strlen(besked)) / 2, "%s", besked);
```
(Kræver `#include <string.h>` for `strlen`.)

pygame-versionen:
```python
tekst = font.render("Game Over", True, (255, 255, 255))
screen.blit(tekst, tekst.get_rect(center=screen.get_rect().center))
```

### 7.3 `addstr()` – en simpel streng

```c
addstr("Hej");
mvaddstr(3, 4, "Hej");
addnstr("Hej med dig", 3);   /* skriver kun de første 3 tegn: "Hej" */
```

De tre familier kan bruges i flæng – det er en smagssag.

---

## 8. Input: at læse fra brugeren

| Familie | Svarer til | Bruges til |
|---|---|---|
| `getch()` | `getchar()` | én tast |
| `scanw()` | `scanf()` | formateret input (fx et tal) |
| `getstr()` | `fgets()` | en hel linje tekst |

```c
int ch = getch();                /* returnerer en int, IKKE en char */

int alder;
mvprintw(2, 0, "Alder: ");
scanw("%d", &alder);             /* bemærk & – scanw skal have adressen */

char navn[80];
mvprintw(3, 0, "Navn: ");
getnstr(navn, 79);               /* max 79 tegn – sikrere end getstr() */
```

> Brug `getnstr()` frem for `getstr()`. `getstr()` har ingen grænse, så hvis brugeren skriver mere end dit array kan rumme, skriver den ud over hukommelsen.

Husk: `scanw` og `getstr` vil gerne have `echo()` slået til, ellers kan brugeren ikke se, hvad de skriver.

---

## 9. Attributter (fed, understreget osv.)

```c
attron(A_BOLD);          /* slå fed TIL                  */
printw("Fed tekst");
attroff(A_BOLD);         /* slå fed FRA                  */

attron(A_REVERSE | A_BLINK);   /* flere på én gang     */
```

| Attribut | Effekt |
|---|---|
| `A_NORMAL` | Normal |
| `A_STANDOUT` | Terminalens "bedste" fremhævning |
| `A_UNDERLINE` | Understreget |
| `A_REVERSE` | Byt forgrund og baggrund (bruges til markering i menuer) |
| `A_BLINK` | Blinkende |
| `A_DIM` | Halv lysstyrke |
| `A_BOLD` | Fed / ekstra lys |
| `A_PROTECT` | Beskyttet |
| `A_INVIS` | Usynlig |
| `A_ALTCHARSET` | Alternativt tegnsæt (streg-tegn) |
| `A_CHARTEXT` | Bitmaske til at trække selve tegnet ud |
| `COLOR_PAIR(n)` | Farvepar nummer n (se [Farver](#11-farver)) |

I pygame svarer det nogenlunde til `font.set_bold(True)`, `font.set_underline(True)` osv.

### attron vs. attrset

| Funktion | Virkning |
|---|---|
| `attron(a)` | Tilføjer `a` til de nuværende attributter |
| `attroff(a)` | Fjerner `a` |
| `attrset(a)` | **Overskriver** alle attributter med `a` |
| `standend()` | Nulstiller alt (= `attrset(A_NORMAL)`) |

Bland dem ikke tilfældigt – så mister man let overblikket over, hvad der er slået til.

Varianter: `wattron(win, ...)`, `attr_get()` (læs nuværende attributter), og `attr_on()`/`attr_set()` som tager typen `attr_t`.

### chgat – ændr tekst der allerede står på skærmen

```c
/* chgat(antal_tegn, attribut, farvepar, NULL)  – -1 = resten af linjen */
chgat(-1, A_REVERSE, 0, NULL);

/* mvchgat(y, x, antal, attribut, farvepar, NULL) */
mvchgat(0, 0, -1, A_BLINK, 1, NULL);
```

Det smarte ved `chgat` er, at markøren **ikke flytter sig**. Sidste argument er altid `NULL`. Brug farvepar `0`, hvis du ikke vil have farve.

---

## 10. Vinduer

Et `WINDOW` er et selvstændigt område af skærmen, du kan tegne i og opdatere for sig. Det er ncurses' svar på en `pygame.Surface`.

```c
WINDOW *win = newwin(højde, bredde, start_y, start_x);
box(win, 0, 0);                       /* ramme med standard-tegn */
mvwprintw(win, 1, 2, "Hej");          /* koordinater er RELATIVE til vinduet */
wrefresh(win);                        /* opdater kun dette vindue */
...
delwin(win);                          /* frigør hukommelsen */
```

| pygame | ncurses |
|---|---|
| `surf = pygame.Surface((w, h))` | `win = newwin(h, w, y, x)` |
| `pygame.draw.rect(surf, farve, surf.get_rect(), 1)` | `box(win, 0, 0)` |
| `surf.blit(tekst, (x, y))` | `mvwprintw(win, y, x, "...")` |
| `screen.blit(surf, (x, y))` + `flip()` | `wrefresh(win)` |
| `surf.get_size()` | `getmaxyx(win, h, w)` |

Bemærk argumentrækkefølgen: pygame siger `(bredde, højde)`, `newwin` siger `(højde, bredde, y, x)`.

`LINES` og `COLS` indeholder terminalens størrelse, så man kan centrere et vindue:

```c
int h = 7, w = 30;
WINDOW *win = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
```

### Ramme-funktioner

```c
box(win, 0, 0);          /* 0, 0 = brug standard lodret/vandret streg */

/* wborder(win, venstre, højre, top, bund, øv-venstre, øv-højre, ne-venstre, ne-højre) */
wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
```
Giver:
```
+------------+
|            |
|            |
+------------+
```

**Slette en ramme:** `box(win, ' ', ' ')` efterlader de fire hjørner! Brug i stedet:

```c
wborder(win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
wrefresh(win);
delwin(win);
```

**Linjer:**

```c
mvhline(y, x, '-', længde);    /* vandret linje */
mvvline(y, x, '|', længde);    /* lodret linje  */
```

### Komplet eksempel: vindue + farver

```c
#include <ncurses.h>
#include <string.h>

int main(void)
{
    initscr();
    cbreak();
    noecho();

    if (has_colors() == FALSE) {
        endwin();
        printf("Din terminal understøtter ikke farver\n");
        return 1;
    }
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLUE);   /* par 1: gul på blå  */
    init_pair(2, COLOR_RED, COLOR_BLACK);     /* par 2: rød på sort */

    int h = 7, w = 30;
    int y = (LINES - h) / 2;
    int x = (COLS - w) / 2;

    refresh();                          /* opdater stdscr først */

    WINDOW *win = newwin(h, w, y, x);   /* højde, bredde, y, x  */
    wbkgd(win, COLOR_PAIR(1));          /* baggrund for hele vinduet */
    box(win, 0, 0);                     /* ramme med standard-tegn */

    mvwprintw(win, 0, 2, " Min boks ");
    wattron(win, A_BOLD);
    mvwprintw(win, 3, (w - 10) / 2, "Hej Max!");
    wattroff(win, A_BOLD);
    wrefresh(win);

    attron(COLOR_PAIR(2));
    mvprintw(LINES - 1, 0, "Tryk en tast for at afslutte");
    attroff(COLOR_PAIR(2));
    refresh();

    getch();
    delwin(win);                        /* frigør hukommelsen */
    endwin();
    return 0;
}
```

> **Tip:** Kald `refresh()` på `stdscr` *før* du tegner dine vinduer første gang. Ellers kan en senere `refresh()` af `stdscr` male hen over dem.

---

## 11. Farver

Farver virker anderledes end i pygame. I pygame giver du en farve hver gang du tegner. I ncurses definerer du på forhånd **farvepar** (forgrund + baggrund) med et nummer, og bruger så nummeret som en attribut.

```c
if (has_colors() == FALSE) { /* terminalen kan ikke farver */ }

start_color();                              /* skal kaldes først */
init_pair(1, COLOR_RED, COLOR_BLACK);       /* par 1 = rød tekst på sort */
init_pair(2, COLOR_GREEN, COLOR_BLACK);

attron(COLOR_PAIR(1));
printw("Rød tekst");
attroff(COLOR_PAIR(1));

attron(COLOR_PAIR(2) | A_BOLD);             /* farve + fed */
printw("Fed grøn tekst");
attroff(COLOR_PAIR(2) | A_BOLD);
```

Det svarer til, hvis du i pygame lavede konstanter på forhånd:

```python
ROED_PAA_SORT = ((255, 0, 0), (0, 0, 0))
tekst = font.render("Rød tekst", True, *ROED_PAA_SORT)
```

Farvepar `0` er reserveret til terminalens standardfarver – start dine egne fra `1`.

### De 8 grundfarver

| Konstant | Værdi |
|---|---|
| `COLOR_BLACK` | 0 |
| `COLOR_RED` | 1 |
| `COLOR_GREEN` | 2 |
| `COLOR_YELLOW` | 3 |
| `COLOR_BLUE` | 4 |
| `COLOR_MAGENTA` | 5 |
| `COLOR_CYAN` | 6 |
| `COLOR_WHITE` | 7 |

### Egne RGB-værdier

```c
if (can_change_color()) {
    init_color(COLOR_RED, 700, 0, 0);    /* r, g, b fra 0 til 1000 */
}
```

⚠️ Skalaen er **0–1000**, ikke 0–255 som i pygame. Omregning: `ncurses = pygame * 1000 / 255`.

Aflæs værdier igen med:

```c
short r, g, b, fg, bg;
color_content(COLOR_RED, &r, &g, &b);
pair_content(1, &fg, &bg);
```

---

## 12. Taster

`getch()` returnerer en `int`. Almindelige tegn er bare deres ASCII-værdi (`'a'`, `'q'` …), specialtaster er konstanter.

```c
keypad(stdscr, TRUE);        /* ellers får du ikke KEY_UP osv. */

int ch = getch();
switch (ch) {
    case KEY_UP:    y--; break;
    case KEY_DOWN:  y++; break;
    case KEY_F(1):  /* F1 */ break;
    case 'q':       /* q  */ break;
    case 10:        /* Enter */ break;
}
```

| ncurses | pygame | Tast |
|---|---|---|
| `KEY_UP` `KEY_DOWN` `KEY_LEFT` `KEY_RIGHT` | `K_UP` `K_DOWN` `K_LEFT` `K_RIGHT` | Piletaster |
| `KEY_F(1)` … `KEY_F(12)` | `K_F1` … `K_F12` | Funktionstaster |
| `10` eller `'\n'` | `K_RETURN` | Enter (`KEY_ENTER` er numpad-Enter) |
| `27` | `K_ESCAPE` | Esc (har lidt forsinkelse i terminaler) |
| `KEY_BACKSPACE` (nogle gange `127` eller `8`) | `K_BACKSPACE` | Backspace |
| `'a'` | `K_a` | Bogstaver |
| `KEY_HOME` `KEY_END` `KEY_NPAGE` `KEY_PPAGE` | `K_HOME` `K_END` `K_PAGEDOWN` `K_PAGEUP` | |
| `KEY_MOUSE` | `MOUSEBUTTONDOWN` m.fl. | Mus-event |
| `KEY_RESIZE` | `VIDEORESIZE` | Terminalen har skiftet størrelse |

### Vigtig forskel til pygame

pygame giver dig `KEYDOWN` **og** `KEYUP`, og du kan spørge `pygame.key.get_pressed()` om en tast holdes nede. **Det kan ncurses ikke.** En terminal sender kun tegn, når de tastes (og gentager dem, hvis tasten holdes). Du kan altså ikke se, hvornår en tast slippes – det gør fx diagonal bevægelse med to taster på én gang svær.

---

## 13. Mus

```c
MEVENT event;

keypad(stdscr, TRUE);
mousemask(ALL_MOUSE_EVENTS, NULL);     /* hvilke events vil du have? */

int ch = getch();
if (ch == KEY_MOUSE) {
    if (getmouse(&event) == OK) {
        if (event.bstate & BUTTON1_PRESSED) {
            mvprintw(0, 0, "Venstre klik ved x=%d, y=%d", event.x, event.y);
        }
    }
}
```

| pygame | ncurses |
|---|---|
| (altid slået til) | `mousemask(ALL_MOUSE_EVENTS, NULL)` |
| `event.type == MOUSEBUTTONDOWN` | `ch == KEY_MOUSE` + `getmouse(&event)` |
| `event.pos` → `(x, y)` pixels | `event.x`, `event.y` i tegn |
| `event.button == 1` | `event.bstate & BUTTON1_PRESSED` |
| `MOUSEMOTION` | `REPORT_MOUSE_POSITION` (virker ikke i alle terminaler) |

`MEVENT`-strukturen:

```c
typedef struct {
    short id;         /* hvilken mus (hvis flere) */
    int x, y, z;      /* position                 */
    mmask_t bstate;   /* knap-tilstand (bitmaske) */
} MEVENT;
```

### Event-masker

For knap 1–4 findes: `BUTTONn_PRESSED`, `BUTTONn_RELEASED`, `BUTTONn_CLICKED`, `BUTTONn_DOUBLE_CLICKED`, `BUTTONn_TRIPLE_CLICKED`.

Plus: `BUTTON_SHIFT`, `BUTTON_CTRL`, `BUTTON_ALT` (modifier holdt nede), `ALL_MOUSE_EVENTS`, `REPORT_MOUSE_POSITION`.

### Andet

- Musens koordinater er i forhold til **hele skærmen**. Brug `wmouse_trafo()` til at omregne til koordinater i et bestemt vindue.
- `mouseinterval(ms)` bestemmer, hvor hurtigt tryk + slip skal ske for at tælle som et klik (standard: ca. 1/6 sekund – HOWTO'en siger 1/5, men nyere ncurses bruger 1/6).

---

## 14. Skærm-manipulation

### Positioner og størrelser (alle er makroer – ingen `&`)

```c
int y, x;
getyx(win, y, x);        /* markørens nuværende position        */
getbegyx(win, y, x);     /* vinduets øverste venstre hjørne      */
getmaxyx(win, y, x);     /* vinduets størrelse (rækker, kolonner) */
getparyx(win, y, x);     /* sub-vinduets position i forældervinduet */
```

### Gem og gendan

| ncurses | Hvad | Nærmeste i pygame |
|---|---|---|
| `scr_dump("fil")` / `scr_restore("fil")` | Gem/hent hele skærmen i en fil | `pygame.image.save(screen, ...)` / `load` |
| `putwin(win, fp)` / `getwin(fp)` | Gem/hent ét vindue (`FILE *`) | |
| `copywin(src, dst, sy, sx, dy1, dx1, dy2, dx2, overlay)` | Kopiér et rektangel fra ét vindue til et andet | `dst.blit(src, pos, area)` |

Ved `copywin` betyder `overlay = TRUE`, at tomme tegn (mellemrum) ikke kopieres – så overskrives det bagved ikke.

---

## 15. Diverse: markør, pause fra curses, ACS-tegn

### curs_set

```c
curs_set(0);   /* usynlig        */
curs_set(1);   /* normal         */
curs_set(2);   /* meget synlig   */
```

### Forlad curses midlertidigt

Hvis du fx vil køre en shell-kommando og så vende tilbage:

```c
def_prog_mode();       /* gem curses-tilstanden              */
endwin();              /* tilbage til normal terminal         */
system("/bin/sh");     /* gør hvad du vil                    */
reset_prog_mode();     /* gendan curses-tilstanden           */
refresh();             /* tegn skærmen igen                  */
```

### ACS-tegn (streger og symboler)

`ACS_`-konstanterne giver dig "grafiske" tegn, der virker i de fleste terminaler. Brug dem med `addch()`:

```c
mvaddch(0, 0, ACS_ULCORNER);
mvhline(0, 1, ACS_HLINE, 10);
mvaddch(0, 11, ACS_URCORNER);
```

| Konstant | Tegn | Konstant | Tegn |
|---|---|---|---|
| `ACS_ULCORNER` | ┌ | `ACS_URCORNER` | ┐ |
| `ACS_LLCORNER` | └ | `ACS_LRCORNER` | ┘ |
| `ACS_HLINE` | ─ | `ACS_VLINE` | │ |
| `ACS_LTEE` | ├ | `ACS_RTEE` | ┤ |
| `ACS_TTEE` | ┬ | `ACS_BTEE` | ┴ |
| `ACS_PLUS` | ┼ | `ACS_BLOCK` | █ |
| `ACS_CKBOARD` | ▒ | `ACS_BOARD` | ▒ eller # (varierer) |
| `ACS_DIAMOND` | ◆ | `ACS_BULLET` | · |
| `ACS_DEGREE` | ° | `ACS_PLMINUS` | ± |
| `ACS_LARROW` | ← | `ACS_RARROW` | → |
| `ACS_UARROW` | ↑ | `ACS_DARROW` | ↓ |
| `ACS_LEQUAL` | ≤ | `ACS_GEQUAL` | ≥ |
| `ACS_NEQUAL` | ≠ | `ACS_PI` | π |
| `ACS_STERLING` | £ | `ACS_LANTERN` | lanterne |
| `ACS_S1` `ACS_S3` `ACS_S7` `ACS_S9` | scan-linjer | | |

(Hvordan tegnene præcis ser ud, afhænger af terminalens skrifttype.)

---

## 16. Panel-biblioteket

**Problem:** Når vinduer overlapper hinanden, skal du selv holde styr på, hvilken rækkefølge de skal opdateres i. Det bliver hurtigt rodet.

**Løsning:** Panel-biblioteket holder vinduerne i en **stak** (en "bunke kort") og tegner dem i den rigtige rækkefølge for dig. Det minder om `pygame.sprite.LayeredUpdates`, hvor hver sprite har et lag.

```c
#include <panel.h>        /* inkluderer også ncurses.h */
/* gcc prog.c -o prog -lpanel -lncurses */
```

### Arbejdsgang

1. Opret vinduer med `newwin()`.
2. Lav et panel til hvert vindue med `new_panel(win)` – det nyeste lægges øverst.
3. Kald `update_panels()` (beregn hvad der er synligt) og `doupdate()` (tegn det).
4. Flyt rundt med `top_panel()`, `hide_panel()`, `move_panel()` osv.
5. Ryd op med `del_panel()`.

```c
#include <panel.h>

int main(void)
{
    WINDOW *wins[3];
    PANEL  *panels[3];
    int i;

    initscr();
    cbreak();
    noecho();

    for (i = 0; i < 3; i++) {
        wins[i] = newwin(10, 40, 2 + i, 4 + i * 5);
        box(wins[i], 0, 0);
        mvwprintw(wins[i], 1, 2, "Panel %d", i);
        panels[i] = new_panel(wins[i]);   /* lægges øverst i bunken */
    }

    update_panels();     /* beregn hvad der er synligt */
    doupdate();          /* tegn det på skærmen        */

    getch();
    top_panel(panels[0]);  /* hent panel 0 op foran */
    update_panels();
    doupdate();

    getch();
    for (i = 0; i < 3; i++) {
        del_panel(panels[i]);
        delwin(wins[i]);
    }
    endwin();
    return 0;
}
```

### Panel-funktioner

| Funktion | Hvad |
|---|---|
| `new_panel(win)` | Opret panel øverst i stakken |
| `del_panel(p)` | Slet panel (vinduet skal slettes separat) |
| `update_panels()` + `doupdate()` | Tegn alle paneler korrekt (brug i stedet for `refresh()`) |
| `top_panel(p)` / `bottom_panel(p)` | Flyt øverst / nederst i stakken |
| `show_panel(p)` / `hide_panel(p)` | Vis / skjul |
| `panel_hidden(p)` | Er panelet skjult? |
| `move_panel(p, y, x)` | Flyt panelet på skærmen (brug **ikke** `mvwin` på panel-vinduer) |
| `replace_panel(p, nyt_win)` | Skift vinduet ud – bruges til at ændre størrelse |
| `panel_window(p)` | Hent vinduet bag panelet |
| `panel_above(p)` / `panel_below(p)` | Naboerne i stakken (`NULL` giver hhv. nederste/øverste) |
| `set_panel_userptr(p, ptr)` / `panel_userptr(p)` | Gem/hent dine egne data på et panel |

`wnoutrefresh()` + `doupdate()` er i øvrigt ncurses' version af at tegne flere ting og så kun kalde `flip()` én gang til sidst – det sparer flimmer.

---

## 17. Menu-biblioteket

I pygame skulle du selv bygge en menu (eller bruge fx `pygame_gui`). ncurses har et færdigt bibliotek.

```c
#include <menu.h>
/* gcc prog.c -o prog -lmenu -lncurses */
```

### Arbejdsgang

1. Initialisér curses.
2. Opret punkter med `new_item(navn, beskrivelse)`.
3. Opret menuen med `new_menu(items)` – listen **skal** slutte med `NULL`.
4. Vis den med `post_menu(menu)` og `refresh()`.
5. Send brugerens taster videre til `menu_driver(menu, REQ_...)` i en løkke.
6. `unpost_menu()`, `free_menu()`, `free_item()` for hvert punkt.
7. `endwin()`.

```c
#include <stdlib.h>
#include <menu.h>

#define ANTAL(a) (sizeof(a) / sizeof(a[0]))

char *valg[] = { "Start spil", "Indstillinger", "Credits", "Afslut" };

int main(void)
{
    ITEM **items;
    MENU *menu;
    int n = ANTAL(valg);
    int i, c;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    items = calloc(n + 1, sizeof(ITEM *));
    for (i = 0; i < n; i++)
        items[i] = new_item(valg[i], "");
    items[n] = NULL;                      /* listen SKAL slutte med NULL */

    menu = new_menu(items);
    set_menu_mark(menu, "> ");
    mvprintw(LINES - 2, 0, "Pil op/ned, Enter vælger");
    post_menu(menu);
    refresh();

    while ((c = getch()) != 10) {         /* 10 = Enter */
        switch (c) {
            case KEY_DOWN: menu_driver(menu, REQ_DOWN_ITEM); break;
            case KEY_UP:   menu_driver(menu, REQ_UP_ITEM);   break;
        }
        refresh();
    }

    const char *valgt = item_name(current_item(menu));
    mvprintw(LINES - 1, 0, "Du valgte: %s", valgt);
    refresh();
    getch();

    unpost_menu(menu);
    free_menu(menu);
    for (i = 0; i < n; i++)
        free_item(items[i]);
    free(items);
    endwin();
    return 0;
}
```

`#define ANTAL(a) (sizeof(a) / sizeof(a[0]))` er C's måde at få længden af et array – svarer til `len(valg)` i Python.

### menu_driver-forespørgsler

| Forespørgsel | Hvad |
|---|---|
| `REQ_UP_ITEM` / `REQ_DOWN_ITEM` | Et punkt op/ned |
| `REQ_LEFT_ITEM` / `REQ_RIGHT_ITEM` | Venstre/højre (i menuer med flere kolonner) |
| `REQ_FIRST_ITEM` / `REQ_LAST_ITEM` | Første/sidste punkt |
| `REQ_NEXT_ITEM` / `REQ_PREV_ITEM` | Næste/forrige punkt |
| `REQ_SCR_ULINE` / `REQ_SCR_DLINE` | Scroll én linje op/ned |
| `REQ_SCR_UPAGE` / `REQ_SCR_DPAGE` | Scroll én side op/ned |
| `REQ_TOGGLE_ITEM` | Markér/afmarkér (kun i menuer med flere valg) |
| `REQ_CLEAR_PATTERN` / `REQ_BACK_PATTERN` | Ryd/slet i søgebufferen |
| `REQ_NEXT_MATCH` / `REQ_PREV_MATCH` | Næste/forrige søgeresultat |

Sender du et almindeligt tegn til `menu_driver`, søger den efter et punkt der begynder med det. Sender du `KEY_MOUSE`, oversætter den selv klikket.

### Nyttige menu-funktioner

| Funktion | Hvad |
|---|---|
| `current_item(menu)` | Det markerede punkt |
| `item_name(item)` / `item_description(item)` | Punktets tekst |
| `item_index(item)` | Punktets nummer i listen |
| `set_menu_win(menu, win)` | Vindue til menuens ramme/titel |
| `set_menu_sub(menu, derwin(win, h, w, y, x))` | Sub-vindue hvor punkterne vises |
| `set_menu_mark(menu, " * ")` | Tegnet foran det valgte punkt |
| `set_menu_format(menu, rækker, kolonner)` | Hvor mange punkter der vises – mere end det gør menuen scrollbar/fler-kolonne |
| `set_menu_fore(menu, attr)` / `set_menu_back(menu, attr)` | Farve på valgt / ikke-valgt punkt |
| `menu_opts_off(menu, O_SHOWDESC)` | Skjul beskrivelser |
| `menu_opts_off(menu, O_ONEVALUE)` | Tillad flere valg (brug `REQ_TOGGLE_ITEM` + `item_value(item)`) |
| `item_opts_off(item, O_SELECTABLE)` | Gør et punkt ikke-valgbart |
| `set_item_userptr(item, ptr)` / `item_userptr(item)` | Gem fx en funktion der skal køres, når punktet vælges |

Bruger du `set_menu_win`, så husk `keypad(menuvindue, TRUE)` og `wrefresh(menuvindue)`.

---

## 18. Form-biblioteket

Formularer med tekstfelter. Opbygget næsten præcis som menuer: felter i stedet for punkter, `form_driver` i stedet for `menu_driver`.

```c
#include <form.h>
/* gcc prog.c -o prog -lform -lncurses */
```

```c
#include <form.h>
#include <string.h>

int main(void)
{
    FIELD *felter[3];
    FORM  *form;
    int ch;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    /* new_field(højde, bredde, y, x, offscreen-rækker, ekstra-buffere) */
    felter[0] = new_field(1, 20, 4, 18, 0, 0);
    felter[1] = new_field(1, 20, 6, 18, 0, 0);
    felter[2] = NULL;                          /* slut på listen */

    set_field_back(felter[0], A_UNDERLINE);
    field_opts_off(felter[0], O_AUTOSKIP);
    set_field_back(felter[1], A_UNDERLINE);
    field_opts_off(felter[1], O_AUTOSKIP);

    form = new_form(felter);
    post_form(form);
    mvprintw(4, 10, "Navn:");
    mvprintw(6, 10, "Band:");
    mvprintw(LINES - 2, 0, "Pil op/ned skifter felt, F1 afslutter");
    refresh();

    while ((ch = getch()) != KEY_F(1)) {
        switch (ch) {
            case KEY_DOWN:
                form_driver(form, REQ_NEXT_FIELD);
                form_driver(form, REQ_END_LINE);
                break;
            case KEY_UP:
                form_driver(form, REQ_PREV_FIELD);
                form_driver(form, REQ_END_LINE);
                break;
            case KEY_BACKSPACE:
            case 127:
                form_driver(form, REQ_DEL_PREV);
                break;
            default:
                form_driver(form, ch);         /* almindeligt tegn */
                break;
        }
    }

    form_driver(form, REQ_VALIDATION);          /* gem det sidste i bufferen */

    char navn[21];                              /* kopiér teksten ud FØR vi frigør feltet */
    strncpy(navn, field_buffer(felter[0], 0), 20);
    navn[20] = '\0';

    unpost_form(form);
    free_form(form);
    free_field(felter[0]);
    free_field(felter[1]);
    endwin();

    printf("Navn: [%s]\n", navn);   /* feltet er fyldt op med mellemrum */
    return 0;
}
```

### new_field-argumenterne

```c
new_field(højde, bredde, top_y, venstre_x, offscreen, nbuffers);
```
- **offscreen** – antal ekstra rækker der ikke vises. `0` = hele feltet vises altid; ellers kan feltet scrolle.
- **nbuffers** – antal ekstra buffere du kan bruge til hvad du vil. Buffer `0` er altid den med brugerens tekst.

### Felt-funktioner

| Funktion | Hvad |
|---|---|
| `field_info(f, &h, &w, &y, &x, &off, &nbuf)` | Omvendt `new_field` – læs værdierne ud |
| `move_field(f, y, x)` | Flyt feltet |
| `set_field_just(f, JUSTIFY_CENTER)` | Justering: `NO_JUSTIFICATION`, `JUSTIFY_LEFT`, `JUSTIFY_RIGHT`, `JUSTIFY_CENTER` |
| `set_field_fore(f, attr)` / `set_field_back(f, attr)` | Tekst- / baggrundsattribut |
| `set_field_pad(f, '_')` | Fyldtegn i tomme pladser |
| `field_opts_on(f, O_...)` / `field_opts_off(f, O_...)` | Slå indstillinger til/fra |
| `set_field_buffer(f, 0, "tekst")` | Sæt feltets tekst |
| `field_buffer(f, 0)` | Hent feltets tekst |
| `field_status(f)` / `set_field_status(f, FALSE)` | Er feltet ændret? |
| `set_field_userptr(f, ptr)` / `field_userptr(f)` | Egne data |
| `set_max_field(f, max)` | Max-størrelse for dynamiske felter |

### Felt-indstillinger (`O_...`)

| Flag | Betydning |
|---|---|
| `O_VISIBLE` | Feltet vises |
| `O_ACTIVE` | Feltet kan få fokus (slå fra for "labels") |
| `O_PUBLIC` | Teksten vises mens man skriver (slå fra til kodeord) |
| `O_EDIT` | Feltet kan redigeres |
| `O_WRAP` | Ord ombrydes i felter med flere linjer |
| `O_BLANK` | Feltet ryddes, når man skriver første tegn |
| `O_AUTOSKIP` | Hop automatisk til næste felt, når feltet er fyldt |
| `O_NULLOK` | Tomt felt springer validering over |
| `O_PASSOK` | Valider kun hvis feltet er ændret |
| `O_STATIC` | Feltet er fast størrelse (slå fra for at gøre det dynamisk) |

### Validering

```c
set_field_type(f, TYPE_ALPHA, min_bredde);             /* kun bogstaver */
set_field_type(f, TYPE_ALNUM, min_bredde);             /* bogstaver + tal */
set_field_type(f, TYPE_INTEGER, præcision, min, max);  /* heltal i interval */
set_field_type(f, TYPE_NUMERIC, præcision, min, max);  /* decimaltal */
set_field_type(f, TYPE_REGEXP, "^[0-9]{4}$");          /* regulært udtryk */
set_field_type(f, TYPE_ENUM, liste, skelne_store_små, unik_match);
```

Valideringen sker, når brugeren forlader feltet – eller når du sender `REQ_VALIDATION`.

### Form-vinduer

Vil du have formen inde i et vindue med ramme, beder du først formen om, hvor meget plads den kræver:

```c
form = new_form(felter);

int rows, cols;
scale_form(form, &rows, &cols);                    /* hvor stor plads kræver felterne? */

WINDOW *win = newwin(rows + 4, cols + 4, 4, 4);    /* lidt større, så der er plads til ramme */
keypad(win, TRUE);
set_form_win(form, win);                           /* vindue til ramme/titel */
set_form_sub(form, derwin(win, rows, cols, 2, 2)); /* sub-vindue til felterne */
box(win, 0, 0);

post_form(form);
wrefresh(win);

while ((ch = wgetch(win)) != KEY_F(1))             /* læs fra vinduet, ikke stdscr */
    form_driver(form, ch);
```

### form_driver-forespørgsler (udvalg)

| Kategori | Forespørgsler |
|---|---|
| Sider | `REQ_NEXT_PAGE`, `REQ_PREV_PAGE`, `REQ_FIRST_PAGE`, `REQ_LAST_PAGE` |
| Mellem felter | `REQ_NEXT_FIELD`, `REQ_PREV_FIELD`, `REQ_FIRST_FIELD`, `REQ_LAST_FIELD`, `REQ_LEFT_FIELD`, `REQ_RIGHT_FIELD`, `REQ_UP_FIELD`, `REQ_DOWN_FIELD` |
| Inde i et felt | `REQ_NEXT_CHAR`, `REQ_PREV_CHAR`, `REQ_NEXT_WORD`, `REQ_PREV_WORD`, `REQ_BEG_LINE`, `REQ_END_LINE`, `REQ_BEG_FIELD`, `REQ_END_FIELD` |
| Redigering | `REQ_DEL_CHAR`, `REQ_DEL_PREV`, `REQ_DEL_LINE`, `REQ_DEL_WORD`, `REQ_CLR_EOL`, `REQ_CLR_FIELD`, `REQ_INS_MODE`, `REQ_OVL_MODE` |
| Andet | `REQ_VALIDATION`, `REQ_NEXT_CHOICE`/`REQ_PREV_CHOICE` (til `TYPE_ENUM`) |

---

## 19. Komplet mini-spil: C vs. pygame vs. Python-curses

Samme program tre gange: flyt en `@` rundt med piletasterne, `q` afslutter.

### C + ncurses

```c
#include <ncurses.h>

int main(void)
{
    int x = 10, y = 5;
    int aktiv = 1;

    initscr();
    cbreak();                 /* taster med det samme      */
    noecho();                 /* vis ikke tasterne         */
    keypad(stdscr, TRUE);     /* piletaster = KEY_UP osv.  */
    curs_set(0);              /* skjul markøren            */
    timeout(16);              /* getch venter max 16 ms    */

    while (aktiv) {
        int ch = getch();     /* ERR hvis ingen tast       */

        switch (ch) {
            case KEY_LEFT:  x--; break;
            case KEY_RIGHT: x++; break;
            case KEY_UP:    y--; break;
            case KEY_DOWN:  y++; break;
            case 'q':       aktiv = 0; break;
        }

        /* hold spilleren inden for skærmen */
        if (x < 0) x = 0;
        if (y < 1) y = 1;
        if (x > COLS - 1)  x = COLS - 1;
        if (y > LINES - 1) y = LINES - 1;

        erase();                                   /* ~ screen.fill() */
        mvprintw(0, 0, "Piletaster flytter @, q afslutter");
        mvaddch(y, x, '@' | A_BOLD);               /* ~ screen.blit() */
        refresh();                                 /* ~ display.flip() */
    }

    endwin();
    return 0;
}
```

### pygame

```python
import pygame

pygame.init()
screen = pygame.display.set_mode((640, 480))
font = pygame.font.SysFont("consolas", 20)
clock = pygame.time.Clock()
x, y = 200, 200
running = True

while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_LEFT:
                x -= 20
            elif event.key == pygame.K_RIGHT:
                x += 20
            elif event.key == pygame.K_UP:
                y -= 20
            elif event.key == pygame.K_DOWN:
                y += 20
            elif event.key == pygame.K_q:
                running = False

    screen.fill((0, 0, 0))
    screen.blit(font.render("Piletaster flytter @, q afslutter", True, (255, 255, 255)), (0, 0))
    screen.blit(font.render("@", True, (255, 255, 255)), (x, y))
    pygame.display.flip()
    clock.tick(60)

pygame.quit()
```

### Python + curses (broen mellem de to)

Pythons indbyggede `curses`-modul er ncurses pakket ind. Funktionerne bliver til metoder på vinduet, og `mv`-varianterne forsvinder (koordinaterne er bare valgfrie argumenter):

```python
import curses

def main(stdscr):
    curses.curs_set(0)          # skjul markøren
    stdscr.timeout(16)          # getch venter max 16 ms
    y, x = 5, 10

    while True:
        ch = stdscr.getch()     # -1 hvis ingen tast
        if ch == curses.KEY_LEFT:
            x -= 1
        elif ch == curses.KEY_RIGHT:
            x += 1
        elif ch == curses.KEY_UP:
            y -= 1
        elif ch == curses.KEY_DOWN:
            y += 1
        elif ch == ord('q'):
            break

        rows, cols = stdscr.getmaxyx()
        x = max(0, min(x, cols - 2))
        y = max(1, min(y, rows - 1))

        stdscr.erase()
        stdscr.addstr(0, 0, "Piletaster flytter @, q afslutter")
        stdscr.addch(y, x, '@', curses.A_BOLD)
        stdscr.refresh()

curses.wrapper(main)    # klarer initscr, cbreak, noecho, keypad og endwin for dig
```

| C | Python `curses` |
|---|---|
| `mvprintw(y, x, "...")` | `stdscr.addstr(y, x, "...")` |
| `mvwprintw(win, y, x, "...")` | `win.addstr(y, x, "...")` |
| `attron(A_BOLD)` | `stdscr.attron(curses.A_BOLD)` |
| `init_pair(1, COLOR_RED, COLOR_BLACK)` | `curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)` |
| `COLOR_PAIR(1)` | `curses.color_pair(1)` |
| `newwin(h, w, y, x)` | `curses.newwin(h, w, y, x)` |
| `getmaxyx(win, h, w)` | `h, w = win.getmaxyx()` |
| `getch() == ERR` | `getch() == -1` |
| `initscr()` … `endwin()` | `curses.wrapper(main)` |

---

## 20. Typiske fejl

1. **Glemt `refresh()`** – intet vises.
2. **(x, y) i stedet for (y, x)** – teksten havner et helt forkert sted.
3. **Glemt `keypad(win, TRUE)`** – piletaster giver mærkelige tal eller escape-tegn.
4. **Glemt `endwin()`** – terminalen er i stykker efter programmet (skriv `reset`).
5. **`&` på makroer** – `getmaxyx(stdscr, &rows, &cols)` er forkert. Ingen `&`.
6. **`box(win, ' ', ' ')` til at slette** – efterlader hjørnerne. Brug `wborder` med 8 mellemrum.
7. **Tegne i nederste højre hjørne** – kan give `ERR`, fordi markøren ikke kan rykke videre.
8. **`clear()` i en game loop** – tvinger hele skærmen til at blive gentegnet og flimrer. Brug `erase()`.
9. **Menu-/felt-listen slutter ikke med `NULL`** – programmet crasher.
10. **Forkert linker-rækkefølge** – `-lncurses -lmenu` giver fejl; det skal være `-lmenu -lncurses`.
11. **`getstr()` uden grænse** – brug `getnstr(buf, størrelse - 1)`.
12. **Blandet `refresh()` og paneler** – når du bruger panel-biblioteket, så brug `update_panels()` + `doupdate()`.

---

## 21. Hurtig-reference

### Start / slut
| | |
|---|---|
| `initscr()` | Start curses |
| `endwin()` | Afslut curses |
| `cbreak()` / `raw()` | Taster med det samme |
| `noecho()` / `echo()` | Vis ikke / vis taster |
| `keypad(win, TRUE)` | Specialtaster |
| `nodelay(win, TRUE)` / `timeout(ms)` / `halfdelay(tiendedele)` | Ikke-blokerende input |
| `curs_set(0/1/2)` | Markørens synlighed |

### Tegne
| | |
|---|---|
| `[mv][w]addch([win,] [y, x,] ch)` | Ét tegn |
| `[mv][w]printw([win,] [y, x,] fmt, ...)` | Formateret tekst |
| `[mv][w]addstr([win,] [y, x,] str)` | Streng |
| `move(y, x)` | Flyt markøren |
| `refresh()` / `wrefresh(win)` | Vis ændringer |
| `wnoutrefresh(win)` + `doupdate()` | Opdater flere vinduer effektivt |
| `erase()` / `clear()` | Ryd skærm (`clear` tvinger fuld gentegning) |
| `clrtoeol()` / `clrtobot()` | Ryd til enden af linjen / skærmen |

### Input
| | |
|---|---|
| `[mv][w]getch()` | Én tast (`int`) |
| `[mv][w]scanw(fmt, &var)` | Formateret input |
| `[mv][w]getnstr(buf, n)` | Linje med max-længde |

### Udseende
| | |
|---|---|
| `attron(a)` / `attroff(a)` / `attrset(a)` / `standend()` | Attributter |
| `chgat(n, a, par, NULL)` | Ændr eksisterende tekst |
| `start_color()` / `has_colors()` | Farver |
| `init_pair(n, fg, bg)` / `COLOR_PAIR(n)` | Farvepar |
| `init_color(farve, r, g, b)` | RGB 0–1000 |
| `wbkgd(win, attr)` | Baggrund for vindue |

### Vinduer
| | |
|---|---|
| `newwin(h, w, y, x)` / `delwin(win)` | Opret / slet |
| `derwin(forælder, h, w, y, x)` | Sub-vindue |
| `box(win, 0, 0)` / `wborder(win, 8 tegn)` | Rammer |
| `mvhline(y, x, ch, n)` / `mvvline(y, x, ch, n)` | Linjer |
| `mvwin(win, y, x)` | Flyt vindue |
| `getyx` / `getbegyx` / `getmaxyx` / `getparyx` | Positioner (makroer) |
| `LINES` / `COLS` | Skærmstørrelse |

### Mus
| | |
|---|---|
| `mousemask(ALL_MOUSE_EVENTS, NULL)` | Slå mus til |
| `getch() == KEY_MOUSE` + `getmouse(&event)` | Læs event |
| `event.x`, `event.y`, `event.bstate & BUTTON1_PRESSED` | Brug event |

---

*Resten af HOWTO'en (kapitel 19–20) handler om widget-biblioteker som CDK og `dialog`, og indeholder små "just for fun"-programmer (Game of Life, Tårnene i Hanoi, otte-dronninger, et skriveprogram m.fl.). De er gode at kigge i, når du har styr på syntaksen herover.*
