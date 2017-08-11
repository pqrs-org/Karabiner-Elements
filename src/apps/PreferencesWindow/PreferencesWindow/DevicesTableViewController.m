#import "DevicesTableViewController.h"
#import "DevicesTableCellView.h"
#import "FnFunctionKeysTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "SimpleModificationsTableViewController.h"

@interface DevicesTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;
@property(weak) IBOutlet NSTableView* externalKeyboardTableView;
@property(weak) IBOutlet SimpleModificationsTableViewController* simpleModificationsTableViewController;
@property(weak) IBOutlet FnFunctionKeysTableViewController* fnFunctionKeysTableViewController;

@end

@implementation DevicesTableViewController

- (void)setup {
  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  [self.tableView reloadData];
                                                  [self.externalKeyboardTableView reloadData];
                                                }];

  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitDevicesAreUpdated
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  [self.tableView reloadData];
                                                  [self.externalKeyboardTableView reloadData];
                                                }];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)valueChanged:(id)sender {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;

  NSInteger row = [self.tableView rowForView:sender];
  if (row != -1) {
    DevicesTableCellView* cellView = [self.tableView viewAtColumn:0 row:row makeIfNecessary:NO];
    libkrbn_device_identifiers deviceIdentifiers = cellView.deviceIdentifiers;
    [coreConfigurationModel setSelectedProfileDeviceIgnore:&(deviceIdentifiers)
                                                           value:(cellView.checkbox.state == NSOffState)];
    [coreConfigurationModel save];
    goto finish;
  }

  row = [self.externalKeyboardTableView rowForView:sender];
  if (row != -1) {
    DevicesTableCellView* cellView = [self.externalKeyboardTableView viewAtColumn:0 row:row makeIfNecessary:NO];
    libkrbn_device_identifiers deviceIdentifiers = cellView.deviceIdentifiers;
    [coreConfigurationModel setSelectedProfileDeviceDisableBuiltInKeyboardIfExists:&(deviceIdentifiers)
                                                                                   value:(cellView.checkbox.state == NSOnState)];
    [coreConfigurationModel save];
    goto finish;
  }

finish:
  [self.simpleModificationsTableViewController updateConnectedDevicesMenu];
  [self.fnFunctionKeysTableViewController updateConnectedDevicesMenu];
}

@end
