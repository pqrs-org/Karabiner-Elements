#import "DevicesTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@implementation DevicesTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [KarabinerKitDeviceManager sharedManager].connectedDevices.devicesCount;
}

@end
