#import "FrontmostApplicationController.h"
#import "frontmost_application_observer_objc.h"

@interface FrontmostApplicationController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;
@property krbn_frontmost_application_observer_objc* observer;

- (void)callback:(NSString*)bundleIdentifier
        filePath:(NSString*)filePath;

@end

static void staticCallback(const char* bundle_identifier,
                           const char* file_path,
                           void* context) {
  FrontmostApplicationController* p = (__bridge FrontmostApplicationController*)(context);
  [p callback:[NSString stringWithUTF8String:bundle_identifier]
      filePath:[NSString stringWithUTF8String:file_path]];
}

@implementation FrontmostApplicationController

- (void)setup {
  krbn_frontmost_application_observer_objc* p = nil;
  krbn_frontmost_application_observer_initialize(&p,
                                                 staticCallback,
                                                 (__bridge void*)(self));
  self.observer = p;
}

- (void)dealloc {
  krbn_frontmost_application_observer_objc* p = self.observer;
  krbn_frontmost_application_observer_terminate(&p);
  self.observer = nil;
}

- (void)callback:(NSString*)bundleIdentifier filePath:(NSString*)filePath {
  if ([@"org.pqrs.Karabiner.EventViewer" isEqualToString:bundleIdentifier] ||
      [@"org.pqrs.Karabiner-EventViewer" isEqualToString:bundleIdentifier]) {
    return;
  }

  NSTextStorage* textStorage = self.textView.textStorage;

  [textStorage beginEditing];

  // Clear if text is huge.
  if (textStorage.length > 1024 * 1024) {
    [textStorage setAttributedString:[[NSAttributedString alloc] initWithString:@""]];
  }

  NSString* bundleIdentifierLine = [NSString stringWithFormat:@"Bundle Identifier:  %@\n", bundleIdentifier];
  NSString* filePathLine = [NSString stringWithFormat:@"File Path:          %@\n\n", filePath];
  NSFont* font = [NSFont fontWithName:@"Menlo" size:11];
  [textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:bundleIdentifierLine
                                                                      attributes:@{NSFontAttributeName : font}]];
  [textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:filePathLine
                                                                      attributes:@{NSFontAttributeName : font}]];

  [textStorage endEditing];
  [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
}

@end
