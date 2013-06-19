#include "v8.h"
#include "cKernel.h"
class V8Kernel{
public:
	static v8::Handle<v8::Value> run( const v8::Arguments& args)
	{
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		unsigned int* retval = new unsigned int[1];
		unsigned int rank = args[0]->Uint32Value();
		v8::Handle<v8::Array> ashape = v8::Handle<v8::Array>::Cast(args[1]->ToObject());
		v8::Handle<v8::Array> atile = v8::Handle<v8::Array>::Cast(args[1]->ToObject());
		unsigned int* shape = new unsigned int[ashape->Length()];
		for(int i=0; i<ashape->Length(); i++)
			shape[i] = ashape->Get(i)->Int32Value();
		
		unsigned int* tile = new unsigned int[atile->Length()];
		for(int i=0; i<atile->Length(); i++)
			tile[i] = atile->Get(i)->Int32Value();

		if(static_cast<CKernel*>(ptr)->Run(rank,shape,tile,retval))
			return v8::Uint32::New(*retval);
		else
			return v8::ThrowException(v8::Exception::Error(
				v8::String::New("run kernel failed")));
	}
	static v8::Handle<v8::Value> setScalarArgument( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		unsigned int number = args[0]->Uint32Value();
		v8::Handle<v8::Value> argument = args[1];
		v8::Handle<v8::Value> isInteger = args[2];
		v8::Handle<v8::Value> highPrecision = args[3];
		static_cast<CKernel*>(ptr)->SetScalarArgument(number,argument,isInteger,highPrecision);
		return v8::Undefined();
	}
	static v8::Handle<v8::Value> numberOfArgsGetter( v8::Local<v8::String> name,
                               const v8::AccessorInfo& info){
		v8::Local<v8::Object> self = info.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		unsigned int numberOfArgs=0;
		static_cast<CKernel*>(ptr)->GetNumberOfArgs(numberOfArgs);
		return v8::Uint32::New(numberOfArgs);
	}

	static v8::Handle<v8::Value> setArgument( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		unsigned int number = args[0]->Uint32Value();
		v8::Local<v8::Object> dataObject = args[1]->ToObject();
		v8::Local<v8::External> wrapdata = v8::Local<v8::External>::Cast(dataObject->GetInternalField(0));
		void *pData = wrapdata->Value();
		static_cast<CKernel*>(ptr)->SetArgument(number,static_cast<CData*>(pData));
		return v8::Undefined();
	}

};