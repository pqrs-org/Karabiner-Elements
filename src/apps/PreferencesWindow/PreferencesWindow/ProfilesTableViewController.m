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

  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  NSArray<KarabinerKitConfigurationProfile*>* profiles = configurationManager.coreConfigurationModel.profiles;
  if (0 <= row && row < (NSInteger)(profiles.count)) {
    NSTextField* name = (NSTextField*)(sender);
    profiles[row].name = name.stringValue;
    [configurationManager save];
  }
}

- (IBAction)addProfile:(id)sender {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  [configurationManager.coreConfigurationModel addProfile];
  [configurationManager save];

  [self.tableView reloadData];
}

- (void)removeProfile:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  [configurationManager.coreConfigurationModel removeProfile:(NSUInteger)(row)];
  [configurationManager save];

  [self.tableView reloadData];
}

- (void)selectProfile:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  [configurationManager.coreConfigurationModel selectProfile:(NSUInteger)(row)];
  [configurationManager save];

  [self.tableView reloadData];

  [[NSNotificationCenter defaultCenter] postNotificationName:kSelectedProfileChanged object:nil];
}

@end
