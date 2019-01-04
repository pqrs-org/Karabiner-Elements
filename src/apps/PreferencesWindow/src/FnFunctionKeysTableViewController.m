#import "FnFunctionKeysTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTableViewController.h"
#import "SimpleModificationsTargetDeviceMenuManager.h"
#import <pqrs/weakify.h>

@interface FnFunctionKeysTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;
@property(weak) IBOutlet NSPopUpButton* connectedDevicesPopupButton;
@property id configurationLoadedObserver;
@property id devicesUpdatedObserver;

@end

@implementation FnFunctionKeysTableViewController

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

  NSString* from = [KarabinerKitJsonUtility createJsonString:@{
    @"key_code" : fromCellView.textField.stringValue,
  }];
  NSString* to = toCellView.popUpButton.selectedItem.representedObject;

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;

  [coreConfigurationModel setSelectedProfileFnFunctionKey:from
                                                       to:to
                                     connectedDeviceIndex:self.selectedConnectedDeviceIndex];
  [coreConfigurationModel save];
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
