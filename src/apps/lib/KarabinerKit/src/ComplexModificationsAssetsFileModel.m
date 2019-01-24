#import "KarabinerKit/ComplexModificationsAssetsFileModel.h"
#import "KarabinerKit/ComplexModificationsAssetsRuleModel.h"

@interface KarabinerKitComplexModificationsAssetsFileModel ()

@property(readwrite) NSUInteger fileIndex;
@property(readwrite) BOOL userFile;
@property(copy, readwrite) NSString* title;
@property(copy, readwrite) NSArray* rules;

@end

@implementation KarabinerKitComplexModificationsAssetsFileModel

- (instancetype)initWithFileIndex:(NSUInteger)index {
  self = [super init];

  if (self) {
    _fileIndex = index;

    _userFile = libkrbn_complex_modifications_assets_manager_user_file(index);

    const char* p = libkrbn_complex_modifications_assets_manager_get_file_title(index);
    _title = (p ? [NSString stringWithUTF8String:p] : @"");

    NSMutableArray* rules = [NSMutableArray new];
    size_t size = libkrbn_complex_modifications_assets_manager_get_rules_size(index);
    for (size_t i = 0; i < size; ++i) {
      [rules addObject:[[KarabinerKitComplexModificationsAssetsRuleModel alloc] initWithFileIndex:index
                                                                                        ruleIndex:i]];
    }
    _rules = rules;
  }

  return self;
}

- (void)unlinkFile {
  libkrbn_complex_modifications_assets_manager_erase_file(self.fileIndex);
}

@end
