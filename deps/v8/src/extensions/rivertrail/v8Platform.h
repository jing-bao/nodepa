#include "cPlatform.h"
#ifndef V8PLATFORM_H
#define V8PLATFORM_H

class V8Platform{
public:
 static v8::Handle<v8::Value> createContext(const v8::Arguments& args){
   v8::Local<v8::Object> self = args.Holder(); 
   v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
   void *ptr = wrap->Value();
   CContext* cContext = new CContext();
   return static_cast<CPlatform*>(ptr)->createContext(cContext);
   //return v8::Undefined();
}

static v8::Handle<v8::Value>CPlatformConstructor(const v8::Arguments& args){
  /*v8::Handle<v8::ObjectTemplate> objectTemplate = v8::ObjectTemplate::New();
  objectTemplate->SetInternalFieldCount(1);
  v8::Handle<v8::Object> cPlatformObject= objectTemplate->NewInstance();
  v8::HandleScope handle_scope;
  CInterface* cInterface = CInterface::getInstance();*/
  CPlatform* cPlatform = new CPlatform();
  return CPlatform::toV8Object(cPlatform);
}

};
#endif