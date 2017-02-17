#import "ProfilesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "ProfilesTableCellView.h"

@interface ProfilesTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;

@end

@implementation ProfilesTableViewController

- (void)setup {
  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  [self.tableView reloadData];
                                                }];
  [self.tableView reloadData];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)valueChanged:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];
  NSTextField* name = (NSTextField*)(sender);

  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
  [coreConfigurationModel2 setProfileNameAtIndex:row name:name.stringValue];
  [coreConfigurationModel2 save];
}

- (IBAction)addProfile:(id)sender {
  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
  [coreConfigurationModel2 addProfile];
  [coreConfigurationModel2 save];

  [self.tableView reloadData];
}

- (void)removeProfile:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
  [coreConfigurationModel2 removeProfileAtIndex:(NSUInteger)(row)];
  [coreConfigurationModel2 save];

  [self.tableView reloadData];
}

- (void)selectProfile:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
  [coreConfigurationModel2 selectProfileAtIndex:(NSUInteger)(row)];
  [coreConfigurationModel2 save];

  [self.tableView reloadData];

  [[NSNotificationCenter defaultCenter] postNotificationName:kSelectedProfileChanged object:nil];
}

@end
