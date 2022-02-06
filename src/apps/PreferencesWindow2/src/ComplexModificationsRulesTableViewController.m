#import "ComplexModificationsRulesTableViewController.h"
#import "ComplexModificationsAssetsOutlineCellView.h"
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

  [self updateUpDownButtons];
}

- (IBAction)openRulesSite:(id)sender {
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://ke-complex-modifications.pqrs.org/"]];
}

- (void)removeRule:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel removeSelectedProfileComplexModificationsRule:row];
  [coreConfigurationModel save];

  [self.tableView reloadData];
}

- (void)updateUpDownButtons {
  self.downButton.enabled = NO;
  self.upButton.enabled = NO;

  if (self.tableView.selectedRow < 0) {
    return;
  }

  if (self.tableView.selectedRow > 0) {
    self.upButton.enabled = YES;
  }
  if (self.tableView.selectedRow < self.tableView.numberOfRows - 1) {
    self.downButton.enabled = YES;
  }
}

- (IBAction)moveRuleUp:(id)sender {
  NSInteger row = self.tableView.selectedRow;

  if (row < 0) {
    return;
  }

  if (row > 0) {
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    [coreConfigurationModel swapSelectedProfileComplexModificationsRules:row
                                                                  index2:row - 1];
    [coreConfigurationModel save];

    [self.tableView reloadData];

    NSIndexSet* indexes = [NSIndexSet indexSetWithIndex:(row - 1)];
    [self.tableView selectRowIndexes:indexes byExtendingSelection:NO];
  }
}

- (IBAction)moveRuleDown:(id)sender {
  NSInteger row = self.tableView.selectedRow;

  if (row < 0) {
    return;
  }

  if (row < self.tableView.numberOfRows - 1) {
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    [coreConfigurationModel swapSelectedProfileComplexModificationsRules:row
                                                                  index2:row + 1];
    [coreConfigurationModel save];

    [self.tableView reloadData];

    NSIndexSet* indexes = [NSIndexSet indexSetWithIndex:(row + 1)];
    [self.tableView selectRowIndexes:indexes byExtendingSelection:NO];
  }
}

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
