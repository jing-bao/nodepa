#include "cInterface.h"
#include "cPlatform.h"
#include "config.h"
#include "v8.h"

class V8Interface{
public:
 static v8::Handle<v8::Value> getPlatform(const v8::Arguments& args){
   v8::Local<v8::Object> self = args.Holder(); 
   v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0)); 
   void *ptr = wrap->Value();
   CPlatform* cPlatform = NULL;
   return static_cast<CInterface*>(ptr)->getPlatform(cPlatform);
   //return v8::Undefined();
 }

 static v8::Handle<v8::Value> CInterfaceConstructor(const v8::Arguments& args){
   v8::Handle<v8::ObjectTemplate> objectTemplate = v8::ObjectTemplate::New();
   objectTemplate->SetInternalFieldCount(1);
   v8::Handle<v8::Object> cInterfaceObject= objectTemplate->NewInstance();
   v8::HandleScope handle_scope;
   CInterface* cInterface = CInterface::getInstance();
   cInterfaceObject->Set(v8::String::New("version"),
   v8::Integer::New(INTERFACE_VERSION),v8::PropertyAttribute::ReadOnly);
   cInterfaceObject->Set(v8::String::New("getPlatform"),
   v8::FunctionTemplate::New(getPlatform)->GetFunction());
   //cInterfaceObject->SetPrototype(v8::String::New("cinterface"));
   cInterfaceObject->SetInternalField(0,v8::External::New(static_cast<CInterface*> (cInterface)));
   //v8::Handle<v8::Array> ar = v8::Local<v8::Array>::Cast(args[0]);
   return handle_scope.Close(cInterfaceObject);
   //return v8::Undefined();
 }
};
