#import "JsonUtility.h"
#include "libkrbn.h"

@implementation JsonUtility

+ (id)loadCString:(const char*)string {
  if (!string) {
    NSLog(@"string is null @ [JsonUtility loadCString]");
    return nil;
  }

  // Do not include the last '\0' to data. (set length == strlen)
  NSData* data = [NSData dataWithBytesNoCopy:(void*)(string)
                                      length:strlen(string)
                                freeWhenDone:NO];

  NSError* error = nil;
  NSDictionary* jsonObject = [NSJSONSerialization JSONObjectWithData:data
                                                             options:0
                                                               error:&error];
  if (error) {
    NSLog(@"JSONObjectWithData error @ [JsonUtility loadCString]: %@", error);
    return nil;
  }

  return jsonObject;
}

+ (id)loadFile:(NSString*)filePath {
  NSInputStream* stream = [NSInputStream inputStreamWithFileAtPath:filePath];
  [stream open];
  NSError* error = nil;
  NSArray* jsonObject = [NSJSONSerialization JSONObjectWithStream:stream
                                                          options:0
                                                            error:&error];
  if (error) {
    NSLog(@"JSONObjectWithStream error @ [JsonUtility loadFile]: %@", error);
    return nil;
  }

  [stream close];

  return jsonObject;
}

+ (void)saveJsonToFile:(id)json filePath:(NSString*)filePath {
  NSOutputStream* stream = [NSOutputStream outputStreamToMemory];
  [stream open];
  NSError* error = nil;
  [NSJSONSerialization writeJSONObject:json
                              toStream:stream
                               options:NSJSONWritingPrettyPrinted
                                 error:&error];
  if (error) {
    NSLog(@"writeJSONObject error @ [JsonUtility saveJsonToFile]: %@", error);
    return;
  }

  NSData* data = [stream propertyForKey:NSStreamDataWrittenToMemoryStreamKey];
  [stream close];

  if (data.length > 0) {
    NSString* string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    char* allocatedJsonString = libkrbn_allocate_beautified_json_string([string UTF8String]);
    if (allocatedJsonString) {
      NSString* jsonString = [NSString stringWithUTF8String:allocatedJsonString];
      error = nil;
      [jsonString writeToFile:filePath
                   atomically:YES
                     encoding:NSUTF8StringEncoding
                        error:&error];
      if (error) {
        NSLog(@"writeToFile error @ [JsonUtility saveJsonToFile]: %@", error);
      }
      free(allocatedJsonString);
    }
  }
}

@end
