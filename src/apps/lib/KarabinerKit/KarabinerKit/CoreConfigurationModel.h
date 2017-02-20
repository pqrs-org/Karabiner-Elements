// -*- Mode: objc -*-

@import Cocoa;
#import "DeviceModel.h"

@interface KarabinerKitCoreConfigurationModel : NSObject

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
@property(readonly) NSString* selectedProfileName;
- (void)addProfile;
- (void)removeProfileAtIndex:(NSUInteger)index;

@property(readonly) NSUInteger selectedProfileSimpleModificationsCount;
- (NSString*)selectedProfileSimpleModificationFirstAtIndex:(NSUInteger)index;
- (NSString*)selectedProfileSimpleModificationSecondAtIndex:(NSUInteger)index;
- (void)setSelectedProfileSimpleModificationAtIndex:(NSUInteger)index from:(NSString*)from to:(NSString*)to;
- (void)addSimpleModificationToSelectedProfile;
- (void)removeSelectedProfileSimpleModificationAtIndex:(NSUInteger)index;

@property(readonly) NSUInteger selectedProfileFnFunctionKeysCount;
- (NSString*)selectedProfileFnFunctionKeyFirstAtIndex:(NSUInteger)index;
- (NSString*)selectedProfileFnFunctionKeySecondAtIndex:(NSUInteger)index;
- (void)setSelectedProfileFnFunctionKey:(NSString*)from to:(NSString*)to;

- (BOOL)selectedProfileDeviceIgnore:(NSUInteger)vendorId
                          productId:(NSUInteger)productId
                         isKeyboard:(BOOL)isKeyboard
                   isPointingDevice:(BOOL)isPointingDevice;
- (void)setSelectedProfileDeviceIgnore:(NSUInteger)vendorId
                             productId:(NSUInteger)productId
                            isKeyboard:(BOOL)isKeyboard
                      isPointingDevice:(BOOL)isPointingDevice
                                 value:(BOOL)value;
- (BOOL)selectedProfileDeviceDisableBuiltInKeyboardIfExists:(NSUInteger)vendorId
                                                  productId:(NSUInteger)productId
                                                 isKeyboard:(BOOL)isKeyboard
                                           isPointingDevice:(BOOL)isPointingDevice;
- (void)setSelectedProfileDeviceDisableBuiltInKeyboardIfExists:(NSUInteger)vendorId
                                                     productId:(NSUInteger)productId
                                                    isKeyboard:(BOOL)isKeyboard
                                              isPointingDevice:(BOOL)isPointingDevice
                                                         value:(BOOL)value;

@property(copy) NSString* selectedProfileVirtualHIDKeyboardKeyboardType;
@property NSInteger selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds;

@end
