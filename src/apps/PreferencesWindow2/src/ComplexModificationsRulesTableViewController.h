// -*- mode: objective-c -*-

@import Cocoa;

@interface ComplexModificationsRulesTableViewController : NSObject

- (void)setup;
- (void)removeRule:(id)sender;
- (void)updateUpDownButtons;
- (IBAction)openAddRulePanel:(id)sender;

@end
