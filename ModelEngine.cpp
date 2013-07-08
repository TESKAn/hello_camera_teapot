#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <memory.h>
#include <time.h>
#include <sys/stat.h>
#include <wchar.h>
#include "bcm_host.h"

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"

#include "fonts/texture-font.h"
#include "fonts/texture-atlas.h"
#include "fonts/vector.h"

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

texture_font_t *font1, *font2;
texture_atlas_t *atlas;

// Text functions

// --------------------------------------------------------------- add_text ---
//void modelEngine::add_text( vertex_buffer_t * buffer, texture_font_t * font, wchar_t * text, vec4 * color, vec2 * pen )
void modelEngine::add_text(texture_font_t * font, wchar_t * text, vec4 * color, vec2 * pen, GLuint vertexBuffer )
{
    size_t i;
    float r = color->red, g = color->green, b = color->blue, a = color->alpha;
    for( i=0; i<wcslen(text); ++i )
    {
        texture_glyph_t *glyph = texture_font_get_glyph( font, text[i] );
        if( glyph != NULL )
        {
            int kerning = 0;
            if( i > 0)
            {
                kerning = texture_glyph_get_kerning( glyph, text[i-1] );
            }
            pen->x += kerning;
            int x0  = (int)( pen->x + glyph->offset_x );
            int y0  = (int)( pen->y + glyph->offset_y );
            int x1  = (int)( x0 + glyph->width );
            int y1  = (int)( y0 - glyph->height );
            float s0 = glyph->s0;
            float t0 = glyph->t0;
            float s1 = glyph->s1;
			float t1 = glyph->t1;

			//GLuint indices[6] = {0,1,2, 0,2,3};
			GLfloat indices[6] = {0,1,2, 0,2,3};
			GLfloat vertexCoordinates[] = {
				x0,y0,0,
				x0,y1,0,
				x1,y1,0,
				x0,y0,0,
				x1,y1,0,
				x1,y0,0,
			};
			for(int i = 0; i < 18; i++)
			{
				vertexCoordinates[i] = vertexCoordinates[i] / 1000;
			}

			GLfloat textureCoordinates[] = {
				s0,t0,
				s0,t1,
				s1,t1,
				s0,t0,
				s1,t1,
				s1,t0,
			};

			GLfloat colors[] = {
				r, g, b, a,
				r, g, b, a,
				r, g, b, a,
				r, g, b, a,
				r, g, b, a,
				r, g, b, a,
			};

            GLfloat vertices[] = {
              x0,y0,0,
              s0,t0,
              r, g, b, a,
              x0,y1,0,
              s0,t1,
              r, g, b, a,
              x1,y1,0,
              s1,t1,
              r, g, b, a,
              x0,y0,0,
              s0,t0,
              r, g, b, a,
              x1,y1,0,
              s1,t1,
              r, g, b, a,
              x1,y0,0,
              s1,t0,
              r, g, b, a
            };
			//vertex_buffer_push_back( buffer, vertices, 4, indices, 6 );
			pen->x += glyph->advance_x;
			// Store vertices to buffer
			// Generate buffer
			GLuint verticeBuffer = 0;
			GLuint textureBuffer = 0;
			GLuint colorBuffer = 0;

			// Generate vertex buffer
			glGenBuffers(1, &verticeBuffer);
			// Bind buffer
			glBindBuffer(GL_ARRAY_BUFFER, verticeBuffer);
			// Create buffer data
			glBufferData(GL_ARRAY_BUFFER, 216, &vertexCoordinates, GL_STATIC_DRAW);
			// Unbind buffer
			glBindBuffer(GL_ARRAY_BUFFER, 0); 
			// Generate texture buffer
			glGenBuffers(1, &textureBuffer);
			// Bind buffer
			glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
			// Create buffer data
			glBufferData(GL_ARRAY_BUFFER, 48, &textureCoordinates, GL_STATIC_DRAW);
			// Unbind buffer
			glBindBuffer(GL_ARRAY_BUFFER, 0); 
			// Generate color buffer
			glGenBuffers(1, &colorBuffer);
			// Bind buffer
			glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
			// Create buffer data
			glBufferData(GL_ARRAY_BUFFER, 48, &colors, GL_STATIC_DRAW);
			// Unbind buffer
			glBindBuffer(GL_ARRAY_BUFFER, 0); 

			glEnableClientState(GL_COLOR_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glEnableClientState( GL_VERTEX_ARRAY );

			// Start with a clear screen
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			// Draw on screen
			// Bind texture
			glBindTexture(GL_TEXTURE_2D, vertexBuffer);
			// Bind buffers
			// Bind vertex buffer
			glBindBuffer(GL_ARRAY_BUFFER, verticeBuffer);
			glVertexPointer(3, GL_FLOAT, 0, NULL);
			// Bind texture buffer
			glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
			glTexCoordPointer(2, GL_FLOAT, 0, NULL);
			// Unbind buffer
			glBindBuffer(GL_ARRAY_BUFFER, colorBuffer); 
			glColorPointer(4, GL_FLOAT, 0, NULL);
	
			glPushMatrix();

			// Move model
			glTranslatef(0.0f,0.0f,20.0f);
			// Rotate model
			glRotatef(0.0f, 1.0, 0.0, 0.0);
			glRotatef(0.0f, 0.0, 1.0, 0.0);
			glRotatef(0.0f, 0.0, 0.0, 1.0);
			// Scale model
			glScalef(15.0f,15.0f,15.0f);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			// Display
			eglSwapBuffers(state.display, state.surface);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glEnableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glPopMatrix();
		}
    }
}




int modelEngine::initFonts()
{
	/* Texture atlas to store individual glyphs */
	atlas = texture_atlas_new( 1024, 1024, 1 );

	font1 = texture_font_new( atlas, "./fonts/fonts/custom.ttf", 50 );
	font2 = texture_font_new( atlas, "./fonts/fonts/ObelixPro.ttf", 70 );

	/* Cache some glyphs to speed things up */
	texture_font_load_glyphs( font1, L" !\"#$%&'()*+,-./0123456789:;<=>?"
		L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
		L"`abcdefghijklmnopqrstuvwxyz{|}~");

	texture_font_load_glyphs( font2, L" !\"#$%&'()*+,-./0123456789:;<=>?"
		L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
		L"`abcdefghijklmnopqrstuvwxyz{|}~");

	texture_atlas_upload(atlas);

	vec2 pen = {-400,150};
    vec4 color = {0.5,0.5,0.5,0.1};
	GLuint bufIndex = atlas->id;

	

	add_text(font1, L"W",&color, &pen, bufIndex);

	/*
	vector_t * vVector = vector_new(sizeof(GLfloat));
	
	buffer = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );


	    float sy, cy, sp, cp, a, b;
    wchar_t disp[100];

    

    vec2 pen = {-400,150};
    vec4 color = {.2,0.2,0.2,1};
    
    add_text( vVector, font1,
              L"freetypeGlesRpi", &color, &pen );

    pen.x = -390;
    pen.y = 140;
    vec4 transColor = {1,0.3,0.3,0.6};

    add_text( vVector, font1,
              L"freetypeGlesRpi", &transColor, &pen );


    pen.x = -190;
    pen.y = 0;

    add_text( vVector, font2,
              L"Roller Racing Demo", &color, &pen );

	int pitch = 10;
	int yaw = 12;
	int roll = 11;
    
    swprintf(disp, 60, L"Pitch %.1f, Yaw %.1f", pitch, yaw);
    pen.x = -100;
    pen.y = -100;

    add_text( vVector, font2,
              disp, &color, &pen );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   
    GLfloat mvp[] = {
      a*cy*cp, -a*sy, -sp*a*cy, 0,
      b*sy*cp, b*cy, -sp*b*cy, 0,
      sp, 0, cp, 0,
      0, 0, 0, 1.0
    };

    glUniformMatrix4fv(mvpHandle, 1, GL_FALSE, (GLfloat *) mvp);

   glBindTexture( GL_TEXTURE_2D, atlas->id );

   glUniform1i ( samplerHandle, 0);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

   glDrawArrays ( GL_TRIANGLES, 0, vVector->size/9 );

   swapBuffers();
   */






}

int modelEngine::setModelShader(int modelIndex, int shaderIndex)
{
	if(modelIndex != -1)
	{
		WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
		model->shader = shaderIndex;
		return 0;
	}
	else
	{
		return -1;
	}
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
	if(modelIndex != -1)
	{
		WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
		model->translate[0] = (GLfloat)x;
		model->translate[1] = (GLfloat)y;
		model->translate[2] = (GLfloat)z;
		return 0;
	}
	else
	{
		return -1;
	}
}

int modelEngine::setRotate(int modelIndex, float x, float y, float z)
{
	if(modelIndex != -1)
	{
		WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
		// Rotate around X = roll
		model->rotate[0] = (GLfloat)x;
		// Rotate around Y = pitch
		model->rotate[1] = (GLfloat)y;
		// Rotate around Z = yaw
		model->rotate[2] = (GLfloat)z;
		return 0;
	}
	else
	{
		return -1;
	}
}

int modelEngine::setScale(int modelIndex, float x, float y, float z)
{
	if(modelIndex != -1)
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
	else
	{
		return -1;
	}
}

int modelEngine::enableModel(int modelIndex, int enable)
{
		if(modelIndex != -1)
	{
		WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
		// Set model enable var
		model->modelEnabled = enable;
		return 0;
	}
	else
	{
		return -1;
	}
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

	//glEnable(GL_CULL_FACE);

	//glCullFace(GL_BACK);

	//glFrontFace(GL_CCW);

	//glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	//checkGLError();

	glEnable(GL_BLEND);
	checkGLError();
	glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
	checkGLError();
	glBlendEquation(GL_FUNC_ADD);
	checkGLError();

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDepthRangef(0.0,1.0);

	glEnable(GL_DEPTH_TEST);

}

int modelEngine::rotateModelIncrement(int modelIndex, char axis, float value)
{
	if(modelIndex != -1)
	{
		WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[modelIndex];
		GLfloat result = 0;
		int paramIndex = 0;
		switch(axis)
		{
		case 'x':
			{
				paramIndex = 0;
				break;
			}
		case 'y':
			{
				paramIndex = 1;
				break;
			}
		case 'z':
			{
				paramIndex = 2;
				break;
			}
		}
		result = model->rotate[paramIndex];
		result = result + (GLfloat)value;
		if(result > 360)
		{
			result = result - 360;
		}
		else if(result < 0)
		{
			result = result + 360;
		}
		model->rotate[paramIndex] = result;
		return 0;
	}
	else
	{
		return -1;
	}
}