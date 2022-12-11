// (C) Copyright Takayama Fumihiko 2022.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#import <Cocoa/Cocoa.h>
#import <pqrs/osx/workspace/impl/objc.h>

#if !__has_feature(objc_arc)
#error "You have to add -fobjc-arc into compile options"
#endif

char* pqrs_osx_workspace_find_application_url_by_bundle_identifier(const char* bundle_identifier) {
  if (bundle_identifier == nil) {
    return nil;
  }

  NSURL* url = [NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:[NSString stringWithUTF8String:bundle_identifier]];
  if (url == nil) {
    return nil;
  }

  const char* string = url.absoluteString.UTF8String;
  const size_t len = strlen(string);
  char* result = malloc(len + 1);
  snprintf(result, len + 1, "%s", string);
  return result;
}
