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

	GLuint verbose;
	GLuint vshader;
	GLuint fshader;
	GLuint mshader;
	GLuint program;
	GLuint program2;
	GLuint tex_fb;
	GLuint tex;
	GLuint buf;
	// julia attribs
	GLuint unif_color, attr_vertex, unif_scale, unif_offset, unif_tex, unif_centre;
	// mandelbrot attribs
	GLuint attr_vertex2, unif_scale2, unif_offset2, unif_centre2;


} MODEL_STATE_T;

void modelEngine_init_ogl(MODEL_STATE_T *state);
void init_model_proj(MODEL_STATE_T *state);
void init_textures(MODEL_STATE_T *state);
GLfloat inc_and_clip_distance(GLfloat distance, GLfloat distance_inc);
void init_shaders(MODEL_STATE_T *state);

#endif