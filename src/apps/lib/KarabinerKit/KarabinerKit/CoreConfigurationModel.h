// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface KarabinerKitCoreConfigurationSimpleModificationsDefinition : NSObject

- (instancetype)init;
- (instancetype)initWithDefinition:(libkrbn_simple_modifications_definition*)definition;

- (libkrbn_simple_modifications_definition)toLibkrbnDefinition;

@property(copy) NSString* type;
@property(copy) NSString* value;

@end

@interface KarabinerKitCoreConfigurationModel : NSObject

- (instancetype)initWithInitializedCoreConfiguration:(libkrbn_core_configuration*)initializedCoreConfiguration;

- (BOOL)save;

@property BOOL globalConfigurationCheckForUpdatesOnStartup;
@property BOOL globalConfigurationShowInMenuBar;
@property BOOL globalConfigurationShowProfileNameInMenuBar;

@property(readonly) NSUInteger profilesCount;
- (NSString*)profileNameAtIndex:(NSUInteger)index;
- (void)setProfileNameAtIndex:(NSUInteger)index name:(NSString*)name;
- (BOOL)profileSelectedAtIndex:(NSUInteger)index;
- (void)selectProfileAtIndex:(NSUInteger)index;
@property(readonly) NSString* selectedProfileName;
- (void)addProfile;
- (void)removeProfileAtIndex:(NSUInteger)index;

- (NSUInteger)selectedProfileSimpleModificationsCount:(NSInteger)connectedDeviceIndex;
- (KarabinerKitCoreConfigurationSimpleModificationsDefinition*)selectedProfileSimpleModificationFirstAtIndex:(NSUInteger)index
                                                                                        connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (KarabinerKitCoreConfigurationSimpleModificationsDefinition*)selectedProfileSimpleModificationSecondAtIndex:(NSUInteger)index
                                                                                         connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (void)setSelectedProfileSimpleModificationAtIndex:(NSUInteger)index
                                               from:(KarabinerKitCoreConfigurationSimpleModificationsDefinition*)from
                                                 to:(KarabinerKitCoreConfigurationSimpleModificationsDefinition*)to
                               connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (void)addSimpleModificationToSelectedProfile:(NSInteger)connectedDeviceIndex;
- (void)removeSelectedProfileSimpleModificationAtIndex:(NSUInteger)index connectedDeviceIndex:(NSInteger)connectedDeviceIndex;

- (NSUInteger)selectedProfileFnFunctionKeysCount:(NSInteger)connectedDeviceIndex;
- (KarabinerKitCoreConfigurationSimpleModificationsDefinition*)selectedProfileFnFunctionKeyFirstAtIndex:(NSUInteger)index
                                                                                   connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (KarabinerKitCoreConfigurationSimpleModificationsDefinition*)selectedProfileFnFunctionKeySecondAtIndex:(NSUInteger)index
                                                                                    connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (void)setSelectedProfileFnFunctionKey:(KarabinerKitCoreConfigurationSimpleModificationsDefinition*)from
                                     to:(KarabinerKitCoreConfigurationSimpleModificationsDefinition*)to
                   connectedDeviceIndex:(NSInteger)connectedDeviceIndex;

@property(readonly) NSUInteger selectedProfileComplexModificationsRulesCount;
- (NSString*)selectedProfileComplexModificationsRuleDescription:(NSUInteger)index;
- (void)removeSelectedProfileComplexModificationsRule:(NSUInteger)index;
- (void)swapSelectedProfileComplexModificationsRules:(NSUInteger)index1 index2:(NSUInteger)index2;
- (int)getSelectedProfileComplexModificationsParameter:(NSString*)name;
- (void)setSelectedProfileComplexModificationsParameter:(NSString*)name value:(int)value;

- (BOOL)selectedProfileDeviceIgnore:(const libkrbn_device_identifiers*)deviceIdentifiers;
- (void)setSelectedProfileDeviceIgnore:(const libkrbn_device_identifiers*)deviceIdentifiers
                                 value:(BOOL)value;
- (BOOL)selectedProfileDeviceManipulateCapsLockLed:(const libkrbn_device_identifiers*)deviceIdentifiers;
- (void)setSelectedProfileDeviceManipulateCapsLockLed:(const libkrbn_device_identifiers*)deviceIdentifiers
                                                value:(BOOL)value;
- (BOOL)selectedProfileDeviceDisableBuiltInKeyboardIfExists:(const libkrbn_device_identifiers*)deviceIdentifiers;
- (void)setSelectedProfileDeviceDisableBuiltInKeyboardIfExists:(const libkrbn_device_identifiers*)deviceIdentifiers
                                                         value:(BOOL)value;

@property(copy) NSString* selectedProfileVirtualHIDKeyboardKeyboardType;
@property NSInteger selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds;

@end
