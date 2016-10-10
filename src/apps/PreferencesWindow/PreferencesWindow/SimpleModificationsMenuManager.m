#import "SimpleModificationsMenuManager.h"
#import "JsonUtility.h"

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

      if (!obj[@"not_from"]) {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                      action:NULL
                                               keyEquivalent:@""];
        item.representedObject = name;
        [self.fromMenu addItem:item];
      }
      if (!obj[@"not_to"]) {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                      action:NULL
                                               keyEquivalent:@""];
        item.representedObject = name;
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

  NSArray* jsonObject = [JsonUtility loadFile:jsonFilePath];
  if (jsonObject) {
    self.fromMenu = [NSMenu new];
    self.fromMenu.autoenablesItems = NO;
    self.toMenu = [NSMenu new];
    self.toMenu.autoenablesItems = NO;

    [self buildMenu:jsonObject];
  }
}

@end
