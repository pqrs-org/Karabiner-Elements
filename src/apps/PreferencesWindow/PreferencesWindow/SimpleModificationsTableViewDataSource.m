#import "SimpleModificationsTableViewDataSource.h"
#import "ConfigurationManager.h"

@interface SimpleModificationsTableViewDataSource ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;

@end

@implementation SimpleModificationsTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  NSDictionary* currentProfile = self.configurationManager.currentProfile;
  if (currentProfile) {
    NSDictionary* simple_modifications = currentProfile[@"simple_modifications"];
    return simple_modifications.count;
  }
  return 0;
}

@end
