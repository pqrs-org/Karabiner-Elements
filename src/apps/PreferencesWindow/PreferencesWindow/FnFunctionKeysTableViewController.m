#import "FnFunctionKeysTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTableViewController.h"
#import "weakify.h"

@interface FnFunctionKeysTableViewController ()

@property(weak) IBOutlet NSTableView* tableView;

@end

@implementation FnFunctionKeysTableViewController

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

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)valueChanged:(id)sender {
  NSInteger row = [self.tableView rowForView:sender];

  SimpleModificationsTableCellView* fromCellView = [self.tableView viewAtColumn:0 row:row makeIfNecessary:NO];
  SimpleModificationsTableCellView* toCellView = [self.tableView viewAtColumn:1 row:row makeIfNecessary:NO];

  NSString* fromValue = fromCellView.textField.stringValue;
  NSString* toValue = toCellView.popUpButton.selectedItem.representedObject;

  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
  [coreConfigurationModel2 setSelectedProfileFnFunctionKey:fromValue to:toValue];
  [coreConfigurationModel2 save];
}

@end
