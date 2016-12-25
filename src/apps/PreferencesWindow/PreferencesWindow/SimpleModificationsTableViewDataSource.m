#import "SimpleModificationsTableViewDataSource.h"
#import "ConfigurationManager.h"

@interface SimpleModificationsTableViewDataSource ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;

@end

@implementation SimpleModificationsTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return self.configurationManager.coreConfigurationModel.simpleModifications.count;
}

@end
