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
    result.selectedImage.hidden = NO;
    result.selected.hidden = NO;
    result.selectProfileButton.hidden = YES;
    result.removeProfileButton.hidden = YES;
  } else {
    result.selectedImage.hidden = YES;
    result.selected.hidden = YES;
    result.selectProfileButton.hidden = NO;
    result.removeProfileButton.hidden = NO;
  }

  result.name.action = @selector(valueChanged:);
  result.name.target = self.profilesTableViewController;

  result.selectProfileButton.action = @selector(selectProfile:);
  result.selectProfileButton.target = self.profilesTableViewController;

  result.removeProfileButton.action = @selector(removeProfile:);
  result.removeProfileButton.target = self.profilesTableViewController;

  return result;
}

@end
