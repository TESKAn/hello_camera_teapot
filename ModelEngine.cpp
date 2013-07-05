#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <memory.h>
#include <time.h>
#include <sys/stat.h>
#include "bcm_host.h"

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"


extern "C" 
{
	#include "models.h"
	#include "modelEngine_c.h"
}
extern "C++" 
{
#include "ModelEngine.hpp"
}

// Shader data
char shaderDataRAM[2048];


const char * shaderDataConst = shaderDataRAM;

GLuint modelEngine::linkShaderProgram(GLuint vertexShader, GLuint fragmentShader)
{
	GLuint programHandle = glCreateProgram();
	glAttachShader(programHandle, vertexShader);
	glAttachShader(programHandle, fragmentShader);
	glLinkProgram(programHandle);


	GLint linkSuccess;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
	if (linkSuccess == GL_FALSE) 
	{
		GLchar messages[256];
		glGetProgramInfoLog(programHandle, sizeof(messages), 0, &messages[0]);
		return -1;
	}
	return programHandle;
}


GLuint modelEngine::loadShader(char *shaderFile, GLenum shaderType)
{
	FILE* shaderDataFile;
	GLuint shaderHandle = 0;
	// Get file stats
	struct stat st;
	int numBytesRead = 0;

	int numShaders = 0;

	// If we have room for shaders
	if(numShaders < MAX_SHADER_PROGRAMS - 1)
	{
		// Create shader
		stat(shaderFile, &st);
		// First read contents of file
		shaderDataFile = fopen(shaderFile, "rb");
		if(shaderDataFile)
		{
			// Read data from file
			char data[2048];
			numBytesRead = fread(shaderDataRAM, 1, st.st_size, shaderDataFile);

			shaderDataRAM[numBytesRead] = 0;
			// Create shader
			shaderHandle = glCreateShader(shaderType); 
			// Set shader source
			glShaderSource(shaderHandle, 1, &shaderDataConst, NULL);
			// Compile shader
			glCompileShader(shaderHandle);
			// Check success
			GLint compileSuccess;
			glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
			if (compileSuccess == GL_FALSE) 
			{
				GLchar messages[256];
				glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
				return -1;
			}


			fclose(shaderDataFile);
		}
		// If successfull
		if(models[numShaders] != 0)
		{
			numShaders++;
			// Return shader handle
			return shaderHandle;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int modelEngine::setTranslate(int modelIndex, float x, float y, float z)
{
	WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
	model->translate[0] = (GLfloat)x;
	model->translate[1] = (GLfloat)y;
	model->translate[2] = (GLfloat)z;

	return 0;
}

int modelEngine::setRotate(int modelIndex, float pitch, float roll, float yaw)
{
	WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
	model->rotate[0] = (GLfloat)pitch;
	model->rotate[1] = (GLfloat)roll;
	model->rotate[2] = (GLfloat)yaw;
	return 0;
}

int modelEngine::redrawModels()
{
	// Set matrix mode
	glMatrixMode(GL_MODELVIEW);
	// Move camera
	glLoadIdentity();
	// move camera back to see the cube
	glTranslatef(0.f, 0.f, -40);

	// Start with a clear screen
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Cycle through all models and redraw them
	WAVEFRONT_MODEL_T *modelData = NULL; 
	int i = 0;
	for(i = 0; i < numModels; i++)
	{
		draw_wavefront(models[i], NULL);
	}
	eglSwapBuffers(state.display, state.surface);

	return 0;
}

int modelEngine::loadWavefrontModel(char *path)
{
	// If we have room for models
	if(numModels < MAX_MODELS - 1)
	{
		// Try load model
		models[numModels] = load_wavefront(path, NULL);
		// If load successfull
		if(models[numModels] != 0)
		{
			numModels++;
			// Return model index
			return numModels - 1;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

void modelEngine::initialize(void)
{
	numModels = 0;
	modelEngine_init_ogl(&state);
	init_model_proj(&state);
	init_textures(&state);
}
