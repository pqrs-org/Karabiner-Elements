#import "KarabinerKit/ComplexModificationsAssetsManager.h"
#import "KarabinerKit/ComplexModificationsAssetsFileModel.h"
#import "libkrbn/libkrbn.h"

@interface KarabinerKitComplexModificationsAssetsManager ()

@property(copy, readwrite) NSArray* assetsFileModels;

@end

@implementation KarabinerKitComplexModificationsAssetsManager

+ (KarabinerKitComplexModificationsAssetsManager*)sharedManager {
  static dispatch_once_t once;
  static KarabinerKitComplexModificationsAssetsManager* manager;
  dispatch_once(&once, ^{
    manager = [KarabinerKitComplexModificationsAssetsManager new];
  });

  return manager;
}

- (instancetype)init {
  self = [super init];

  if (self) {
    libkrbn_enable_complex_modifications_assets_manager();

    _assetsFileModels = @[];
  }

  return self;
}

- (void)dealloc {
  libkrbn_disable_complex_modifications_assets_manager();
}

- (void)reload {
  libkrbn_complex_modifications_assets_manager_reload();

  NSMutableArray* models = [NSMutableArray new];
  size_t size = libkrbn_complex_modifications_assets_manager_get_files_size();
  for (size_t i = 0; i < size; ++i) {
    [models addObject:[[KarabinerKitComplexModificationsAssetsFileModel alloc] initWithFileIndex:i]];
  }
  self.assetsFileModels = models;
}

@end
