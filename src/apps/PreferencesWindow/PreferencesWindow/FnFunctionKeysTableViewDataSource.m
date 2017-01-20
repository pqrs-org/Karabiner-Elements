#import "FnFunctionKeysTableViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@interface FnFunctionKeysTableViewDataSource ()
@end

@implementation FnFunctionKeysTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel.currentProfile.fnFunctionKeys.count;
}

@end
