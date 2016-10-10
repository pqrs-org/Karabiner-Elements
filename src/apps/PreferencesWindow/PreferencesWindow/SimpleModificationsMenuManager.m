#import "SimpleModificationsMenuManager.h"

@interface SimpleModificationsMenuManager ()

@property(readwrite) NSMenu* fromMenu;
@property(readwrite) NSMenu* toMenu;

@end

@implementation SimpleModificationsMenuManager

- (void)buildMenu:(NSArray*)array {
  [array enumerateObjectsUsingBlock:^(id obj, NSUInteger index, BOOL* stop) {
    NSString* category = obj[@"category"];
    NSArray* children = obj[@"children"];
    NSString* name = obj[@"name"];

    if (category && children) {
      [self.fromMenu addItem:[NSMenuItem separatorItem]];
      [self.toMenu addItem:[NSMenuItem separatorItem]];

      {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:category
                                                      action:NULL
                                               keyEquivalent:@""];
        [self.fromMenu addItem:item];
        item.enabled = NO;
      }
      {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:category
                                                      action:NULL
                                               keyEquivalent:@""];
        [self.toMenu addItem:item];
        item.enabled = NO;
      }

      [self buildMenu:children];

    } else if (name) {
      NSString* label = obj[@"label"];
      if (!label) {
        label = name;
      }
      label = [NSString stringWithFormat:@"  %@", label];

      {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                      action:NULL
                                               keyEquivalent:@""];
        [self.fromMenu addItem:item];
      }
      {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                      action:NULL
                                               keyEquivalent:@""];
        [self.toMenu addItem:item];
      }
    }
  }];
}

- (void)setup {
  NSString* jsonFilePath = [[NSBundle mainBundle] pathForResource:@"simple_modifications" ofType:@"json"];
  if (!jsonFilePath) {
    NSLog(@"simple_modifications.json is not found.");
    return;
  }

  NSInputStream* stream = [NSInputStream inputStreamWithFileAtPath:jsonFilePath];
  [stream open];
  NSError* error = nil;
  NSArray* jsonObject = [NSJSONSerialization JSONObjectWithStream:stream
                                                          options:0
                                                            error:&error];
  [stream close];

  if (error) {
    NSLog(@"JSONObjectWithStream error @ loadJsonFile: %@", error);
  }

  self.fromMenu = [NSMenu new];
  self.fromMenu.autoenablesItems = NO;
  self.toMenu = [NSMenu new];
  self.toMenu.autoenablesItems = NO;

  [self buildMenu:jsonObject];
}

@end
