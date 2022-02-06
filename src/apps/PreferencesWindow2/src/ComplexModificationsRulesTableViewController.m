#import "ComplexModificationsRulesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import <pqrs/weakify.h>

@interface ComplexModificationsRulesTableViewController ()

@property(weak) IBOutlet NSButton* downButton;
@property(weak) IBOutlet NSButton* upButton;
@property(weak) IBOutlet NSOutlineView* assetsOutlineView;
@property(weak) IBOutlet NSPanel* addRulePanel;
@property(weak) IBOutlet NSTabViewItem* complexModificationsTabViewItem;
@property(weak) IBOutlet NSTabViewItem* rulesTabViewItem;
@property(weak) IBOutlet NSTableView* tableView;
@property(weak) IBOutlet NSWindow* window;
@property KarabinerKitSmartObserverContainer* observers;

@end

@implementation ComplexModificationsRulesTableViewController

- (IBAction)openAddRulePanel:(id)sender {
  if (self.complexModificationsTabViewItem.tabState != NSSelectedTab) {
    [self.complexModificationsTabViewItem.tabView selectTabViewItem:self.complexModificationsTabViewItem];
  }
  if (self.rulesTabViewItem.tabState != NSSelectedTab) {
    [self.rulesTabViewItem.tabView selectTabViewItem:self.rulesTabViewItem];
  }

  [[KarabinerKitComplexModificationsAssetsManager sharedManager] reload];
  [self.assetsOutlineView reloadData];
  [self.assetsOutlineView expandItem:nil expandChildren:YES];

  [self.window beginSheet:self.addRulePanel
        completionHandler:^(NSModalResponse returnCode){}];
}

@end
