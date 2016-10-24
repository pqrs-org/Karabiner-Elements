#import "DevicesTableViewDataSource.h"
#import "ConfigurationManager.h"

@interface DevicesTableViewDataSource ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;

@end

@implementation DevicesTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return self.configurationManager.configurationCoreModel.simpleModifications.count;
}

@end
