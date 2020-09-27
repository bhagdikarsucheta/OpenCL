#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<fstream>
#include<sstream>
#include"C:/Program Files (x86)/IntelSWTools/system_studio_2020/OpenCL/sdk/include/CL/opencl.h"

const int ARRAY_SIZE = 1000;

//Create an OpenCL context on the first available platform using either a GPU or CPU depending on what is available

cl_context CreateContext()
{
	cl_int errNum;
	cl_uint numPlatforms;
	cl_platform_id firstPlatformId;
	cl_context context = NULL;

	//First select an OpenCL Platform to run on,
	//For this example we simple choose first available platform.Normally we would query all available platforms and select appropriate one

	errNum = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
	if (errNum != CL_SUCCESS || numPlatforms <= 0)
	{
		std::cerr << "Failed to find any OpenCL Platforms" << std::endl;
		return NULL;
	}

	//Next create an OpenCL context on the platform.Attempt to create a GPU-based context and if that 
	//failes try to create a CPU based context.

	cl_context_properties contextProperties[] =
	{
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)firstPlatformId,
		0
	};

	context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU, NULL, NULL, &errNum);


	if (errNum != CL_SUCCESS)
	{
		std::cout << "Could not create GPU context, trying CPU..." << std::endl;
		context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_CPU, NULL, NULL, &errNum);

		if (errNum != CL_SUCCESS)
		{
			std::cout << "Failed to create an OpenCL GPU or CPU context..." << std::endl;
			return NULL;
		}
	}

	return context;
}




//create a command queue on the first device available on the context
cl_command_queue CreateCommandQueue(cl_context context, cl_device_id*device)
{
	cl_int errNum;
	cl_device_id * devices;
	cl_command_queue commandQueue = NULL;
	size_t deviceBufferSize = -1;

	//First get the size of the devices buffer
	errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &deviceBufferSize);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Failed to call clGetContextInfo ";
		return NULL;
	}
	if (deviceBufferSize <= 0)
	{
		std::cerr << "No devices available.";
		return NULL;
	}

	//Allocate memory for the device buffer
	devices = new cl_device_id[deviceBufferSize / sizeof(cl_device_id)];
	errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, devices, NULL);
	if (errNum != CL_SUCCESS)
	{
		delete[]devices;
		std::cerr << "Failed to get device IDs";
		return NULL;
	}

	//In this program we choose first available device.In other program
	//we would likely use all available devices or choose the highest performance device

	commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
	if (commandQueue == NULL)
	{
		delete[] devices;
		std::cerr << "Failed to create command queue for device 0";
		return NULL;
	}

	*device = devices[0];
	delete[] devices;
	return commandQueue;
}



//Create an OpenCL Program from the kernel source file
cl_program CreateProgram(cl_context context, cl_device_id device, const char * fileName)
{
	cl_int errNum;
	cl_program program;

	std::ifstream kernelFile(fileName, std::ios::in);
	if (!kernelFile.is_open())
	{
		std::cerr << "Failed to open File for Reading: " << fileName << std::endl;
		return NULL;
	}

	std::ostringstream oss;
	oss << kernelFile.rdbuf();

	std::string srcStdStr = oss.str();
	const char *srcStr = srcStdStr.c_str();
	program = clCreateProgramWithSource(context, 1, (const char**)&srcStr, NULL, NULL);

	if (program == NULL)
	{
		std::cerr << "Failed to create CL Program from source." << std::endl;
		return NULL;
	}

	errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		//Determine the reason for Failure
		char buildLog[16384];
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);

		std::cerr << "Error in kernel: " << std::endl;
		std::cerr << buildLog;
		clReleaseProgram(program);
		return NULL;
	}
	return program;
}


//Create Memory objects used as the arguments to the kernel
//The kernel taked three arguments : result(output),a(input),b(input)

bool CreateMemObjects(cl_context context, cl_mem memObjects[3], float *a, float *b)
{
	memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(float)*ARRAY_SIZE, a, NULL);

	memObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(float)*ARRAY_SIZE, b, NULL);

	memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE,
		sizeof(float) * ARRAY_SIZE, NULL, NULL);

	if (memObjects[0] == NULL || memObjects[1] == NULL || memObjects[2] == NULL)
	{
		std::cerr << "Error Creating Memory Objects." << std::endl;
		return NULL;
	}
	return true;
}


//Cleanup any created OpenCL resources

void Cleanup(cl_context context, cl_command_queue commandQueue,
	cl_program program, cl_kernel kernel, cl_mem memObjects[3])
{

	for (int i = 0; i < 3; i++)
	{
		if (memObjects[i] != 0)
			clReleaseMemObject(memObjects[i]);
	}
	if (commandQueue != 0)
		clReleaseCommandQueue(commandQueue);

	if (kernel != 0)
		clReleaseKernel(kernel);

	if (program != 0)
		clReleaseProgram(program);

	if (context != 0)
		clReleaseContext(context);
}


//main() for HelloWorld Example

int main()
{
	cl_context context = 0;
	cl_command_queue commandQueue = 0;
	cl_program program = 0;
	cl_device_id device = 0;
	cl_kernel kernel = 0;
	cl_mem memObjects[3] = { 0,0,0 };
	cl_int errNum;
	void Cleanup(cl_context context, cl_command_queue commandQueue,
		cl_program program, cl_kernel kernel, cl_mem memObjects[3]);

	//Create an OpenCL context on first available platform
	context = CreateContext();
	if (context == NULL)
	{
		std::cerr << "Failed to create CL Program from source." << std::endl;
		return NULL;
	}

	//Create an OpenCL context on first available platform
	commandQueue = CreateCommandQueue(context, &device);
	if (commandQueue == NULL)
	{
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	//Create OpenCL prgram from HelloWorld.cl kernel source
	program = CreateProgram(context, device, "HelloWorld.cl");
	if (commandQueue == NULL)
	{
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}
	//Create OpenCL kernel
	kernel = clCreateKernel(program, "hello_kernel", NULL);
	if (kernel == NULL)
	{
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	//Create Memory Objects that will be used as areguments to kernel
	//First create host memory arrays that will be used to store arguments to the kernel
	float result[ARRAY_SIZE];
	float a[ARRAY_SIZE];
	float b[ARRAY_SIZE];
	for (int i = 0; i < ARRAY_SIZE; i++)
	{
		a[i] = (float)i;
		b[i] = (float)(i * 2);
	}
	if (!CreateMemObjects(context, memObjects, a, b))
	{
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	// Set the Kernel arguments(result,a,b
	errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &memObjects[0]);
	errNum |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &memObjects[1]);
	errNum |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &memObjects[2]);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error setting kernel for arguments." << std::endl;
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	size_t globalWorkSize[1] = { ARRAY_SIZE };
	size_t localWorkSize[1] = { 1 };


	//Queue the kernel up for execution
	errNum = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL,
		globalWorkSize, localWorkSize,
		0, NULL, NULL);

	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error queing kernel for execution." << std::endl;
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	//Read the output buffer back to the host
	errNum = clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE, 0, ARRAY_SIZE * sizeof(float), result,
		0, NULL, NULL);
	if (errNum != CL_SUCCESS)
	{
		std::cerr << "Error reading result buffer." << std::endl;
		Cleanup(context, commandQueue, program, kernel, memObjects);
		return 1;
	}

	//Output the result buffer
	for (int i = 0; i < ARRAY_SIZE; i++)
	{
		std::cout << result[i] << "	";
	}
	std::cout << std::endl;
	std::cout << "Executed program successfully. " << std::endl;
	Cleanup(context, commandQueue, program, kernel, memObjects);
	return 0;
}


