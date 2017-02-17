#import "ProfilesTableViewDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "ProfilesTableCellView.h"
#import "ProfilesTableViewController.h"

@interface ProfilesTableViewDelegate ()

@property(weak) IBOutlet ProfilesTableViewController* profilesTableViewController;

@end

@implementation ProfilesTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;

  ProfilesTableCellView* result = [tableView makeViewWithIdentifier:@"ProfilesCellView" owner:self];

  result.name.stringValue = [coreConfigurationModel2 profileNameAtIndex:row];
  result.name.action = @selector(valueChanged:);
  result.name.target = self.profilesTableViewController;

  if (coreConfigurationModel2.profilesCount == 1) {
    result.selectedImage.hidden = YES;
    result.selected.hidden = YES;
    result.selectProfileButton.hidden = YES;
    result.removeProfileButton.hidden = YES;
  } else {
    if ([coreConfigurationModel2 profileSelectedAtIndex:row]) {
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
  }

  result.selectProfileButton.action = @selector(selectProfile:);
  result.selectProfileButton.target = self.profilesTableViewController;

  result.removeProfileButton.action = @selector(removeProfile:);
  result.removeProfileButton.target = self.profilesTableViewController;

  return result;
}

@end
