#ifndef _MODEL_ENGINE_H_
#define _MODEL_ENGINE_H_

// Include gl.h - else it wont recognize GLuint
#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "fonts/texture-font.h"
#include "fonts/texture-atlas.h"
#include "fonts/vector.h"

// Include models.h - else wont recognize MODEL_T
extern "C" 
{
	#include "models.h"
	#include "modelEngine_c.h"
}

#define MAX_MODELS			10
#define MAX_SHADER_PROGRAMS	20

extern char shaderDataRAM1[2048];
extern char shaderDataRAM2[2048];
extern char shaderDataRAM3[2048];
extern char shaderDataRAM4[2048];

extern int shaderDataLengthRAM1;
extern int shaderDataLengthRAM2;
extern int shaderDataLengthRAM3;
extern int shaderDataLengthRAM4;

extern const char * shaderDataConst1;
extern const char * shaderDataConst2;
extern const char * shaderDataConst3;
extern const char * shaderDataConst4;

extern const int* shaderDataLength1;
extern const int* shaderDataLength2;
extern const int* shaderDataLength3;
extern const int* shaderDataLength4;

// Define structure to hold shader data
struct SHADER_DATA
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;
	GLuint Position;
	GLuint SourceColor;
};

struct TEXT_DATA
{
	GLuint vertexBuffer;
	GLuint texBuffer;
	vec4 fontColor;
	int vertexBufferSize;
	int texBufferSize;
	int characters;
	int textReady;
	vec3 offset;
};

// Define class for everything
class modelEngine
{
private:
	// data for models
	MODEL_T models[MAX_MODELS];
	// Data for fonts
	TEXT_DATA modelText[MAX_MODELS];
	MODEL_STATE_T state;
	int numModels;
	GLuint shaderProgramHandles[MAX_SHADER_PROGRAMS];
	SHADER_DATA shaders[MAX_SHADER_PROGRAMS];
	int numShaders;

public:
	texture_font_t *font1, *font2;
	texture_atlas_t *atlas;
	// Functions
	// Constructor
	modelEngine();
	// Orphan GL buffer
	int orphanArrayBuffer(GLuint buffer, int size);
	// Create array of chars in GL buffer
	int createText(int modelIndex, texture_font_t * font, wchar_t * text, vec4 * color, vec2 * pen, vec3 * offset);
	int testText(int modelIndex);
	// Add text
	//void add_text( vertex_buffer_t * buffer, texture_font_t * font, wchar_t * text, vec4 * color, vec2 * pen );
	void add_text( texture_font_t * font, wchar_t * text, vec4 * color, vec2 * pen , GLuint vertexBuffer , GLfloat Zoffset);
	// Initialize fonts
	int initFonts();
	// Set model shader program
	int setModelShader(int modelIndex, int shaderIndex);
	// Check if we have room for additional shader program
	int getNextShaderNumber();
	// Link two shaders to program
	GLuint linkShaderProgram(int shaderIndex);
	// Load shaders
	GLuint loadShader(char *shaderFile, GLenum shaderType, int shaderIndex);
	// Update translation
	int setTranslate(int modelIndex, float x, float y, float z);
	// Update rotation
	int setRotate(int modelIndex, float x, float y, float z);
	// Update scale
	int setScale(int modelIndex, float x, float y, float z);
	// Enable/disable model draw
	int enableModel(int modelIndex, int enable);
	// Redraw models
	int redrawModels();

	void showlog(GLint shader);

	void showprogramlog(GLint shader);

	void initialize(void);
	// Load model
	int loadWavefrontModel(char* path);
	// Initialize OGL
	void init_ogl(MODEL_STATE_T *state);
	// Rotate model around specified axis by specified amount
	int rotateModelIncrement(int modelIndex, char axis, float value);
};


#endif