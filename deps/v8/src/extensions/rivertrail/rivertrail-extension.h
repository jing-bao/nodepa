#ifndef V8_EXTENSIONS_RIVERTRAIL_EXTENSION_H_
#define V8_EXTENSIONS_RIVERTRAIL_EXTENSION_H_

#include "v8.h"

namespace v8 {
namespace internal {

class RiverTrailExtension : public v8::Extension {
 public:
  RiverTrailExtension() : v8::Extension("v8/rivertrail", kSource) {}
  virtual v8::Handle<v8::FunctionTemplate> GetNativeFunction(
      v8::Handle<v8::String> name);
  static void Register();
 private:
  static const char* const kSource;
};

} }  // namespace v8::internal

#endif  // V8_EXTENSIONS_RIVERTRAIL_EXTENSION_H_
