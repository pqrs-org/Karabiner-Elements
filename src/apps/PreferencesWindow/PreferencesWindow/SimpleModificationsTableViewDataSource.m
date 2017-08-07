#import "SimpleModificationsTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"
#import "SimpleModificationsTableViewController.h"

@interface SimpleModificationsTableViewDataSource ()

@property(weak) IBOutlet SimpleModificationsTableViewController* simpleModificationsTableViewController;

@end

@implementation SimpleModificationsTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  NSInteger connectedDeviceIndex = self.simpleModificationsTableViewController.selectedConnectedDeviceIndex;
  return [[KarabinerKitConfigurationManager sharedManager].coreConfigurationModel selectedProfileSimpleModificationsCount:connectedDeviceIndex];
}

@end
