#import "FnFunctionKeysTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@implementation FnFunctionKeysTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel.selectedProfileFnFunctionKeysCount;
}

@end
