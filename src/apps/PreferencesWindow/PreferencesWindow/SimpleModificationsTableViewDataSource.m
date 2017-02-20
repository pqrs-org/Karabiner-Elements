#import "SimpleModificationsTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@implementation SimpleModificationsTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel.selectedProfileSimpleModificationsCount;
}

@end
