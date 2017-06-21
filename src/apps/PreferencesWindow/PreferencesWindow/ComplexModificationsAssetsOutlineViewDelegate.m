#import "ComplexModificationsAssetsOutlineViewDelegate.h"
#import "ComplexModificationsAssetsOutlineCellView.h"
#import "ComplexModificationsRulesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"

@interface ComplexModificationsAssetsOutlineViewDelegate ()

@property(weak) IBOutlet ComplexModificationsRulesTableViewController* complexModificationsRulesTableViewController;

@end

@implementation ComplexModificationsAssetsOutlineViewDelegate

- (NSView*)outlineView:(NSOutlineView*)outlineView viewForTableColumn:(NSTableColumn*)tableColumn item:(id)item {
  KarabinerKitComplexModificationsAssetsFileModel* fileModel = nil;
  if ([item class] == [KarabinerKitComplexModificationsAssetsFileModel class]) {
    fileModel = (KarabinerKitComplexModificationsAssetsFileModel*)(item);
  }

  KarabinerKitComplexModificationsAssetsRuleModel* ruleModel = nil;
  if ([item class] == [KarabinerKitComplexModificationsAssetsRuleModel class]) {
    ruleModel = (KarabinerKitComplexModificationsAssetsRuleModel*)(item);
  }

  if ([tableColumn.identifier isEqualToString:@"ComplexModificationsAssetsNameColumn"]) {
    NSTableCellView* result = [outlineView makeViewWithIdentifier:@"ComplexModificationsAssetsNameCellView" owner:self];
    if (fileModel) {
      result.textField.stringValue = fileModel.title;
    } else if (ruleModel) {
      result.textField.stringValue = ruleModel.ruleDescription;
    }
    return result;

  } else if ([tableColumn.identifier isEqualToString:@"ComplexModificationsAssetsButtonsColumn"]) {
    if (ruleModel) {
      ComplexModificationsAssetsOutlineCellView* result = [outlineView makeViewWithIdentifier:@"ComplexModificationsAssetsButtonsCellView" owner:self];
      result.enableButton.action = @selector(addRule:);
      result.enableButton.target = self.complexModificationsRulesTableViewController;
      result.fileIndex = ruleModel.fileIndex;
      result.ruleIndex = ruleModel.ruleIndex;

      return result;
    }
  }

  return nil;
}

@end
