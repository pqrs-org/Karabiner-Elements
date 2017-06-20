// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface KarabinerKitComplexModificationsAssetsFileModel : NSObject

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbn_complex_modifications_assets_manager index:(NSInteger)index;

@property(readonly) NSString* title;
@property(readonly) NSArray* ruleDescriptions;

@end
