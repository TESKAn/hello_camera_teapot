OBJS=serial.o bitmap.o triangle.o RaspiCamControl.o RaspiCLI.o RaspiPreview.o models.o project.o modelEngine_c.o ModelEngine.o fonts/texture-font.o fonts/texture-atlas.o fonts/vector.o
BIN=hello_camera_teapot.bin
LDFLAGS+=-lilclient -lmmal -lmmal_core -lmmal_util -lfreetype


include ../Makefile.include
