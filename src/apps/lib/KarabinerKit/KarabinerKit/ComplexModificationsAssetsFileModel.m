#import "ComplexModificationsAssetsFileModel.h"
#import "ComplexModificationsAssetsRuleModel.h"

@interface KarabinerKitComplexModificationsAssetsFileModel ()

@property(readwrite) NSUInteger fileIndex;
@property(copy, readwrite) NSString* title;
@property(copy, readwrite) NSArray* rules;
@property libkrbn_complex_modifications_assets_manager* libkrbn_complex_modifications_assets_manager;

@end

@implementation KarabinerKitComplexModificationsAssetsFileModel

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbn_complex_modifications_assets_manager
                          index:(NSUInteger)index {
  self = [super init];

  if (self) {
    _fileIndex = index;

    const char* p = libkrbn_complex_modifications_assets_manager_get_file_title(libkrbn_complex_modifications_assets_manager, index);
    _title = (p ? [NSString stringWithUTF8String:p] : @"");

    NSMutableArray* rules = [NSMutableArray new];
    size_t size = libkrbn_complex_modifications_assets_manager_get_file_rules_size(libkrbn_complex_modifications_assets_manager, index);
    for (size_t i = 0; i < size; ++i) {
      [rules addObject:[[KarabinerKitComplexModificationsAssetsRuleModel alloc] initWithManager:libkrbn_complex_modifications_assets_manager
                                                                                      fileIndex:index
                                                                                      ruleIndex:i]];
    }
    _rules = rules;

    _libkrbn_complex_modifications_assets_manager = libkrbn_complex_modifications_assets_manager;
  }

  return self;
}

- (void)unlinkFile {
  libkrbn_complex_modifications_assets_manager_erase_file(self.libkrbn_complex_modifications_assets_manager, self.fileIndex);
}

@end
