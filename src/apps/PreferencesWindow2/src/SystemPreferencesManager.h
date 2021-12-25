// -*- mode: objective-c -*-

@import Cocoa;
#import "SystemPreferencesModel.h"

@interface SystemPreferencesManager : NSObject

@property(readonly) SystemPreferencesModel* systemPreferencesModel;

- (void)setup;

- (void)updateSystemPreferencesValues:(SystemPreferencesModel*)model;

@end
