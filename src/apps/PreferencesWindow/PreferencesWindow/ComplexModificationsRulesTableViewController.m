#import "ComplexModificationsRulesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"

@interface ComplexModificationsRulesTableViewController ()

@property(weak) IBOutlet NSPanel* addRulePanel;
@property(weak) IBOutlet NSTableView* tableView;
@property(weak) IBOutlet NSWindow* window;

@end

@implementation ComplexModificationsRulesTableViewController

- (void)setup {
  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  [self.tableView reloadData];
                                                }];

  [[NSNotificationCenter defaultCenter] addObserverForName:kSelectedProfileChanged
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  [self.tableView reloadData];
                                                }];
}

- (void)removeRule:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  NSLog(@"removeRule called %d", (int)(row));
}

- (IBAction)openAddRulePanel:(id)sender {
  [self.window beginSheet:self.addRulePanel
        completionHandler:^(NSModalResponse returnCode) {
          NSLog(@"addRulePanel closed %d", (int)(returnCode));
        }];
}

- (IBAction)closeAddRulePanel:(id)sender {
  [self.window endSheet:self.addRulePanel];
}

@end
