#include "v8.h"
#include "cData.h"

class V8Data{
public:
	static v8::Handle<v8::Value> getValue( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		v8::Handle<v8::Array> retval = v8::Array::New(static_cast<CData*>(ptr)->GetLength());
		
		if(static_cast<CData*>(ptr)->getValue(retval)){
			//retval->SetPrototype(v8::String::New("#<Float64Array>"));
			return retval;
		}
		else
			return v8::ThrowException(v8::Exception::Error(
				v8::String::New("get value failed")));
	}
	static v8::Handle<v8::Value> writeTo( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		v8::Handle<v8::Object> dest=args[0]->ToObject();
		static_cast<CData*>(ptr)->writeTo(dest);
		return v8::Undefined();
	} 
};