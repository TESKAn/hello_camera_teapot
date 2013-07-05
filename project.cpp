
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

int main()
{
	initApp();

	int run = 1;

	modelEngine ME;

	ME.initialize();

	// Load shader
	GLuint vertex = ME.loadShader("/opt/vc/src/hello_pi/hello_camera_teapot/SimpleVertex.glsl", GL_VERTEX_SHADER);
	GLuint fragment = ME.loadShader("/opt/vc/src/hello_pi/hello_camera_teapot/SimpleFragment.glsl", GL_FRAGMENT_SHADER);
	GLuint programHandle = ME.linkShaderProgram(vertex, fragment);

	GLuint vertex1 = ME.loadShader("/opt/vc/src/hello_pi/hello_camera_teapot/SimpleVertex.glsl", GL_VERTEX_SHADER);
	GLuint fragment1 = ME.loadShader("/opt/vc/src/hello_pi/hello_camera_teapot/SimpleFragment.glsl", GL_FRAGMENT_SHADER);
	GLuint programHandle1 = ME.linkShaderProgram(vertex, fragment);

	// Load models
	navBallModel = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/navball.obj");
	// Set position of navBall model
	//ME.setTranslate(navBallModel, 1.0f, 0.0f, 0.0f);

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