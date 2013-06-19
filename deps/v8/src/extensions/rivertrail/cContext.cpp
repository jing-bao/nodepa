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

#include "cContext.h"
#include "v8Context.h"
#include "debug.h"


#ifdef CLPROFILE
void CL_CALLBACK CContext::CollectTimings( cl_event event, cl_int status, CContext *data)
{
	cl_int result;
	CContext *instance =  data;
	
	DEBUG_LOG_STATUS("CollectTimings", "enquiring for runtimes...");

	result = clGetEventProfilingInfo( event, CL_PROFILING_COMMAND_START, sizeof (cl_ulong), &(instance->clp_exec_start), NULL);
	if (result != CL_SUCCESS) {
		DEBUG_LOG_ERROR("CollectTimings", result);
		instance->clp_exec_start = 0;
	}

	result = clGetEventProfilingInfo( event, CL_PROFILING_COMMAND_END, sizeof (cl_ulong), &(instance->clp_exec_end), NULL);
	if (result != CL_SUCCESS) {
		DEBUG_LOG_ERROR("CollectTimings", result);
		instance->clp_exec_end = 0;
	}

	DEBUG_LOG_STATUS("CollectTimings", "Collected start " << instance->clp_exec_start << " and end " << instance->clp_exec_end);
}
#endif /* CLPROFILE */
//
void CL_CALLBACK CContext::ReportCLError( const char *err_info, const void *private_info, size_t cb, void *user_data)
{
	DEBUG_LOG_CLERROR(err_info);
}
//
CContext::CContext()
{
	DEBUG_LOG_CREATE("CContext", this);
	buildLog = NULL;
	buildLogSize = 0;
	cmdQueue = NULL;
#ifdef CLPROFILE
	clp_exec_start = 0;
	clp_exec_end = 0;
#endif /* CLPROFILE */
#ifdef WINDOWS_ROUNDTRIP
	wrt_exec_start.QuadPart = -1;
	wrt_exec_end.QuadPart = -1;
#endif /* WINDOWS_ROUNDTRIP */
}

v8::Handle<v8::Value> CContext::toV8Object(CContext* cContext){
	v8::Handle<v8::ObjectTemplate> objectTemplate = v8::ObjectTemplate::New();
	objectTemplate->SetInternalFieldCount(1);
	v8::Handle<v8::Object> cContextObject= objectTemplate->NewInstance();
	v8::HandleScope handle_scope;
	cContextObject->SetInternalField(0,v8::External::New(static_cast<CContext*> (cContext)));
	cContextObject->Set(v8::String::New("compileKernel"),
		v8::FunctionTemplate::New(V8Context::compileKernel)->GetFunction());
	cContextObject->SetAccessor(v8::String::New("buildLog"),
		V8Context::buildLogGetter,0,v8::Undefined(),v8::DEFAULT,v8::PropertyAttribute::ReadOnly);
	cContextObject->SetAccessor(v8::String::New("lastExecutionTime"),
		V8Context::lastExecutionTimeGetter,0,v8::Undefined(),v8::DEFAULT,v8::PropertyAttribute::ReadOnly);
	cContextObject->SetAccessor(v8::String::New("lastRoundTripTime"),
		V8Context::lastRoundTripTimeGetter,0,v8::Undefined(),v8::DEFAULT,v8::PropertyAttribute::ReadOnly);
	cContextObject->Set(v8::String::New("canBeMapped"),
		v8::FunctionTemplate::New(V8Context::canBeMapped)->GetFunction());
	cContextObject->Set(v8::String::New("mapData"),
		v8::FunctionTemplate::New(V8Context::mapData)->GetFunction());
	cContextObject->Set(v8::String::New("cloneData"),
		v8::FunctionTemplate::New(V8Context::cloneData)->GetFunction());
	cContextObject->Set(v8::String::New("allocateData"),
		v8::FunctionTemplate::New(V8Context::allocateData)->GetFunction());
	cContextObject->Set(v8::String::New("allocateData2"),
		v8::FunctionTemplate::New(V8Context::allocateData2)->GetFunction());
	return handle_scope.Close(cContextObject);
}

//
int CContext::InitContext(cl_platform_id platform)
{
	cl_int err_code;
	cl_device_id *devices;
	size_t cb;

	cl_context_properties context_properties[3] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, NULL};
	
	context = clCreateContextFromType(context_properties, CL_DEVICE_TYPE_CPU, ReportCLError, this, &err_code);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("InitContext", err_code);
		return err_code;
	}

	err_code = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, NULL, &cb);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("InitContext", err_code);
		return err_code;
	}

	devices = new cl_device_id[cb];
	if (devices == NULL) {
		DEBUG_LOG_STATUS("InitContext", "Cannot allocate device list");
		return CL_MEM_OBJECT_ALLOCATION_FAILURE;
	}
//
	err_code = clGetContextInfo(context, CL_CONTEXT_DEVICES, cb, devices, NULL);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("InitContext", err_code);
		delete devices;
		return err_code;
	}

	cmdQueue = clCreateCommandQueue(context, devices[0], 
#ifdef CLPROFILE 
		CL_QUEUE_PROFILING_ENABLE |
#endif /* CLPROFILE */
#ifdef OUTOFORDERQUEUE
		CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
#endif /* OUTOFORDERQUEUE */
		0,
		&err_code);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("InitContext", err_code);
		delete devices;
		return err_code;
	}

	DEBUG_LOG_STATUS("InitContext", "queue is " << cmdQueue);

	delete devices;

	kernelFailureMem = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, &err_code);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("InitContext", err_code);
		return err_code;
	}

	return CL_SUCCESS;
}
//
CContext::~CContext()
{
	DEBUG_LOG_DESTROY("CContext", this);
	if (buildLog != NULL) {
		buildLog=NULL;
	}
	if (cmdQueue != NULL) {
		clReleaseCommandQueue(cmdQueue);
	}
}
//
///* dpoIKernel compileKernel (in AString source, in AString kernelName, [optional] in AString options); */
bool CContext::compileKernel(const char* & source, const char* & kernelName, const char* & options, CKernel *out )
{
	cl_program program;
	cl_kernel kernel;
	cl_int err_code, err_code2;
	cl_uint numDevices;
	cl_device_id *devices = NULL;
	size_t actual;
	char *sourceStr, *optionsStr = NULL, *kernelNameStr;
	sourceStr = (char*) malloc(strlen(source));
	kernelNameStr = new char[strlen(kernelName)];
	
	int result;

	strcpy(sourceStr,source);
	DEBUG_LOG_STATUS("CompileKernel", "Source: " << sourceStr);
	program = clCreateProgramWithSource(context, 1, (const char**)&sourceStr, NULL, &err_code);

	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("CompileKernel", err_code);
		delete sourceStr;
		return false;
	}

	if(options!=NULL){
		optionsStr = new char[strlen(options)];
		strcpy(optionsStr, options);
	}
	err_code = clBuildProgram(program, 0, NULL, optionsStr, NULL, NULL);
	
	if (err_code != CL_SUCCESS) {
		delete optionsStr;
		DEBUG_LOG_ERROR("CompileKernel", err_code);
	}
		
	err_code2 = clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &numDevices, NULL);
	if (err_code2 != CL_SUCCESS) {
		DEBUG_LOG_ERROR("CompileKernel", err_code2);
		goto FAIL;
	} 

	devices = new cl_device_id[numDevices * sizeof(cl_device_id)];
	err_code2 = clGetProgramInfo(program, CL_PROGRAM_DEVICES, numDevices * sizeof(cl_device_id), devices, NULL);
	if (err_code2 != CL_SUCCESS) {
		DEBUG_LOG_ERROR("CompileKernel", err_code);
		goto FAIL;
	} 
	if (buildLog != NULL) {
		 buildLog = NULL;
	}
	buildLogSize = 0;
	err_code2 = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, &actual);
	if (actual > buildLogSize) {
		
		buildLog = new char [actual * sizeof(char)];
		if (buildLog == NULL) {
			DEBUG_LOG_STATUS("CompileKernel", "Cannot allocate buildLog");
			buildLogSize = 0;
			goto DONE;
		}
		buildLogSize = actual;
		err_code2 = clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, buildLogSize, buildLog, &actual);
	}
			
	if (err_code2 != CL_SUCCESS) {
		DEBUG_LOG_ERROR("CompileKernel", err_code);
		goto FAIL;
	}

	DEBUG_LOG_STATUS("CompileKernel", "buildLog: " << buildLog);
	goto DONE;

FAIL:
	if (buildLog != NULL) {
		delete buildLog;
		buildLog = NULL;
		buildLogSize = 0;
	}

DONE:
	if (devices != NULL) {
		delete devices;
	}
	
	strcpy(kernelNameStr, kernelName);
	kernel = clCreateKernel(program, kernelNameStr, &err_code);
	
	clReleaseProgram(program);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("CompileKernel", err_code);
		delete kernelNameStr;
		return err_code;
	}


	if (out == NULL) {
		clReleaseKernel(kernel);
		DEBUG_LOG_STATUS("CompileKernel", "Cannot create new dpoCKernel object");
		return false;
	}

	/* all kernels share the single buffer for the failure code */
	result = out->InitKernel(cmdQueue, kernel, kernelFailureMem);
	if (result != CL_SUCCESS) {
		clReleaseKernel(kernel);
	}
	
	//if(sourceStr!=NULL)
	//	delete sourceStr;
	/*if(kernelNameStr!=NULL)
		delete kernelNameStr;
	if(optionsStr!=NULL)
		delete optionsStr;*/

	return true;
}
////
///* readonly attribute AString buildLog; */
bool CContext::GetBuildLog(char* & aBuildLog)
{
	if (buildLog != NULL) {
		aBuildLog = new char[buildLogSize];
		aBuildLog= buildLog;		
		return true;
	} else {
		return false;
	}
}

bool CContext::MapData( v8::Handle<v8::Object> source, CData *_retval )
{
  cl_int err_code;
  bool result = false;
  int length = source->Get(v8::String::New("length"))->Int32Value();
  v8::String::Utf8Value v8type(source->GetPrototype()->ToDetailString());
  const char * stype = *v8type ? *v8type : "<string conversion failed>";
  int type;
  size_t arrayByteLength = source->Get(v8::String::New("byteLength"))->Int32Value();
  if (length>0) {
    // we have a typed array
	result = true;
	//_retval = new CData();
    if (_retval == NULL) {
      DEBUG_LOG_STATUS("MapData", "Cannot create new CData object");
      return false;
    }

    cl_mem_flags flags = CL_MEM_READ_ONLY;
    void *tArrayBuffer = NULL;
	if(strcmp(stype,"#<Float64Array>") == 0){
		type= ArrayType::Float64Array;
		tArrayBuffer = new double[length];
		double* temp = new double[length];	
		for(int i=0; i<length; i++){
			temp[i] = source->Get(v8::Integer::New(i))->NumberValue();
		}
		memcpy(tArrayBuffer,temp,arrayByteLength);
		delete temp;
		flags |= CL_MEM_USE_HOST_PTR;
	}
	else if(strcmp(stype,"#<Float32Array>") ==  0){
		type = ArrayType::Float32Array;
		tArrayBuffer = new float[length];
		float* temp = new float[length];		
		for(int i=0; i<length; i++){
			temp[i] = source->Get(v8::Integer::New(i))->NumberValue();
		}
		memcpy(tArrayBuffer,temp,arrayByteLength);
		delete temp;
		flags |= CL_MEM_USE_HOST_PTR;
	}
	//source->
	else if(strcmp(stype,"#<Uint8ClampedArray>") ==  0){
		type = ArrayType::Uint8ClampedArray;
		tArrayBuffer = new unsigned char[length];
		unsigned char* temp = new unsigned char[length];
		for(int i=0; i<length; i++){
			temp[i] = source->Get(v8::Integer::New(i))->Int32Value();
		}
		memcpy(tArrayBuffer,temp,arrayByteLength);
		delete temp;
		flags |= CL_MEM_USE_HOST_PTR;
	}
	else{
		return false;
	}

    cl_mem memObj = clCreateBuffer(context, flags,
		arrayByteLength, tArrayBuffer , &err_code);
    if (err_code != CL_SUCCESS) {
      DEBUG_LOG_ERROR("MapData", err_code);
      return false;
    }

    result = _retval->InitCData( cmdQueue, memObj, type, length, 
        arrayByteLength, tArrayBuffer);
	delete tArrayBuffer;
#ifdef SUPPORT_MAPPING_ARRAYS
  } else if (JSVAL_IS_OBJECT(source)) {
    // maybe it is a regular array. 
    //
    // WARNING: We map a pointer to the actual array here. All this works on CPU only
    //          and only of the OpenCL compiler knows what to do! For the current Intel OpenCL SDK
    //          this works but your milage may vary.
    const jsval *elems = UnsafeDenseArrayElements(cx, JSVAL_TO_OBJECT(source));
    if (elems != NULL) {
      data = new dpoCData();
      if (data == NULL) {
        DEBUG_LOG_STATUS("MapData", "Cannot create new CData object");
        return NS_ERROR_OUT_OF_MEMORY;
      }
	  cl_mem memObj = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(double *), &elems, &err_code);
      if (err_code != CL_SUCCESS) {
        DEBUG_LOG_ERROR("MapData", err_code);
        return NS_ERROR_NOT_AVAILABLE;
      }
     // result = data->InitCData(cx, cmdQueue, memObj, 0 /* bogus type */, 1, sizeof(double *), JSVAL_TO_OBJECT(source));
#ifndef DEBUG_OFF
    } else {
        DEBUG_LOG_STATUS("MapData", "No elements returned!");
#endif /* DEBUG_OFF */
    }
#endif /* SUPPORT_MAPPING_ARRAYS */
  }

  
  
  return result;
}
////
//
bool CContext::CloneData(const v8::Handle<v8::Value> & source, CData *_retval )
{
	// not implemented
	return false;
}
////
//
bool CContext::AllocateData( v8::Handle<v8::Object> templ, unsigned int length, CData *_retval )
{
	cl_int err_code;
	bool result = true;
    v8::String::Utf8Value v8type(templ->GetPrototype()->ToDetailString());
    const char * stype = *v8type ? *v8type : "<string conversion failed>";
    int type;
    size_t arrayByteLength = templ->Get(v8::String::New("byteLength"))->Int32Value();
	int templLength = templ->Get(v8::String::New("length"))->Int32Value();
	size_t bytePerElements = arrayByteLength/templLength;
	
	if(strcmp(stype,"#<Float64Array>") == 0){
		type = ArrayType::Float64Array;
	}
	else if(strcmp(stype,"#<Float32Array>") ==  0){
		type = ArrayType::Float32Array;
	}
	else if(strcmp(stype,"#<Uint8ClampedArray>") ==  0){
		type = ArrayType::Uint8ClampedArray;
	}
	else{
		return false;
	}

	/*if (!JS_EnterLocalRootScope(cx)) {
		DEBUG_LOG_STATUS("AllocateData", "Cannot root local scope");
		return NS_ERROR_NOT_AVAILABLE;
	}*/

	if (length<=0) {
		DEBUG_LOG_STATUS("AllocateData", "size not provided, assuming template's size");
		return false;
	}

	if (_retval == NULL) {
		DEBUG_LOG_STATUS("AllocateData", "Cannot create new CData object");
		return false;
	}


	//DEBUG_LOG_STATUS("AllocateData", "length " << length << " bytePerElements " << bytePerElements);

#ifdef PREALLOCATE_IN_JS_HEAP
	JSObject *jsArray = js_CreateTypedArray(cx, JS_GetTypedArrayType(tArray), length);
	if (!jsArray) {
		DEBUG_LOG_STATUS("AllocateData", "Cannot create typed array");
		return NS_ERROR_OUT_OF_MEMORY;
	}

	cl_mem memObj = clCreateBuffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 
                                                JS_GetTypedArrayByteLength(jsArray), JS_GetTypedArrayData(jsArray), &err_code);
#else /* PREALLOCATE_IN_JS_HEAP */
	cl_mem memObj = clCreateBuffer(context, CL_MEM_READ_WRITE, length * bytePerElements, NULL, &err_code);
#endif /* PREALLOCATE_IN_JS_HEAP */
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("AllocateData", err_code);
		return false;
	}
	void* tArrayBuffer = NULL;
	result = _retval->InitCData( cmdQueue, memObj, type, length, length * bytePerElements, tArrayBuffer);
	delete tArrayBuffer;
    return result;
}
////
//
bool CContext::AllocateData2(CData *templ, unsigned int length, CData *_retval ) 
{
	// this cast is only safe as long as no other implementations of the CData interface exist
	CData *cData = (CData *) templ;
	cl_int err_code;
	bool result;
	size_t bytePerElements;
#ifdef PREALLOCATE_IN_JS_HEAP
	jsval jsBuffer;
#endif /* PREALLOCATE_IN_JS_HEAP */

	/*if (!JS_EnterLocalRootScope(cx)) {
		DEBUG_LOG_STATUS("AllocateData2", "Cannot root local scope");
		return NS_ERROR_NOT_AVAILABLE;
	}*/

	if (_retval == NULL) {
		DEBUG_LOG_STATUS("AllocateData2", "Cannot create new dpoCData object");
		return false;
	}
	
	if (length == 0) {
		DEBUG_LOG_STATUS("AllocateData2", "length not provided, assuming template's size");
		length = cData->GetLength();
	}

	bytePerElements = cData->GetSize() / cData->GetLength();

	DEBUG_LOG_STATUS("AllocateData2", "length " << length << " bytePerElements " << bytePerElements);

#ifdef PREALLOCATE_IN_JS_HEAP
	JSObject *jsArray = js_CreateTypedArray(cx, cData->GetType(), length);
	if (!jsArray) {
		DEBUG_LOG_STATUS("AllocateData2", "Cannot create typed array");
		return NS_ERROR_OUT_OF_MEMORY;
	}

	cl_mem memObj = clCreateBuffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 
                                                JS_GetTypedArrayByteLength(jsArray), JS_GetTypedArrayData(jsArray), &err_code);
#else /* PREALLOCATE_IN_JS_HEAP */
	void* jsArray = NULL;
	cl_mem memObj = clCreateBuffer(context, CL_MEM_READ_WRITE, length * bytePerElements, NULL, &err_code);
#endif /* PREALLOCATE_IN_JS_HEAP */
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("AllocateData2", err_code);
		return false;
	}

	result = _retval->InitCData( cmdQueue, memObj, cData->GetType(), length, length * bytePerElements, jsArray);


	//JS_LeaveLocalRootScope(cx);
		
    return result;
}
////	
///* [implicit_jscontext] bool canBeMapped (in jsval source); */
bool CContext::CanBeMapped( const v8::Handle<v8::Value> & source, bool &_retval )
{
#ifdef SUPPORT_MAPPING_ARRAYS
  if (!JSVAL_IS_OBJECT(source)) {
    *_retval = false;
  } else {
    *_retval = IsNestedDenseArrayOfFloats(cx, JSVAL_TO_OBJECT(source));
  }
#else /* SUPPORT_MAPPING_ARRAYS */
  _retval = false;
#endif /* SUPPORT_MAPPING_ARRAYS */

	return true;
}
//
///* readonly attribute PRUint64 lastExecutionTime; */
bool CContext::GetLastExecutionTime(unsigned long *_retval ) 
{
#ifdef CLPROFILE
	if ((clp_exec_end == 0) || (clp_exec_start == 0)) {
		*_retval = 0;
		return false;
	} else {
		*_retval = clp_exec_end - clp_exec_start;
		return true;
	}
#else /* CLPROFILE */
	return NS_ERROR_NOT_IMPLEMENTED;
#endif /* CLPROFILE */
}
//
///* readonly attribute PRUint64 lastRoundTripTime; */
bool CContext::GetLastRoundTripTime(unsigned long *_retval ) 
{
#ifdef WINDOWS_ROUNDTRIP
	if ((wrt_exec_start.QuadPart == -1) || (wrt_exec_end.QuadPart == -1)) {
		*_retval = 0;
		return NS_ERROR_NOT_AVAILABLE;
	} else {
		LARGE_INTEGER freq;
		if (!QueryPerformanceFrequency(&freq)) {
			DEBUG_LOG_STATUS("GetLastRoundTrupTime", "cannot read performance counter frequency.");
			return NS_ERROR_NOT_AVAILABLE;
		}
		double diff = (double) (wrt_exec_end.QuadPart - wrt_exec_start.QuadPart);
		double time = diff / (double) freq.QuadPart * 1000000000;
		*_retval = (PRUint64) time;
		return NS_OK;
	}
#else /* WINDOWS_ROUNDTRIP */
	*_retval = 0;
	return false;
#endif /* WINDOWS_ROUNDTRIP */
}
//
//#ifdef WINDOWS_ROUNDTRIP
//void CContext::RecordBeginOfRoundTrip(dpoIContext *parent) {
//	CContext *self = (CContext *) parent;
//	if (!QueryPerformanceCounter(&(self->wrt_exec_start))) {
//		DEBUG_LOG_STATUS("RecordBeginOfRoundTrip", "querying performance counter failed");
//		self->wrt_exec_start.QuadPart = -1;
//	}
//}
//
//void CContext::RecordEndOfRoundTrip(dpoIContext *parent) {
//	CContext *self = (CContext *) parent;
//	if (self->wrt_exec_start.QuadPart == -1) {
//		DEBUG_LOG_STATUS("RecordEndOfRoundTrip", "no previous start data");
//		return;
//	}
//	if (!QueryPerformanceCounter(&(self->wrt_exec_end))) {
//		DEBUG_LOG_STATUS("RecordEndOfRoundTrip", "querying performance counter failed");
//		self->wrt_exec_start.QuadPart = -1;
//		self->wrt_exec_end.QuadPart = -1;
//	}
//}
//#endif /* WINDOWS_ROUNDTRIP */

