#ifndef _MODEL_ENGINE_H_
#define _MODEL_ENGINE_H_

// Include gl.h - else it wont recognize GLuint
#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
// Include models.h - else wont recognize MODEL_T
extern "C" 
{
	#include "models.h"
	#include "modelEngine_c.h"
}

#define MAX_MODELS			10
#define MAX_SHADER_PROGRAMS	20

// Define class for everything
class modelEngine
{
private:
	// data for models
	MODEL_T models[MAX_MODELS];
	MODEL_STATE_T state;
	int numModels;
	GLuint shaderProgramHandles[MAX_SHADER_PROGRAMS];

public:
	// Functions
	// Link two shaders to program
	GLuint linkShaderProgram(GLuint vertexShader, GLuint fragmentShader);
	// Load shaders
	GLuint loadShader(char *shaderFile, GLenum shaderType);
	// Update translation
	int setTranslate(int modelIndex, float x, float y, float z);
	// Update rotation
	int setRotate(int model, float pitch, float roll, float yaw);
	// Redraw models
	int redrawModels();
	void initialize(void);
	// Load model
	int loadWavefrontModel(char* path);
	// Initialize OGL
	void init_ogl(MODEL_STATE_T *state);
};


#endif