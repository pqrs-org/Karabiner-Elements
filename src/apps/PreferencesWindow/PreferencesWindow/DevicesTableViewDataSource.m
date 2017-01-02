#import "DevicesTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@interface DevicesTableViewDataSource ()
@end

@implementation DevicesTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [KarabinerKitDeviceManager sharedManager].deviceModels.count;
}

@end
