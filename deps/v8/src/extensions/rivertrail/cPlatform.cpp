#pragma once
#include "cPlatform.h"
#include "debug.h"
//#include "v8Context.h"
#include "v8Platform.h"

cl_platform_id CPlatform::platform = NULL;
CPlatform::CPlatform(cl_platform_id aPlatform)
{
	DEBUG_LOG_CREATE("CPlatform", this);
	platform = aPlatform;
}
//
CPlatform::~CPlatform()
{
	DEBUG_LOG_DESTROY("CPlatform", this);
}

bool CPlatform::getNumberOfDevices(int out)
{
	cl_uint devices;
	cl_int err_code = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &devices);
	
	if (err_code != CL_SUCCESS)
	{
		return false;
	}

	out = devices;
    return true;
}
////
bool CPlatform::getPlatformPropertyHelper(cl_platform_info param, char* & out)
{
	char *rString = NULL;
	size_t length;
	cl_int err;
	int result;

	err = clGetPlatformInfo(platform, param, 0, NULL, &length);

	if (err == CL_SUCCESS) {
		rString = new char[length+1];
		err = clGetPlatformInfo(platform, param, length, rString, NULL);
		out = rString;	
		result = CL_SUCCESS;
	} else {
		result = err;
	}
	return 0;
}
//

//
///* dpoIContext createContext (in long target); */
v8::Handle<v8::Value> CPlatform::createContext(CContext* out)
{
	//out = new CContext();
	int result = out->InitContext(platform);
	if(result == CL_SUCCESS){
		return CContext::toV8Object(out);
	}
	return v8::Undefined();
}

v8::Handle<v8::Value> CPlatform::toV8Object(CPlatform* cPlatform){
		v8::Handle<v8::ObjectTemplate> objectTemplate = v8::ObjectTemplate::New();
		objectTemplate->SetInternalFieldCount(1);
		v8::Handle<v8::Object> cPlatformObject= objectTemplate->NewInstance();
		v8::HandleScope handle_scope;
		cPlatformObject->SetInternalField(0,v8::External::New(static_cast<CPlatform*> (cPlatform)));
		cPlatformObject->Set(v8::String::New("createContext"),
			v8::FunctionTemplate::New(V8Platform::createContext)->GetFunction());
		int numberOfDevices=0;
		cPlatform->getNumberOfDevices(numberOfDevices);
		cPlatformObject->Set(v8::String::New("numberOfDevices"),
			v8::Int32::New(numberOfDevices),v8::PropertyAttribute::ReadOnly);
		char* extension="";
		//cPlatformObject->set
		cPlatform->getPlatformPropertyHelper(CL_PLATFORM_EXTENSIONS,extension);
		cPlatformObject->Set(v8::String::New("extensions"),
			v8::String::New(extension),v8::PropertyAttribute::ReadOnly);
		char* profile="";
		cPlatform->getPlatformPropertyHelper(CL_PLATFORM_PROFILE,profile);
		cPlatformObject->Set(v8::String::New("profile"),
			v8::String::New(profile),v8::PropertyAttribute::ReadOnly);
		char* vendor="";
		cPlatform->getPlatformPropertyHelper(CL_PLATFORM_VENDOR,vendor);
		cPlatformObject->Set(v8::String::New("vendor"),
			v8::String::New(vendor),v8::PropertyAttribute::ReadOnly);
		char* version="";
		cPlatform->getPlatformPropertyHelper(CL_PLATFORM_VERSION,version);
		cPlatformObject->Set(v8::String::New("version"),
			v8::String::New(version),v8::PropertyAttribute::ReadOnly);
		char* name="";
		cPlatform->getPlatformPropertyHelper(CL_PLATFORM_NAME,name);
		cPlatformObject->Set(v8::String::New("name"),
			v8::String::New(name),v8::PropertyAttribute::ReadOnly);
		return handle_scope.Close(cPlatformObject);
	//return v8::Undefined();
	}