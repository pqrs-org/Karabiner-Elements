// -*- mode: objective-c -*-

@import Cocoa;

@interface KarabinerKitJsonUtility : NSObject

+ (id)loadString:(NSString*)string;
+ (id)loadCString:(const char*)string;
+ (id)loadFile:(NSString*)filePath;

+ (NSString*)createJsonString:(id)json
                      options:(NSJSONWritingOptions)options;
+ (NSString*)createJsonString:(id)json;
+ (NSString*)createPrettyPrintedString:(NSString*)string;
+ (NSString*)createCompactJsonString:(NSString*)string;

@end
