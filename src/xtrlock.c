/*
 * xtrlock.c
 *
 * X Transparent Lock
 *
 * Copyright (C)1993,1994 Ian Jackson
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xos.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>

#include "config.h"

#include "lock.bitmap"
#include "mask.bitmap"

//#define DEBUGPRN
#include "dbg.h"

#define TIMEOUTPERATTEMPT 30000
#define MAXGOODWILL  (TIMEOUTPERATTEMPT*5)
#define INITIALGOODWILL MAXGOODWILL
#define GOODWILLPORTION 0.3
enum { MNONE, MBG, MBLANK };

enum { AUTH_NONE, AUTH_NOW, AUTH_FAILED, AUTH_MAX };

static struct {
    char *fg, *bg;
    Cursor c;
} cursors[AUTH_MAX] = {
    [ AUTH_NONE ] = { .bg = "steelblue3", .fg = "grey25" },
    [ AUTH_NOW ] = { .bg = "cadetblue3", .fg = "grey25" },
    [ AUTH_FAILED ] = { .bg = "red", .fg = "grey25" },
};

Display *display;
Window root;
int screen;
Colormap cmap;

int bg_action = MBG;
char *pam_module = "system-local-login";

int auth_pam(char *user, char *password, char *module);

int passwordok(char *password)
{
    return auth_pam(getenv("USER"), password, pam_module);
}

#define err(fmt, args...)                       \
    do { fprintf(stderr, fmt, ## args); } while(0)


void
handle_error(Display * d, XErrorEvent * ev)
{
    char buf[256];

    XGetErrorText(display, ev->error_code, buf, 256);
    DBG("X error: %s\n", buf);
}

Window
create_window(unsigned long values, XSetWindowAttributes *attrib)
{
    return XCreateWindow(display, root,
        0, 0,
        DisplayWidth(display, screen),
        DisplayHeight(display, screen),
        0, CopyFromParent, CopyFromParent, CopyFromParent,
        values, attrib);
}

/*
 * Obtains pixmap from a root window property.
 * @aname: name of the root's property, eg _XROOTPMAP_ID
 * It does these checks
 *  - property exists
 *  - its value is a pixmap
 *  - that pixmap still exists
 * Return: pixmap on success, None otherwise
 */
static Pixmap
get_prop_pixmap(char *aname)
{
    int rc, rf, idummy;
    unsigned long nitems, ldummy;
    unsigned char *ret;
    unsigned int uidummy;
    Atom pa = XInternAtom(display, aname, True);
    Atom rpa;
    Pixmap pix = None;
    Window rw = None;
    
    DBG("atom: name %s, id %lu\n", aname, pa);
    if (pa == None)
        return None;
    rc = XGetWindowProperty(display, root,
        pa, 0, 1, False, XA_PIXMAP,
        &rpa, &rf, &nitems, &ldummy, &ret);
    if (rc != Success) {
        DBG("can't get '%s' prop\n", aname);
        return None;
    }
    DBG("nitems %lu, format %d, type %lu, pixmap type %lu\n", nitems, rf,
        rpa, XA_PIXMAP);
    if (nitems == 1 && rf == 32 && rpa == XA_PIXMAP) {
        pix = *((Pixmap *) ret); 
    }
    XFree(ret);
    DBG("pix 0x%lx\n", pix);
    if (pix == None)
        return None;
    
    rc = XGetGeometry(display, pix, &rw, &idummy, &idummy,
        &uidummy, &uidummy, &uidummy, &uidummy);
    DBG("XGetGeometry %d; rwin %lx\n", rc, rw);
    if (rw != None)
        return pix;

    return None;
}


static Pixmap
get_bg_pixmap()
{
    Pixmap pix = None;

    pix = get_prop_pixmap("_XROOTPMAP_ID");
    if (pix != None)
        return pix;
    ERR("root['_XROOTPMAP_ID'] is not valid pixmap\n");
    
    return None;
}


/* If window creation fails, Xlib will exit an application */
Window
create_window_full(int mode)
{
    XSetWindowAttributes attrib;
    unsigned long values = 0;
    Window window;
    Pixmap pix = None;
    
    values = CWOverrideRedirect;
    attrib.override_redirect = True;

    if (mode == MNONE) {
        window = XCreateWindow(display, root, 0, 0, 1, 1, 0,
            CopyFromParent, InputOnly, CopyFromParent, values, &attrib);
        XSync(display, False);
        return window;
    }
    if (mode == MBG) {
        pix = get_bg_pixmap();
        if (pix == None) {
            ERR("Can't get bg pixmap. Falling back to 'blank' mode'\n");
            mode = MBLANK;
        }
    }

    if (mode == MBG) {
        values |= CWBackPixmap;
        attrib.background_pixmap = pix;
        window = create_window(values, &attrib);
    } else if (mode == MBLANK) {
        values |= CWBackPixel;
        attrib.background_pixel = BlackPixel(display, screen);
        window = create_window(values, &attrib);
    } else
        exit(1);

    XClearWindow(display, window);
    XSync(display, False);
    return window;
}

static void
create_cursors(void)
{
    Pixmap csr_source,csr_mask;
    XColor dummy, def_bg, def_fg, bg, fg;
    int i;
    
    csr_source = XCreateBitmapFromData(display, root, lock_bits,
        lock_width, lock_height);
    csr_mask = XCreateBitmapFromData(display, root, mask_bits, mask_width,
        mask_height);

    if (!XAllocNamedColor(display, cmap, "white", &dummy, &def_fg)
        || !XAllocNamedColor(display, cmap, "black", &dummy, &def_bg)) {
        err("Can't allocate basic colors: black, white\n");
        exit(1);
    }
     
    for (i = 0; i < AUTH_MAX; i++) {
        if (!XAllocNamedColor(display, cmap, cursors[i].fg, &dummy, &fg))
            fg = def_fg;
        if (!XAllocNamedColor(display, cmap, cursors[i].bg, &dummy, &bg))
            bg = def_bg;
        cursors[i].c = XCreatePixmapCursor(display, csr_source, csr_mask,
            &fg, &bg, lock_x_hot, lock_y_hot);
    }
}

void
lock(int mode)
{
    XEvent ev;
    KeySym ks;
    char cbuf[10], rbuf[128];
    int clen, rlen=0, state = AUTH_NONE, old_state = -1;
    long goodwill= INITIALGOODWILL, timeout= 0;
    Window window;

    display = XOpenDisplay(0);
    if (display == NULL) {
        err("cannot open display\n");
        exit(1);
    }
    XSetErrorHandler((XErrorHandler) handle_error);
    screen = DefaultScreen(display);
    root = DefaultRootWindow(display);
    cmap = DefaultColormap(display, screen);
    window = create_window_full(mode);

    XSelectInput(display,window,KeyPressMask|KeyReleaseMask);
    create_cursors();
    XMapWindow(display,window);
    XRaiseWindow(display,window);
    XSync(display, False);
    if (XGrabKeyboard(display, window, False, GrabModeAsync, GrabModeAsync,
            CurrentTime) != GrabSuccess) {
        err("can't grab keyboard\n");
        exit(1);
    }
  
    for (;;) {
        if (rlen)
            state = AUTH_NOW;
        else
            state = AUTH_NONE;
        if (old_state != state) {
            old_state = state;
            if (XGrabPointer(display, window, False,
                    0, GrabModeAsync, GrabModeAsync, None,
                    cursors[state].c, CurrentTime) != GrabSuccess)
                err("can't grab pointer\n");
        }

        XNextEvent(display,&ev);
        switch (ev.type) {
        case KeyPress:
            if (ev.xkey.time < timeout) {
                XBell(display,0);
                break;
            }
            clen= XLookupString(&ev.xkey,cbuf,9,&ks,0);
            switch (ks) {
            case XK_Escape: case XK_Clear:
                rlen=0; break;
            case XK_Delete: case XK_BackSpace:
                if (rlen>0)
                    rlen--;
                break;
            case XK_Linefeed: case XK_Return:
                if (rlen==0)
                    break;
                rbuf[rlen]=0;
                XGrabPointer(display, window, False,
                    0, GrabModeAsync, GrabModeAsync, None,
                    cursors[AUTH_FAILED].c, CurrentTime);
                if (passwordok(rbuf))
                    exit(0);
                XBell(display,0);
                rlen= 0;
                if (timeout) {
                    goodwill+= ev.xkey.time - timeout;
                    if (goodwill > MAXGOODWILL)
                        goodwill= MAXGOODWILL;
                }
                timeout = -goodwill*GOODWILLPORTION;
                goodwill += timeout;
                timeout += ev.xkey.time + TIMEOUTPERATTEMPT;
                break;
            default:
                if (clen != 1)
                    break;
                /* allow space for the trailing \0 */
                if (rlen < (sizeof(rbuf) - 1)) {
                    rbuf[rlen]=cbuf[0];
                    rlen++;
                }
                break;
            }
            break;
        default:
            break;
        }
    }
}

void
help()
{
    printf("%s %s - PAM based X11 screen locker\n",
        PROJECT_NAME, PROJECT_VERSION);
    printf("Usage: %s [options...]\n", PROJECT_NAME);
    printf("Options:\n");
    printf(" -h      This help message\n");
    printf(" -p MOD  PAM module, default is system-local-login\n");
    printf(" -b BG   background action, none, blank or bg, default is bg\n");
}

int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "b:p:h")) != -1) {
        switch (opt) {
        case 'h':
            help();
            exit(0);

        case 'p':
            pam_module = optarg;
            break;

        case 'b':
            if (!strcmp(optarg, "none"))
                bg_action = MNONE;
            else if (!strcmp(optarg, "blank"))
                bg_action = MBLANK;
             else if (!strcmp(optarg, "bg"))
                bg_action = MBG;
            else {
                err("unknown bg action '%s'\n", optarg);
                exit(1);
            }
            break;

        default:
            exit(1);
        }
    }
    lock(bg_action);
    return 0;
}
