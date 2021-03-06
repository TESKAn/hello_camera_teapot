
// C++ application
#include <wchar.h>

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
// Model indexes
int navBallModel = -1;
int cursorModel = -1;
int infoPanel_BL = -1;
int infoPanel_BR = -1;
int infoPanel_TL = -1;
int infoPanel_TR = -1;
// Default rotations
float navBallRotation = -10.0f;

int main()
{
	initApp();

	int run = 1;

	modelEngine ME;

	ME.initialize();

	// Text test parameters
	// Create color for text
	vec2 pen = {-0,0};
	vec2 scale = {0.5,0.5};
    vec4 color = {1.0,1.0,0.0,0.9};
	vec3 offset = {0.05,-0.05, 0.05};

	// Load models
	navBallModel = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/navball.obj");
	cursorModel = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/navballHousing.obj");
	infoPanel_BL = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/infoPanel_BL.obj");
	infoPanel_BR = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/infoPanel_BR.obj");
	infoPanel_TL = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/infoPanel_TL.obj");
	infoPanel_TR = ME.loadWavefrontModel("/opt/vc/src/hello_pi/hello_camera_teapot/models/infoPanel_TR.obj");

	// Load some text for model
	ME.createText(infoPanel_BL, ME.font1, L"Yaw:\n0.46 deg", &color, &pen, &offset, &scale);

	// Set position and scale of navBall model
	ME.setTranslate(navBallModel, 0.0f, -1.25f, 35.0f);
	ME.setScale(navBallModel, 1.0f, 1.0f, 1.0f);
	ME.setRotate(navBallModel, navBallRotation, 0.0f, 0.0f);

	// Set position and scale of cursor model
	ME.setTranslate(cursorModel, 0.0f, -1.25f, 35.0f);
	ME.setScale(cursorModel, 1.0f, 1.0f, 1.0f);
	ME.setRotate(cursorModel, navBallRotation, 0.0f, 0.0f);

	// Set position and scale of infoPanel_BL model
	ME.setTranslate(infoPanel_BL, -1.5f, -1.25f, 35.0f);
	ME.setScale(infoPanel_BL, 1.0f, 1.0f, 1.0f);
	ME.setRotate(infoPanel_BL, navBallRotation, 0.0f, 0.0f);

	// Set position and scale of infoPanel_BR model
	ME.setTranslate(infoPanel_BR, 1.5f, -1.25f, 35.0f);
	ME.setScale(infoPanel_BR, 1.0f, 1.0f, 1.0f);
	ME.setRotate(infoPanel_BR, navBallRotation, 0.0f, 0.0f);

	// Set position and scale of infoPanel_TL model
	ME.setTranslate(infoPanel_TL, 0.0f, -1.25f, 35.0f);
	ME.setScale(infoPanel_TL, 1.0f, 1.0f, 1.0f);
	ME.setRotate(infoPanel_TL, navBallRotation, 0.0f, 0.0f);

	// Set position and scale of infoPanel_TR model
	ME.setTranslate(infoPanel_TR, 0.0f, -1.25f, 35.0f);
	ME.setScale(infoPanel_TR, 1.0f, 1.0f, 1.0f);
	ME.setRotate(infoPanel_TR, navBallRotation, 0.0f, 0.0f);

	// Enable/disable models
	ME.enableModel(navBallModel, 1);
	ME.enableModel(cursorModel, 1);
	ME.enableModel(infoPanel_BL, 1);
	ME.enableModel(infoPanel_BR, 1);
	ME.enableModel(infoPanel_TL, 0);
	ME.enableModel(infoPanel_TR, 0);

	pthread_create(&pSerialThread, NULL, serialThread, (void *) &serialData);

	while(run)
	{
		if(serialData.status == 1)
		{
			serialData.status = 0;
			// Update variables
			pitch = (short)(serialData.data[0]);
			roll = (short)(serialData.data[1]);
			yaw = (short)(serialData.data[2]);

			pitch = pitch * 0.05729577f;
			roll = roll * 0.05729577f;
			yaw = yaw * 0.05729577f;

			// Try to print angle
			wchar_t buffer [100];
			int cx;
			cx = swprintf ( buffer, 100, L"Roll:\n%.2f deg\nPitch:\n%.2f deg\nYaw:\n%.2f deg", roll, pitch, yaw );

			pen.x = 0;
			pen.y = 0;

			ME.createText(infoPanel_BL, ME.font1, buffer, &color, &pen, &offset, &scale);

			// Increase pitch by navball rotation
			pitch += navBallRotation;
			// Update angles
			ME.setRotate(navBallModel, pitch, yaw, roll);
			// Draw
			ME.redrawModels();
		}
	}
	stopApp();
	return 0;
}