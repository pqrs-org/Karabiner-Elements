#import "SimpleModificationsTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTargetDeviceMenuManager.h"
#import "weakify.h"

@interface SimpleModificationsTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;
@property(weak) IBOutlet NSPopUpButton* connectedDevicesPopupButton;

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
                                                  [self.tableView reloadData];
                                                  [self updateConnectedDevicesMenu];
                                                }];

  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitDevicesAreUpdated
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  [self updateConnectedDevicesMenu];
                                                }];

  [self updateConnectedDevicesMenu];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)valueChanged:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  SimpleModificationsTableCellView* fromCellView = [self.tableView viewAtColumn:0 row:row makeIfNecessary:NO];
  SimpleModificationsTableCellView* toCellView = [self.tableView viewAtColumn:1 row:row makeIfNecessary:NO];

  NSString* fromValue = fromCellView.popUpButton.selectedItem.representedObject;
  if (fromValue && ![fromValue isEqualToString:@""]) {
    // If toCellView is not selected, set fromCellView value to toCellView.
    NSString* toValue = toCellView.popUpButton.selectedItem.representedObject;
    if (!toValue || [toValue isEqualToString:@""]) {
      [SimpleModificationsTableViewController selectPopUpButtonMenu:toCellView.popUpButton representedObject:fromValue];
      toValue = toCellView.popUpButton.selectedItem.representedObject;
    }
    toCellView.popUpButton.enabled = YES;

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    [coreConfigurationModel setSelectedProfileSimpleModificationAtIndex:row
                                                                   from:fromValue
                                                                     to:toValue
                                                   connectedDeviceIndex:self.selectedConnectedDeviceIndex];
    [coreConfigurationModel save];
  }
}

- (void)removeItem:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel removeSelectedProfileSimpleModificationAtIndex:row
                                                    connectedDeviceIndex:self.selectedConnectedDeviceIndex];
  [coreConfigurationModel save];

  [self.tableView reloadData];
}

- (IBAction)addItem:(id)sender {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel addSimpleModificationToSelectedProfile:self.selectedConnectedDeviceIndex];

  [self.tableView reloadData];
}

- (IBAction)reloadTableView:(id)sender {
  [self.tableView reloadData];
}

- (void)updateConnectedDevicesMenu {
  self.connectedDevicesPopupButton.menu = [SimpleModificationsTargetDeviceMenuManager createMenu];
}

- (NSInteger)selectedConnectedDeviceIndex {
  NSInteger index = self.connectedDevicesPopupButton.indexOfSelectedItem;
  if (index < 0) {
    return -1;
  }
  return index - 1;
}

@end
