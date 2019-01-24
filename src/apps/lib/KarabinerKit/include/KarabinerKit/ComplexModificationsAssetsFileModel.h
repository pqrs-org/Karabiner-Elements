// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn/libkrbn.h"

@interface KarabinerKitComplexModificationsAssetsFileModel : NSObject

@property(readonly) NSUInteger fileIndex;
@property(readonly) BOOL userFile;
@property(readonly) NSString* title;
@property(readonly) NSArray* rules;

- (instancetype)initWithFileIndex:(NSUInteger)index;
- (void)unlinkFile;

@end
