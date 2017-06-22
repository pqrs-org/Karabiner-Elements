#import "ComplexModificationsRulesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"

@interface ComplexModificationsRulesTableViewController ()

@property(weak) IBOutlet NSOutlineView* assetsOutlineView;
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
  //NSInteger row = [self.tableView rowForView:sender];
}

- (IBAction)openAddRulePanel:(id)sender {
  [[KarabinerKitComplexModificationsAssetsManager sharedManager] reload];
  [self.assetsOutlineView reloadData];
  [self.assetsOutlineView expandItem:nil expandChildren:YES];

  [self.window beginSheet:self.addRulePanel
        completionHandler:^(NSModalResponse returnCode){}];
}

- (void)eraseImportedFile:(id)sender {
}

- (void)addRule:(id)sender {
}

- (IBAction)expandRules:(id)sender {
  [self.assetsOutlineView expandItem:nil expandChildren:YES];
}

- (IBAction)collapseRules:(id)sender {
  [self.assetsOutlineView collapseItem:nil collapseChildren:YES];
}

- (IBAction)closeAddRulePanel:(id)sender {
  [self.window endSheet:self.addRulePanel];
}

@end
