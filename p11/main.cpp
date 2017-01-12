#include <iostream>
#include <CL/cl.hpp>

static const int N = 50000;

int main()
{
	std::vector<cl::Platform> all_platforms;
	cl::Platform::get(&all_platforms);
	if (all_platforms.size() == 0)
	{
		std::cout << "No platforms found. Check OpenCL installation!" << std::endl;
		exit(1);
	}
	cl::Platform default_platform = all_platforms[0];
	std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

	std::vector<cl::Device> all_devices;
	default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
	if (all_devices.size() == 0)
	{
		std::cout << "No devices found. Check OpenCL installation!" << std::endl;
		exit(1);
	}
	cl::Device default_device = all_devices[0];
	std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << std::endl;

	cl::Context context({ default_device });
	cl::Program::Sources sources;

	std::string kernel_code =
		" void kernel func(global const float* A, global const float* B, global float* C)  "
	    " {                                                                                "
		" 		C[get_global_id(0)] = sin(A[get_global_id(0)]) + cos(B[get_global_id(0)]); "
		" }                                                                                ";

	sources.push_back({ kernel_code.c_str(), kernel_code.length() });

	cl::Program program(context, sources);
	if (program.build({ default_device }) != CL_SUCCESS)
	{
		std::cout << " Error building: " <<
			program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
		exit(1);
	}

	float A[N], B[N];

	for (int i = 0; i < N; i++)
	{
		A[i] = i;
		B[i] = i % 3;
	}

	cl::Buffer buffer_A(context, CL_MEM_READ_ONLY, sizeof(float) * N);
	cl::Buffer buffer_B(context, CL_MEM_READ_ONLY, sizeof(float) * N);
	cl::Buffer buffer_C(context, CL_MEM_WRITE_ONLY, sizeof(float) * N);

	cl::CommandQueue queue(context, default_device);

	queue.enqueueWriteBuffer(buffer_A, CL_TRUE, 0, sizeof(float) * N, A);
	queue.enqueueWriteBuffer(buffer_B, CL_TRUE, 0, sizeof(float) * N, B);

	cl::Kernel kernel_func = cl::Kernel(program, "func");
	kernel_func.setArg(0, buffer_A);
	kernel_func.setArg(1, buffer_B);
	kernel_func.setArg(2, buffer_C);
	queue.enqueueNDRangeKernel(kernel_func, cl::NullRange, cl::NDRange(N), cl::NullRange);
	queue.finish();

	float C[N];
	queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, sizeof(float) * N, C);

	return 0;
}