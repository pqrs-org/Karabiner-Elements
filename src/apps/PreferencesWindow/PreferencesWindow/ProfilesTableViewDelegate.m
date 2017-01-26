#import "ProfilesTableViewDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "ProfilesTableCellView.h"
#import "ProfilesTableViewController.h"

@interface ProfilesTableViewDelegate ()

@property(weak) IBOutlet ProfilesTableViewController* profilesTableViewController;

@end

@implementation ProfilesTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];

  KarabinerKitConfigurationProfile* profile = configurationManager.coreConfigurationModel.profiles[row];

  ProfilesTableCellView* result = [tableView makeViewWithIdentifier:@"ProfilesCellView" owner:self];
  result.name.stringValue = profile.name;
  if (profile.selected) {
    result.selected.hidden = NO;
    result.removeProfileButton.hidden = YES;
  } else {
    result.selected.hidden = YES;
    result.removeProfileButton.hidden = NO;
  }

  result.name.action = @selector(valueChanged:);
  result.name.target = self.profilesTableViewController;

  result.removeProfileButton.action = @selector(removeProfile:);
  result.removeProfileButton.target = self.profilesTableViewController;

  return result;
}

@end
