#import "DevicesTableViewController.h"
#import "ConfigurationManager.h"
#import "CoreConfigurationModel.h"
#import "DevicesTableCellView.h"
#import "NotificationKeys.h"

@interface DevicesTableViewController ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;
@property(weak) IBOutlet NSTableView* tableView;

@end

@implementation DevicesTableViewController

- (void)setup {
  [[NSNotificationCenter defaultCenter] addObserverForName:kConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  [self.tableView reloadData];
                                                }];

  [[NSNotificationCenter defaultCenter] addObserverForName:kDevicesAreUpdated
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
  NSInteger row = [self.tableView rowForView:sender];
  DevicesTableCellView* cellViewCheckbox = [self.tableView viewAtColumn:0 row:row makeIfNecessary:NO];
  DevicesTableCellView* cellViewPopUp = [self.tableView viewAtColumn:1 row:row makeIfNecessary:NO];
  [self.configurationManager.configurationCoreModel setDeviceConfiguration:cellViewCheckbox.deviceIdentifiers
                                                                    ignore:(cellViewCheckbox.checkbox.state != NSOnState)
                                                              keyboardType:[cellViewPopUp.popUpButton.selectedItem.representedObject unsignedIntValue]];
  [self.configurationManager save];
}

@end
