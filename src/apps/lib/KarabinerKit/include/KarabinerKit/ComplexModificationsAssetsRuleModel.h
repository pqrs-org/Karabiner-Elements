// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn/libkrbn.h"

@class KarabinerKitCoreConfigurationModel;

@interface KarabinerKitComplexModificationsAssetsRuleModel : NSObject

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbnComplexModificationsAssetsManager
                      fileIndex:(NSUInteger)fileIndex
                      ruleIndex:(NSUInteger)ruleIndex;

- (void)addRuleToCoreConfigurationProfile:(KarabinerKitCoreConfigurationModel*)coreConfigurationModel;

@property(readonly) NSUInteger fileIndex;
@property(readonly) NSUInteger ruleIndex;
@property(readonly) NSString* ruleDescription;

@end
