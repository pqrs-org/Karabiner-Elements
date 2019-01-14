// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn/libkrbn.h"

@interface KarabinerKitComplexModificationsAssetsFileModel : NSObject

@property(readonly) NSUInteger fileIndex;
@property(readonly) BOOL userFile;
@property(readonly) NSString* title;
@property(readonly) NSArray* rules;

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbnComplexModificationsAssetsManager index:(NSUInteger)index;
- (void)unlinkFile;

@end
