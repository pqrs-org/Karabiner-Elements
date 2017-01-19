// -*- Mode: objc -*-

@import Cocoa;
#import "DeviceModel.h"

@interface KarabinerKitGlobalConfiguration : NSObject

@property BOOL checkForUpdatesOnStartup;
@property BOOL showInMenubar;

@end

@interface KarabinerKitDeviceConfiguration : NSObject

@property(readonly) KarabinerKitDeviceIdentifiers* deviceIdentifiers;
@property BOOL ignore;
@property BOOL disableBuiltInKeyboardIfExists;

@end

@interface KarabinerKitCoreConfigurationModel : NSObject

@property KarabinerKitGlobalConfiguration* globalConfiguration;
@property(copy, readonly) NSArray<NSDictionary*>* simpleModifications;
@property(copy, readonly) NSArray<NSDictionary*>* fnFunctionKeys;
@property(copy, readonly) NSArray<KarabinerKitDeviceConfiguration*>* devices;
@property(copy, readonly) NSDictionary* simpleModificationsDictionary;
@property(copy, readonly) NSDictionary* fnFunctionKeysDictionary;
@property(copy, readonly) NSDictionary* virtualHIDKeyboardDictionary;
@property(copy, readonly) NSArray* devicesArray;
@property(copy) NSString* virtualHIDKeyboardType;
@property NSUInteger virtualHIDKeyboardCapsLockDelayMilliseconds;

- (instancetype)initWithProfile:(NSDictionary*)jsonObject currentProfileJsonObject:(NSDictionary*)profile;

- (void)addSimpleModification;
- (void)removeSimpleModification:(NSUInteger)index;
- (void)replaceSimpleModification:(NSUInteger)index from:(NSString*)from to:(NSString*)to;

- (void)replaceFnFunctionKey:(NSString*)from to:(NSString*)to;

- (void)setDeviceConfiguration:(KarabinerKitDeviceIdentifiers*)deviceIdentifiers
                            ignore:(BOOL)ignore
    disableBuiltInKeyboardIfExists:(BOOL)disableBuiltInKeyboardIfExists;

@end
