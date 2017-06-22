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
  if ([item class] == KarabinerKitComplexModificationsAssetsFileModel.class) {
    fileModel = (KarabinerKitComplexModificationsAssetsFileModel*)(item);
  }

  KarabinerKitComplexModificationsAssetsRuleModel* ruleModel = nil;
  if ([item class] == KarabinerKitComplexModificationsAssetsRuleModel.class) {
    ruleModel = (KarabinerKitComplexModificationsAssetsRuleModel*)(item);
  }

  if ([tableColumn.identifier isEqualToString:@"ComplexModificationsAssetsNameColumn"]) {
    NSTableCellView* result = [outlineView makeViewWithIdentifier:@"ComplexModificationsAssetsNameCellView" owner:self];
    if (fileModel) {
      result.textField.stringValue = fileModel.title;
      [result.textField setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];

    } else if (ruleModel) {
      result.textField.stringValue = ruleModel.ruleDescription;
      [result.textField setFont:[NSFont systemFontOfSize:[NSFont systemFontSize]]];
    }
    return result;

  } else if ([tableColumn.identifier isEqualToString:@"ComplexModificationsAssetsButtonsColumn"]) {
    if (fileModel) {
      ComplexModificationsAssetsOutlineCellView* result = [outlineView makeViewWithIdentifier:@"ComplexModificationsAssetsButtonsCellView" owner:self];
      result.eraseButton.hidden = NO;
      result.eraseButton.action = @selector(eraseImportedFile:);
      result.eraseButton.target = self.complexModificationsRulesTableViewController;
      result.enableButton.hidden = YES;
      result.fileIndex = fileModel.fileIndex;
      return result;
    }

    if (ruleModel) {
      ComplexModificationsAssetsOutlineCellView* result = [outlineView makeViewWithIdentifier:@"ComplexModificationsAssetsButtonsCellView" owner:self];
      result.eraseButton.hidden = YES;
      result.enableButton.action = @selector(addRule:);
      result.enableButton.target = self.complexModificationsRulesTableViewController;
      result.enableButton.hidden = NO;
      result.fileIndex = ruleModel.fileIndex;
      result.ruleIndex = ruleModel.ruleIndex;
      return result;
    }
  }

  return nil;
}

@end
