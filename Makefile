OBJS=serial.o bitmap.o triangle.o video.o RaspiCamControl.o RaspiCLI.o RaspiPreview.o models.o project.o
BIN=hello_camera_teapot.bin
LDFLAGS+=-lilclient -lmmal -lmmal_core -lmmal_util 

include ../Makefile.include


