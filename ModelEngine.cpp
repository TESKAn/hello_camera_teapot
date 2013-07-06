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
char shaderDataRAM1[2048];
char shaderDataRAM2[2048];

int shaderDataLengthRAM1 = 0;
int shaderDataLengthRAM2 = 0;


const char * shaderDataConst1 = shaderDataRAM1;
const char * shaderDataConst2 = shaderDataRAM2;

const int* shaderDataLength1 = &shaderDataLengthRAM1;
const int* shaderDataLength2 = &shaderDataLengthRAM2;

int modelEngine::setModelShader(int modelIndex, int shaderIndex)
{
	WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
	model->shader = shaderIndex;
}

int modelEngine::getNextShaderNumber()
{
	if(numShaders < MAX_SHADER_PROGRAMS)
	{
		return numShaders;
	}
	else
	{
		return -1;
	}
}


GLuint modelEngine::linkShaderProgram(int shaderIndex)
{
	GLuint programHandle = glCreateProgram();
	glAttachShader(programHandle, shaders[shaderIndex].vertexShader);
	glAttachShader(programHandle, shaders[shaderIndex].fragmentShader);
	glLinkProgram(programHandle);
	showprogramlog(programHandle);
	shaders[shaderIndex].program = programHandle;

	// Get and store attributes
	GLint attribLocation = 0;
	attribLocation = glGetAttribLocation(programHandle, "Position");
	if(attribLocation != GL_INVALID_OPERATION)
	{
		shaders[shaderIndex].Position = attribLocation;
	}
	attribLocation = glGetAttribLocation(programHandle, "SourceColor");
	if(attribLocation != GL_INVALID_OPERATION)
	{
		shaders[shaderIndex].SourceColor = attribLocation;
	}
	// Enable attribute
	glVertexAttribPointer(shaders[shaderIndex].Position, 4, GL_FLOAT, 0, 16, 0);
    glEnableVertexAttribArray(shaders[shaderIndex].Position);

	numShaders++;

	return programHandle;
}


GLuint modelEngine::loadShader(char *shaderFile, GLenum shaderType, int shaderIndex)
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
			numBytesRead = fread(shaderDataRAM1, 1, st.st_size, shaderDataFile);

			shaderDataLengthRAM1 = numBytesRead;

			// Create shader
			shaderHandle = glCreateShader(shaderType); 
			// Set shader source
			glShaderSource(shaderHandle, 1, &shaderDataConst1, shaderDataLength1);
			// Compile shader
			glCompileShader(shaderHandle);
			checkGLError();
			// Check success
			showlog(shaderHandle);

			fclose(shaderDataFile);
		}
		// If successfull
		if(shaderHandle != 0)
		{
			// Store shader 
			if(shaderType == GL_VERTEX_SHADER)
			{
				shaders[shaderIndex].vertexShader = shaderHandle;
			}
			else
			{
				shaders[shaderIndex].fragmentShader = shaderHandle;
			}
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
	// Rotate around X = roll
	model->rotate[0] = (GLfloat)roll;
	// Rotate around Y = pitch
	model->rotate[1] = (GLfloat)pitch;
	// Rotate around Z = yaw
	model->rotate[2] = (GLfloat)yaw;
	return 0;
}

int modelEngine::setScale(int modelIndex, float x, float y, float z)
{
	WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
	// Rotate around X = roll
	model->scale[0] = (GLfloat)x;
	// Rotate around Y = pitch
	model->scale[1] = (GLfloat)y;
	// Rotate around Z = yaw
	model->scale[2] = (GLfloat)z;
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

void modelEngine::showlog(GLint shader)
{
   // Prints the compile log for a shader
   char log[1024];
   glGetShaderInfoLog(shader,sizeof log,NULL,log);
   printf("%d:shader:\n%s\n", shader, log);
}

void modelEngine::showprogramlog(GLint shader)
{
   // Prints the information log for a program object
   char log[1024];
   glGetProgramInfoLog(shader,sizeof log,NULL,log);
   printf("%d:program:\n%s\n", shader, log);
}


void modelEngine::initialize(void)
{
	numModels = 0;
	numShaders = 0;
	modelEngine_init_ogl(&state);
	init_model_proj(&state);
	glEnable(GL_TEXTURE_2D);
	checkGLError();
	//init_textures(&state);

	// Shaders test
	//init_shaders(&state);
}
