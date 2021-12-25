#import "SimpleModificationsMenuManager.h"
#import "KarabinerKit/KarabinerKit.h"

@interface SimpleModificationsMenuManager ()

@property(readwrite) NSMenu* fromMenu;
@property(readwrite) NSMenu* toMenu;
@property(readwrite) NSMenu* toMenuWithInherited;

@end

@implementation SimpleModificationsMenuManager

- (void)setup {
  NSString* jsonFilePath = [[NSBundle mainBundle] pathForResource:@"simple_modifications" ofType:@"json"];
  if (!jsonFilePath) {
    NSLog(@"simple_modifications.json is not found.");
    return;
  }

  NSArray* jsonObject = [KarabinerKitJsonUtility loadFile:jsonFilePath];
  if (jsonObject) {
    self.fromMenu = [NSMenu new];
    self.fromMenu.autoenablesItems = NO;
    self.toMenu = [NSMenu new];
    self.toMenu.autoenablesItems = NO;
    self.toMenuWithInherited = [NSMenu new];
    self.toMenuWithInherited.autoenablesItems = NO;

    // ----------------------------------------

    {
      NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"---------------------------------------- (use default key)"
                                                    action:NULL
                                             keyEquivalent:@""];
      item.representedObject = [KarabinerKitJsonUtility createJsonString:@{}];
      [self.toMenuWithInherited addItem:item];
      item.enabled = YES;
      [self.toMenuWithInherited addItem:[NSMenuItem separatorItem]];
    }

    // ----------------------------------------

    for (NSDictionary* dict in jsonObject) {
      NSString* category = dict[@"category"];
      NSString* label = dict[@"label"];
      NSArray* data = dict[@"data"];

      if (category) {
        [self.fromMenu addItem:[NSMenuItem separatorItem]];
        [self.toMenu addItem:[NSMenuItem separatorItem]];
        [self.toMenuWithInherited addItem:[NSMenuItem separatorItem]];

        {
          NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:category
                                                        action:NULL
                                                 keyEquivalent:@""];
          item.enabled = NO;
          [self.fromMenu addItem:item];
        }
        {
          NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:category
                                                        action:NULL
                                                 keyEquivalent:@""];
          item.enabled = NO;
          [self.toMenu addItem:item];
          [self.toMenuWithInherited addItem:[item copy]];
        }
      } else if (label && data.count > 0) {
        label = [NSString stringWithFormat:@"  %@", label];

        if (!dict[@"not_from"]) {
          NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                        action:NULL
                                                 keyEquivalent:@""];
          item.representedObject = [KarabinerKitJsonUtility createJsonString:data[0]];
          [self.fromMenu addItem:item];
        }
        if (!dict[@"not_to"]) {
          NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                        action:NULL
                                                 keyEquivalent:@""];
          item.representedObject = [KarabinerKitJsonUtility createJsonString:data];
          [self.toMenu addItem:item];
          [self.toMenuWithInherited addItem:[item copy]];
        }
      }
    }
  }
}

@end
