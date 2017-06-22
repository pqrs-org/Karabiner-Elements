#import "ComplexModificationsAssetsRuleModel.h"

@interface KarabinerKitComplexModificationsAssetsRuleModel ()

@property(readwrite) NSUInteger fileIndex;
@property(readwrite) NSUInteger ruleIndex;
@property(copy, readwrite) NSString* ruleDescription;

@end

@implementation KarabinerKitComplexModificationsAssetsRuleModel

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbn_complex_modifications_assets_manager
                      fileIndex:(NSUInteger)fileIndex
                      ruleIndex:(NSUInteger)ruleIndex {
  self = [super init];

  if (self) {
    _fileIndex = fileIndex;
    _ruleIndex = ruleIndex;

    const char* p = libkrbn_complex_modifications_assets_manager_get_file_rule_description(libkrbn_complex_modifications_assets_manager, fileIndex, ruleIndex);
    _ruleDescription = (p ? [NSString stringWithUTF8String:p] : @"");
  }

  return self;
}

@end
