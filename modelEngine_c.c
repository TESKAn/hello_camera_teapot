#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <memory.h>


#include "bcm_host.h"

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"

#include "modelEngine_c.h"

/***********************************************************
* Name: init_ogl
*
* Arguments:
*       CUBE_STATE_T *state - holds OGLES model info
*
* Description: Sets the display, OpenGL|ES context and screen stuff
*
* Returns: void
*
***********************************************************/
void modelEngine_init_ogl(MODEL_STATE_T *state)
{
	int32_t success = 0;
	EGLBoolean result;
	EGLint num_config;

	static EGL_DISPMANX_WINDOW_T nativewindow;

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 16,
		//EGL_SAMPLES, 4,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};

	static const EGLint context_attributes[] = 
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLConfig config;

	// get an EGL display connection
	state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	assert(state->display!=EGL_NO_DISPLAY);

	// initialize the EGL display connection
	result = eglInitialize(state->display, NULL, NULL);
	assert(EGL_FALSE != result);

	// get an appropriate EGL frame buffer configuration
	// this uses a BRCM extension that gets the closest match, rather than standard which returns anything that matches
	result = eglSaneChooseConfigBRCM(state->display, attribute_list, &config, 1, &num_config);
	assert(EGL_FALSE != result);

	// Bind API
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);

	// create an EGL rendering context
	state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, context_attributes);
	//state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT, NULL);
	assert(state->context!=EGL_NO_CONTEXT);

	// create an EGL window surface
	success = graphics_get_display_size(0 /* LCD */, &state->screen_width, &state->screen_height);
	assert( success >= 0 );

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = state->screen_width;
	dst_rect.height = state->screen_height;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = state->screen_width << 16;
	src_rect.height = state->screen_height << 16;        

	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );

	dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
		0/*layer*/, &dst_rect, 0/*src*/,
		&src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

	nativewindow.element = dispman_element;
	nativewindow.width = state->screen_width;
	nativewindow.height = state->screen_height;
	vc_dispmanx_update_submit_sync( dispman_update );

	state->surface = eglCreateWindowSurface( state->display, config, &nativewindow, NULL );
	assert(state->surface != EGL_NO_SURFACE);

	// connect the context to the surface
	result = eglMakeCurrent(state->display, state->surface, state->surface, state->context);
	assert(EGL_FALSE != result);

	// Set background color and clear buffers
	// Background alfa = 0
	glClearColor(0.15f, 0.25f, 0.35f, 0.0f);
	checkGLError();

	// Enable back face culling.
	glEnable(GL_CULL_FACE);
	checkGLError();

	glMatrixMode(GL_MODELVIEW);
	checkGLError();
}

/***********************************************************
* Name: init_model_proj
*
* Arguments:
*       CUBE_STATE_T *state - holds OGLES model info
*
* Description: Sets the OpenGL|ES model to default values
*
* Returns: void
*
***********************************************************/
void init_model_proj(MODEL_STATE_T *state)
{
	float nearp = 1.0f;
	float farp = 500.0f;
	float hht;
	float hwd;

	//glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
	//checkGLError();

	glViewport(0, 0, (GLsizei)state->screen_width, (GLsizei)state->screen_height);
	checkGLError();

	glMatrixMode(GL_PROJECTION);
	checkGLError();
	glLoadIdentity();
	checkGLError();

	hht = nearp * (float)tan(45.0 / 2.0 / 180.0 * M_PI);
	hwd = hht * (float)state->screen_width / (float)state->screen_height;

	glFrustumf(-hwd, hwd, -hht, hht, nearp, farp);
	checkGLError();

	glEnableClientState( GL_VERTEX_ARRAY );
	checkGLError();
}
/***********************************************************
* Name: init_textures
*
* Arguments:
*       CUBE_STATE_T *state - holds OGLES model info
*
* Description:   Initialise OGL|ES texture surfaces to use image
*                buffers
*
* Returns: void
*
***********************************************************/
void init_textures(MODEL_STATE_T *state)
{
	glEnable(GL_TEXTURE_2D);
	checkGLError();
}

/***********************************************************
* Name: inc_and_clip_distance
*
* Arguments:
*       GLfloat distance     current distance
*       GLfloat distance_inc distance increment
*
* Description:   Increments or decrements distance by distance_inc units
*                Clips to range
*
* Returns: new value of angle
*
***********************************************************/
GLfloat inc_and_clip_distance(GLfloat distance, GLfloat distance_inc)
{
	distance += distance_inc;

	if (distance >= 120.0f)
		distance = 120.f;
	else if (distance <= 40.0f)
		distance = 40.0f;

	return distance;
}
