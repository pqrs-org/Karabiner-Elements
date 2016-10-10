#import "SimpleModificationsMenuManager.h"

@interface SimpleModificationsMenuManager ()

@property(readwrite) NSMenu* fromMenu;
@property(readwrite) NSMenu* toMenu;

@end

@implementation SimpleModificationsMenuManager

- (void)buildMenu:(NSArray*)array fromMenu:(NSMenu*)fromMenu toMenu:(NSMenu*)toMenu {
  [array enumerateObjectsUsingBlock:^(id obj, NSUInteger index, BOOL* stop) {
    NSString* category = obj[@"category"];
    NSArray* children = obj[@"children"];
    NSString* name = obj[@"name"];

    if (category && children) {
      NSMenu* fromSubmenu = [NSMenu new];
      NSMenu* toSubmenu = [NSMenu new];

      [self buildMenu:children fromMenu:fromSubmenu toMenu:toSubmenu];

      {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:category
                                                      action:NULL
                                               keyEquivalent:@""];
        item.submenu = fromSubmenu;
        [fromMenu addItem:item];
      }
      {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:category
                                                      action:NULL
                                               keyEquivalent:@""];
        item.submenu = toSubmenu;
        [toMenu addItem:item];
      }

    } else if (name) {
      {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:name
                                                      action:NULL
                                               keyEquivalent:@""];
        [fromMenu addItem:item];
      }
      {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:name
                                                      action:NULL
                                               keyEquivalent:@""];
        [toMenu addItem:item];
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
  self.toMenu = [NSMenu new];

  [self buildMenu:jsonObject fromMenu:self.fromMenu toMenu:self.toMenu];
}

@end
