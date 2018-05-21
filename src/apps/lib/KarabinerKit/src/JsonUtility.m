#import "KarabinerKit/JsonUtility.h"
#include "libkrbn.h"

@implementation KarabinerKitJsonUtility

+ (id)loadString:(NSString*)string {
  if (!string) {
    NSLog(@"Error: string == nil @ [JsonUtility loadString]");
    return nil;
  }

  NSError* error = nil;
  id jsonObject = [NSJSONSerialization JSONObjectWithData:[string dataUsingEncoding:NSUTF8StringEncoding]
                                                  options:0
                                                    error:&error];

  if (error) {
    NSLog(@"JSONObjectWithData error @ [JsonUtility loadString]: %@", error);
    return nil;
  }

  return jsonObject;
}

+ (id)loadCString:(const char*)string {
  return [self loadString:[NSString stringWithUTF8String:string]];
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

+ (NSString*)createJsonString:(id)json {
  NSError* error = nil;
  NSData* data = [NSJSONSerialization dataWithJSONObject:json
                                                 options:NSJSONWritingPrettyPrinted
                                                   error:&error];
  if (error) {
    NSLog(@"[JsonUtility createJsonString] error: %@", error);
    return nil;
  }

  if (!data) {
    NSLog(@"[JsonUtility createJsonString] error: data == nil");
    return nil;
  }

  return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

+ (void)saveJsonToFile:(id)json filePath:(NSString*)filePath {
  NSString* string = [self createJsonString:json];
  if (string) {
    bool result = libkrbn_save_beautified_json_string(filePath.UTF8String, string.UTF8String);
    if (!result) {
      NSLog(@"libkrbn_save_beautified_json_string @ [JsonUtility saveJsonToFile]");
    }
  }
}

+ (NSString*)createPrettyPrintedString:(NSString*)string {
  id jsonObject = [self loadString:string];
  if (jsonObject) {
    return [self createJsonString:jsonObject];
  }
  return nil;
}

@end
