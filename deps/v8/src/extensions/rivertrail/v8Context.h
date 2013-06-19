
#include "cKernel.h"

#ifndef V8CONTEXT_H
#define V8CONTEXT_H
class V8Context{
public:
	static v8::Handle<v8::Value> compileKernel(const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		v8::String::Utf8Value v8source(args[0]);
		v8::String::Utf8Value v8kernelName(args[1]);
		const char* options;
		if(!args[2]->IsUndefined()){
			v8::String::Utf8Value v8options(args[2]);
			options = *v8options ? *v8options : "<string conversion failed>";
		}
		else
			options = NULL;
		const char* source = *v8source ? *v8source : "<string conversion failed>";
		const char* kernelName = *v8kernelName ? *v8kernelName : "<string conversion failed>";
		CKernel* cKernel = new CKernel();
		if(static_cast<CContext*>(ptr)->compileKernel(source,kernelName,options,cKernel))
			return CKernel::toV8Object(cKernel);
		else
			return v8::ThrowException(v8::Exception::Error(
				v8::String::New("compiler kernel failed")));
	}

    // Functions for mapping data.
	static v8::Handle<v8::Value> mapData( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		v8::Handle<v8::Object> source = args[0]->ToObject();
		
		CData* retval = new CData();
		if(static_cast<CContext*>(ptr)->MapData(source,retval)){
			return CData::toV8Object(retval);
		}
		return v8::ThrowException(v8::Exception::Error(
				v8::String::New("map data failed")));

	}
	
	static v8::Handle<v8::Value> cloneData( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		v8::Handle<v8::Value> source = args[0];
		CData* retval = new CData();
		if(static_cast<CContext*>(ptr)->CloneData(source,retval)){
			return CData::toV8Object(retval);
		}
		return v8::ThrowException(v8::Exception::Error(
				v8::String::New("clone data failed")));
	}
	
	static v8::Handle<v8::Value> allocateData( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		v8::Handle<v8::Object> templ = args[0]->ToObject();
		unsigned int length = args[1]->Uint32Value();
		CData* retval = new CData();
		if(static_cast<CContext*>(ptr)->AllocateData(templ, length, retval)){
			return CData::toV8Object(retval);
		}
		return v8::ThrowException(v8::Exception::Error(
				v8::String::New("allocate data failed")));
	}
	
	static v8::Handle<v8::Value> allocateData2( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		v8::Local<v8::Object> dataSelf = args[1]->ToObject();
		v8::Local<v8::External> dataWrap = v8::Local<v8::External>::Cast(dataSelf->GetInternalField(0));
		void *dptr = dataWrap->Value();
		CData* templ = static_cast<CData*>(dptr);
		unsigned int length = args[1]->Uint32Value();
		CData* retval = new CData();
		if(static_cast<CContext*>(ptr)->AllocateData2(templ,length, retval)){
			return CData::toV8Object(retval);
		}
		return v8::Undefined();
	}
  
	static v8::Handle<v8::Value> canBeMapped( const v8::Arguments& args){
		v8::Local<v8::Object> self = args.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		v8::Handle<v8::Value> source = args[0];
		bool retval=false;
		static_cast<CContext*>(ptr)->CanBeMapped(source,retval);
		return v8::Boolean::New(retval);
	}

	static v8::Handle<v8::Value> buildLogGetter( v8::Local<v8::String> name,
                               const v8::AccessorInfo& info){								   
		v8::Local<v8::Object> self = info.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		char* buildLog = "";
		static_cast<CContext*>(ptr)->GetBuildLog(buildLog);
		return v8::String::New(buildLog);		
	}

	static v8::Handle<v8::Value> lastExecutionTimeGetter( v8::Local<v8::String> name,
                               const v8::AccessorInfo& info){				   
		v8::Local<v8::Object> self = info.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		unsigned long* lastExecutionTime = new unsigned long[1];
		*lastExecutionTime = 0;
		static_cast<CContext*>(ptr)->GetLastExecutionTime(lastExecutionTime);
		return v8::Uint32::New(*lastExecutionTime);
	}

	static v8::Handle<v8::Value> lastRoundTripTimeGetter( v8::Local<v8::String> name,
                               const v8::AccessorInfo& info){
		v8::Local<v8::Object> self = info.Holder(); 
		v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
		void *ptr = wrap->Value();
		unsigned long* lastRoundTripTime = new unsigned long[1];
		*lastRoundTripTime = 0;
		static_cast<CContext*>(ptr)->GetLastRoundTripTime(lastRoundTripTime);
		return v8::Uint32::New(*lastRoundTripTime);
		
	}
};
#endif