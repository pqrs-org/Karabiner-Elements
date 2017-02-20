#import "DevicesTableViewController.h"
#import "DevicesTableCellView.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"

@interface DevicesTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;
@property(weak) IBOutlet NSTableView* externalKeyboardTableView;

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

  [[NSNotificationCenter defaultCenter] addObserverForName:kSelectedProfileChanged
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
    [coreConfigurationModel setSelectedProfileDeviceIgnore:cellView.deviceIdentifiers.vendorId
                                                  productId:cellView.deviceIdentifiers.productId
                                                 isKeyboard:cellView.deviceIdentifiers.isKeyboard
                                           isPointingDevice:cellView.deviceIdentifiers.isPointingDevice
                                                      value:(cellView.checkbox.state == NSOffState)];
    [coreConfigurationModel save];
    return;
  }

  row = [self.externalKeyboardTableView rowForView:sender];
  if (row != -1) {
    DevicesTableCellView* cellView = [self.externalKeyboardTableView viewAtColumn:0 row:row makeIfNecessary:NO];
    [coreConfigurationModel setSelectedProfileDeviceDisableBuiltInKeyboardIfExists:cellView.deviceIdentifiers.vendorId
                                                                          productId:cellView.deviceIdentifiers.productId
                                                                         isKeyboard:cellView.deviceIdentifiers.isKeyboard
                                                                   isPointingDevice:cellView.deviceIdentifiers.isPointingDevice
                                                                              value:(cellView.checkbox.state == NSOnState)];
    [coreConfigurationModel save];
    return;
  }
}

@end
