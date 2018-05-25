// -*- Mode: objc -*-

@import Cocoa;

@interface KarabinerKitJsonUtility : NSObject

+ (id)loadString:(NSString*)string;
+ (id)loadCString:(const char*)string;
+ (id)loadFile:(NSString*)filePath;

+ (NSString*)createJsonString:(id)json;
+ (NSString*)createPrettyPrintedString:(NSString*)string;

@end
