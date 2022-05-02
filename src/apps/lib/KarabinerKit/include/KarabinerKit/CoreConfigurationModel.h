// -*- mode: objective-c -*-

@import Cocoa;
#import "libkrbn/libkrbn.h"

@interface KarabinerKitCoreConfigurationModel : NSObject

- (instancetype)initWithInitializedCoreConfiguration:(libkrbn_core_configuration*)initializedCoreConfiguration;

- (void)save;

@property BOOL globalConfigurationCheckForUpdatesOnStartup;
@property BOOL globalConfigurationShowInMenuBar;
@property BOOL globalConfigurationShowProfileNameInMenuBar;

@property(readonly) NSUInteger profilesCount;
- (NSString*)profileNameAtIndex:(NSUInteger)index;
- (void)setProfileNameAtIndex:(NSUInteger)index name:(NSString*)name;
- (BOOL)profileSelectedAtIndex:(NSUInteger)index;
- (void)selectProfileAtIndex:(NSUInteger)index;
@property(readonly) NSString* selectedProfileName;

@end
