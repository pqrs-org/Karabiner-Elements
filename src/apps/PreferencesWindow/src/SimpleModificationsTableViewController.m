#import "SimpleModificationsTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTargetDeviceMenuManager.h"
#import <pqrs/weakify.h>

@interface SimpleModificationsTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;
@property(weak) IBOutlet NSTabViewItem* simpleModificationsTabViewItem;
@property(weak) IBOutlet NSPopUpButton* connectedDevicesPopupButton;
@property id configurationLoadedObserver;
@property id devicesUpdatedObserver;

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

  // Insert new item for definition
  if (definition && ![definition isEqualToString:@"{}"]) {
    [popUpButton.menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:definition
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = definition;
    [popUpButton.menu addItem:item];

    [popUpButton selectItem:item];
    return;
  }

  [popUpButton selectItem:nil];
}

- (void)setup {
  self.configurationLoadedObserver = [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                                                       object:nil
                                                                                        queue:[NSOperationQueue mainQueue]
                                                                                   usingBlock:^(NSNotification* note) {
                                                                                     [self.tableView reloadData];
                                                                                     [self updateConnectedDevicesMenu];
                                                                                   }];

  self.devicesUpdatedObserver = [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitDevicesAreUpdated
                                                                                  object:nil
                                                                                   queue:[NSOperationQueue mainQueue]
                                                                              usingBlock:^(NSNotification* note) {
                                                                                [self updateConnectedDevicesMenu];
                                                                              }];

  [self updateConnectedDevicesMenu];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self.configurationLoadedObserver];
  [[NSNotificationCenter defaultCenter] removeObserver:self.devicesUpdatedObserver];
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

- (void)openSimpleModificationsTab {
  if (self.simpleModificationsTabViewItem.tabState != NSSelectedTab) {
    [self.simpleModificationsTabViewItem.tabView selectTabViewItem:self.simpleModificationsTabViewItem];
  }
}

- (void)addItemFromJson:(NSString*)jsonString {
  id json = [KarabinerKitJsonUtility loadString:jsonString];
  if ([json isKindOfClass:NSDictionary.class]) {
    NSString* fromJsonString = [KarabinerKitJsonUtility createJsonString:@{}];
    NSDictionary* from = json[@"from"];
    if (from) {
      fromJsonString = [KarabinerKitJsonUtility createJsonString:from];
    }

    NSString* toJsonString = [KarabinerKitJsonUtility createJsonString:@{}];
    NSDictionary* to = json[@"to"];
    if (to) {
      toJsonString = [KarabinerKitJsonUtility createJsonString:to];
    }

    NSInteger selectedConnectedDeviceIndex = self.selectedConnectedDeviceIndex;

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    [coreConfigurationModel addSimpleModificationToSelectedProfile:selectedConnectedDeviceIndex];

    NSUInteger index = [coreConfigurationModel selectedProfileSimpleModificationsCount:selectedConnectedDeviceIndex] - 1;
    [coreConfigurationModel setSelectedProfileSimpleModificationAtIndex:index
                                                                   from:fromJsonString
                                                                     to:toJsonString
                                                   connectedDeviceIndex:selectedConnectedDeviceIndex];

    [self.tableView reloadData];
  }
}

@end
