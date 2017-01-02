// -*- Mode: objc -*-

@import Cocoa;

@interface KarabinerKitJsonUtility : NSObject

+ (id)loadCString:(const char*)string;
+ (id)loadFile:(NSString*)filePath;

+ (void)saveJsonToFile:(id)json filePath:(NSString*)filePath;

@end
