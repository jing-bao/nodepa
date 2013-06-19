#include "rivertrail-extension.h"
#include "v8Interface.h"

namespace v8 {
namespace internal {

const char* const RiverTrailExtension::kSource = "native function CInterface();";


v8::Handle<v8::FunctionTemplate> RiverTrailExtension::GetNativeFunction(
    v8::Handle<v8::String> str) {
  ASSERT(strcmp(*v8::String::AsciiValue(str), "CInterface") == 0);
  return v8::FunctionTemplate::New(V8Interface::CInterfaceConstructor);
}


void RiverTrailExtension::Register() {
  static RiverTrailExtension rivertrail_extension;
  static v8::DeclareExtension declaration(&rivertrail_extension);
}

} }  // namespace v8::internal
