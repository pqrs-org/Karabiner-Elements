#import "KarabinerKit/ComplexModificationsAssetsManager.h"
#import "KarabinerKit/ComplexModificationsAssetsFileModel.h"
#import "libkrbn.h"

@interface KarabinerKitComplexModificationsAssetsManager ()

@property libkrbn_complex_modifications_assets_manager* libkrbn_complex_modifications_assets_manager;
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
    libkrbn_complex_modifications_assets_manager* p = NULL;
    if (libkrbn_complex_modifications_assets_manager_initialize(&p)) {
      _libkrbn_complex_modifications_assets_manager = p;
    }

    _assetsFileModels = @[];
  }

  return self;
}

- (void)dealloc {
  if (self.libkrbn_complex_modifications_assets_manager) {
    libkrbn_complex_modifications_assets_manager* p = self.libkrbn_complex_modifications_assets_manager;
    libkrbn_complex_modifications_assets_manager_terminate(&p);
  }
}

- (void)reload {
  libkrbn_complex_modifications_assets_manager_reload(self.libkrbn_complex_modifications_assets_manager);

  NSMutableArray* models = [NSMutableArray new];
  size_t size = libkrbn_complex_modifications_assets_manager_get_files_size(self.libkrbn_complex_modifications_assets_manager);
  for (size_t i = 0; i < size; ++i) {
    [models addObject:[[KarabinerKitComplexModificationsAssetsFileModel alloc] initWithManager:self.libkrbn_complex_modifications_assets_manager index:i]];
  }
  self.assetsFileModels = models;
}

@end
