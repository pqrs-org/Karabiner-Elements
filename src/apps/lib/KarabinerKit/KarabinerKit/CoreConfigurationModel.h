// -*- Mode: objc -*-

@import Cocoa;
#import "DeviceModel.h"

@interface KarabinerKitGlobalConfiguration : NSObject

@property BOOL checkForUpdatesOnStartup;
@property BOOL showInMenuBar;
@property BOOL showProfileNameInMenuBar;

- (instancetype)initWithJsonObject:(NSDictionary*)global;

@end

@interface KarabinerKitDeviceConfiguration : NSObject

@property(readonly) KarabinerKitDeviceIdentifiers* deviceIdentifiers;
@property BOOL ignore;
@property BOOL disableBuiltInKeyboardIfExists;

@end

@interface KarabinerKitConfigurationProfile : NSObject

@property(copy) NSString* name;
@property BOOL selected;
@property(copy, readonly) NSArray<NSDictionary*>* simpleModifications;
@property(copy, readonly) NSArray<NSDictionary*>* fnFunctionKeys;
@property(copy, readonly) NSArray<KarabinerKitDeviceConfiguration*>* devices;
@property(copy) NSString* virtualHIDKeyboardType;
@property NSUInteger virtualHIDKeyboardCapsLockDelayMilliseconds;

@property(copy, readonly) NSDictionary* jsonObject;

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject;

- (void)addSimpleModification;
- (void)removeSimpleModification:(NSUInteger)index;
- (void)replaceSimpleModification:(NSUInteger)index from:(NSString*)from to:(NSString*)to;

- (void)replaceFnFunctionKey:(NSString*)from to:(NSString*)to;

- (void)setDeviceConfiguration:(KarabinerKitDeviceIdentifiers*)deviceIdentifiers
                            ignore:(BOOL)ignore
    disableBuiltInKeyboardIfExists:(BOOL)disableBuiltInKeyboardIfExists;

@end

@interface KarabinerKitCoreConfigurationModel : NSObject

@property(readonly) KarabinerKitGlobalConfiguration* globalConfiguration;
@property(copy, readonly) NSArray<KarabinerKitConfigurationProfile*>* profiles;
@property(readonly) KarabinerKitConfigurationProfile* currentProfile;

- (instancetype)initWithJsonObject:(NSDictionary*)jsonObject;

- (void)addProfile;
- (void)removeProfile:(NSUInteger)index;
- (void)selectProfile:(NSUInteger)index;

@end

@interface KarabinerKitCoreConfigurationModel2 : NSObject

@property(readonly) BOOL isLoaded;
- (BOOL)save;

@property BOOL globalConfigurationCheckForUpdatesOnStartup;
@property BOOL globalConfigurationShowInMenuBar;
@property BOOL globalConfigurationShowProfileNameInMenuBar;

@property(readonly) NSUInteger profilesCount;
- (NSString*)profileNameAtIndex:(NSUInteger)index;
- (void)setProfileNameAtIndex:(NSUInteger)index name:(NSString*)name;
- (BOOL)profileSelectedAtIndex:(NSUInteger)index;
- (void)selectProfileAtIndex:(NSUInteger)index;
- (void)addProfile;
- (void)removeProfileAtIndex:(NSUInteger)index;

@property(readonly) NSUInteger selectedProfileSimpleModificationsCount;
- (NSString*)selectedProfileSimpleModificationFirstAtIndex:(NSUInteger)index;
- (NSString*)selectedProfileSimpleModificationSecondAtIndex:(NSUInteger)index;
- (void)setSelectedProfileSimpleModificationAtIndex:(NSUInteger)index from:(NSString*)from to:(NSString*)to;
- (void)addSimpleModificationToSelectedProfile;
- (void)removeSelectedProfileSimpleModificationAtIndex:(NSUInteger)index;

@property(copy) NSString* selectedProfileVirtualHIDKeyboardKeyboardType;
@property NSInteger selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds;

@end
