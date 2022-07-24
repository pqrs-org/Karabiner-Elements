#import "OpenAtLoginObjc.h"

@implementation OpenAtLoginObjc

+ (LSSharedFileListItemRef)copyLSSharedFileListItemRef:(LSSharedFileListRef)loginItems appURL:(NSURL*)appURL {
  if (!loginItems) return NULL;

  LSSharedFileListItemRef retval = NULL;

  UInt32 seed = 0U;
  CFArrayRef currentLoginItemsRef = LSSharedFileListCopySnapshot(loginItems, &seed);
  NSArray* currentLoginItems = CFBridgingRelease(currentLoginItemsRef);
  for (id itemObject in currentLoginItems) {
    LSSharedFileListItemRef item = (__bridge LSSharedFileListItemRef)itemObject;

    UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
    CFURLRef urlRef = NULL;
    OSStatus err = LSSharedFileListItemResolve(item, resolutionFlags, &urlRef, NULL);
    if (err == noErr) {
      NSURL* url = CFBridgingRelease(urlRef);
      BOOL foundIt = [url isEqual:appURL];

      if (foundIt) {
        retval = item;
        break;
      }
    }
  }

  if (retval) {
    CFRetain(retval);
  }

  return retval;
}

+ (LSSharedFileListRef)createLoginItems {
  return LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
}

+ (BOOL)enabled:(NSURL*)appURL {
  BOOL result = NO;

  LSSharedFileListRef loginItems = [self createLoginItems];
  if (loginItems) {
    LSSharedFileListItemRef item = [self copyLSSharedFileListItemRef:loginItems appURL:appURL];
    if (item) {
      result = YES;

      CFRelease(item);
    }

    CFRelease(loginItems);
  }

  return result;
}

+ (void)enable:(NSURL*)appURL {
  if ([self enabled:appURL]) {
    return;
  }

  LSSharedFileListRef loginItems = [self createLoginItems];
  if (loginItems) {
    LSSharedFileListItemRef item = LSSharedFileListInsertItemURL(loginItems, kLSSharedFileListItemLast, NULL, NULL, (__bridge CFURLRef)(appURL), NULL, NULL);
    if (item) {
      CFRelease(item);
    }

    CFRelease(loginItems);
  }
}

+ (void)disable:(NSURL*)appURL {
  LSSharedFileListRef loginItems = [self createLoginItems];
  if (loginItems) {
    LSSharedFileListItemRef item = [self copyLSSharedFileListItemRef:loginItems appURL:appURL];
    if (item) {
      LSSharedFileListItemRemove(loginItems, item);

      CFRelease(item);
    }

    CFRelease(loginItems);
  }
}

@end
