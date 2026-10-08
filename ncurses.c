#include <ncurses.h>  /* ncurses-bibliotek til terminal-grafik */
#include <stdint.h>   /* uint64_t og int64_t til store heltal */
#include <string.h>   /* strlen, strtok, strchr, strcat */
#include <stdlib.h>   /* strtoull til at konvertere strenge til tal */
#include <time.h>     /* time og localtime til at vise dato og tid */
#include <locale.h>   /* setlocale til korrekt tegnsæt */

/* ── RSA matematiske funktioner ──────────────────────────────────── */
#define P 4294967291ULL  /* stort primtal P */
#define Q 2147483647ULL  /* stort primtal Q */
typedef uint64_t u64;    /* kortere navn for 64-bit heltal uden fortegn */

/* Største fælles divisor — bruges til at tjekke om e og phi er coprime */
static u64 gcd(u64 a, u64 b)
{
    while (b)
    {
        u64 t = b;      /* gem b midlertidigt */
        b = a % b;      /* rest af a divideret med b */
        a = t;          /* a bliver det gamle b */
    }
    return a;           /* returnerer største fælles divisor */
}

/* Modulær invers — finder d så e*d ≡ 1 (mod phi) via udvidet Euklid */
static u64 modinv(u64 e, u64 phi)
{
    int64_t r0 = (int64_t)phi, r1 = (int64_t)e, s0 = 0, s1 = 1; /* startværdier */
    while (r1)
    {
        int64_t q = r0 / r1, tr = r1, ts = s1; /* beregn kvotient og gem gamle værdier */
        r1 = r0 - q * r1;  /* opdater rest */
        r0 = tr;            /* ryk én plads frem */
        s1 = s0 - q * s1;  /* opdater koefficient */
        s0 = ts;            /* ryk én plads frem */
    }
    return (u64)(s0 < 0 ? s0 + (int64_t)phi : s0); /* gør resultatet positivt */
}

/* Overløbssikker addition modulo m */
static u64 addm(u64 a, u64 b, u64 m)
{
    a %= m;  /* reducér a */
    b %= m;  /* reducér b */
    return a >= m - b ? a - (m - b) : a + b; /* undgår overløb ved store tal */
}

/* Overløbssikker multiplikation modulo m via binær metode */
static u64 mulm(u64 a, u64 b, u64 m)
{
    u64 r = 0;  /* resultatet starter på 0 */
    a %= m;     /* reducér a */
    while (b)
    {
        if (b & 1)              /* hvis det laveste bit er sat */
            r = addm(r, a, m);  /* læg a til resultatet */
        a = addm(a, a, m);      /* fordobl a */
        b >>= 1;                /* skift b én bit til højre */
    }
    return r; /* returnerer a*b mod m */
}

/* Modulær eksponentiering — beregner b^e mod m via binær metode */
static u64 powm(u64 b, u64 e, u64 m)
{
    u64 r = 1;  /* resultatet starter på 1 */
    b %= m;     /* reducér basen */
    while (e)
    {
        if (e & 1)              /* hvis det laveste bit i eksponenten er sat */
            r = mulm(r, b, m);  /* gang resultatet med b */
        b = mulm(b, b, m);      /* kvadrér basen */
        e >>= 1;                /* skift eksponenten én bit til højre */
    }
    return r; /* returnerer b^e mod m */
}

/* Globale RSA-nøgler */
static u64 N, E, D;

/* Udregner RSA-nøgler fra primtallene P og Q */
static void init_rsa(void)
{
    N = P * Q;                    /* modulen n = p * q */
    u64 phi = (P - 1) * (Q - 1); /* Eulers phi = (p-1)*(q-1) */
    E = 65537ULL;                 /* offentlig eksponent (standard RSA-værdi) */
    if (gcd(E, phi) != 1)         /* tjek at e og phi er coprime */
        E = 0;                    /* ugyldig nøgle hvis de ikke er coprime */
    D = modinv(E, phi);           /* privat eksponent d = e^-1 mod phi */
}

/* ── Farvekonstanter ─────────────────────────────────────────────── */
#define CG 1  /* grønt på sort — standard tekstfarve */
#define CS 2  /* sort på grønt — markeret/valgt element */
#define CT 3  /* hvidt på sort — klartekst-output */
#define CR 4  /* rødt på sort — krypteret output */
#define CC 5  /* cyan på sort — titler og statusbeskeder */

/* Opretter de fem farvepar til ncurses */
static void setup_colors(void)
{
    start_color();                             /* aktivér farvesupport */
    init_pair(CG, COLOR_GREEN, COLOR_BLACK);   /* grønt på sort */
    init_pair(CS, COLOR_BLACK, COLOR_GREEN);   /* sort på grønt (valgt) */
    init_pair(CT, COLOR_WHITE, COLOR_BLACK);   /* hvidt på sort */
    init_pair(CR, COLOR_RED, COLOR_BLACK);     /* rødt på sort */
    init_pair(CC, COLOR_CYAN, COLOR_BLACK);    /* cyan på sort */
}

/* ── Layout-konstanter ───────────────────────────────────────────── */
#define HDR_H 4   /* højde på header i linjer */
#define FTR_H 3   /* højde på footer i linjer */
#define SIDE_W 24 /* bredde på sidebar i tegn */

/* Pointere til de fire vinduer */
static WINDOW *Whdr, *Wside, *Wmain, *Wftr;

/* Opretter alle fire ncurses-vinduer og placerer dem korrekt på skærmen */
static void make_wins(void)
{
    int body_h = LINES - HDR_H - FTR_H; /* højde af midterzonen */
    int main_w = COLS - SIDE_W;          /* bredde af hovedpanelet */
    Whdr  = newwin(HDR_H,  COLS,   0,             0);      /* header øverst */
    Wside = newwin(body_h, SIDE_W, HDR_H,         0);      /* sidebar til venstre */
    Wmain = newwin(body_h, main_w, HDR_H,         SIDE_W); /* hovedpanel til højre */
    Wftr  = newwin(FTR_H,  COLS,   LINES - FTR_H, 0);      /* footer nederst */
    keypad(Wside, TRUE); /* aktivér piletaster i sidebar */
    keypad(Wmain, TRUE); /* aktivér piletaster i hovedpanel */
}

/* ── Header ──────────────────────────────────────────────────────── */
/* Tegner headerlinjen med titel, klassifikation og aktuelt tidspunkt */
static void draw_hdr(void)
{
    werase(Whdr);                    /* ryd vinduet */
    wattron(Whdr, COLOR_PAIR(CG));
    box(Whdr, 0, 0);                 /* tegn grøn kantramme */
    wattroff(Whdr, COLOR_PAIR(CG));

    wattron(Whdr, COLOR_PAIR(CT) | A_BOLD);
    mvwprintw(Whdr, 1, 3, "CIPHER TERMINAL  //  RSA-63 KRYPTERINGSSYSTEM"); /* programtitel */
    wattroff(Whdr, COLOR_PAIR(CT) | A_BOLD);

    wattron(Whdr, COLOR_PAIR(CR) | A_BOLD);
    mvwprintw(Whdr, 2, 3, "[ TOP SECRET ]");  /* klassifikationsmærke i rødt */
    wattroff(Whdr, COLOR_PAIR(CR) | A_BOLD);

    wattron(Whdr, COLOR_PAIR(CG));
    mvwprintw(Whdr, 2, 20, "SIKKERHEDS ADGANG: NIVEAU-5"); /* adgangsniveau */
    wattroff(Whdr, COLOR_PAIR(CG));

    time_t t = time(NULL);          /* hent nuværende tidspunkt */
    struct tm *tm = localtime(&t);  /* konvertér til lokal tid */
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm); /* formater som tekst */
    wattron(Whdr, COLOR_PAIR(CG));
    mvwprintw(Whdr, 1, COLS - (int)strlen(ts) - 3, "%s", ts); /* vis tid i højre side */
    wattroff(Whdr, COLOR_PAIR(CG));

    wrefresh(Whdr); /* opdatér skærmen */
}

/* ── Sidebar ─────────────────────────────────────────────────────── */
/* Menuvalg vist i sidebaren */
static const char *MITEMS[] = {
    "  [1]  KRYPTER      ",
    "  [2]  DEKRYPTER    ",
    "  [3]  AFSLUT       ",
};
#define NM 3 /* antal menupunkter */

/* Tegner sidebaren og fremhæver det valgte menupunkt */
static void draw_side(int sel)
{
    werase(Wside);                  /* ryd sidebaren */
    wattron(Wside, COLOR_PAIR(CG));
    box(Wside, 0, 0);               /* grøn kantramme */
    wattroff(Wside, COLOR_PAIR(CG));

    wattron(Wside, COLOR_PAIR(CC) | A_BOLD);
    mvwprintw(Wside, 1, 2, ">> OPERATIONER <<"); /* overskrift */
    wattroff(Wside, COLOR_PAIR(CC) | A_BOLD);

    wattron(Wside, COLOR_PAIR(CG));
    mvwhline(Wside, 2, 1, ACS_HLINE, SIDE_W - 2); /* vandret skillelinje */
    wattroff(Wside, COLOR_PAIR(CG));

    for (int i = 0; i < NM; i++) /* løb over alle menupunkter */
    {
        int row = 4 + i * 2; /* placering med mellemrum mellem punkterne */
        if (i == sel)        /* er dette det valgte punkt? */
        {
            wattron(Wside, COLOR_PAIR(CS) | A_BOLD);
            mvwprintw(Wside, row, 1, "%s", MITEMS[i]); /* fremhæv med inverteret farve */
            wattroff(Wside, COLOR_PAIR(CS) | A_BOLD);
        }
        else
        {
            wattron(Wside, COLOR_PAIR(CG));
            mvwprintw(Wside, row, 1, "%s", MITEMS[i]); /* normalt grønt */
            wattroff(Wside, COLOR_PAIR(CG));
        }
    }
    wrefresh(Wside); /* vis ændringerne */
}

/* ── Footer ──────────────────────────────────────────────────────── */
/* Tegner footeren med en kontekstafhængig hjælpetekst */
static void draw_ftr(const char *msg)
{
    werase(Wftr);                    /* ryd footeren */
    wattron(Wftr, COLOR_PAIR(CG));
    box(Wftr, 0, 0);                 /* grøn kantramme */
    wattroff(Wftr, COLOR_PAIR(CG));
    wattron(Wftr, COLOR_PAIR(CG));
    mvwprintw(Wftr, 1, 2, ">> %s", msg); /* vis beskeden */
    wattroff(Wftr, COLOR_PAIR(CG));
    wrefresh(Wftr); /* opdatér skærmen */
}

/* ── Hjælpefunktioner til hovedpanelet ──────────────────────────── */
/* Rydder hovedpanelet og viser en overskrift med skillelinje */
static void mpane_title(const char *title)
{
    werase(Wmain);                   /* ryd panelet */
    wattron(Wmain, COLOR_PAIR(CG));
    box(Wmain, 0, 0);                /* grøn kantramme */
    wattroff(Wmain, COLOR_PAIR(CG));
    wattron(Wmain, COLOR_PAIR(CC) | A_BOLD);
    mvwprintw(Wmain, 1, 2, "%s", title); /* vis overskrift i cyan */
    wattroff(Wmain, COLOR_PAIR(CC) | A_BOLD);
    wattron(Wmain, COLOR_PAIR(CG));
    mvwhline(Wmain, 2, 1, ACS_HLINE, COLS - SIDE_W - 2); /* vandret linje under titlen */
    wattroff(Wmain, COLOR_PAIR(CG));
    wrefresh(Wmain); /* opdatér skærmen */
}

/* Viser en prompt og læser brugerens input ind i buf */
static void get_str(int row, int col, const char *prompt, char *buf, int max)
{
    wattron(Wmain, COLOR_PAIR(CG));
    mvwprintw(Wmain, row, col, "%s", prompt); /* vis promptteksten */
    wattroff(Wmain, COLOR_PAIR(CG));

    int fx = col + (int)strlen(prompt); /* startkolonne for inputfeltet */
    int fw = COLS - SIDE_W - fx - 3;   /* bredde af inputfeltet */
    wattron(Wmain, COLOR_PAIR(CS));
    mvwhline(Wmain, row, fx, ' ', fw);  /* markér inputfeltet med inverteret baggrund */
    wattroff(Wmain, COLOR_PAIR(CS));
    wrefresh(Wmain); /* vis prompten før vi venter på input */

    echo();          /* vis tegn mens brugeren skriver */
    curs_set(1);     /* vis markøren */
    wattron(Wmain, COLOR_PAIR(CS));
    wmove(Wmain, row, fx);              /* flyt markøren til inputfeltet */
    wgetnstr(Wmain, buf, max - 1);      /* læs input fra brugeren */
    wattroff(Wmain, COLOR_PAIR(CS));
    noecho();        /* skjul tegn igen */
    curs_set(0);     /* skjul markøren igen */
}

/* ── Skærmbilleder ───────────────────────────────────────────────── */
/* Krypteringsskærmen — læser klartekst og viser krypterede tal */
static void do_encrypt(void)
{
    mpane_title("KRYPTER BESKED");                       /* vis titel */
    draw_ftr("Skriv klartekst og tryk ENTER");           /* vis hjælpetekst */

    char plain[256] = {0};                               /* buffer til klartekst */
    get_str(4, 2, "KLARTEKST : ", plain, sizeof(plain)); /* læs klartekst fra brugeren */

    int len = (int)strlen(plain); /* antal tegn i beskeden */

    char out[8192] = {0}; /* buffer til alle krypterede tal som tekst */
    for (int i = 0; i < len; i++) /* kryptér hvert tegn for sig */
    {
        char blk[32];
        u64 c = powm((u64)(unsigned char)plain[i], E, N); /* RSA: c = m^e mod n */
        snprintf(blk, sizeof(blk), "%llu", (unsigned long long)c); /* konvertér til tekst */
        if (i)
            strcat(out, " "); /* mellemrum mellem tallene */
        strcat(out, blk);     /* tilføj tallet til outputbufferen */
    }

    /* Fuldskærms outputvindue uden venstre/højre kant så
       terminalen kun kopierer tallene og ikke kantrammetegn */
    int body_h = LINES - HDR_H - FTR_H;
    WINDOW *Wout = newwin(body_h, COLS, HDR_H, 0); /* nyt vindue der dækker hele skærmen */
    wbkgd(Wout, COLOR_PAIR(CG));  /* grøn baggrundsfarve */
    werase(Wout);                 /* ryd vinduet */

    wattron(Wout, COLOR_PAIR(CC) | A_BOLD);
    mvwprintw(Wout, 1, 2, "KRYPTERET OUTPUT  //  %d blokke", len); /* vis antal blokke */
    wattroff(Wout, COLOR_PAIR(CC) | A_BOLD);

    wattron(Wout, COLOR_PAIR(CG));
    mvwhline(Wout, 2, 0, ACS_HLINE, COLS);    /* skillelinje */
    mvwprintw(Wout, 3, 0, ">>> KOPIER START"); /* kopieringsmarkering */
    wattroff(Wout, COLOR_PAIR(CG));

    /* Tal starter i kolonne 0 så der ikke er kantrammetegn på disse rækker */
    int cx = 0, cy = 4; /* startposition for tallene */
    wattron(Wout, COLOR_PAIR(CR) | A_BOLD);
    char *p = out; /* peger på starten af outputbufferen */
    while (*p)     /* fortsæt til bufferen er tom */
    {
        char *sp = strchr(p, ' ');                           /* find næste mellemrum */
        int wlen = sp ? (int)(sp - p + 1) : (int)strlen(p); /* længde af dette tal */
        if (cx + wlen >= COLS) /* er der plads på linjen? */
        {
            cy++; /* næste linje */
            cx = 0; /* start fra venstre igen */
        }
        if (cy >= body_h - 4) /* stop hvis vi nærmer os bunden */
            break;
        mvwprintw(Wout, cy, cx, "%.*s", wlen, p); /* udskriv tallet */
        cx += wlen; /* ryk markøren frem */
        p  += wlen; /* ryk bufferpointeren frem */
    }
    wattroff(Wout, COLOR_PAIR(CR) | A_BOLD);

    wattron(Wout, COLOR_PAIR(CG));
    mvwprintw(Wout, cy + 1, 0, ">>> KOPIER SLUT");   /* slutmarkering */
    mvwhline(Wout, cy + 2, 0, ACS_HLINE, COLS);      /* skillelinje */
    wattroff(Wout, COLOR_PAIR(CG));

    wattron(Wout, COLOR_PAIR(CC));
    mvwprintw(Wout, cy + 3, 2, "[OK] Kryptering faerdig."); /* statusbesked */
    wattroff(Wout, COLOR_PAIR(CC));

    wrefresh(Wout); /* vis outputvinduet */
    draw_ftr("Vaelg tal mellem markoererne for at kopiere  |  Tryk en tast for at vende tilbage");
    wgetch(Wout);   /* vent på tastetryk */
    delwin(Wout);   /* slet det midlertidige vindue */
}

/* Dekrypteringsskærmen — læser krypterede tal og viser klartekst */
static void do_decrypt(void)
{
    mpane_title("DEKRYPTER BESKED"); /* vis titel */
    draw_ftr("Indsaet krypterede tal adskilt med mellemrum og tryk ENTER");

    char raw[2048] = {0};                                   /* buffer til krypteret input */
    get_str(4, 2, "KRYPTERET TEKST : ", raw, sizeof(raw));  /* læs krypterede tal fra brugeren */

    wattron(Wmain, COLOR_PAIR(CG));
    mvwhline(Wmain, 8, 1, ACS_HLINE, COLS - SIDE_W - 2); /* skillelinje mellem input og output */
    mvwprintw(Wmain, 9, 2, "KLARTEKST  :");                /* label for den dekrypterede tekst */
    wattroff(Wmain, COLOR_PAIR(CG));

    wattron(Wmain, COLOR_PAIR(CT) | A_BOLD);
    int px = 15, py = 9; /* startposition for klarteksten (lige efter labelen) */
    char *tok = strtok(raw, " "); /* opdel inputtet ved mellemrum */
    while (tok) /* behandl hvert tal */
    {
        u64 c = strtoull(tok, NULL, 10);  /* konvertér tekst til tal */
        u64 m = powm(c, D, N);            /* RSA: m = c^d mod n */
        if (m >= 32 && m < 127)           /* kun udskrivbare ASCII-tegn */
        {
            if (px >= COLS - SIDE_W - 3)  /* er vi nået til linjens ende? */
            {
                py++; /* næste linje */
                px = 2; /* start fra venstre */
            }
            mvwprintw(Wmain, py, px, "%c", (char)m); /* udskriv det dekrypterede tegn */
            px++; /* ryk én plads frem */
        }
        tok = strtok(NULL, " "); /* hent næste tal */
    }
    wattroff(Wmain, COLOR_PAIR(CT) | A_BOLD);

    wattron(Wmain, COLOR_PAIR(CC));
    mvwprintw(Wmain, py + 2, 2, "[OK] Dekryptering faerdig."); /* statusbesked */
    wattroff(Wmain, COLOR_PAIR(CC));
    wrefresh(Wmain); /* opdatér skærmen */
    draw_ftr("Faerdig  |  Tryk en tast for at vende tilbage");
    wgetch(Wmain);   /* vent på tastetryk */
}

/* Viser venteskærm når intet er valgt */
static void draw_idle(void)
{
    mpane_title("KLAR"); /* vis "KLAR" som titel */
    wattron(Wmain, COLOR_PAIR(CG) | A_DIM);
    mvwprintw(Wmain, 4, 2, "Vaelg en operation fra menuen."); /* vejledning til brugeren */
    mvwprintw(Wmain, 5, 2, "Brug piletaster, 1-3 eller klik.");
    wattroff(Wmain, COLOR_PAIR(CG) | A_DIM);
    wrefresh(Wmain); /* vis skærmen */
}

/* ── Hoved-event-løkke ───────────────────────────────────────────── */
#define FOOTER_MSG "Piletaster eller 1-3 for at vaelge  |  ENTER for at bekraefte  |  Q for at afslutte"

/* Tegner hele grænsefladen forfra */
static void redraw_main(void)
{
    draw_hdr();           /* header med titel og tid */
    draw_idle();          /* venteskærm i midten */
    draw_ftr(FOOTER_MSG); /* footer med tastebeskrivelse */
}

/* Kører det valgte menupunkt */
static void run_sel(int sel, int *running)
{
    switch (sel)
    {
    case 0:
        do_encrypt(); /* start kryptering */
        break;
    case 1:
        do_decrypt(); /* start dekryptering */
        break;
    case 2:
        *running = 0; /* afslut programmet */
        return;
    }
    redraw_main(); /* genindlæs startskærmen når funktionen er færdig */
}

int main(void)
{
    setlocale(LC_ALL, "");  /* brug systemets tegnsæt */
    init_rsa();             /* udregn RSA-nøgler */
    initscr();              /* initialisér ncurses */
    noecho();               /* vis ikke taster mens de trykkes */
    cbreak();               /* send taster direkte uden Enter */
    curs_set(0);            /* skjul markøren */
    if (has_colors())       /* tjek om terminalen understøtter farver */
        setup_colors();     /* opsæt farvepar */

    make_wins();    /* opret alle vinduer */
    redraw_main();  /* tegn startskærmen */

    int sel = 0, running = 1; /* sel = valgt menupunkt, running = programmet kører */
    draw_side(sel);            /* tegn sidebaren med første punkt valgt */

    while (running) /* hovedløkken kører indtil brugeren vælger AFSLUT */
    {
        int ch = wgetch(Wside); /* vent på tastetryk i sidebaren */
        switch (ch)
        {
        case KEY_UP:
            sel = (sel - 1 + NM) % NM; /* ryk valget op (wraparound) */
            break;
        case KEY_DOWN:
            sel = (sel + 1) % NM; /* ryk valget ned (wraparound) */
            break;
        case '\n':
        case '\r':
        case KEY_ENTER:
            run_sel(sel, &running); /* bekræft valget med Enter */
            break;
        case '1':
            sel = 0;
            run_sel(0, &running); /* genvejstast 1 = KRYPTER */
            break;
        case '2':
            sel = 1;
            run_sel(1, &running); /* genvejstast 2 = DEKRYPTER */
            break;
        case '3':
        case 'q':
        case 'Q':
            running = 0; /* genvejstast 3 eller Q = AFSLUT */
            break;
        }
        if (running)
            draw_side(sel); /* opdatér sidebaren med nyt valg */
    }

    delwin(Whdr);  /* frigiv header-vinduet */
    delwin(Wside); /* frigiv sidebar-vinduet */
    delwin(Wmain); /* frigiv hovedpanel-vinduet */
    delwin(Wftr);  /* frigiv footer-vinduet */
    endwin();      /* luk ncurses og gendan terminalen */
    return 0;
}
