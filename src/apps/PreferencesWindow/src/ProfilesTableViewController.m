#import "ProfilesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "ProfilesTableCellView.h"
#import <pqrs/weakify.h>

@interface ProfilesTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;
@property KarabinerKitSmartObserverContainer* observers;

@end

@implementation ProfilesTableViewController

- (void)setup {
  self.observers = [KarabinerKitSmartObserverContainer new];
  @weakify(self);

  {
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    id o = [center addObserverForName:kKarabinerKitConfigurationIsLoaded
                               object:nil
                                queue:[NSOperationQueue mainQueue]
                           usingBlock:^(NSNotification* note) {
                             @strongify(self);
                             if (!self) {
                               return;
                             }

                             [self.tableView reloadData];
                           }];
    [self.observers addObserver:o notificationCenter:center];
  }

  [self.tableView reloadData];
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
