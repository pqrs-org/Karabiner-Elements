// -*- Mode: objc -*-

@import Cocoa;

@interface KarabinerKitComplexModificationsAssetsManager : NSObject

@property(readonly) NSArray* assetsFileModels;

+ (KarabinerKitComplexModificationsAssetsManager*)sharedManager;

- (void)reload;

@end
