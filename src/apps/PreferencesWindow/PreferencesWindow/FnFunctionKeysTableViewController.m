#import "FnFunctionKeysTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTableViewController.h"
#import "weakify.h"

@interface FnFunctionKeysTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;

@end

@implementation FnFunctionKeysTableViewController

- (void)setup {
  [[NSNotificationCenter defaultCenter] addObserverForName:kConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
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

    NSString* fromValue = fromCellView.textField.stringValue;

    // If toCellView is not selected, set fromCellView value to toCellView.
    NSString* toValue = toCellView.popUpButton.selectedItem.representedObject;
    if (!toValue || [toValue isEqualToString:@""]) {
      [SimpleModificationsTableViewController selectPopUpButtonMenu:toCellView.popUpButton representedObject:fromValue];
      toValue = toCellView.popUpButton.selectedItem.representedObject;
    }

    [configurationManager.coreConfigurationModel replaceFnFunctionKey:fromValue to:toValue];
  }

  [configurationManager save];
}

@end
