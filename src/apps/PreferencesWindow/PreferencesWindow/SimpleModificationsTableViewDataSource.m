#import "SimpleModificationsTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@interface SimpleModificationsTableViewDataSource ()
@end

@implementation SimpleModificationsTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel.simpleModifications.count;
}

@end
