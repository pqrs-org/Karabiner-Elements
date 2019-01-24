#import "KarabinerKit/ComplexModificationsAssetsRuleModel.h"
#import "KarabinerKit/CoreConfigurationModel.h"

@interface KarabinerKitComplexModificationsAssetsRuleModel ()

@property(readwrite) NSUInteger fileIndex;
@property(readwrite) NSUInteger ruleIndex;
@property(copy, readwrite) NSString* ruleDescription;

@end

@interface KarabinerKitCoreConfigurationModel ()

@property libkrbn_core_configuration* libkrbnCoreConfiguration;

@end

@implementation KarabinerKitComplexModificationsAssetsRuleModel

- (instancetype)initWithFileIndex:(NSUInteger)fileIndex
                        ruleIndex:(NSUInteger)ruleIndex {
  self = [super init];

  if (self) {
    _fileIndex = fileIndex;
    _ruleIndex = ruleIndex;

    const char* p = libkrbn_complex_modifications_assets_manager_get_rule_description(fileIndex, ruleIndex);
    _ruleDescription = (p ? [NSString stringWithUTF8String:p] : @"");
  }

  return self;
}

- (void)addRuleToCoreConfigurationProfile:(KarabinerKitCoreConfigurationModel*)coreConfigurationModel {
  libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(self.fileIndex,
                                                                                               self.ruleIndex,
                                                                                               coreConfigurationModel.libkrbnCoreConfiguration);
}

@end
