// -*- Mode: objc -*-

@import Cocoa;
#import "DeviceModel.h"

@interface KarabinerKitGlobalConfiguration : NSObject

@property BOOL checkForUpdatesOnStartup;
@property BOOL showInMenubar;

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

// For JSON serialization
@property(copy, readonly) NSDictionary* simpleModificationsDictionary;
@property(copy, readonly) NSDictionary* fnFunctionKeysDictionary;
@property(copy, readonly) NSDictionary* virtualHIDKeyboardDictionary;
@property(copy, readonly) NSArray* devicesArray;

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

@end
