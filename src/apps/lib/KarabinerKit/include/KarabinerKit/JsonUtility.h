// -*- Mode: objc -*-

@import Cocoa;

@interface KarabinerKitJsonUtility : NSObject

+ (id)loadString:(NSString*)string;
+ (id)loadCString:(const char*)string;
+ (id)loadFile:(NSString*)filePath;

+ (NSString*)createJsonString:(id)json;
+ (void)saveJsonToFile:(id)json
               filePath:(NSString*)filePath
    parentDirectoryMode:(mode_t)parentDirectoryMode
               fileMode:(mode_t)fileMode;

+ (NSString*)createPrettyPrintedString:(NSString*)string;

@end
