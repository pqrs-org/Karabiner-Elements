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
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)valueChanged:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];
  if (row == -1) {
    row = [self.externalKeyboardTableView rowForView:sender];
  }
  if (row == -1) {
    NSLog(@"rowForView error @ [DevicesTableViewController valueChanged]");
    return;
  }
  DevicesTableCellView* cellViewCheckbox = [self.tableView viewAtColumn:0 row:row makeIfNecessary:NO];
  DevicesTableCellView* cellViewExternalKeyboard = [self.externalKeyboardTableView viewAtColumn:0 row:row makeIfNecessary:NO];

  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];

  [configurationManager.coreConfigurationModel.currentProfile setDeviceConfiguration:cellViewCheckbox.deviceIdentifiers
                                                                              ignore:(cellViewCheckbox.checkbox.state != NSOnState)
                                                      disableBuiltInKeyboardIfExists:(cellViewExternalKeyboard.checkbox.state == NSOnState)];
  [configurationManager save];
}

@end
