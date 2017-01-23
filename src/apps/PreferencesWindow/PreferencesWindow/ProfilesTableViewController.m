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
}

- (void)removeProfile:(id)sender {
}

- (IBAction)addProfile:(id)sender {
}

@end
