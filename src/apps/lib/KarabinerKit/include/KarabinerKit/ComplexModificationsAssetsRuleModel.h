// -*- mode: objective-c -*-

@import Cocoa;
#import "libkrbn/libkrbn.h"

@class KarabinerKitCoreConfigurationModel;

@interface KarabinerKitComplexModificationsAssetsRuleModel : NSObject

- (instancetype)initWithFileIndex:(NSUInteger)fileIndex
                        ruleIndex:(NSUInteger)ruleIndex;

- (void)addRuleToCoreConfigurationProfile:(KarabinerKitCoreConfigurationModel*)coreConfigurationModel;

@property(readonly) NSUInteger fileIndex;
@property(readonly) NSUInteger ruleIndex;
@property(readonly) NSString* ruleDescription;

@end
