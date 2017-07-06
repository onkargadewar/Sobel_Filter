#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <CL/opencl.h>
#include "util.h"


#ifndef PLATFOORM
#define DEFAULT_PLATFORM 2
#endif


int main(int argc, char **argv)
{

	cl_int errNum;
	cl_context context;
	cl_uint platformCount, size, deviceCount, sectionSize;
	cl_long count = 0;
	cl_platform_id* platforms;
	cl_device_id* devices;
	char *value;
	double run_time;
	size_t valueSize, length;
	FILE *fptr, *inFile;
	const char *filename = "F:\\workspace\\Sobel_filter\\Sobel_filter\\sobel.cl";
	const char *imagename = "C:\\Users\\USER\\Pictures\\in.ppm";
	const char *src;
	char ch, buff[16];
	PPMImage *img;
	cl_program program;
	cl_command_queue commandQueues;
	cl_kernel kernelsobel;
	cl_mem buffers[2];
	cl_event keventA, keventB;
	int c, rgb_comp_color;
	int  index = 0;
	// get all platforms
	clGetPlatformIDs(0, NULL, &platformCount);
	platforms = malloc(sizeof(cl_platform_id) * platformCount);
	clGetPlatformIDs(platformCount, platforms, NULL);

	for (int i = 0; i < platformCount; i++)
	{
		// get all devices
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &deviceCount);
		//clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, 0, NULL, &deviceCount);
		devices = malloc(sizeof(cl_device_id) * deviceCount);
		clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, deviceCount, devices, NULL);
		//clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, deviceCount, devices, NULL);

		clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 0, NULL, &valueSize);
		value = malloc(valueSize);
		clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, valueSize, value, NULL);
		printf("%d. Platform: %s\n", i + 1, value);
		free(value);

		// for each device print critical attributes
		for (int j = 0; j < deviceCount; j++)
		{
			int k = 1, l = 1;
			// print device name
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, 0, NULL, &valueSize);
			value = malloc(valueSize);
			clGetDeviceInfo(devices[j], CL_DEVICE_NAME, valueSize, value, NULL);
			printf(" %d.%d. Device: %s\n", i + 1, j + 1, value);
			free(value);
		}

	}

	cl_context_properties contextProperties[] = { CL_CONTEXT_PLATFORM,(cl_context_properties)platforms[DEFAULT_PLATFORM],0 };//1st device of last platform
	context = clCreateContext(contextProperties, 1, &(devices[0]), NULL, NULL, &errNum);
	error(errNum, "creating context");

	for (int i = 0; i < NUM_OF_BUFFERS; i++)
	{
		buffers[i] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(PPMPixel) * NUM_OF_PIXELS, NULL, &errNum);
		error(errNum, "clCreateBuffer");
	}

	//open the kernel file for reading
	fptr = fopen(filename, "rb");//As we are opening non-text files
	if (fptr == NULL)
	{
		printf("Cannot open file");
		exit(0);
	}
	else
	{
		fseek(fptr, 0L, SEEK_END);
		length = ftell(fptr);
		fseek(fptr, 0, SEEK_SET);
		src = (const char*)malloc((length + 1) * sizeof(const char));
		if (src)
		{
			fread(src, sizeof(const char), length, fptr);
		}
		fclose(fptr);

	}

	//open PPM file for reading
	inFile = fopen(imagename, "rb");
	if (!inFile) {
		fprintf(stderr, "Unable to open file '%s'\n", inFile);
		exit(1);
	}

	//read image format
	if (!fgets(buff, sizeof(buff), inFile)) {
		perror(inFile);
		exit(1);
	}

	//check the image format
	if (buff[0] != 'P' || buff[1] != '3') {
		fprintf(stderr, "Invalid image format (must be 'P3')\n");
		exit(1);
	}

	//alloc memory form image
	img = (PPMImage *)malloc(sizeof(PPMImage));
	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//read image size information
	if (fscanf(inFile, "%d %d", &img->x, &img->y) != 2) {
		fprintf(stderr, "Invalid image size (error loading '%s')\n", inFile);
		exit(1);
	}

	//read rgb component
	if (fscanf(inFile, "%d", &rgb_comp_color) != 1) {
		fprintf(stderr, "Invalid rgb component (error loading '%s')\n", inFile);
		exit(1);
	}

	//check rgb component depth
	if (rgb_comp_color != RGB_COMPONENT_COLOR) {
		fprintf(stderr, "'%s' does not have 8-bits components\n", inFile);
		exit(1);
	}

	while (fgetc(inFile) != '\n');
	//memory allocation for pixel data
	//img->data = (PPMPixel*)malloc(img->x * img->y * sizeof(PPMPixel));
	
	for (int i=0;i<NUM_OF_PIXELS;i++)
	{
		img->data[i] = (PPMPixel*)malloc(sizeof(PPMPixel));
	}
	
	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//check for comments
	c = getc(inFile);

	//printf("\n%c\n", c); to check starting of comments

	bool flag = true;
	while (c != EOF)
	{
		if (c == '#')
		{
			flag = false;
			while (getc(inFile) != '\n');

		}
		else {
			if (flag == false) {
				break;
			}
		}
		c = getc(inFile);
	}
	
	ungetc(c, inFile);

	printf("\n\n***********Contents read from the file***********************\n\n");

	
	//Add index to data so that values are not overwritten

	while (true && index<NUM_OF_PIXELS)
	{
		int ret = fscanf(inFile, "%u %u %u", &img->data[index]->red, &img->data[index]->green, &img->data[index]->blue);
		printf("%u %u %u %u ", img->data[index]->red, img->data[index]->green, img->data[index]->blue, img->data[index]->alpha);
		if (ret == EOF)
			break;
		index++;
	}

	//int noofelements = img->x*img->y;

	//Attempts to read  3* sizeof(cl_int) elements of each of which is img->x * img->y byte long(complete image)
	//fread() returns the number of data elements it was able to read 
	//If it reaches end-of-file (or an error condition) before reading the full img->y bytes, it indicates exactly how many bytes it read.

	/*
	size_t noofelements = fread(img->data, sizeof(cl_uchar), 4*img->x * img->y, inFile);
	printf("\nNumber of elements read: %d", noofelements);


	if (noofelements != (4*img->x*img->y)) {
		printf("\nError loading image" );
		exit(1);
	}
	*/

	fclose(inFile);


	//Create program from source
	program = clCreateProgramWithSource(context, 1, (const char **)&src, &length, &errNum);//constant char pointer
	if (program == NULL)
	{
		clReleaseProgram(program);
	}
	error(errNum, "clCreateProgramWithSource");

	//Build program
	errNum = clBuildProgram(program, 1, &devices[0], NULL, NULL, NULL);
	error(errNum, "clBuildProgram");
	if (errNum != CL_SUCCESS)
	{
		char buildLog[16834];
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);
		printf("\nError in OpenCL C source\n");
		printf("%s", buildLog);
		//error(errNum, "clBuildProgram");	
		clReleaseProgram(program);

	}

	commandQueues = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, &errNum);

	if (commandQueues == NULL)
	{
		error(errNum, "clCreateCommandQueue");
	}

	/* To check if clEnqueueWriteBuffer works for homogeneous data

	int a[64];

	for (int i = 0; i < 64; i++)
	{
		a[i] = rand() % 64;

	}

	errNum = clEnqueueWriteBuffer(commandQueues,buffers[0], CL_TRUE,0, 64 * sizeof(cl_int),a,0,NULL,NULL);

	cl_int *mapPtr = (cl_int*)clEnqueueMapBuffer(commandQueues, buffers[0], CL_TRUE, CL_MAP_WRITE, 0, 64 * sizeof(cl_int), 0, NULL, NULL, &errNum);
	*/

	//test code to copy data from image to buffer
	//fillDataBuffer(img->data, buffers[0],sizeof(cl_uchar));
	
	for (int i=0;i<NUM_OF_PIXELS;i++)
	{ 
	errNum = clEnqueueWriteBuffer(commandQueues, buffers[0], CL_TRUE, (0 + i*sizeof(PPMPixel)), sizeof(PPMPixel), (void*)img->data[i], 0, NULL, NULL);
	}

	cl_uchar *mapPtr = (cl_uchar*)clEnqueueMapBuffer(commandQueues, buffers[0], CL_TRUE, CL_MAP_READ, 0, NUM_OF_PIXELS * sizeof(PPMPixel), 0, NULL, NULL, &errNum);
	error(errNum, "clEnqueueMapBuffer");

	if (errNum != CL_SUCCESS)
	{
		printf("Error while writing contents of contents of image to the input buffer\n");
		error(errNum, "clEnqueueWriteBuffer");
		return 1;
	}
	
	printf("\n\n***********Data from image copied to buffer***********\n\n");

	for (int i = 0; i < NUM_OF_PIXELS * sizeof(PPMPixel); i++)
	{
		printf("%u  ", mapPtr[i]);
	}

		
	//Create kernel for sobel filter
	kernelsobel = clCreateKernel(program, "SobelDetector", &errNum);
	if (kernelsobel == NULL)
	{
		error(errNum, "clCreatekernelsobel");
		clReleaseKernel(kernelsobel);
	}

	//Set the kernel arguments to read data from an input
	errNum = clSetKernelArg(kernelsobel, 0, sizeof(cl_mem), &buffers[0]);
	errNum |= clSetKernelArg(kernelsobel, 1, sizeof(cl_mem), &buffers[1]);
	
	if (errNum != CL_SUCCESS)
	{
		printf("Error setting kernelsobel's arguments \n");
		clReleaseKernel(kernelsobel);
	}

	//Create the size holders
	size_t * globalWorksize = (size_t*)malloc(sizeof(size_t) * 2);
	size_t * localWorksize = (size_t*)malloc(sizeof(size_t) * 2);

	//Set the size
	globalWorksize[0] = (img->x); globalWorksize[1] = (img->y);
	localWorksize[0] = 1; localWorksize[1] = 1;
		
	errNum = clEnqueueNDRangeKernel(commandQueues, kernelsobel, 2, NULL, globalWorksize, localWorksize, 0, NULL, &keventA);
	if (errNum != CL_SUCCESS)
	{
		//printf("Error queuing kernelsobel for execution\n");
		error(errNum, "clEnqueueNDRangeKernel");
		return 1;
	}

	//Event profiling code for edge detection
	clWaitForEvents(1, &keventA);
	errNum = clGetEventProfilingInfo(keventA, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &time_start, NULL);
	errNum = clGetEventProfilingInfo(keventA, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &time_end, NULL);
	total_time = (double)(time_end - time_start);

	printf("\n\nThe execution time in seconds for edge detection with Sobel filter = %f secs\n\n", total_time*1.0e-9);

	cl_uchar *mapPtr1 = (cl_uchar*)clEnqueueMapBuffer(commandQueues, buffers[1], CL_TRUE, CL_MAP_READ, 0, NUM_OF_PIXELS * sizeof(PPMPixel), 0, NULL, NULL, &errNum);
	error(errNum, "clEnqueueMapBuffer");
	
	errNum = clEnqueueReadBuffer(commandQueues, buffers[1], CL_TRUE, 0, NUM_OF_PIXELS*sizeof(PPMPixel), (void*)mapPtr1, 0, NULL, NULL);

	if (errNum != CL_SUCCESS)
	{
		printf("Error while reading contents of processed image from the output buffer\n");
		error(errNum, "clEnqueueReadBuffer");
		return 1;
	}
	

	printf("\n\n***********Processed data from image copied to output buffer***********\n\n");

	for (int i = 0; i < NUM_OF_PIXELS * sizeof(PPMPixel); i++)
	{
		printf("%u  ", mapPtr1[i]);
		
	}

	


}