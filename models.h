/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MODELS_T
#define MODELS_T
typedef struct opqaue_model_s * MODEL_T;

int checkGLError();
MODEL_T load_wavefront(const char *modelname, const char *texturename);
MODEL_T cube_wavefront(void);
void unload_wavefront(MODEL_T m);
int draw_wavefront(MODEL_T m, GLuint texture);

enum {VBO_VERTEX, VBO_NORMAL, VBO_TEXTURE, VBO_MAX};

#define MAX_MATERIALS 4
#define MAX_MATERIAL_NAME 32

typedef struct wavefront_material_s {
	GLuint vbo[VBO_MAX];
	int numverts;
	char name[MAX_MATERIAL_NAME];
	GLuint texture;
	GLuint attr_vertex;
} WAVEFRONT_MATERIAL_T;

typedef struct wavefront_model_s {
	WAVEFRONT_MATERIAL_T material[MAX_MATERIALS];
	int num_materials;
	GLuint texture;
	// Translation matrix
	GLfloat translate[3];
	// Rotation matrix
	GLfloat rotate[3];
	// Scale matrix
	GLfloat scale[3];
	// Shader program to use
	int shader;
	// Material section
	GLfloat ambientColor[4];
	GLfloat difuseColor[4];
	GLfloat specularColor[4];
	GLfloat shininess[1];
	GLfloat emmisionColor[4];
	GLfloat transparency[1];
	// Mark if model is enabled for display
	int modelEnabled;
} WAVEFRONT_MODEL_T;

#endif
