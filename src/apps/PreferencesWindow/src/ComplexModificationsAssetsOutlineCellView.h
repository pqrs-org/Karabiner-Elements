// -*- mode: objective-c -*-

@import Cocoa;

@interface ComplexModificationsAssetsOutlineCellView : NSTableCellView

@property(weak) IBOutlet NSButton* eraseButton;
@property(weak) IBOutlet NSButton* enableButton;
@property(weak) IBOutlet NSButton* enableAllButton;
@property NSUInteger fileIndex;
@property NSUInteger ruleIndex;

@end
