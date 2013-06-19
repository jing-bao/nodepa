//
//
#include "cInterface.h"
#include "debug.h"
#include "iostream"



CInterface::CInterface()
{
  	DEBUG_LOG_CREATE("CInterface", this);
	platforms = NULL;
}

CInterface::~CInterface()
{
	DEBUG_LOG_DESTROY("CInterface", this);
	if(platforms!=NULL)
		platforms = NULL;
}
//
int CInterface::InitPlatformInfo()
{
	cl_int err_code;


	cl_uint nplatforms;
	
	err_code = clGetPlatformIDs( 0, NULL, &nplatforms);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR( "InitPlatformInfo", err_code);
		return err_code;
	}

	platforms = new cl_platform_id[nplatforms];

	err_code = clGetPlatformIDs( nplatforms, platforms, &noOfPlatforms);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR( "InitPlatformInfo", err_code);
		return err_code;
	}

	return err_code;

}


//
///* IPlatform getPlatform (); */
//
v8::Handle<v8::Value> CInterface::getPlatform(CPlatform* out)
{
	int result = CL_SUCCESS;
	cl_int err_code;
	const cl_uint maxNameLength = 256;
	char name[maxNameLength];
	
	if (platforms == NULL) {
		result = InitPlatformInfo();
	}

	if (result != CL_SUCCESS) 
		return v8::Undefined();
		
	for (cl_uint i = 0; i < noOfPlatforms; i++) {
		err_code = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, maxNameLength*sizeof(char), name, NULL);
		if (err_code != CL_SUCCESS) {
			DEBUG_LOG_ERROR( "GetIntelPlatform", err_code);
			return v8::Undefined();
		}
		if ((strcmp(name, "Intel(R) OpenCL") == 0) || (strcmp(name, "Apple") == 0)) {
			out = new CPlatform( platforms[i]);		
			if (out == NULL) {
				return v8::Undefined();
			} else {
				return CPlatform::toV8Object(out);
			}
		}
	}
	return  v8::Undefined();
}
////
//
//
//#ifdef DPO_SCOPE_TRIAL
///* [implicit_jscontext] jsval searchScope (in jsval scope, in AString name); */
//NS_IMETHODIMP dpoCInterface::SearchScope(const jsval & scope, const nsAString & name, JSContext *cx, jsval *_retval NS_OUTPARAM)
//{
//	JSObject *scopeObj, *parentObj;
//	JSBool result;
//	char *propName;
//
//	if (!JSVAL_IS_OBJECT(scope)) {
//		*_retval = JSVAL_VOID;
//		return NS_ERROR_ILLEGAL_VALUE;
//	}
//
//	scopeObj = JSVAL_TO_OBJECT(scope);
//	parentObj = JS_GetParent(cx, scopeObj);
//
//	if (parentObj == NULL) {
//		*_retval = JSVAL_VOID;
//		return NS_ERROR_NOT_AVAILABLE;
//	}
//
//	*_retval = OBJECT_TO_JSVAL(parentObj);
//	return NS_OK;
//
//	propName = ToNewUTF8String(name);
//	result = JS_LookupPropertyWithFlags(cx, parentObj, propName, 0, _retval);
//	nsMemory::Free(propName);
//
//	if (result == JS_FALSE) {
//		*_retval = JSVAL_VOID;
//		return NS_ERROR_NOT_AVAILABLE;
//	}
//	
//    return NS_OK;
//}
//#endif /* DPO_SCOPE_TRIAL */
//

CInterface* CInterface::singleton = NULL;

CInterface *CInterface::getInstance()
{
	CInterface *result;

	if (singleton == NULL) {
		singleton = new CInterface();
	}

	result = singleton;

	return result;
}