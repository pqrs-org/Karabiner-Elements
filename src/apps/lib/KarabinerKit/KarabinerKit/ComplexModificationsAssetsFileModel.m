#import "ComplexModificationsAssetsFileModel.h"

@interface KarabinerKitComplexModificationsAssetsFileModel ()

@property(copy, readwrite) NSString* title;
@property(copy, readwrite) NSArray* ruleDescriptions;

@end

@implementation KarabinerKitComplexModificationsAssetsFileModel

- (instancetype)initWithManager:(libkrbn_complex_modifications_assets_manager*)libkrbn_complex_modifications_assets_manager index:(NSInteger)index {
  self = [super init];

  if (self) {
    {
      const char* p = libkrbn_complex_modifications_assets_manager_get_file_title(libkrbn_complex_modifications_assets_manager, index);
      _title = p ? [NSString stringWithUTF8String:p] : @"";
    }
  }

  return self;
}

@end
