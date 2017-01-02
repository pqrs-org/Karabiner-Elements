#import "SimpleModificationsTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "SimpleModificationsTableCellView.h"
#import "weakify.h"

@interface SimpleModificationsTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;

@end

@implementation SimpleModificationsTableViewController

+ (void)selectPopUpButtonMenu:(NSPopUpButton*)popUpButton representedObject:(NSString*)representedObject {
  NSArray* items = popUpButton.itemArray;
  if (items) {
    for (NSMenuItem* item in items) {
      if ([item.representedObject isEqualToString:representedObject]) {
        [popUpButton selectItem:item];
        return;
      }
    }
  }
  [popUpButton selectItem:nil];
}

- (void)setup {
  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  NSLog(@"configuration is loaded");
                                                  [self.tableView reloadData];
                                                }];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)valueChanged:(id)sender {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];

  NSInteger rows = [self.tableView numberOfRows];
  for (NSInteger i = 0; i < rows; ++i) {
    SimpleModificationsTableCellView* fromCellView = [self.tableView viewAtColumn:0 row:i makeIfNecessary:NO];
    SimpleModificationsTableCellView* toCellView = [self.tableView viewAtColumn:1 row:i makeIfNecessary:NO];

    NSString* fromValue = fromCellView.popUpButton.selectedItem.representedObject;
    if (fromValue && ![fromValue isEqualToString:@""]) {
      // If toCellView is not selected, set fromCellView value to toCellView.
      NSString* toValue = toCellView.popUpButton.selectedItem.representedObject;
      if (!toValue || [toValue isEqualToString:@""]) {
        [SimpleModificationsTableViewController selectPopUpButtonMenu:toCellView.popUpButton representedObject:fromValue];
        toValue = toCellView.popUpButton.selectedItem.representedObject;
      }
      toCellView.popUpButton.enabled = YES;

      [configurationManager.coreConfigurationModel replaceSimpleModification:(NSUInteger)(i) from:fromValue to:toValue];
    }
  }

  [configurationManager save];
}

- (void)removeItem:(id)sender {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];

  NSInteger row = [self.tableView rowForView:sender];
  [configurationManager.coreConfigurationModel removeSimpleModification:(NSUInteger)(row)];
  [self.tableView reloadData];

  [configurationManager save];
}

- (IBAction)addItem:(id)sender {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];

  [configurationManager.coreConfigurationModel addSimpleModification];
  [self.tableView reloadData];

  [configurationManager save];
}

@end
