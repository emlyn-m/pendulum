// main.cpp

#include "Matrix.h"
#include "Ball.h"
#include "constants.h"
#include <vector>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>

#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xutil.h>

#define USE_CHOOSE_FBCONFIG


static int Xscreen;
static Atom del_atom;
static Colormap cmap;
static Display *Xdisplay;
static XVisualInfo *visual;
static XRenderPictFormat *pict_format;
static GLXFBConfig *fbconfigs, fbconfig;
static int numfbconfigs;
static GLXContext render_context;
static Window Xroot, window_handle;
static GLXWindow glX_window_handle;
static int width, height;

static int VisData[] = {
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_DOUBLEBUFFER, True,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DEPTH_SIZE, 16,
    None
};

using namespace constants;

static void fatalError(const char *why) {
	fprintf(stderr, "%s", why);
	exit(0x666);
}


void decompose(Matrix A, Matrix* lower, Matrix* upper) {
    
    float sum;
    float lSum;
    
    for (int i=0; i<numBalls; i++) {
        for (int k=i; k<numBalls; k++) {
            sum = 0;
            for (int j=0; j<i; j++)
                sum += ((*lower).values[i][j] * (*upper).values[j][k]);
 
            (*upper).values[i][k] = A[i][k] - sum;
        }
        for (int k=i; k<numBalls; k++) {
            if (i == k) {
                (*lower).values[i][i] = 1;
            } else {
                lSum = 0;
                for (int j = 0; j < i; j++) {
                    lSum += ((*lower).values[k][j] * (*upper).values[j][i]);
                }
                
                (*lower).values[k][i] = ((float) (A[i][k] - lSum)) / ((float) (*upper).values[i][i]);
            }
        }
    }
}

Matrix solve(Matrix A, Matrix b) {
    Matrix lower(numBalls, numBalls);
    Matrix upper(numBalls, numBalls);
    
        
    decompose(A, &lower, &upper);

    Matrix y(numBalls, 1);
    
    float sum;
    for (int i=0; i<numBalls; i++) {
        sum = 0;
        
        for (int j=0; j<i-1; j++) {
            sum += lower.values[i][j] * *y[j];
        }
        
        *y[i] = (*b.values[i] - sum) / lower.values[i][i];
    }
    
    
    Matrix x(numBalls, 1);
    
    for (int i=numBalls-1; i>0; i--) {
        sum = 0;
        for (int j=i+1; j>numBalls; j--) { 
            sum += upper.values[i][j] * *x[j];
        }
        *x[i] = (*y[i] - sum) / upper.values[i][i]; 
    }
    
    return x;
}



Matrix generateA(Ball* balls) {
    Matrix A(numBalls, numBalls);
    
    for (int i=1; i<=numBalls; i++) {
        for (int j=1; j<=numBalls; j++) {
            A[i-1][j-1] = stringLength * cos(balls[i].angle - balls[j].angle) * (numBalls - std::max(i, j)) * ballMass;
        }
    }
    
    return A;
    
}

Matrix generateB(Ball* balls) {
    Matrix b(numBalls, 1);
    
    for (int i=1; i<=numBalls; i++) {
        *b[i-1] = (-gravity * sin(balls[i].angle) * (numBalls - i) * ballMass);
        
        for (int j=1; j<=numBalls; j++) {
            *b[i-1] -= (stringLength * (sin(balls[i].angle - balls[j].angle) + (pow(balls[j].angle, 2) * sin(balls[i].angle - balls[j].angle)) * (numBalls - std::max(i, j)) * ballMass));
        }
    }
    
    return b;
}
    

static int isExtensionSupported(const char *extList, const char *extension) {
 
  const char *start;
  const char *where, *terminator;
 
  /* Extension names should not have spaces. */
  where = strchr(extension, ' ');
  if ( where || *extension == '\0' )
    return 0;
 
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  for ( start = extList; ; ) {
    where = strstr( start, extension );
 
    if ( !where )
      break;
 
    terminator = where + strlen( extension );
 
    if ( where == start || *(where - 1) == ' ' )
      if ( *terminator == ' ' || *terminator == '\0' )
        return 1;
 
    start = terminator;
  }
  return 0;
}

static Bool WaitForMapNotify(Display *d, XEvent *e, char *arg) {
	return d && e && arg && (e->type == MapNotify) && (e->xmap.window == *(Window*)arg);
}

static void describe_fbconfig(GLXFBConfig fbconfig) {
	int doublebuffer;
	int red_bits, green_bits, blue_bits, alpha_bits, depth_bits;

	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_DOUBLEBUFFER, &doublebuffer);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_RED_SIZE, &red_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_GREEN_SIZE, &green_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_BLUE_SIZE, &blue_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_ALPHA_SIZE, &alpha_bits);
	glXGetFBConfigAttrib(Xdisplay, fbconfig, GLX_DEPTH_SIZE, &depth_bits);

	fprintf(stderr, "FBConfig selected:\n"
		"Doublebuffer: %s\n"
		"Red Bits: %d, Green Bits: %d, Blue Bits: %d, Alpha Bits: %d, Depth Bits: %d\n",
		doublebuffer == True ? "Yes" : "No", 
		red_bits, green_bits, blue_bits, alpha_bits, depth_bits);
}

static void windowCreate() {
	XEvent event;
	int x,y, attr_mask;
	XSizeHints hints;
	XWMHints *startup_state;
	XTextProperty textprop;
	XSetWindowAttributes attr = {0,};
	static const char *title = "Pendulum";

	Xdisplay = XOpenDisplay(NULL);
	if (!Xdisplay) {
		fatalError("Couldn't connect to X server\n");
	}
	Xscreen = DefaultScreen(Xdisplay);
	Xroot = RootWindow(Xdisplay, Xscreen);

	fbconfigs = glXChooseFBConfig(Xdisplay, Xscreen, VisData, &numfbconfigs);
	fbconfig = 0;
	for(int i = 0; i<numfbconfigs; i++) {
		visual = (XVisualInfo*) glXGetVisualFromFBConfig(Xdisplay, fbconfigs[i]);
		if(!visual)
			continue;

		pict_format = XRenderFindVisualFormat(Xdisplay, visual->visual);
		if(!pict_format)
			continue;

		fbconfig = fbconfigs[i];
		if(pict_format->direct.alphaMask > 0) {
			break;
		}
		XFree(visual);
	}

	if(!fbconfig) {
		fatalError("No matching FB config found");
	}

	describe_fbconfig(fbconfig);

	/* Create a colormap - only needed on some X clients, eg. IRIX */
	cmap = XCreateColormap(Xdisplay, Xroot, visual->visual, AllocNone);

	attr.colormap = cmap;
	attr.background_pixmap = None;
	attr.border_pixmap = None;
	attr.border_pixel = 0;
	attr.event_mask =
		StructureNotifyMask |
		EnterWindowMask |
		LeaveWindowMask |
		ExposureMask |
		ButtonPressMask |
		ButtonReleaseMask |
		OwnerGrabButtonMask |
		KeyPressMask |
		KeyReleaseMask;

	attr_mask = 
	//	CWBackPixmap|
		CWColormap|
		CWBorderPixel|
		CWEventMask;

	width = DisplayWidth(Xdisplay, DefaultScreen(Xdisplay))/2;
	height = DisplayHeight(Xdisplay, DefaultScreen(Xdisplay))/2;
	x=width/2, y=height/2;

	window_handle = XCreateWindow(	Xdisplay,
					Xroot,
					x, y, width, height,
					0,
					visual->depth,
					InputOutput,
					visual->visual,
					attr_mask, &attr);

	if( !window_handle ) {
		fatalError("Couldn't create the window\n");
	}

#if USE_GLX_CREATE_WINDOW
	fputs("glXCreateWindow ", stderr);
	int glXattr[] = { None };
	glX_window_handle = glXCreateWindow(Xdisplay, fbconfig, window_handle, glXattr);
	if( !glX_window_handle ) {
		fatalError("Couldn't create the GLX window\n");
	}
#else
	glX_window_handle = window_handle;
#endif

	textprop.value = (unsigned char*)title;
	textprop.encoding = XA_STRING;
	textprop.format = 8;
	textprop.nitems = strlen(title);

	hints.x = x;
	hints.y = y;
	hints.width = width;
	hints.height = height;
	hints.flags = USPosition|USSize;

	startup_state = XAllocWMHints();
	startup_state->initial_state = NormalState;
	startup_state->flags = StateHint;

	XSetWMProperties(Xdisplay, window_handle,&textprop, &textprop,
			NULL, 0,
			&hints,
			startup_state,
			NULL);

	XFree(startup_state);

	XMapWindow(Xdisplay, window_handle);
	XIfEvent(Xdisplay, &event, WaitForMapNotify, (char*)&window_handle);

	if ((del_atom = XInternAtom(Xdisplay, "WM_DELETE_WINDOW", 0)) != None) {
		XSetWMProtocols(Xdisplay, window_handle, &del_atom, 1);
	}
}


static int ctxErrorHandler( Display *dpy, XErrorEvent *ev ) {
    fputs("Error at context creation\n", stderr);
    return 0;
}

static void renderContextCreate() {
	int dummy;
	if (!glXQueryExtension(Xdisplay, &dummy, &dummy)) {
		fatalError("OpenGL not supported by X server\n");
	}

#if USE_GLX_CREATE_CONTEXT_ATTRIB
	#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
	#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
	render_context = NULL;
	if( isExtensionSupported( glXQueryExtensionsString(Xdisplay, DefaultScreen(Xdisplay)), "GLX_ARB_create_context" ) ) {
		typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
		glXCreateContextAttribsARBProc glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB("glXCreateContextAttribsARB" );
		if( glXCreateContextAttribsARB ) {
			int context_attribs[] =
			{
				GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
				GLX_CONTEXT_MINOR_VERSION_ARB, 1,
				//GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
				None
			};

			int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&ctxErrorHandler);
			
			render_context = glXCreateContextAttribsARB( Xdisplay, fbconfig, 0, True, context_attribs );

			XSync( Xdisplay, False );
			XSetErrorHandler( oldHandler );
		} else {
			fputs("glXCreateContextAttribsARB could not be retrieved\n", stderr);
		}
	} else {
			fputs("glXCreateContextAttribsARB not supported\n", stderr);
	}

	if(!render_context)
	{
		fputs("using fallback\n", stderr);
#else
	{
#endif
		render_context = glXCreateNewContext(Xdisplay, fbconfig, GLX_RGBA_TYPE, 0, True);
		if (!render_context) {
			fatalError("Failed to create a GL context\n");
		}
	}

	if (!glXMakeContextCurrent(Xdisplay, glX_window_handle, glX_window_handle, render_context)) {
		fatalError("glXMakeCurrent failed for window\n");
	}
}

    
static int messageQueueUpdate() {
	XEvent event;
	XConfigureEvent *xc;

	while (XPending(Xdisplay))
	{
		XNextEvent(Xdisplay, &event);
		switch (event.type)
		{
		case ClientMessage:
			if (event.xclient.data.l[0] == del_atom)
			{
				return 0;
			}
		break;

		case ConfigureNotify:
			xc = &(event.xconfigure);
			width = xc->width;
			height = xc->height;
			break;
		}
	}
	return 1;
}

    

    
static void windowRedraw() {
	float const aspect = (float)width / (float)height;
    
	glDrawBuffer(GL_BACK);

	glViewport(0, 0, std::min(width, height), std::min(width, height));

	// Clear with alpha = 0.0, i.e. full transparency
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

#if 0
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

    
float maxDist(Ball* balls) {
    float currMaxDist = 99999.f;
    float tmpDist = 0.f;
    
    for (int i=0; i<=numBalls; i++) {
        for (int j=0; j<=numBalls; j++) {
            if (balls[i].real && balls[j].real && i != j) {
                tmpDist = pow(pow(balls[i].position[0] - balls[j].position[0], 2) + pow(balls[i].position[1] - balls[j].position[1], 2), .5);
                std::cout << 't'<<tmpDist << std::endl;
                if (tmpDist > currMaxDist) { currMaxDist = tmpDist; }
            }
        }
    }
    return currMaxDist;
}
    
int main() {
    
    std::chrono::system_clock::time_point a = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point b = std::chrono::system_clock::now();
    
    srand(time(0));
    
    Ball* balls = new Ball[numBalls+1];
    
    Matrix AMat(numBalls, numBalls);
    Matrix bMat(numBalls, 1);
            
    //initialize balls
    balls[0] = Ball(rootPosition);
    for (int i=1; i<=numBalls; i++) {
        std::cout << i << std::endl;
        do {
            balls[i] = Ball(balls[i-1].position);
        } while (maxDist(balls) <= 2*drawCircleSize);
    }
    windowCreate();
    renderContextCreate();
    
    float timeNow = time(0);
    
    
    
    while (messageQueueUpdate()) {
        
        //limit mspt code
        a = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> work_time = a - b;
        
        if (work_time.count() < pendulumMspt) {
            std::chrono::duration<double, std::milli> delta_ms(pendulumMspt - work_time.count());
            auto delta_ms_duration = std::chrono::duration_cast<std::chrono::milliseconds>(delta_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(delta_ms_duration.count()));
        }
        b = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> sleep_time = b - a;
        
        
        AMat = generateA(balls);
        bMat = generateB(balls);
        
        Matrix angularAccels = solve(AMat, bMat);
        angularAccels.display("alpha");
        
        windowRedraw();

        balls[0].draw();

        for (int i=1; i<=numBalls; i++) {
            
            std::cout << balls[i].position[0] << ' ' << balls[i].position[1] << std::endl;
            
            balls[i].tick(pendulumMspt, *angularAccels[i-1], balls[i].position);
            balls[i].draw();
        }
        
        timeNow = time(0);
        
        glXSwapBuffers(Xdisplay, glX_window_handle);

    }
    
    
    return 0;
}