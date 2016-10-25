#import "DevicesTableViewDataSource.h"
#import "DeviceManager.h"

@interface DevicesTableViewDataSource ()

@property(weak) IBOutlet DeviceManager* deviceManager;

@end

@implementation DevicesTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return self.deviceManager.deviceModels.count;
}

@end
