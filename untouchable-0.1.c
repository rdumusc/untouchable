/*  
    untouchable - filter all input and make Kindle Touch "untouchable" :)
    Copyright (c) 2012 by baf (www.fabiszewski.net), with MIT license:
    http://www.opensource.org/licenses/mit-license.php

    gcc untouchable.c -O2 -Wall -lX11 -o untouchable
*/

#include <stdio.h> // printf
#include <stdbool.h> // bool
#include <string.h> // memset
#include <stdlib.h> // strtol
#include <unistd.h> // usleep

#if defined(__GNUC__) && (__GNUC__ >= 4)
# define _X_SENTINEL(x) __attribute__ ((__sentinel__(x)))
# define _X_ATTRIBUTE_PRINTF(x,y) __attribute__((__format__(__printf__,x,y)))
#else
# define _X_SENTINEL(x)
# define _X_ATTRIBUTE_PRINTF(x,y)
#endif /* GNUC >= 4 */

#include <X11/keysym.h>
#include <X11/XKBlib.h>

void pageForward(Display *display);
void pageBackward(Display *display);
void SendMouseEvent(Display *display, int button, int x, int y, bool pressed);
void usage();

bool debug = false;

int main(int argc, char * argv[]) {
	Window root;
	Display * display;
	XEvent xev;
	Time clickTime = CurrentTime;
	int longPressDuration = 1500;
	int timeDiff;
	int screenWidth;
	
	// parse options
	while ((argc > 1) && (argv[1][0] == '-')){
		switch (argv[1][1]){
			case 't':
				++argv;
				if (--argc <= 1){
					printf("Option -t requires numeric argument\n");
					usage();
					return 1;
				}
				longPressDuration = strtol(argv[1],0,0); 
				break;
                        case 'd':
				debug = true;
                                break;
                        case 'h':
				usage();
                                return 0;
		}
		++argv;
		--argc;
	}

	if(debug) printf("Long press duration: %i\n", longPressDuration);

	display = XOpenDisplay(NULL);
	if(display == NULL) {
		if(debug) printf("Could not open display\n");
		return 1;
	}

	root = DefaultRootWindow(display);
	screenWidth = WidthOfScreen(DefaultScreenOfDisplay(display));
	if(debug) printf("Screen width: %i\n", screenWidth);

	XAllowEvents(display, AsyncBoth, CurrentTime);
	// grab key events
	if(debug) printf("Grab key events\n");
	XGrabKeyboard(display, root, true, GrabModeAsync, GrabModeAsync, CurrentTime); 

	bool go = true;
	bool grabOn = false; // we start grabbing mouse events only after first press of home button
	while(go){
		XNextEvent(display, &xev);
	        int keycode = (int) XLookupKeysym(&xev.xkey, 0);

		switch(xev.type){
			case ButtonPress:
				if(debug) printf("Button press - %i\n", xev.xbutton.button);
				if(debug) printf("Coordinates - %i, %i\n", xev.xbutton.x, xev.xbutton.y);
			break;

			case ButtonRelease:
				if(debug) printf("Button release - %i\n", xev.xbutton.button);
				if(xev.xbutton.x>screenWidth-50 && xev.xbutton.y<50)
					go = false;
			break;
			case KeyPress:
				clickTime = xev.xkey.time;
				if(debug) printf("Key press - %i\n", keycode);
			break;
			case KeyRelease:
				timeDiff = (long) xev.xkey.time - (long) clickTime;
				if(debug) printf("Key release - %i\n", keycode);
				if(debug) printf("Key press duration - %i\n", timeDiff);
				// FIXME: test for keycode is rather useless :)
			        if(keycode == XK_Home && grabOn){
					if(timeDiff < longPressDuration)
						pageForward(display);
					else
						pageBackward(display);
				} else {
					// first press of home button
					if(debug) printf("Grab mouse events\n");
					grabOn = true;
					XGrabButton(display, AnyButton, AnyModifier, root, true, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None);
				}
			break;
		}
	}

	XUngrabKeyboard(display, CurrentTime);
	if(grabOn) XUngrabButton(display, AnyButton, AnyModifier, root);

	XCloseDisplay(display);

	return 0;
}

void pageForward(Display *display){
	SendMouseEvent(display, Button1, 500, 300, true);
	usleep(10000);
	SendMouseEvent(display, Button1, 500, 300, false);
}

void pageBackward(Display *display){
        SendMouseEvent(display, Button1, 50, 300, true);
        usleep(10000);
        SendMouseEvent(display, Button1, 50, 300, false);
}

void SendMouseEvent(Display *display, int button, int x, int y, bool pressed){
	XEvent event;
	memset(&event, 0, sizeof(event));
	event.xbutton.type = pressed ? ButtonPress : ButtonRelease;
	event.xbutton.button = button;
	event.xbutton.same_screen = true;
	XQueryPointer(display, RootWindow(display, DefaultScreen(display)), 
		&event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, 
		&event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	
	event.xbutton.subwindow = event.xbutton.window;
	
	while(event.xbutton.subwindow){
		event.xbutton.window = event.xbutton.subwindow;
		
		XQueryPointer(display, event.xbutton.window, 
			&event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, 
			&event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
	}
	event.xbutton.x = x;
	event.xbutton.y = y;
	if(debug) printf("Sending %i %i event: %ix%i\n", event.xbutton.type, event.xbutton.button, x, y); 

	if (!XSendEvent(display, PointerWindow, true, 0xfff, &event))
		printf("Failed to send mouse event\n");
	XFlush(display);
}

void usage(){
	printf("Usage: untouchable [OPTION]\n");
	printf("	-t <milliseconds> - minimal time home button should be hold to be treated as long press (default 1500)\n");
	printf("	-d               - debug\n");
	printf("	-h               - show this message\n");
}
