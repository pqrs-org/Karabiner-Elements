#import "ComplexModificationsAssetsRuleModel.h"
#import "CoreConfigurationModel.h"

@interface KarabinerKitComplexModificationsAssetsRuleModel ()

@property(readwrite) NSUInteger fileIndex;
@property(readwrite) NSUInteger ruleIndex;
@property(copy, readwrite) NSString* ruleDescription;
@property libkrbn_complex_modifications_assets_manager* libkrbnComplexModificationsAssetsManager;

@end

@interface KarabinerKitCoreConfigurationModel ()

@property libkrbn_core_configuration* libkrbnCoreConfiguration;

@end

@implementation KarabinerKitComplexModificationsAssetsRuleModel

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbnComplexModificationsAssetsManager
                      fileIndex:(NSUInteger)fileIndex
                      ruleIndex:(NSUInteger)ruleIndex {
  self = [super init];

  if (self) {
    _fileIndex = fileIndex;
    _ruleIndex = ruleIndex;

    const char* p = libkrbn_complex_modifications_assets_manager_get_file_rule_description(libkrbnComplexModificationsAssetsManager, fileIndex, ruleIndex);
    _ruleDescription = (p ? [NSString stringWithUTF8String:p] : @"");

    _libkrbnComplexModificationsAssetsManager = libkrbnComplexModificationsAssetsManager;
  }

  return self;
}

- (void)addRuleToCoreConfigurationProfile:(KarabinerKitCoreConfigurationModel*)coreConfigurationModel {
  libkrbn_complex_modifications_assets_manager_add_rule_to_core_configuration_selected_profile(self.libkrbnComplexModificationsAssetsManager,
                                                                                               self.fileIndex,
                                                                                               self.ruleIndex,
                                                                                               coreConfigurationModel.libkrbnCoreConfiguration);
}

@end
