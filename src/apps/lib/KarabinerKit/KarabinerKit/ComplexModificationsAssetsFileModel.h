// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface KarabinerKitComplexModificationsAssetsFileModel : NSObject

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbn_complex_modifications_assets_manager index:(NSUInteger)index;

@property(readonly) NSUInteger fileIndex;
@property(readonly) NSString* title;
@property(readonly) NSArray* rules;

@end
