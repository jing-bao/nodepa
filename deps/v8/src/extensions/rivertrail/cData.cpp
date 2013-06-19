
//
#include "debug.h"
#include "v8Data.h"
#include "debug.h"

CData::CData() 
{
	DEBUG_LOG_CREATE("CData", this);
	queue = NULL;
	memObj = NULL;
	theArray = NULL;
	//theContext = NULL;
#ifdef PREALLOCATE_IN_JS_HEAP
	mapped = false;
#endif /* PREALLOCATE_IN_JS_HEAP */
}
//
CData::~CData()
{
	DEBUG_LOG_DESTROY("CData", this);
	if (memObj != NULL) {
		clReleaseMemObject( memObj);
	}
	if ((queue != NULL) && retained) {
        DEBUG_LOG_STATUS("~CData", "releasing queue object");
		clReleaseCommandQueue( queue);
	}
	if (theArray) {
        DEBUG_LOG_STATUS("~CData", "releasing array object");
		theArray = NULL;
    }
}
//
bool CData::InitCData( cl_command_queue aQueue, cl_mem aMemObj, int aType, int aLength, int aSize, void* anArray)
{
	cl_int err_code;

	type = aType;
	length = aLength;
	size = aSize;
	memObj = aMemObj;
	//theContext = cx;
	/*void* a = new int[9];
	int* b = new int[9];
	b=(int*)a;*/
	if (anArray) {
		// tell the JS runtime that we hold this typed array
		switch (type){
		case Float64Array:
			{
				theArray = new double[length];
				memcpy(theArray,anArray,size);
				break;
			}
		case Float32Array:
			{
				theArray = new float[length];
				memcpy(theArray,anArray,size);
				break;
			}
		case Uint8ClampedArray:
			{
				theArray = new unsigned char[length];
				memcpy(theArray,anArray,size);
				break;
			}
		default:
			{
				theArray = NULL;
				break;
			}
		}
	} else {
		theArray = NULL;
	}

	DEBUG_LOG_STATUS("InitCData", "queue is " << aQueue << " buffer is " << aMemObj);

	err_code = clRetainCommandQueue( queue);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("initCData", err_code);
		// SAH: we should really fail here but a bug in the whatif OpenCL 
		//      makes the above retain operation always fail :-(
		retained = false;
	} else {
		retained = true;
	}
	queue = aQueue;

	return true;
}

v8::Handle<v8::Value> CData::toV8Object(CData* cData){
	v8::Handle<v8::ObjectTemplate> objectTemplate = v8::ObjectTemplate::New();
	objectTemplate->SetInternalFieldCount(1);
	v8::Handle<v8::Object> cDataObject= objectTemplate->NewInstance();
	v8::HandleScope handle_scope;
	cDataObject->SetInternalField(0,v8::External::New(static_cast<CData*> (cData)));
	cDataObject->Set(v8::String::New("getValue"),v8::FunctionTemplate::New(V8Data::getValue)->GetFunction());
	cDataObject->Set(v8::String::New("writeTo"),v8::FunctionTemplate::New(V8Data::writeTo)->GetFunction());
	cDataObject->Set(v8::String::New("dpoIData"),v8::String::New("dpoIData"));
	cDataObject->Set(v8::String::New("elementType"),v8::Int32::New(cData->GetType()));
	return handle_scope.Close(cDataObject);
}
//
/* [implicit_jscontext] readonly attribute jsval value; */
bool CData::getValue( v8::Handle<v8::Array> aValue)
{
	cl_int err_code;
#ifdef PREALLOCATE_IN_JS_HEAP
	void *mem;
#endif /* PREALLOCATE_IN_JS_HEAP */

	if (theArray) {
#ifdef PREALLOCATE_IN_JS_HEAP
		if (!mapped) {
			DEBUG_LOG_STATUS("GetValue", "memory is " << theArray);
			void *mem = clEnqueueMapBuffer(queue, memObj, CL_TRUE, CL_MAP_READ, 0, size, 0, NULL, NULL, &err_code);

			if (err_code != CL_SUCCESS) {
				DEBUG_LOG_ERROR("GetValue", err_code);
				return NS_ERROR_NOT_AVAILABLE;
			}
#ifndef DEBUG_OFF
			if (!js_IsTypedArray(theArray)) {
				DEBUG_LOG_STATUS("GetValue", "Cannot access typed array");
				return NS_ERROR_NOT_AVAILABLE;
			}

			if (mem != JS_GetTypedArrayData(theArray)) {
				DEBUG_LOG_STATUS("GetValue", "EnqueueMap returned wrong pointer");
			}
#endif /* DEBUG_OFF */
			mapped = true;
		}
#endif /* PREALLOCATE_IN_JS_HEAP */
		//v8::Handle<v8::Array> arrayObject;
		//arrayObject = v8::Array::New(length);
		switch(type){
		case Float64Array:
			{
				double*temp = new double[length];
				temp=(double*)theArray;
				for(int i=0; i<length; i++)
					aValue->Set(v8::Int32::New(i),v8::Number::New(temp[i]));
				delete temp;
				break;
			}
		case Float32Array:
			{
				float*temp = new float[length];
				temp=(float*)theArray;
				for(int i=0; i<length; i++)
					aValue->Set(v8::Int32::New(i),v8::Number::New(temp[i]));
				delete temp;
				break;
			}
		case Uint8ClampedArray:
			{
				unsigned char* temp = new unsigned char[length];
				temp=(unsigned char*)theArray;
				for(int i=0; i<length; i++)
					aValue->Set(v8::Int32::New(i),v8::Uint32::New(temp[i]));
				delete temp;
			}
		default:
			return false;
		}
		return true;
	} else {
        	// tell the runtime that we cache this array object
		//HOLD_JS_OBJECTS(this, CData);
		
		switch (type) {
		case Float64Array:
			{
				double* doubleA = new double[length];
				err_code = clEnqueueReadBuffer(queue, memObj, CL_TRUE, 0, size, doubleA, 0, NULL, NULL);
				if (err_code != CL_SUCCESS) {
					DEBUG_LOG_ERROR("GetValue", err_code);
					delete doubleA;
					return false;
				}
				for(int i=0; i<length; i++){
					double b = doubleA[i];
					aValue->Set(v8::Int32::New(i),v8::Number::New(doubleA[i]));
				}
				delete doubleA;
				break;
			}
		case Float32Array:
			{
				float* floatA = new float[length];			
				err_code = clEnqueueReadBuffer(queue, memObj, CL_TRUE, 0, size, floatA, 0, NULL, NULL);
				if (err_code != CL_SUCCESS) {
					DEBUG_LOG_ERROR("GetValue", err_code);
					delete floatA;
					return false;
				}			
				for(int i=0; i<length; i++)
					aValue->Set(v8::Int32::New(i),v8::Number::New(floatA[i]));
				delete floatA;
				break;
			}
		default:
			// we only return float our double arrays, so fail in all other cases 
			return false;
		}	
	
		DEBUG_LOG_STATUS("GetValue", "materialized typed array");
		
		return true;
	}
}

/* [implicit_jscontext] void writeTo (in jsval dest); */
bool CData::writeTo(const v8::Handle<v8::Object>&  dest)
{
	v8::Handle<v8::Array> destArray;
	cl_int err_code;
	
	if (!dest->IsArray()) {
		return false;
	}
	destArray = v8::Local<v8::Array>::Cast(dest->ToObject());
	
	if (!destArray->IsArray()) {
		return false;
	}
	int destType;
	void* temp;
	if(destArray->Get(v8::Integer::New(0))->IsNumber()){
		destType = 1;
		temp = new double[length];
	}
	else if(destArray->Get(v8::Integer::New(0))->IsInt32() || 
		destArray->Get(v8::Integer::New(0))->IsUint32()){
		destType = 0;
		temp = new int[length];
	}
	else{
		return false;
	}
	if ((destType != type) || (destArray->Length() != length)) {
		delete temp;
		return false;
	}
	
	err_code = clEnqueueReadBuffer(queue, memObj, CL_TRUE, 0, size, temp, 0, NULL, NULL);
	if (err_code != CL_SUCCESS) {
		DEBUG_LOG_ERROR("WriteTo", err_code);
		delete temp;
		return false;
	}
	delete temp;
    return true;
}

cl_mem CData::GetContainedBuffer()
{
	return memObj;
}

int CData::GetSize()
{
	return size;
}

int CData::GetType()
{
	return type;
}

int CData::GetLength()
{
	return length;
}
