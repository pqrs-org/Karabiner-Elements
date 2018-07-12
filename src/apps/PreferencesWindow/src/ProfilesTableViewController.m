#import "ProfilesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "ProfilesTableCellView.h"

@interface ProfilesTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;
@property id configurationLoadedObserver;

@end

@implementation ProfilesTableViewController

- (void)setup {
  self.configurationLoadedObserver = [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                                                       object:nil
                                                                                        queue:[NSOperationQueue mainQueue]
                                                                                   usingBlock:^(NSNotification* note) {
                                                                                     [self.tableView reloadData];
                                                                                   }];
  [self.tableView reloadData];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self.configurationLoadedObserver];
}

- (void)valueChanged:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];
  NSTextField* name = (NSTextField*)(sender);

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel setProfileNameAtIndex:row name:name.stringValue];
  [coreConfigurationModel save];
}

- (IBAction)addProfile:(id)sender {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel addProfile];
  [coreConfigurationModel save];

  [self.tableView reloadData];
}

- (void)removeProfile:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel removeProfileAtIndex:(NSUInteger)(row)];
  [coreConfigurationModel save];

  [self.tableView reloadData];
}

- (void)selectProfile:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel selectProfileAtIndex:(NSUInteger)(row)];
  [coreConfigurationModel save];

  [self.tableView reloadData];

  // Ensure reloading data
  [[NSNotificationCenter defaultCenter] postNotificationName:kKarabinerKitConfigurationIsLoaded object:nil];
}

@end
