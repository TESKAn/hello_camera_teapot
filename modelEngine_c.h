#ifndef _MODEL_ENGINE_C_H_
#define _MODEL_ENGINE_C_H_

typedef struct
{
	uint32_t screen_width;
	uint32_t screen_height;
	// OpenGL|ES objects
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
} MODEL_STATE_T;

void modelEngine_init_ogl(MODEL_STATE_T *state);
void init_model_proj(MODEL_STATE_T *state);
void init_textures(MODEL_STATE_T *state);
GLfloat inc_and_clip_distance(GLfloat distance, GLfloat distance_inc);

#endif