#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <CL/opencl.h>

#define NUM_OF_BUFFERS 2
cl_ulong time_start, time_end;
double total_time;
#define CREATOR "RPFELGUEIRAS"
#define RGB_COMPONENT_COLOR 255


const char* get_error_string(cl_int error);
void error(cl_int err, char* actionName, ...);
//Cleanup any created OpenCL resources
void Cleanup(cl_context context, cl_command_queue commandQueue,
	cl_program program, cl_kernel kernel);

typedef struct {
	unsigned char red, green, blue;
} PPMPixel;

typedef struct {
	int x, y;
	PPMPixel *data;
} PPMImage;