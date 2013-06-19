
#include "config.h"
#include "opencl_compat.h"
#include "cPlatform.h"

#ifndef CINTERFACE_H
#define CINTERFACE_H
class CInterface 
{
public:

	v8::Handle<v8::Value> getPlatform(CPlatform* out);
	static CInterface *getInstance();
	CInterface();
	~CInterface();

protected:
  static  CInterface* singleton;
  cl_platform_id *platforms;
  cl_uint noOfPlatforms;
  int InitPlatformInfo();
};
#endif