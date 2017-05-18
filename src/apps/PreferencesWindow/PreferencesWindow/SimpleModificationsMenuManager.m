#import "SimpleModificationsMenuManager.h"
#import "KarabinerKit/KarabinerKit.h"

@interface SimpleModificationsMenuManager ()

@property(readwrite) NSMenu* fromMenu;
@property(readwrite) NSMenu* toMenu;
@property(readwrite) NSMenu* vendorIdMenu;
@property(readwrite) NSMenu* productIdMenu;

@end

@implementation SimpleModificationsMenuManager

- (void) setupVendor {
  NSLog(@"In SetupVendor()");
  self.vendorIdMenu = [NSMenu new];
  self.vendorIdMenu.autoenablesItems = NO;
  self.productIdMenu = [NSMenu new];
  self.productIdMenu.autoenablesItems = NO;
  
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  NSArray *pairs = [coreConfigurationModel selectedProfileSimpleModificationVendorProductIdPairs];
  if (pairs != nil) {
    for (id id_ in pairs) {
      VendorProductIdPair *pair = (VendorProductIdPair *)id_;
      NSString *str = [pair toString];
      NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:str action:NULL keyEquivalent:@""];
      item.representedObject = pair;
      [self.vendorIdMenu addItem:item];
    }
  }
  
  NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:@"Not Specified" action:NULL keyEquivalent:@""];
  item.representedObject = [[VendorProductIdPair alloc] initWithVendorId: 0 productId: 0];
  [self.vendorIdMenu addItem:item];
}

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
  
    for (NSDictionary* dict in jsonObject) {
      NSString* category = dict[@"category"];
      NSString* name = dict[@"name"];

      if (category) {
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
      } else if (name) {
        NSString* label = dict[@"label"];
        if (!label) {
          label = name;
        }
        label = [NSString stringWithFormat:@"  %@", label];

        if (!dict[@"not_from"]) {
          NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                        action:NULL
                                                 keyEquivalent:@""];
          item.representedObject = name;
          [self.fromMenu addItem:item];
        }
        if (!dict[@"not_to"]) {
          NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label
                                                        action:NULL
                                                 keyEquivalent:@""];
          item.representedObject = name;
          [self.toMenu addItem:item];
        }
      }
    }
  }

}

@end
