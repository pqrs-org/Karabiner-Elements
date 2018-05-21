#import "KarabinerKit/ComplexModificationsAssetsFileModel.h"
#import "KarabinerKit/ComplexModificationsAssetsRuleModel.h"

@interface KarabinerKitComplexModificationsAssetsFileModel ()

@property(readwrite) NSUInteger fileIndex;
@property(readwrite) BOOL userFile;
@property(copy, readwrite) NSString* title;
@property(copy, readwrite) NSArray* rules;
@property libkrbn_complex_modifications_assets_manager* libkrbnComplexModificationsAssetsManager;

@end

@implementation KarabinerKitComplexModificationsAssetsFileModel

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbnComplexModificationsAssetsManager
                          index:(NSUInteger)index {
  self = [super init];

  if (self) {
    _fileIndex = index;

    _userFile = libkrbn_complex_modifications_assets_manager_is_user_file(libkrbnComplexModificationsAssetsManager, index);

    const char* p = libkrbn_complex_modifications_assets_manager_get_file_title(libkrbnComplexModificationsAssetsManager, index);
    _title = (p ? [NSString stringWithUTF8String:p] : @"");

    NSMutableArray* rules = [NSMutableArray new];
    size_t size = libkrbn_complex_modifications_assets_manager_get_file_rules_size(libkrbnComplexModificationsAssetsManager, index);
    for (size_t i = 0; i < size; ++i) {
      [rules addObject:[[KarabinerKitComplexModificationsAssetsRuleModel alloc] initWithManager:libkrbnComplexModificationsAssetsManager
                                                                                      fileIndex:index
                                                                                      ruleIndex:i]];
    }
    _rules = rules;

    _libkrbnComplexModificationsAssetsManager = libkrbnComplexModificationsAssetsManager;
  }

  return self;
}

- (void)unlinkFile {
  libkrbn_complex_modifications_assets_manager_erase_file(self.libkrbnComplexModificationsAssetsManager, self.fileIndex);
}

@end
