#import "ComplexModificationsAssetsOutlineViewDataSource.h"
#import "KarabinerKit/KarabinerKit.h"

@implementation ComplexModificationsAssetsOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView*)outlineView numberOfChildrenOfItem:(id)item {
  return [self getChildren:item].count;
}

- (id)outlineView:(NSOutlineView*)outlineView child:(NSInteger)index ofItem:(id)item {
  return [self getChildren:item][index];
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item {
  return [self getChildren:item].count > 0;
}

- (NSArray*)getChildren:(id)item {
  if ([item class] == [KarabinerKitComplexModificationsAssetsFileModel class]) {
    KarabinerKitComplexModificationsAssetsFileModel* model = (KarabinerKitComplexModificationsAssetsFileModel*)(item);
    return model.rules;
  }

  if (!item) {
    // item is root.
    return [KarabinerKitComplexModificationsAssetsManager sharedManager].assetsFileModels;
  }

  return nil;
}

@end
