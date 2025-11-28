/* See LICENSE file for copyright and license details. */

#include "extra.h"
#include "movestack.c"

#include <X11/XF86keysym.h>

#define STATUSBAR "dwm-status"

typedef unsigned int uint;
enum showtab_modes { showtab_never, showtab_auto, showtab_nmodes, showtab_always};

static const uint  borderpx                = 2;            /* border pixel of windows */
static const uint  snap                    = 32;           /* snap pixel */
static const int   showbar                 = 1;            /* 0 means no bar */
static const int   topbar                  = 0;            /* 0 means bottom bar */
static const float mfact                   = 0.5;          /* factor of master area size [0.05..0.95] */
static const int   nmaster                 = 1;            /* number of clients in master area */
static const int   resizehints             = 0;            /* 1 means respect size hints in tiled resizals */
static const int   lockfullscreen          = 1;            /* 1 will force focus on the fullscreen window */
static const int   showtab                 = showtab_auto; /* Default tab bar show mode */
static const int   toptab                  = True;         /* False means bottom tab bar */
static const Gap   default_gap             = {.isgap = 1, .realgap = 20, .gappx = 20};
static const int   showsystray             = 1;            /* 0 means no systray */
static const uint  systraypinning          = 0;            /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const uint  systrayonleft           = 0;            /* 0: systray in the right corner, >0: systray on left of status text */
static const uint  systrayspacing          = 2;            /* systray spacing */
static const int   systraypinningfailfirst = 1;            /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/

static const char *fonts[] = {
    "SF Mono:size=10",
    "NotoColorEmoji:size=11"
};

static const Layout layouts[] = {
    /* symbol arrange function */
    { "[]=",   tile },    /* first entry is default */
    { "[M]",   monocle },
    { "><>",   NULL },    /* no layout function means floating behavior */
    { "[[D]]", deck },
};

static const char dmenufont[] = "monospace:size=10";
static const char col_gray1[] = "#222222";
static const char col_gray2[] = "#444444";
static const char col_gray3[] = "#bbbbbb";
static const char col_gray4[] = "#eeeeee";
static const char col_cyan[]  = "#004466";
static const char selborderclr[]  = "#99BBDD";
static const char *colors[][3] = {
    /*               fg         bg         border   */
    [SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
    [SchemeSel]  = { col_gray4, col_cyan,  selborderclr  },
};

#define VASTR(...) ((const char*[]){__VA_ARGS__, NULL})
#define CMD(...)   { .v = VASTR( __VA_ARGS__) }
#define SHCMD(cmd) { .v = VASTR("/bin/sh", "-c", cmd) }

#define MODS \
    MOD(Mod, Mod4Mask) \
    MOD(Alt, Mod1Mask)

enum {
#define MOD(name, mod)                 \
    name            = mod,             \
    Shift##name     = mod|ShiftMask,   \
    Ctrl##name      = mod|ControlMask, \
    CtrlShift##name = mod|ShiftMask|ControlMask,
    MODS
#undef MOD
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
#define TAGKEYS \
    TAGKEY(XK_1, 0) \
    TAGKEY(XK_2, 1) \
    TAGKEY(XK_3, 2) \
    TAGKEY(XK_4, 3) \
    TAGKEY(XK_5, 4) \
    TAGKEY(XK_6, 5) \
    TAGKEY(XK_7, 6) \
    TAGKEY(XK_8, 7) \
    TAGKEY(XK_9, 8) \

#define SCRATCHPADS \
    SP(0, Mod, XK_g, 2, "log", VASTR("st", "-n", "log", "-e", "log")) \
    SP(1, Mod, XK_c, 2, "mate-calc", VASTR("mate-calc")) \
    SP(2, Mod, XK_i, 1, "gpick", VASTR("gpick")) \

typedef struct {
    const char *name;
    const void *cmd;
} Sp;
static Sp scratchpads[] = {
    /* name, cmd  */
    #define SP(tagnum, mod, key, isfloating, name, cmd) \
        {name, cmd},
        SCRATCHPADS
    #undef SP
};

static const Rule rules[] = {
    /* xprop(1):
     *  WM_CLASS(STRING) = instance, class
     *  WM_NAME(STRING) = title
     *  isfloatng: 0: noflaot, 1: float, 2: float and center
     */
    /* class              instance    title       tags mask     isfloating   monitor */
    { "Nitroge",          NULL,       NULL,       0,            1,           -1 },
    { "Blueman-manager",  NULL,       NULL,       0,            1,           -1 },
    { "Pavucontrol",      NULL,       NULL,       0,            1,           -1 },
    { "Gnome-calculator", NULL,       NULL,       0,            2,           -1 },
    { "VirtualBox",       NULL,       NULL,       0,            1,           -1 },
    { "Firefox",          NULL,       NULL,       1 << 8,       0,           -1 },
    #define SP(tagnum, mod, key, isfloating, name, cmd) \
        { NULL, name, NULL, SPTAG(tagnum), isfloating, -1 },
        SCRATCHPADS
    #undef SP
};

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };

static const Key keys[] = {
    /* modifier                     key                function        argument */
    #define SP(tagnum, mod, key, isfloating, name, cmd) \
        {mod, key, togglescratch, {.ui = tagnum }},
        SCRATCHPADS
    #undef SP
    #define TAGKEY(key, tagnum) \
        { Mod,          key, view,       {.ui = 1 << tagnum} }, \
        { CtrlMod,      key, toggleview, {.ui = 1 << tagnum} }, \
        { ShiftMod,     key, tag,        {.ui = 1 << tagnum} }, \
        { CtrlShiftMod, key, toggletag,  {.ui = 1 << tagnum} },
        TAGKEYS
    #undef TAGKEY
    { Mod,      XK_0,            view,           {.ui = ~0 } },
    { ShiftMod, XK_0,            tag,            {.ui = ~0 } },
    { Mod,      XK_comma,        focusmon,       {.i = -1 } },
    { Mod,      XK_period,       focusmon,       {.i = +1 } },
    { ShiftMod, XK_comma,        tagmon,         {.i = -1 } },
    { ShiftMod, XK_period,       tagmon,         {.i = +1 } },
    { ShiftMod, XK_x,            spawn,          CMD("rofi", "-show", "run") },
    { Mod,      XK_x,            spawn,          {.v = dmenucmd } },
    { Mod,      XK_t,            spawn,          CMD("st") },
    { Mod,      XK_a,            spawn,          CMD("rofi", "-show", "drun") },
    { Mod,      XK_r,            spawn,          CMD("st", "-e", "ranger") },
    { Mod,      XK_b,            spawn,          CMD("pcmanfm") },
    { ShiftMod, XK_b,            spawn,          CMD("nitrogen", "--random", "--set-zoom-fill", "--save") },
    { ShiftMod, XK_p,            spawn,          SHCMD("source ~/.xprofile") },
    { ShiftMod, XK_d,            spawn,          CMD("xkill") },
    { Mod,      XK_slash,        spawn,          CMD("dmenu-emoji", "!") },
    { ShiftMod, XK_slash,        spawn,          CMD("dmenu-emoji") },
    { CtrlMod,  XK_b,            togglebar,      {0} },
    { Mod,      XK_backslash,    setgaps,        {.i = GAP_TOGGLE} },
    { Alt,      XK_backslash,    setgaps,        {.i = +5} },
    { ShiftAlt, XK_backslash,    setgaps,        {.i = -5} },
    { Mod,      XK_w,            tabmode,        {-1} },
    { Mod,      XK_j,            focusstack,     {.i = +1 } },
    { Mod,      XK_k,            focusstack,     {.i = -1 } },
    { ShiftMod, XK_j,            movestack,      {.i = +1 } },
    { ShiftMod, XK_k,            movestack,      {.i = -1 } },
    { Mod,      XK_p,            incnmaster,     {.i = -1 } },
    { Mod,      XK_n,            incnmaster,     {.i = +1 } },
    { Mod,      XK_h,            setmfact,       {.f = -0.03} },
    { Mod,      XK_l,            setmfact,       {.f = +0.03} },
    { Mod,      XK_s,            zoom,           {0} },
    { Mod,      XK_Tab,          view,           {0} },
    { Mod,      XK_d,            killclient,     {0} },
    { Mod,      XK_m,            setlayout,      {0} },
    { Mod,      XK_v,            setlayout,      {.v = &layouts[3]} },
    { Mod,      XK_f,            togglefullscr,  {0} },
    { ShiftMod, XK_f,            togglefloating, {0} },
    { Mod,      XK_bracketright, nextlayout,     {.i = +1} },
    { Mod,      XK_bracketleft,  nextlayout,     {.i = -1} },
    { ShiftMod, XK_r,            self_restart,   {0} },
    { ShiftMod, XK_q,            quit,           {0} },

#define screenshot_path "$HOME/Pictures/screenshot-$(date +%y-%m-%d-%H-%M-%S).png"
#define notif "notify-send 'Screenshot taken!'"
    {0,   XK_Print, spawn, SHCMD("import -window root " screenshot_path " && " notif) },
    {Mod, XK_Print, spawn, SHCMD("import " screenshot_path " && " notif) },
#undef notif
#define notif "notify-send -r 1 -t 2000 -i audio-volume-medium "
    {0, XF86XK_AudioRaiseVolume,  spawn, SHCMD(notif"\"volume: $(pamixer --allow-boost --increase 5 --get-volume-human)\"") },
    {0, XF86XK_AudioLowerVolume,  spawn, SHCMD(notif"\"volume: $(pamixer --allow-boost --decrease 5 --get-volume-human)\"") },
    {0, XF86XK_AudioMute,         spawn, SHCMD(notif"\"volume: $(pamixer --toggle-mute  --get-volume-human)\"") },
    {0, XF86XK_AudioMicMute,      spawn, SHCMD(notif"\"mic: $(pactl set-source-mute @DEFAULT_SOURCE@ toggle && pactl get-source-mute @DEFAULT_SOURCE@)\"") },
#undef notif
    {0, XF86XK_AudioPrev,         spawn, CMD("playerctl", "prev") },
    {0, XF86XK_AudioNext,         spawn, CMD("playerctl", "next") },
    {0, XF86XK_AudioPause,        spawn, CMD("playerctl", "play-pause") },
    {0, XF86XK_AudioPlay,         spawn, CMD("playerctl", "play-pause") },
    {0, XF86XK_AudioStop,         spawn, CMD("playerctl", "stop") },
    {0, XF86XK_MonBrightnessUp,   spawn, CMD("xbacklight", "-inc", "15") },
    {0, XF86XK_MonBrightnessDown, spawn, CMD("xbacklight", "-dec", "15") },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */

#define LeftClick   Button1
#define MiddleClick Button2
#define RightClick  Button3
#define ScrollUp    Button4
#define ScrollDown  Button5

static const Button buttons[] = {
    /* click + mask       button   function        argument */
    { ClkLtSymbol,   0,   LeftClick,   setlayout,         {0} },
    { ClkLtSymbol,   0,   RightClick,  setlayout,         {.v = &layouts[2]} },
    { ClkWinTitle,   0,   MiddleClick, zoom,              {0} },
    { ClkStatusText, 0,   LeftClick,   sigstatusbar,      {.i = 1} },
    { ClkStatusText, 0,   MiddleClick, sigstatusbar,      {.i = 2} },
    { ClkStatusText, 0,   RightClick,  sigstatusbar,      {.i = 3} },
    { ClkStatusText, 0,   ScrollUp,    sigstatusbar,      {.i = 4} },
    { ClkStatusText, 0,   ScrollDown,  sigstatusbar,      {.i = 5} },
    { ClkClientWin,  Mod, LeftClick,   movemouse,         {0} },
    { ClkClientWin,  Mod, MiddleClick, togglefloating,    {0} },
    { ClkClientWin,  Mod, RightClick,  resizemouse,       {0} },
    { ClkTagBar,     0,   LeftClick,   view,              {0} },
    { ClkTagBar,     0,   RightClick,  toggleview,        {0} },
    { ClkTagBar,     Mod, LeftClick,   tag,               {0} },
    { ClkTagBar,     Mod, RightClick,  toggletag,         {0} },
    { ClkTabBar,     0,   LeftClick,   focuswin,          {0} },
    { ClkTabBar,     0,   MiddleClick, tabkillclient,     {0} },
    { ClkTabBar,     0,   RightClick,  tabtogglefloating, {0} },
};

