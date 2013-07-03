
// C++ application

extern "C" {
    #include "triangle.h"
	#include <time.h>
	#include <pthread.h>
}
extern "C++" {
	#include "serial.hpp"
}

float pitch;
float roll;
float yaw;


long int start_time;
long int time_difference;
struct timespec gettime_now;
serialDataStructure serialData;
static pthread_t pSerialThread;


int main()
{

	initApp();

	int run = 1;
	int sleepTimes = 0;

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

			refreshOGL(pitch, roll, yaw);
		}

	}
	stopApp();
	return 0;
}