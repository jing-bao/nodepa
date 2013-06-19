

#include "config.h"
//#pragma once
//#include "v8\include\v8.h"
#include "opencl_compat.h"
#include "cContext.h"
#ifndef CPLATFORM_H
#define CPLATFORM_H
class CPlatform
{
public:
	CPlatform(){};
    CPlatform(cl_platform_id platform);
    ~CPlatform();
	v8::Handle<v8::Value> createContext(CContext* out);
	static v8::Handle<v8::Value> toV8Object(CPlatform* cPlatform);
	bool getPlatformPropertyHelper( cl_platform_info param, char* & out);
	bool getNumberOfDevices(int out);
protected:
	static cl_platform_id platform;
	
	/* additional members */
	
	int numberOfDevices;
};

#endif