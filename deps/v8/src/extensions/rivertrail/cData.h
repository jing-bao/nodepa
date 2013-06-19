#include "config.h"
#include "opencl_compat.h"
#include "v8.h"

enum ArrayType{
	Float64Array = 0,
	Float32Array = 1,
	Uint8ClampedArray = 2
};
class CData 
{
public:
  CData();
  ~CData();
  bool InitCData( cl_command_queue aQueue, cl_mem aMemObj, int aType, int aLength, int aSize, void* anArray);
  cl_mem GetContainedBuffer();
  int GetType();
  int GetSize();
  int GetLength();
  bool getValue(v8::Handle<v8::Array> aValue);
  bool writeTo(const v8::Handle<v8::Object> & dest);
  static v8::Handle<v8::Value> toV8Object(CData* cData);
protected:
  /* additional members */
  cl_command_queue queue;
  cl_mem memObj;
  int type;
  int length;
  int size;
  void* theArray;
  bool retained;
#ifdef PREALLOCATE_IN_JS_HEAP
  bool mapped;
#endif /* PREALLOCATE_IN_JS_HEAP */
};
