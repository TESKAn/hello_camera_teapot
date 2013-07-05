/*
Copyright (c) 2012, Broadcom Europe Ltd
Copyright (c) 2012, OtherCrashOverride
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

// A rotating cube rendered with OpenGL|ES. Three images used as textures on the cube faces.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <memory.h>


#include "bcm_host.h"

#include "GLES/gl.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "cube_texture_and_coords.h"

#include "triangle.h"
#include <pthread.h>

#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"

#include "RaspiCamControl.h"
#include "RaspiPreview.h"
#include "RaspiCLI.h"

#include <semaphore.h>

#include "models.h"
//#include "serial.h"


#define PATH "./"

#define IMAGE_SIZE_WIDTH 1920
#define IMAGE_SIZE_HEIGHT 1080

#ifndef M_PI
#define M_PI 3.141592654
#endif


typedef struct
{
	uint32_t screen_width;
	uint32_t screen_height;
	// OpenGL|ES objects
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	GLuint tex;
	// model rotation vector and direction
	GLfloat rot_angle_x_inc;
	GLfloat rot_angle_y_inc;
	GLfloat rot_angle_z_inc;
	// current model rotation angles
	GLfloat rot_angle_x;
	GLfloat rot_angle_y;
	GLfloat rot_angle_z;
	// current distance from camera
	GLfloat distance;
	GLfloat distance_inc;

	MODEL_T model;
} CUBE_STATE_T;

static void exit_func(void);
static volatile int terminate;
static CUBE_STATE_T _state, *state=&_state;

static void* eglImage = 0;
//static pthread_t thread1;

static int recordVideo = 0;

//===============
// Camera functions

/// Camera number to use - we only have one camera, indexed from 0.
#define CAMERA_NUMBER 0

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

// Video format information
#define VIDEO_FRAME_RATE_NUM 30
#define VIDEO_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

// Max bitrate we allow for recording
const int MAX_BITRATE = 30000000; // 30Mbits/s


int mmal_status_to_int(MMAL_STATUS_T status);

/** Structure containing all state information for the current run
*/
typedef struct
{
	int timeout;                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
	int width;                          /// Requested width of image
	int height;                         /// requested height of image
	int bitrate;                        /// Requested bitrate
	int framerate;                      /// Requested frame rate (fps)
	char *filename;                     /// filename of output file
	int verbose;                        /// !0 if want detailed run information
	int demoMode;                       /// Run app in demo mode
	int demoInterval;                   /// Interval between camera settings changes
	int immutableInput;                /// Flag to specify whether encoder works in place or creates a new buffer. Result is preview can display either
	/// the camera output or the encoder output (with compression artifacts)
	RASPIPREVIEW_PARAMETERS preview_parameters;   /// Preview setup parameters
	RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters

	MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
	MMAL_COMPONENT_T *encoder_component;   /// Pointer to the encoder component
	MMAL_CONNECTION_T *preview_connection; /// Pointer to the connection from camera to preview
	MMAL_CONNECTION_T *encoder_connection; /// Pointer to the connection from camera to encoder

	MMAL_POOL_T *encoder_pool; /// Pointer to the pool of buffers used by encoder output port
} RASPIVID_STATE;

/** Struct used to pass information in encoder port userdata to callback
*/
typedef struct
{
	FILE *file_handle;                   /// File handle to write buffer data to.
	VCOS_SEMAPHORE_T complete_semaphore; /// semaphore which is posted when we reach end of frame (indicates end of capture or fault)
	RASPIVID_STATE *pstate;            /// pointer to our state in case required in callback
} PORT_USERDATA;

// Our main data storage vessel..
RASPIVID_STATE stateCamera;

MMAL_STATUS_T status = -1;
MMAL_PORT_T *camera_preview_port = NULL;
MMAL_PORT_T *camera_video_port = NULL;
MMAL_PORT_T *camera_still_port = NULL;
MMAL_PORT_T *preview_input_port = NULL;
MMAL_PORT_T *encoder_input_port = NULL;
MMAL_PORT_T *encoder_output_port = NULL;
FILE *output_file = NULL;

/**
* Assign a default set of parameters to the state passed in
*
* @param state Pointer to state structure to assign defaults to
*/
static void default_status(RASPIVID_STATE *state)
{
	if (!state)
	{
		vcos_assert(0);
		return;
	}

	// Default everything to zero
	memset(state, 0, sizeof(RASPIVID_STATE));

	// Now set anything non-zero
	state->timeout = 5000;     // 5s delay before take image
	state->width = 1920;       // Default to 1080p
	state->height = 1080;
	state->bitrate = 17000000; // This is a decent default bitrate for 1080p
	state->framerate = VIDEO_FRAME_RATE_NUM;
	state->demoMode = 0;
	state->demoInterval = 250; // ms
	state->immutableInput = 1;

	// Setup preview window defaults
	raspipreview_set_defaults(&state->preview_parameters);

	// Set up the camera_parameters to default
	raspicamcontrol_set_defaults(&state->camera_parameters);
}

/**
* Dump image state parameters to printf. Used for debugging
*
* @param state Pointer to state structure to assign defaults to
*/
static void dump_status(RASPIVID_STATE *state)
{
	if (!state)
	{
		vcos_assert(0);
		return;
	}

	fprintf(stderr, "Width %d, Height %d, filename %s\n", state->width, state->height, state->filename);
	fprintf(stderr, "bitrate %d, framerate %d, time delay %d\n", state->bitrate, state->framerate, state->timeout);

	raspipreview_dump_parameters(&state->preview_parameters);
	raspicamcontrol_dump_parameters(&state->camera_parameters);
}

/**
*  buffer header callback function for camera control
*
*  Callback will dump buffer data to the specific file
*
* @param port Pointer to port from which callback originated
* @param buffer mmal buffer header pointer
*/
static void camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	if (buffer->cmd == MMAL_EVENT_PARAMETER_CHANGED)
	{
	}
	else
	{
		vcos_log_error("Received unexpected camera control callback event, 0x%08x", buffer->cmd);
	}

	mmal_buffer_header_release(buffer);
}

/**
*  buffer header callback function for encoder
*
*  Callback will dump buffer data to the specific file
*
* @param port Pointer to port from which callback originated
* @param buffer mmal buffer header pointer
*/
static void encoder_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	MMAL_BUFFER_HEADER_T *new_buffer;

	// We pass our file handle and other stuff in via the userdata field.

	PORT_USERDATA *pData = (PORT_USERDATA *)port->userdata;

	if (pData)
	{
		if(recordVideo)
		{
			vcos_assert(pData->file_handle);

			if (buffer->length)
			{
				mmal_buffer_header_mem_lock(buffer);

				fwrite(buffer->data, 1, buffer->length, pData->file_handle);

				mmal_buffer_header_mem_unlock(buffer);
			}
		}
	}
	else
	{
		vcos_log_error("Received a encoder buffer callback with no state");
	}

	// release buffer back to the pool
	mmal_buffer_header_release(buffer);

	// and send one back to the port (if still open)
	if (port->is_enabled)
	{
		MMAL_STATUS_T status;

		new_buffer = mmal_queue_get(pData->pstate->encoder_pool->queue);

		if (new_buffer)
			status = mmal_port_send_buffer(port, new_buffer);

		if (!new_buffer || status != MMAL_SUCCESS)
			vcos_log_error("Unable to return a buffer to the encoder port");
	}
}


/**
* Create the camera component, set up its ports
*
* @param state Pointer to state control struct
*
* @return 0 if failed, pointer to component if successful
*
*/
static MMAL_COMPONENT_T *create_camera_component(RASPIVID_STATE *state)
{
	MMAL_COMPONENT_T *camera = 0;
	MMAL_ES_FORMAT_T *format;
	MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;
	MMAL_STATUS_T status;

	/* Create the component */
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("Failed to create camera component");
		goto error;
	}

	if (!camera->output_num)
	{
		vcos_log_error("Camera doesn't have output ports");
		goto error;
	}

	preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
	video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
	still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

	// Enable the camera, and tell it its control callback function
	status = mmal_port_enable(camera->control, camera_control_callback);

	if (status)
	{
		vcos_log_error("Unable to enable control port : error %d", status);
		goto error;
	}

	//  set up the camera configuration
	{
		MMAL_PARAMETER_CAMERA_CONFIG_T cam_config =
		{
			{ MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config) },
			.max_stills_w = state->width,
			.max_stills_h = state->height,
			.stills_yuv422 = 0,
			.one_shot_stills = 0,
			.max_preview_video_w = state->width,
			.max_preview_video_h = state->height,
			.num_preview_video_frames = 3,
			.stills_capture_circular_buffer_height = 0,
			.fast_preview_resume = 0,
			.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
		};
		mmal_port_parameter_set(camera->control, &cam_config.hdr);
	}

	// Now set up the port formats

	// Set the encode format on the Preview port
	// HW limitations mean we need the preview to be the same size as the required recorded output

	format = preview_port->format;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->encoding_variant = MMAL_ENCODING_I420;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->es->video.width = state->width;
	format->es->video.height = state->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = state->width;
	format->es->video.crop.height = state->height;
	format->es->video.frame_rate.num = state->framerate;
	format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;

	status = mmal_port_format_commit(preview_port);

	if (status)
	{
		vcos_log_error("camera viewfinder format couldn't be set");
		goto error;
	}

	// Set the encode format on the video  port

	format = video_port->format;
	format->encoding_variant = MMAL_ENCODING_I420;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->es->video.width = state->width;
	format->es->video.height = state->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = state->width;
	format->es->video.crop.height = state->height;
	format->es->video.frame_rate.num = state->framerate;
	format->es->video.frame_rate.den = VIDEO_FRAME_RATE_DEN;

	status = mmal_port_format_commit(video_port);

	if (status)
	{
		vcos_log_error("camera video format couldn't be set");
		goto error;
	}

	// Ensure there are enough buffers to avoid dropping frames
	if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;


	// Set the encode format on the still  port

	format = still_port->format;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->encoding_variant = MMAL_ENCODING_I420;

	format->es->video.width = state->width;
	format->es->video.height = state->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = state->width;
	format->es->video.crop.height = state->height;
	format->es->video.frame_rate.num = 1;
	format->es->video.frame_rate.den = 1;

	status = mmal_port_format_commit(still_port);

	if (status)
	{
		vcos_log_error("camera still format couldn't be set");
		goto error;
	}

	/* Ensure there are enough buffers to avoid dropping frames */
	if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

	/* Enable component */
	status = mmal_component_enable(camera);

	if (status)
	{
		vcos_log_error("camera component couldn't be enabled");
		goto error;
	}

	raspicamcontrol_set_all_parameters(camera, &state->camera_parameters);

	state->camera_component = camera;

	if (state->verbose)
		fprintf(stderr, "Camera component done\n");

	return camera;

error:

	if (camera)
		mmal_component_destroy(camera);

	return 0;
}

/**
* Destroy the camera component
*
* @param state Pointer to state control struct
*
*/
static void destroy_camera_component(RASPIVID_STATE *state)
{
	if (state->camera_component)
	{
		mmal_component_destroy(state->camera_component);
		state->camera_component = NULL;
	}
}

/**
* Create the encoder component, set up its ports
*
* @param state Pointer to state control struct
*
* @return 0 if failed, pointer to component if successful
*
*/
static MMAL_COMPONENT_T *create_encoder_component(RASPIVID_STATE *state)
{
	MMAL_COMPONENT_T *encoder = 0;
	MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL;
	MMAL_STATUS_T status;
	MMAL_POOL_T *pool;

	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER, &encoder);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("Unable to create video encoder component");
		goto error;
	}

	if (!encoder->input_num || !encoder->output_num)
	{
		vcos_log_error("Video encoder doesn't have input/output ports");
		goto error;
	}

	encoder_input = encoder->input[0];
	encoder_output = encoder->output[0];

	// We want same format on input and output
	mmal_format_copy(encoder_output->format, encoder_input->format);

	// Only supporting H264 at the moment
	encoder_output->format->encoding = MMAL_ENCODING_H264;

	encoder_output->format->bitrate = state->bitrate;

	encoder_output->buffer_size = encoder_output->buffer_size_recommended;

	if (encoder_output->buffer_size < encoder_output->buffer_size_min)
		encoder_output->buffer_size = encoder_output->buffer_size_min;

	encoder_output->buffer_num = encoder_output->buffer_num_recommended;

	if (encoder_output->buffer_num < encoder_output->buffer_num_min)
		encoder_output->buffer_num = encoder_output->buffer_num_min;

	// Commit the port changes to the output port
	status = mmal_port_format_commit(encoder_output);

	if (status != MMAL_SUCCESS)
	{
		vcos_log_error("Unable to set format on video encoder output port");
		goto error;
	}


	// Set the rate control parameter
	if (0)
	{
		MMAL_PARAMETER_VIDEO_RATECONTROL_T param = {{ MMAL_PARAMETER_RATECONTROL, sizeof(param)}, MMAL_VIDEO_RATECONTROL_DEFAULT};
		status = mmal_port_parameter_set(encoder_output, &param.hdr);
		if (status != MMAL_SUCCESS)
		{
			vcos_log_error("Unable to set ratecontrol");
			goto error;
		}

	}

	if (mmal_port_parameter_set_boolean(encoder_input, MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT, state->immutableInput) != MMAL_SUCCESS)
	{
		vcos_log_error("Unable to set immutable input flag");
		// Continue rather than abort..
	}

	//  Enable component
	status = mmal_component_enable(encoder);

	if (status)
	{
		vcos_log_error("Unable to enable video encoder component");
		goto error;
	}

	/* Create pool of buffer headers for the output port to consume */
	pool = mmal_port_pool_create(encoder_output, encoder_output->buffer_num, encoder_output->buffer_size);

	if (!pool)
	{
		vcos_log_error("Failed to create buffer header pool for encoder output port %s", encoder_output->name);
	}

	state->encoder_pool = pool;
	state->encoder_component = encoder;

	if (state->verbose)
		fprintf(stderr, "Encoder component done\n");

	return encoder;

error:
	if (encoder)
		mmal_component_destroy(encoder);

	return 0;
}

/**
* Destroy the encoder component
*
* @param state Pointer to state control struct
*
*/
static void destroy_encoder_component(RASPIVID_STATE *state)
{
	// Get rid of any port buffers first
	if (state->encoder_pool)
	{
		mmal_port_pool_destroy(state->encoder_component->output[0], state->encoder_pool);
	}

	if (state->encoder_component)
	{
		mmal_component_destroy(state->encoder_component);
		state->encoder_component = NULL;
	}
}

/**
* Connect two specific ports together
*
* @param output_port Pointer the output port
* @param input_port Pointer the input port
* @param Pointer to a mmal connection pointer, reassigned if function successful
* @return Returns a MMAL_STATUS_T giving result of operation
*
*/
static MMAL_STATUS_T connect_ports(MMAL_PORT_T *output_port, MMAL_PORT_T *input_port, MMAL_CONNECTION_T **connection)
{
	MMAL_STATUS_T status;

	status =  mmal_connection_create(connection, output_port, input_port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);

	if (status == MMAL_SUCCESS)
	{
		status =  mmal_connection_enable(*connection);
		if (status != MMAL_SUCCESS)
			mmal_connection_destroy(*connection);
	}

	return status;
}

/**
* Checks if specified port is valid and enabled, then disables it
*
* @param port  Pointer the port
*
*/
static void check_disable_port(MMAL_PORT_T *port)
{
	if (port && port->is_enabled)
		mmal_port_disable(port);
}

/**
* Handler for sigint signals
*
* @param signal_number ID of incoming signal.
*
*/
static void signal_handler(int signal_number)
{
	// Going to abort on all signals
	vcos_log_error("Aborting program\n");

	// TODO : Need to close any open stuff...how?

	exit(255);
}

// End camera functions
//================


//------------------------------------------------------------------------------

static void exit_func(void)
	// Function to be passed to atexit().
{
	if (eglImage != 0)
	{
		if (!eglDestroyImageKHR(state->display, (EGLImageKHR) eglImage))
			printf("eglDestroyImageKHR failed.");
	}

	// clear screen
	glClear( GL_COLOR_BUFFER_BIT );
	eglSwapBuffers(state->display, state->surface);

	// Release OpenGL resources
	eglMakeCurrent( state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
	eglDestroySurface( state->display, state->surface );
	eglDestroyContext( state->display, state->context );
	eglTerminate( state->display );

	printf("\ncube closed\n");
} // exit_func()

//==============================================================================

//int main ()
void initApp()
{
	bcm_host_init();

	printf("Note: ensure you have sufficient gpu_mem configured\n");

	// Register our application with the logging system
	vcos_log_register("RaspiVid", VCOS_LOG_CATEGORY);

	signal(SIGINT, signal_handler);

	default_status(&stateCamera);

	stateCamera.verbose = 1;
	stateCamera.preview_parameters.wantPreview = 1;
	stateCamera.preview_parameters.wantFullScreenPreview = 1;
	stateCamera.filename = ""; //"test.h264";
	stateCamera.camera_parameters.vflip = 1;
	stateCamera.timeout = 10000;

	// Setup and start camera
	if (stateCamera.verbose)
	{
		fprintf(stderr, "\nRaspiVid Camera App\n");
		fprintf(stderr, "===================\n\n");

		dump_status(&stateCamera);
	}
	if (!create_camera_component(&stateCamera))
	{
		vcos_log_error("%s: Failed to create camera component", __func__);
	}
	else if (!raspipreview_create(&stateCamera.preview_parameters))
	{
		vcos_log_error("%s: Failed to create preview component", __func__);
		destroy_camera_component(&stateCamera);
	}
	else if (!create_encoder_component(&stateCamera))
	{
		vcos_log_error("%s: Failed to create encode component", __func__);
		raspipreview_destroy(&stateCamera.preview_parameters);
		destroy_camera_component(&stateCamera);
	}
	else
	{
		PORT_USERDATA callback_data;

		if (stateCamera.verbose)
			fprintf(stderr, "Starting component connection stage\n");

		camera_preview_port = stateCamera.camera_component->output[MMAL_CAMERA_PREVIEW_PORT];
		camera_video_port   = stateCamera.camera_component->output[MMAL_CAMERA_VIDEO_PORT];
		camera_still_port   = stateCamera.camera_component->output[MMAL_CAMERA_CAPTURE_PORT];
		preview_input_port  = stateCamera.preview_parameters.preview_component->input[0];
		encoder_input_port  = stateCamera.encoder_component->input[0];
		encoder_output_port = stateCamera.encoder_component->output[0];

		if (stateCamera.preview_parameters.wantPreview )
		{
			if (stateCamera.verbose)
			{
				fprintf(stderr, "Connecting camera preview port to preview input port\n");
				fprintf(stderr, "Starting video preview\n");
			}

			// Connect camera to preview
			status = connect_ports(camera_preview_port, preview_input_port, &stateCamera.preview_connection);
		}
		if (status == MMAL_SUCCESS)
		{
			VCOS_STATUS_T vcos_status;

			if (stateCamera.verbose)
				fprintf(stderr, "Connecting camera stills port to encoder input port\n");

			// Now connect the camera to the encoder
			status = connect_ports(camera_video_port, encoder_input_port, &stateCamera.encoder_connection);

			if (status != MMAL_SUCCESS)
			{
				vcos_log_error("%s: Failed to connect camera video port to encoder input", __func__);
				goto error;
			}

			if (stateCamera.filename)
			{
				if (stateCamera.filename[0] == '-')
				{
					output_file = stdout;

					// Ensure we don't upset the output stream with diagnostics/info
					stateCamera.verbose = 0;
				}
				else
				{
					if (stateCamera.verbose)
						fprintf(stderr, "Opening output file \"%s\"\n", stateCamera.filename);

					output_file = fopen(stateCamera.filename, "wb");
				}

				if (!output_file)
				{
					// Notify user, carry on but discarding encoded output buffers
					vcos_log_error("%s: Error opening output file: %s\nNo output file will be generated\n", __func__, stateCamera.filename);
				}
			}

			// Set up our userdata - this is passed though to the callback where we need the information.
			callback_data.file_handle = output_file;
			callback_data.pstate = &stateCamera;

			vcos_status = vcos_semaphore_create(&callback_data.complete_semaphore, "RaspiStill-sem", 0);
			vcos_assert(vcos_status == VCOS_SUCCESS);


			encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)&callback_data;

			if (stateCamera.verbose)
				fprintf(stderr, "Enabling encoder output port\n");

			// Enable the encoder output port and tell it its callback function
			status = mmal_port_enable(encoder_output_port, encoder_buffer_callback);

			if (status != MMAL_SUCCESS)
			{
				vcos_log_error("Failed to setup encoder output");
				goto error;
			}

			if (stateCamera.demoMode)
			{
				// Run for the user specific time..
				int num_iterations = stateCamera.timeout / stateCamera.demoInterval;
				int i;

				if (stateCamera.verbose)
					fprintf(stderr, "Running in demo mode\n");

				for (i=0;i<num_iterations;i++)
				{
					raspicamcontrol_cycle_test(stateCamera.camera_component);
					vcos_sleep(stateCamera.demoInterval);
				}
			}
			else
			{
				// Only encode stuff if we have a filename and it opened
				if (output_file)
				{
					if (stateCamera.verbose)
						fprintf(stderr, "Starting video capture\n");

					if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS)
					{
						goto error;
					}

					// Send all the buffers to the encoder output port
					{
						int num = mmal_queue_length(stateCamera.encoder_pool->queue);
						int q;
						for (q=0;q<num;q++)
						{
							MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(stateCamera.encoder_pool->queue);

							if (!buffer)
								vcos_log_error("Unable to get a required buffer %d from pool queue", q);

							if (mmal_port_send_buffer(encoder_output_port, buffer)!= MMAL_SUCCESS)
								vcos_log_error("Unable to send a buffer to encoder output port (%d)", q);

						}
					}
				}
			}
		}
		else
		{
			mmal_status_to_int(status);
			vcos_log_error("%s: Failed to connect camera to preview", __func__);
		}

		// Clear application state
		memset( state, 0, sizeof( *state ) );

		/*
		// Start OGLES
		init_ogl(state);

		// Setup the model world
		init_model_proj(state);

		// initialise the OGLES texture(s)
		init_textures(state);

		state->model = load_wavefront("/opt/vc/src/hello_pi/hello_camera_teapot/models/navball.obj", NULL);
		*/
		return;
		/*
		while (!terminate)
		{
			update_model(state);
			redraw_scene(state);
		}

		

		if (stateCamera.verbose)
			fprintf(stderr, "Finished capture\n");
			*/
error:

		mmal_status_to_int(status);

		if (stateCamera.verbose)
			fprintf(stderr, "Closing down\n");

		// Disable all our ports that are not handled by connections
		check_disable_port(camera_still_port);
		check_disable_port(encoder_output_port);

		if (stateCamera.preview_parameters.wantPreview )
			mmal_connection_destroy(stateCamera.preview_connection);
		mmal_connection_destroy(stateCamera.encoder_connection);

		// Can now close our file. Note disabling ports may flush buffers which causes
		// problems if we have already closed the file!
		if (output_file && output_file != stdout)
			fclose(output_file);

		/* Disable components */
		if (stateCamera.encoder_component)
			mmal_component_disable(stateCamera.encoder_component);

		if (stateCamera.preview_parameters.preview_component)
			mmal_component_disable(stateCamera.preview_parameters.preview_component);

		if (stateCamera.camera_component)
			mmal_component_disable(stateCamera.camera_component);

		destroy_encoder_component(&stateCamera);
		raspipreview_destroy(&stateCamera.preview_parameters);
		destroy_camera_component(&stateCamera);

		if (stateCamera.verbose)
			fprintf(stderr, "Close down completed, all components disconnected, disabled and destroyed\n\n");
	}
	if (status != 0)
		raspicamcontrol_check_configuration(128);
	exit_func();
	return;
}


void stopApp()
{
		if (stateCamera.verbose)
		fprintf(stderr, "Finished capture\n");


		mmal_status_to_int(status);

		if (stateCamera.verbose)
			fprintf(stderr, "Closing down\n");

		// Disable all our ports that are not handled by connections
		check_disable_port(camera_still_port);
		check_disable_port(encoder_output_port);

		if (stateCamera.preview_parameters.wantPreview )
			mmal_connection_destroy(stateCamera.preview_connection);
		mmal_connection_destroy(stateCamera.encoder_connection);

		// Can now close our file. Note disabling ports may flush buffers which causes
		// problems if we have already closed the file!
		if (output_file && output_file != stdout)
			fclose(output_file);

		/* Disable components */
		if (stateCamera.encoder_component)
			mmal_component_disable(stateCamera.encoder_component);

		if (stateCamera.preview_parameters.preview_component)
			mmal_component_disable(stateCamera.preview_parameters.preview_component);

		if (stateCamera.camera_component)
			mmal_component_disable(stateCamera.camera_component);

		destroy_encoder_component(&stateCamera);
		raspipreview_destroy(&stateCamera.preview_parameters);
		destroy_camera_component(&stateCamera);

		if (stateCamera.verbose)
			fprintf(stderr, "Close down completed, all components disconnected, disabled and destroyed\n\n");
	
	if (status != 0)
		raspicamcontrol_check_configuration(128);
	exit_func();
}