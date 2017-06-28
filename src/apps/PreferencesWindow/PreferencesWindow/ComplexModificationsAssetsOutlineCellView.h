// -*- Mode: objc -*-

@import Cocoa;

@interface ComplexModificationsAssetsOutlineCellView : NSTableCellView

@property(weak) IBOutlet NSButton* eraseButton;
@property(weak) IBOutlet NSButton* enableButton;
@property NSUInteger fileIndex;
@property NSUInteger ruleIndex;

@end
