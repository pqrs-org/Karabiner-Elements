// -*- Mode: objc -*-

@import Cocoa;

@interface ComplexModificationsRulesTableViewController : NSObject

- (void)setup;
- (void)removeRule:(id)sender;
- (void)updateUpDownButtons;
- (IBAction)openAddRulePanel:(id)sender;
- (void)eraseImportedFile:(id)sender;
- (void)addRule:(id)sender;
- (void)addAllRules:(id)sender;

@end
