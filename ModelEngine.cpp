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

// Constructor
modelEngine::modelEngine()
{

}


// Orphan GL buffer
int modelEngine::orphanArrayBuffer(GLuint buffer, int size)
{
	// Bind buffer
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	checkGLError();
	// Orphan
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
	checkGLError();
	// Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return checkGLError();
}

// Text functions
// Create text string with specified font
// Begin at [0,0]
// Store data to modelText[modelIndex]
int modelEngine::createText(int modelIndex, texture_font_t * font, wchar_t * text, vec4 * color, vec2 * pen, vec3 * offset, vec2 * scale)
{
	size_t i;
	vec2 startPen = *pen;
	//Zoffset = Zoffset * 400;
	// Make room to store vertices and texture coordinates for 50 characters
	GLfloat vertices[3600];
	int verticesIndex = 0;
	GLfloat texCoordinates[2400];
	int texIndex = 0;
	// Return -1 if model index is invalid
	if(modelIndex == -1)
	{
		return -1;
	}
    float r = color->red, g = color->green, b = color->blue, a = color->alpha;
    for( i=0; i<wcslen(text); ++i )
    {
		// Check for special character
		wchar_t currentChar = text[i];
		wchar_t* currentCharPointer = text + i;
		//wchar_t newLineChar = L"\n";
		//if((text + i) == L"\n")
		int result = wcsncmp(currentCharPointer, L"\n", 1);
		if(result == 0)
		{
			// New line found
			pen->x = startPen.x;
			pen->y = startPen.y - font->height;
			startPen.y = pen->y;
		}
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
				vertexCoordinates[i] = vertexCoordinates[i] / 400;
			}
			GLfloat textureCoordinates[] = {
				s0,t0,
				s0,t1,
				s1,t1,
				s0,t0,
				s1,t1,
				s1,t0,
			};
			// Copy vertex data
			memcpy (vertices + verticesIndex, vertexCoordinates, 18 * sizeof(GLfloat));
			// Increase pointer 
			verticesIndex += 18;
			// Copy texture data
			memcpy (texCoordinates + texIndex, textureCoordinates, 12 * sizeof(GLfloat));
			// Increase pointer
			texIndex += 12;
			// Increase pen position
			pen->x += glyph->advance_x;
		}
	}
	// Scale all data in X and Y direction
	for(int i = 0; i < verticesIndex; i+=3)
	{
		vertices[i] = vertices[i] * scale->x;
		vertices[i+1] = vertices[i+1] * scale->y;
	}
	// Store data to buffers
	// First, orphan GL buffers
	// Check if text has been enabled
	if(modelText[modelIndex].textReady != 0)
	{
		glDeleteBuffers(1,  &modelText[modelIndex].vertexBuffer);
		glDeleteBuffers(1,  &modelText[modelIndex].texBuffer);
		//orphanArrayBuffer(modelText[modelIndex].vertexBuffer, modelText[modelIndex].vertexBufferSize);
		//orphanArrayBuffer(modelText[modelIndex].texBuffer, modelText[modelIndex].texBufferSize);
	}

	// Generate vertex buffer
	glGenBuffers(1, &modelText[modelIndex].vertexBuffer);
	// Bind buffer
	glBindBuffer(GL_ARRAY_BUFFER, modelText[modelIndex].vertexBuffer);
	// Create buffer data
	glBufferData(GL_ARRAY_BUFFER, verticesIndex * sizeof(GLfloat), &vertices, GL_STATIC_DRAW);
	checkGLError();
	// Unbind buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	// Store number of vertices
	modelText[modelIndex].vertexBufferSize = verticesIndex;

	// Generate texture buffer
	glGenBuffers(1, &modelText[modelIndex].texBuffer);
	// Bind buffer
	glBindBuffer(GL_ARRAY_BUFFER, modelText[modelIndex].texBuffer);
	// Create buffer data
	glBufferData(GL_ARRAY_BUFFER, texIndex * sizeof(GLfloat), &texCoordinates, GL_STATIC_DRAW);
	checkGLError();
	// Unbind buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	// Store number of vertices
	modelText[modelIndex].texBufferSize = texIndex;

	// Store number of characters
	modelText[modelIndex].characters = wcslen(text);

	// Store font color
	modelText[modelIndex].fontColor.r = color->r;
	modelText[modelIndex].fontColor.g = color->g;
	modelText[modelIndex].fontColor.b = color->b;
	modelText[modelIndex].fontColor.a = color->a;
	// Store font offset on model
	modelText[modelIndex].offset = *offset;

	modelText[modelIndex].textReady = 1;

	return 0;
}

int modelEngine::testText(int modelIndex)
{
		//===========
	// Test draw

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState( GL_VERTEX_ARRAY );

	//glDisable(GL_TEXTURE_2D);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);

	// Start with a clear screen
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	checkGLError();
	// Draw on screen
	// Bind texture
	glBindTexture(GL_TEXTURE_2D, atlas->id);
	checkGLError();
	// Bind buffers
	// Bind vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, modelText[modelIndex].vertexBuffer);
	checkGLError();
	glVertexPointer(3, GL_FLOAT, 0, NULL);
	checkGLError();
	// Bind texture buffer
	glBindBuffer(GL_ARRAY_BUFFER, modelText[modelIndex].texBuffer);
	checkGLError();
	glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	checkGLError();
	// Bind color buffer
	//glBindBuffer(GL_ARRAY_BUFFER, colorBuffer); 
	//glColorPointer(4, GL_FLOAT, 0, NULL);

	glPushMatrix();
	checkGLError();

	// Move model
	glTranslatef(0.0f,0.0f,20.0f);
	// Rotate model
	glRotatef(0.0f, 1.0, 0.0, 0.0);
	glRotatef(0.0f, 0.0, 1.0, 0.0);
	glRotatef(0.0f, 0.0, 0.0, 1.0);
	// Scale model
	glScalef(15.0f,15.0f,15.0f);

	glColor4f(0.0f, 0.6f, 1.0f, 1.0f);

	glDrawArrays(GL_TRIANGLES, 0, modelText[modelIndex].characters * 6);
	checkGLError();

	// Display
	eglSwapBuffers(state.display, state.surface);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	//glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glPopMatrix();

	//===========
	return 0;
}


void modelEngine::add_text(texture_font_t * font, wchar_t * text, vec4 * color, vec2 * pen, GLuint vertexBuffer, GLfloat Zoffset)
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

			GLfloat indices[6] = {0,1,2, 0,2,3};
			GLfloat vertexCoordinates[] = {
				x0,y0,Zoffset,
				x0,y1,Zoffset,
				x1,y1,Zoffset,
				x0,y0,Zoffset,
				x1,y1,Zoffset,
				x1,y0,Zoffset,
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

			// Enabling color array makes font dissapear
			//glEnableClientState(GL_COLOR_ARRAY);

			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glEnableClientState( GL_VERTEX_ARRAY );

			//glDisable(GL_TEXTURE_2D);
			//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);

			// Start with a clear screen
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

			// Draw on screen
			// Bind texture
			glBindTexture(GL_TEXTURE_2D, atlas->id);
			// Bind buffers
			// Bind vertex buffer
			glBindBuffer(GL_ARRAY_BUFFER, verticeBuffer);
			glVertexPointer(3, GL_FLOAT, 0, NULL);
			// Bind texture buffer
			glBindBuffer(GL_ARRAY_BUFFER, textureBuffer);
			glTexCoordPointer(2, GL_FLOAT, 0, NULL);
			// Bind color buffer
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

			glColor4f(0.0f, 0.6f, 1.0f, 0.5f);

			glDrawArrays(GL_TRIANGLES, 0, 6);

			// Display
			eglSwapBuffers(state.display, state.surface);

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glEnableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			//glEnable(GL_TEXTURE_2D);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
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
    vec4 color = {0.5,0.5,0.5,0.5};
	GLuint bufIndex = atlas->id;
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
	int j = 0;
	for(j = 0; j < numModels; j++)
	{
		//===========
		int i;
		WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[j];

		// Check if model is enabled
		if(model->modelEnabled)
		{
			// Save old matrix
			glPushMatrix();

			// Move model
			glTranslatef(model->translate[0], model->translate[1], model->translate[2]);
			// Rotate model
			glRotatef(model->rotate[0], 1.0, 0.0, 0.0);
			glRotatef(model->rotate[1], 0.0, 1.0, 0.0);
			glRotatef(model->rotate[2], 0.0, 0.0, 1.0);
			// Scale model
			glScalef(model->scale[0], model->scale[1], model->scale[2]);

			for (i=0; i<model->num_materials; i++) {
				WAVEFRONT_MATERIAL_T *mat = model->material + i;
				if (mat->texture == -1) continue;
				glBindTexture(GL_TEXTURE_2D, mat->texture ? mat->texture:0);
				if (mat->vbo[VBO_VERTEX]) {
					glBindBuffer(GL_ARRAY_BUFFER, mat->vbo[VBO_VERTEX]);
					glVertexPointer(3, GL_FLOAT, 0, NULL);
				}
				if (mat->vbo[VBO_NORMAL]) {   
					glEnableClientState(GL_NORMAL_ARRAY);
					glBindBuffer(GL_ARRAY_BUFFER, mat->vbo[VBO_NORMAL]);
					glNormalPointer(GL_FLOAT, 0, NULL);
				} else {
					glDisableClientState(GL_NORMAL_ARRAY);
				}
				if (mat->vbo[VBO_TEXTURE]) {   
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glBindBuffer(GL_ARRAY_BUFFER, mat->vbo[VBO_TEXTURE]);
					glTexCoordPointer(2, GL_FLOAT, 0, NULL);
				} else {
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
				glDrawArrays(GL_TRIANGLES, 0, mat->numverts);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// Check if model has text data in it
			if(modelText[j].textReady)
			{
				glPopMatrix();
				// If yes, draw fonts
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glEnableClientState( GL_VERTEX_ARRAY );

				// Alpha test enable
				glEnable(GL_ALPHA_TEST);
				// If fragment has alpha greater than value, draw it, otherwise discard
				glAlphaFunc(GL_GREATER, 0.5);



				// Save old matrix
				glPushMatrix();

				// Move model
				glTranslatef(model->translate[0] + modelText[j].offset.x, model->translate[1] + modelText[j].offset.y, model->translate[2] + modelText[j].offset.z);
				// Rotate model
				glRotatef(model->rotate[0], 1.0, 0.0, 0.0);
				glRotatef(model->rotate[1], 0.0, 1.0, 0.0);
				glRotatef(model->rotate[2], 0.0, 0.0, 1.0);
				// Scale model
				glScalef(model->scale[0], model->scale[1], model->scale[2]);

				// Move model
				//glTranslatef(modelText[j].offset.x, modelText[j].offset.y, modelText[j].offset.z); 
				// Draw on screen
				// Bind texture atlas texture
				glBindTexture(GL_TEXTURE_2D, atlas->id);
				// Bind buffers
				// Bind vertex buffer
				glBindBuffer(GL_ARRAY_BUFFER, modelText[j].vertexBuffer);
				glVertexPointer(3, GL_FLOAT, 0, NULL);
				// Bind texture buffer
				glBindBuffer(GL_ARRAY_BUFFER, modelText[j].texBuffer);
				glTexCoordPointer(2, GL_FLOAT, 0, NULL);

				glColor4f(modelText[j].fontColor.r, modelText[j].fontColor.g, modelText[j].fontColor.b, modelText[j].fontColor.a);

				glDrawArrays(GL_TRIANGLES, 0, modelText[j].characters * 6);

				glBindBuffer(GL_ARRAY_BUFFER, 0);

				//glEnableClientState(GL_NORMAL_ARRAY);
				//glDisableClientState(GL_COLOR_ARRAY);
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				// Alpha test disable
				glDisable(GL_ALPHA_TEST);

			}
			// At end restore matrix
			glPopMatrix();
		}
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
			// Enable model
			WAVEFRONT_MODEL_T *model = (WAVEFRONT_MODEL_T *)models[numModels];
			model->modelEnabled = 1;
			// Set model text
			modelText[numModels].textReady = 0;
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

	glEnable(GL_CULL_FACE);

	glCullFace(GL_BACK);

	glFrontFace(GL_CCW);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	glDepthRangef(0.0,1.0);

	glEnable(GL_DEPTH_TEST);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);

	// Enable lighting
	

	mat_specular[0] = 1.0;
	mat_specular[1] = 1.0;
	mat_specular[2] = 1.0;
	mat_specular[3] = 1.0;

	mat_shininess[0] = 50.0;

	light_position[0] = 0.0;
	light_position[1] = 5.0;
	light_position[2] = 36.0;
	light_position[3] = 0.0;

	//glClearColor (0.0, 0.0, 0.0, 0.0);
	GLfloat mat_ambient[] = { 1.0, 0.5, 0.5, 1.0 };
	glShadeModel (GL_SMOOTH);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
	checkGLError();
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
	checkGLError();
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
	checkGLError();

	GLfloat light_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
	GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	checkGLError();
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	checkGLError();
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	checkGLError();
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	checkGLError();


	//glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	//glLightfv(GL_LIGHT0, GL_AMBIENT, light_position);
	checkGLError();
	glEnable(GL_LIGHTING);
	checkGLError();
	glEnable(GL_LIGHT0);
	checkGLError();

	// Initialize fonts
	initFonts();
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