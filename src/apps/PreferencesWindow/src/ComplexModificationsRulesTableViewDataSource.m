#import "ComplexModificationsRulesTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@implementation ComplexModificationsRulesTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel.selectedProfileComplexModificationsRulesCount;
}

@end
