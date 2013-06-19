///*
// * Copyright (c) 2011, Intel Corporation
// * All rights reserved.
// *
// * Redistribution and use in source and binary forms, with or without 
// * modification, are permitted provided that the following conditions are met:
// *
// * - Redistributions of source code must retain the above copyright notice, 
// *   this list of conditions and the following disclaimer.
// * - Redistributions in binary form must reproduce the above copyright notice, 
// *   this list of conditions and the following disclaimer in the documentation 
// *   and/or other materials provided with the distribution.
// *
// * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
// * THE POSSIBILITY OF SUCH DAMAGE.
// */
//
#include "cKernel.h"
#include "debug.h"
#include "config.h"
#include "v8Kernel.h"

#ifdef WINDOWS_ROUNDTRIP
#include "windows.h"
#endif /* WINDOWS_ROUNDTRIP */
//

CKernel::CKernel()
{
	DEBUG_LOG_CREATE("CKernel", this);
	//parent = parent;
	kernel = NULL;
	cmdQueue = NULL;
}
//
CKernel::~CKernel()
{
	DEBUG_LOG_DESTROY("CKernel", this);
	if (kernel != NULL) {
		clReleaseKernel(kernel);
	}
}


v8::Handle<v8::Value> CKernel::toV8Object(CKernel* cKernel){
	v8::Handle<v8::ObjectTemplate> objectTemplate = v8::ObjectTemplate::New();
	objectTemplate->SetInternalFieldCount(1);
	v8::Handle<v8::Object> cKernelObject= objectTemplate->NewInstance();
	v8::HandleScope handle_scope;
	cKernelObject->SetInternalField(0,v8::External::New(static_cast<CKernel*> (cKernel)));
	cKernelObject->Set(v8::String::New("dpoIKernel"),v8::String::New("dpoIKernel"));
	cKernelObject->SetAccessor(v8::String::New("numberOfArgs"),V8Kernel::numberOfArgsGetter,0);
	cKernelObject->Set(v8::String::New("setScalarArgument"),v8::FunctionTemplate::New(V8Kernel::setScalarArgument)->GetFunction());
	cKernelObject->Set(v8::String::New("run"),v8::FunctionTemplate::New(V8Kernel::run)->GetFunction());
	cKernelObject->Set(v8::String::New("setArgument"),v8::FunctionTemplate::New(V8Kernel::setArgument)->GetFunction());
	return handle_scope.Close(cKernelObject);
}
int CKernel::InitKernel(cl_command_queue aCmdQueue, cl_kernel aKernel, cl_mem aFailureMem)
{
	cl_int err_code;

	kernel = aKernel;
	err_code = clRetainCommandQueue( aCmdQueue);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("initCData", err_code);
		return err_code;
	}
	cmdQueue = aCmdQueue;

	failureMem = aFailureMem;

	err_code = clSetKernelArg(kernel, 0, sizeof(cl_mem), &failureMem);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("initCData", err_code);
		return err_code;
	}

	return CL_SUCCESS;
}
//
///* readonly attribute PRUint32 numberOfArgs; */
bool CKernel::GetNumberOfArgs(unsigned int &aNumberOfArgs)
{
	cl_uint result;
	cl_int err_code;

	err_code = clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, sizeof(cl_uint), &result, NULL);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("GetNumberOfArgs", err_code);
		return false;
	}

	/* skip internal arguments when counting */
	aNumberOfArgs = result - NUMBER_OF_ARTIFICIAL_ARGS;

    return false;
}
//
///* void setArgument (in PRUint32 number, in CData argument); */
bool CKernel::SetArgument( unsigned int number, CData *argument)
{
	cl_int err_code;
	cl_mem buffer;

	/* skip internal arguments */
	number = number + NUMBER_OF_ARTIFICIAL_ARGS;

	buffer = argument->GetContainedBuffer();
	DEBUG_LOG_STATUS("SetArgument", "buffer is " << buffer);

	err_code = clSetKernelArg(kernel, number, sizeof(cl_mem), &buffer);
	
	

	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("SetArgument", err_code);
		return false;
	}

    return true;
}
//
//// High precision is true when arguments are passed as doubles and false when passed as floats.
///* void setScalarArgument (  PRUint32 number,  jsval argument,  jsval isInteger,  jsval highPrecision); */
bool CKernel::SetScalarArgument(unsigned int number, const v8::Handle<v8::Value> & argument, 
		const v8::Handle<v8::Value> & isInteger, const v8::Handle<v8::Value> & highPrecision)
{
	cl_int err_code;
	bool isIntegerB;
	bool isHighPrecisionB;

	/* skip internal arguments */
	number = number + NUMBER_OF_ARTIFICIAL_ARGS;

	if (!isInteger->IsBoolean()) {
		DEBUG_LOG_STATUS("SetScalarArgument", "illegal isInteger argument.");

		return false;
	}
	isIntegerB = isInteger->BooleanValue();
	
	if (!highPrecision->IsBoolean()) {
		DEBUG_LOG_STATUS("SetScalarArgument", "illegal highPrecision argument.");

		return false;
	}
	isHighPrecisionB = highPrecision->BooleanValue();

	if (!argument->IsNumber()) {
		DEBUG_LOG_STATUS("SetScalarArgument", "illegal number argument.");

		return false;
	}

	if (argument->IsInt32()) {
		int value = argument->Int32Value();
		DEBUG_LOG_STATUS("SetScalarArgument", "(argument->IsInt32()) isIntegerB: " << isIntegerB  << " isHighPrecisionB " << isHighPrecisionB);

		if (isIntegerB) {
			DEBUG_LOG_STATUS("SetScalarArgument", "(argument->IsInt32()) setting integer argument " << number << " to integer value " << value);
			cl_int intVal = (cl_int) value;
			err_code = clSetKernelArg(kernel, number, sizeof(cl_int), &intVal);
		} else if (isHighPrecisionB) {
			DEBUG_LOG_STATUS("SetScalarArgument", "setting double argument " << number << " to integer value " << value);
			cl_double doubleVal = (cl_double) value;
			err_code = clSetKernelArg(kernel, number, sizeof(cl_double), &doubleVal);
		} else {
			DEBUG_LOG_STATUS("SetScalarArgument", "setting float argument " << number << " to integer value " << value);
			cl_float floatVal = (cl_float) value;
			err_code = clSetKernelArg(kernel, number, sizeof(cl_float), &floatVal);
		}

		if (err_code != CL_SUCCESS) {
			DEBUG_LOG_ERROR("SetScalarArgument", err_code);
			return false;
		}
	} else if (argument->IsNumber()) {
		double value = argument->NumberValue();
		DEBUG_LOG_STATUS("SetScalarArgument", "(argument->IsNumber()) isIntegerB: " << isIntegerB  << " isHighPrecisionB " << isHighPrecisionB);

		if (isIntegerB) {
			DEBUG_LOG_STATUS("SetScalarArgument", "setting int formal argument " << number << " using double value " << value);
			cl_int intVal = (cl_int) value;
			err_code = clSetKernelArg(kernel, number, sizeof(cl_int), &intVal);
		} else if (isHighPrecisionB) {
			DEBUG_LOG_STATUS("SetScalarArgument", "setting double formal argument " << number << " using double value " << value);
			cl_double doubleVal = (cl_double) value;
			err_code = clSetKernelArg(kernel, number, sizeof(cl_double), &doubleVal);
		} else {
			DEBUG_LOG_STATUS("SetScalarArgument", "setting float formal argument " << number << " using double value " << value);
			cl_float floatVal = (cl_float) value;
			err_code = clSetKernelArg(kernel, number, sizeof(cl_float), &floatVal);
		}

		if (err_code != CL_SUCCESS) {
			DEBUG_LOG_ERROR("SetScalarArgument", err_code);
			return false;
		}
	} else {
		DEBUG_LOG_STATUS("SetScalarArgument", "illegal number argument.");

		return false;
	}

	return true;
}
//
///* PRUint32 run (in PRUint32 rank, [array, size_is (rank)] in PRUint32 shape, [array, size_is (rank), optional] in PRUint32 tile); */
bool CKernel::Run(unsigned int rank, unsigned int *shape, unsigned int *tile, unsigned int *_retval)
{
	cl_int err_code;
	cl_event runEvent, readEvent, writeEvent;
	size_t *global_work_size;
	size_t *local_work_size;
	const int zero = 0;

	DEBUG_LOG_STATUS("Run", "preparing execution of kernel");

    if (sizeof(size_t) == sizeof(unsigned int)) {
		global_work_size = (size_t *) shape;
	} else {
		global_work_size = new size_t[rank];
		if (global_work_size == NULL) {
			DEBUG_LOG_STATUS("Run", "allocation of global_work_size failed");
			return false;
		}
		for (int cnt = 0; cnt < rank; cnt++) {
			global_work_size[cnt] = shape[cnt];
		}
	}

#ifdef USE_LOCAL_WORKSIZE
	if (tile == NULL) {
		local_work_size = NULL;
	} else {
		if ((sizeof(size_t) == sizeof(PRUint32))) {
			local_work_size = (size_t *) tile;
		} else {
			local_work_size = new size_t[rank];
			if (local_work_size == NULL) {
				DEBUG_LOG_STATUS("Run", "allocation of local_work_size failed");
				return false;
			}
			for (int cnt = 0; cnt < rank; cnt++) {
				local_work_size[cnt] = (size_t) tile[cnt];
			}
		}
	}
#else /* USE_LOCAL_WORKSIZE */
	local_work_size = NULL;
#endif /* USE_LOCAL_WORKSIZE */

	DEBUG_LOG_STATUS("Run", "setting failure code to 0");

	err_code = clEnqueueWriteBuffer(cmdQueue, failureMem, CL_FALSE, 0, sizeof(int), &zero, 0, NULL, &writeEvent);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("Run", err_code);
		return false;
	}

	DEBUG_LOG_STATUS("Run", "enqueing execution of kernel");

#ifdef WINDOWS_ROUNDTRIP
	CContext::RecordBeginOfRoundTrip(parent);
#endif /* WINDOWS_ROUNDTRIP */

	err_code = clEnqueueNDRangeKernel(cmdQueue, kernel, rank, NULL, global_work_size, NULL, 1, &writeEvent, &runEvent);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("Run", err_code);
		return false;
	}

	DEBUG_LOG_STATUS("Run", "reading failure code");

	

	err_code = clEnqueueReadBuffer(cmdQueue, failureMem, CL_FALSE, 0, sizeof(int), _retval, 1, &runEvent, &readEvent);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("Run", err_code);
		return false;
	}

	DEBUG_LOG_STATUS("Run", "waiting for execution to finish");
	
	// For now we always wait for the run to complete.
	// In the long run, we may want to interleave this with JS execution and only sync on result read.
	err_code = clWaitForEvents( 1, &readEvent);
	
	DEBUG_LOG_STATUS("Run", "first event fired");		
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("Run", err_code);
		return false;
	}
#ifdef WINDOWS_ROUNDTRIP
	CContext::RecordEndOfRoundTrip(parent);
#endif /* WINDOWS_ROUNDTRIP */
	
#ifdef CLPROFILE
#ifdef CLPROFILE_ASYNC
	err_code = clSetEventCallback( readEvent, CL_COMPLETE, &CContext::CollectTimings, parent);
	
	DEBUG_LOG_STATUS("Run", "second event fired");
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("Run", err_code);
		return NS_ERROR_ABORT;
	}
#else /* CLPROFILE_ASYNC */
	//CContext::CollectTimings(readEvent,CL_COMPLETE,parent);
#endif /* CLPROFILE_ASYNC */
#endif /* CLPROFILE */
		
	DEBUG_LOG_STATUS("Run", "execution completed successfully, start cleanup");
	
	if (global_work_size != (size_t *) shape) {
		delete global_work_size;
	}
#ifdef USE_LOCAL_WORKSIZE
	if (local_work_size != (size_t *) tile) {
		delete local_work_size;
	}
#endif /* USE_LOCAL_WORKSIZE */
	
	err_code = clReleaseEvent(readEvent);
	err_code = clReleaseEvent(runEvent);
	err_code = clReleaseEvent(writeEvent);

	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("Run", err_code);
		return false;
	}

	DEBUG_LOG_STATUS("Run", "cleanup complete");

    return true;
}
