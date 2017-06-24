#import "ComplexModificationsRulesTableViewController.h"
#import "ComplexModificationsAssetsOutlineCellView.h"
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
  NSInteger row = [self.tableView rowForView:sender];

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel removeSelectedProfileComplexModificationsRule:row];
  [coreConfigurationModel save];

  [self.tableView reloadData];
}

- (IBAction)openAddRulePanel:(id)sender {
  [[KarabinerKitComplexModificationsAssetsManager sharedManager] reload];
  [self.assetsOutlineView reloadData];
  [self.assetsOutlineView expandItem:nil expandChildren:YES];

  [self.window beginSheet:self.addRulePanel
        completionHandler:^(NSModalResponse returnCode){}];
}

- (void)eraseImportedFile:(NSButton*)sender {
  if (sender.superview.class == ComplexModificationsAssetsOutlineCellView.class) {
    ComplexModificationsAssetsOutlineCellView* view = (ComplexModificationsAssetsOutlineCellView*)(sender.superview);

    NSArray* files = [KarabinerKitComplexModificationsAssetsManager sharedManager].assetsFileModels;
    if (view.fileIndex < files.count) {
      KarabinerKitComplexModificationsAssetsFileModel* model = files[view.fileIndex];

      NSAlert* alert = [NSAlert new];

      alert.messageText = @"Confirmation";
      alert.informativeText = @"Are you sure you want to erase this imported file?";
      [alert addButtonWithTitle:@"Erase"];
      [alert addButtonWithTitle:@"Cancel"];

      [alert beginSheetModalForWindow:self.addRulePanel
                    completionHandler:^(NSModalResponse returnCode) {
                      if (returnCode == NSAlertFirstButtonReturn) {
                        [model unlinkFile];

                        [[KarabinerKitComplexModificationsAssetsManager sharedManager] reload];
                        [self.assetsOutlineView reloadData];
                        [self.assetsOutlineView expandItem:nil expandChildren:YES];
                      }
                    }];
    }
  }
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
