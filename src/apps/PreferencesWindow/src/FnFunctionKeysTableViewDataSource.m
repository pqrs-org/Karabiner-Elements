#import "FnFunctionKeysTableViewDataSource.h"
#import "FnFunctionKeysTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"

@interface FnFunctionKeysTableViewDataSource ()

@property(weak) IBOutlet FnFunctionKeysTableViewController* fnFunctionKeysTableViewController;

@end

@implementation FnFunctionKeysTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  NSInteger connectedDeviceIndex = self.fnFunctionKeysTableViewController.selectedConnectedDeviceIndex;
  return [[KarabinerKitConfigurationManager sharedManager].coreConfigurationModel selectedProfileFnFunctionKeysCount:connectedDeviceIndex];
}

@end
