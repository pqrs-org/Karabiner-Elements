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

+ (void)selectPopUpButtonMenu:(NSPopUpButton*)popUpButton
                   definition:(NSString*)definition {
  if (definition) {
    NSString* jsonString = [KarabinerKitJsonUtility createPrettyPrintedString:definition];
    if (jsonString) {
      NSArray* items = popUpButton.itemArray;
      if (items) {
        for (NSMenuItem* item in items) {
          if ([item.representedObject isEqualToString:jsonString]) {
            [popUpButton selectItem:item];
            return;
          }
        }
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

  NSString* from = fromCellView.popUpButton.selectedItem.representedObject;
  if ([from isKindOfClass:NSString.class]) {
    // If toCellView is not selected, set fromCellView value to toCellView.
    NSString* to = toCellView.popUpButton.selectedItem.representedObject;
    if (![to isKindOfClass:NSString.class] ||
        [to length] == 0) {
      [SimpleModificationsTableViewController selectPopUpButtonMenu:toCellView.popUpButton
                                                         definition:from];
      to = toCellView.popUpButton.selectedItem.representedObject;
    }
    toCellView.popUpButton.enabled = YES;

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    [coreConfigurationModel setSelectedProfileSimpleModificationAtIndex:row
                                                                   from:from
                                                                     to:to
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
  [self.tableView reloadData];
}

- (NSInteger)selectedConnectedDeviceIndex {
  NSInteger index = self.connectedDevicesPopupButton.indexOfSelectedItem;
  if (index < 0) {
    return -1;
  }
  return index - 1;
}

@end
