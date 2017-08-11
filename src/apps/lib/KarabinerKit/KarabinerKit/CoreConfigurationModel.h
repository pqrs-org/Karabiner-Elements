// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

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
- (NSString*)selectedProfileSimpleModificationFirstAtIndex:(NSUInteger)index connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (NSString*)selectedProfileSimpleModificationSecondAtIndex:(NSUInteger)index connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (void)setSelectedProfileSimpleModificationAtIndex:(NSUInteger)index from:(NSString*)from to:(NSString*)to connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (void)addSimpleModificationToSelectedProfile:(NSInteger)connectedDeviceIndex;
- (void)removeSelectedProfileSimpleModificationAtIndex:(NSUInteger)index connectedDeviceIndex:(NSInteger)connectedDeviceIndex;

- (NSUInteger)selectedProfileFnFunctionKeysCount:(NSInteger)connectedDeviceIndex;
- (NSString*)selectedProfileFnFunctionKeyFirstAtIndex:(NSUInteger)index connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (NSString*)selectedProfileFnFunctionKeySecondAtIndex:(NSUInteger)index connectedDeviceIndex:(NSInteger)connectedDeviceIndex;
- (void)setSelectedProfileFnFunctionKey:(NSString*)from to:(NSString*)to connectedDeviceIndex:(NSInteger)connectedDeviceIndex;

@property(readonly) NSUInteger selectedProfileComplexModificationsRulesCount;
- (NSString*)selectedProfileComplexModificationsRuleDescription:(NSUInteger)index;
- (void)removeSelectedProfileComplexModificationsRule:(NSUInteger)index;
- (void)swapSelectedProfileComplexModificationsRules:(NSUInteger)index1 index2:(NSUInteger)index2;
- (int)getSelectedProfileComplexModificationsParameter:(NSString*)name;
- (void)setSelectedProfileComplexModificationsParameter:(NSString*)name value:(int)value;

- (BOOL)selectedProfileDeviceIgnore:(libkrbn_device_identifiers*)deviceIdentifiers;
- (void)setSelectedProfileDeviceIgnore:(libkrbn_device_identifiers*)deviceIdentifiers
                                 value:(BOOL)value;
- (BOOL)selectedProfileDeviceDisableBuiltInKeyboardIfExists:(libkrbn_device_identifiers*)deviceIdentifiers;
- (void)setSelectedProfileDeviceDisableBuiltInKeyboardIfExists:(libkrbn_device_identifiers*)deviceIdentifiers
                                                         value:(BOOL)value;

@property(copy) NSString* selectedProfileVirtualHIDKeyboardKeyboardType;
@property NSInteger selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds;

@end
