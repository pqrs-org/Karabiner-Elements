#import "ProfilesTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@implementation ProfilesTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2.profilesSize;
}

@end
