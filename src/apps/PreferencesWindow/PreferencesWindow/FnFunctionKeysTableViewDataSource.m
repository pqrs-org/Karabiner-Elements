#import "FnFunctionKeysTableViewDataSource.h"
#import "ConfigurationManager.h"

@interface FnFunctionKeysTableViewDataSource ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;

@end

@implementation FnFunctionKeysTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return self.configurationManager.coreConfigurationModel.fnFunctionKeys.count;
}

@end
