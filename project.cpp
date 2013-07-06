
// C++ application
#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "GLES2/gl2.h"

extern "C" {
    #include "triangle.h"
	#include <time.h>
	#include <pthread.h>
}
extern "C++" {
	#include "serial.hpp"
	#include "ModelEngine.hpp"
}

float pitch;
float roll;
float yaw;


long int start_time;
long int time_difference;
struct timespec gettime_now;
serialDataStructure serialData;
static pthread_t pSerialThread;
int navBallModel = 0;
int cursorModel = 0;

int main()
{
	initApp();

	int run = 1;

	modelEngine ME;

	ME.initialize();

	// Load shader
	/*
	int navballShader = ME.getNextShaderNumber();
	if(navballShader != -1)
	{
		GLuint vertex = ME.loadShader("/opt/vc/src/hello_pi/hello_camera_teapot/SimpleVertex.glsl", GL_VERTEX_SHADER, navballShader);
		GLuint fragment = ME.loadShader("/opt/vc/src/hello_pi/hello_camera_teapot/SimpleFragment.glsl", GL_FRAGMENT_SHADER, navballShader);
		GLuint programHandle = ME.linkShaderProgram(navballShader);
	}*/

	// Load models
	cursorModel = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/cursor.obj");
	navBallModel = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/navball.obj");
	

	// Set navball shader
	//ME.setModelShader(navBallModel, navballShader);

	// Set position and scale of navBall model
	ME.setTranslate(navBallModel, 0.0f, -8.0f, 0.0f);
	ME.setScale(navBallModel, 10.0f, 10.0f, 10.0f);

	// Set position and scale of cursor model
	ME.setTranslate(cursorModel, 0.0f, -0.1f, 39.5f);
	ME.setScale(cursorModel, 1.0f, 1.0f, 1.0f);
	ME.setRotate(cursorModel, 0.0f, 90.0f, 0.0f);

	pthread_create(&pSerialThread, NULL, serialThread, (void *) &serialData);

	while(run)
	{
		if(serialData.status == 1)
		{
			serialData.status = 0;
			// Update variables
			pitch = (short)(serialData.data[2]);
			roll = (short)(serialData.data[0]);
			yaw = (short)(serialData.data[1]);

			pitch = pitch * 0.05729577f;
			roll = roll * 0.05729577f;
			yaw = yaw * 0.05729577f;
			// Update angles
			ME.setRotate(navBallModel, pitch, roll, yaw);
			// Draw
			ME.redrawModels();
		}
	}
	stopApp();
	return 0;
}