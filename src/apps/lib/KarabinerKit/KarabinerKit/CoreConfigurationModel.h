// -*- Mode: objc -*-

@import Cocoa;
#import "libkrbn.h"

@interface VendorProductIdPair: NSObject

@property (assign, readonly) NSUInteger vendorId;
@property (assign, readonly) NSUInteger productId;

- (instancetype) initWithVendorId:(NSUInteger)vendorId productId:(NSUInteger)productId;
- (NSString *) toString;

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

@property(readonly) NSUInteger selectedProfileSimpleModificationsCount;
- (NSString*)selectedProfileSimpleModificationFirstAtIndex:(NSUInteger)index;
- (NSString*)selectedProfileSimpleModificationSecondAtIndex:(NSUInteger)index;
- (NSUInteger)selectedProfileSimpleModificationVendorIdAtIndex:(NSUInteger)index;
- (NSUInteger)selectedProfileSimpleModificationProductIdAtIndex:(NSUInteger)index;
- (BOOL)selectedProfileSimpleModificationDisabledAtIndex:(NSUInteger)index;
- (NSArray *)selectedProfileSimpleModificationVendorProductIdPairs;

- (void)setSelectedProfileSimpleModificationAtIndex:(NSUInteger)index from:(NSString*)from to:(NSString*)to;
- (void)setSelectedProfileSimpleModificationVendorProductIdAtIndex:(NSUInteger)index vendorId:(NSUInteger)vid productId:(NSUInteger)pid;
- (void)addSimpleModificationToSelectedProfile;
- (void)removeSelectedProfileSimpleModificationAtIndex:(NSUInteger)index;

@property(readonly) NSUInteger selectedProfileFnFunctionKeysCount;
- (NSString*)selectedProfileFnFunctionKeyFirstAtIndex:(NSUInteger)index;
- (NSString*)selectedProfileFnFunctionKeySecondAtIndex:(NSUInteger)index;
- (void)setSelectedProfileFnFunctionKey:(NSString*)from to:(NSString*)to;

- (void)selectedProfileDeviceProductManufacturerByVendorId:(NSUInteger)vendorId productId:(NSUInteger)productId product:(NSMutableString *)product manufacturer:(NSMutableString *)manufacturer;


- (BOOL)selectedProfileDeviceIgnore:(NSUInteger)vendorId
                          productId:(NSUInteger)productId
                         isKeyboard:(BOOL)isKeyboard
                   isPointingDevice:(BOOL)isPointingDevice;
- (void)setSelectedProfileDeviceIgnore:(NSUInteger)vendorId
                             productId:(NSUInteger)productId
                          manufacturer:(NSString*)manufacturer
                               product:(NSString*)product
                            isKeyboard:(BOOL)isKeyboard
                      isPointingDevice:(BOOL)isPointingDevice
                                 value:(BOOL)value;
- (BOOL)selectedProfileDeviceDisableBuiltInKeyboardIfExists:(NSUInteger)vendorId
                                                  productId:(NSUInteger)productId
                                                 isKeyboard:(BOOL)isKeyboard
                                           isPointingDevice:(BOOL)isPointingDevice;
- (void)setSelectedProfileDeviceDisableBuiltInKeyboardIfExists:(NSUInteger)vendorId
                                                     productId:(NSUInteger)productId
                                                  manufacturer:(NSString*)manufacturer
                                                       product:(NSString*)product
                                                    isKeyboard:(BOOL)isKeyboard
                                              isPointingDevice:(BOOL)isPointingDevice
                                                         value:(BOOL)value;

@property(copy) NSString* selectedProfileVirtualHIDKeyboardKeyboardType;
@property NSInteger selectedProfileVirtualHIDKeyboardCapsLockDelayMilliseconds;

@end
