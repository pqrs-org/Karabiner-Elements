// -*- Mode: objc -*-

@import Cocoa;

@interface UpdaterController : NSObject

+ (void)checkForUpdatesInBackground;
+ (void)checkForUpdatesStableOnly;
+ (void)checkForUpdatesWithBetaVersion;

@end
