// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface KarabinerKitComplexModificationsAssetsRuleModel : NSObject

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbn_complex_modifications_assets_manager
                      fileIndex:(NSUInteger)fileIndex
                      ruleIndex:(NSUInteger)ruleIndex;

@property(readonly) NSUInteger fileIndex;
@property(readonly) NSUInteger ruleIndex;
@property(readonly) NSString* ruleDescription;

@end
