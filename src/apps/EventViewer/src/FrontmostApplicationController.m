#import "FrontmostApplicationController.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface FrontmostApplicationController ()

@property NSMutableAttributedString* text;
@property(unsafe_unretained) IBOutlet NSTextView* textView;

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
  self.text = [NSMutableAttributedString new];

  libkrbn_enable_frontmost_application_monitor(staticCallback,
                                               (__bridge void*)(self));
}

- (void)dealloc {
  libkrbn_disable_frontmost_application_monitor();
}

- (void)callback:(NSString*)bundleIdentifier filePath:(NSString*)filePath {
  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    // Update self.text

    if (![@"org.pqrs.Karabiner.EventViewer" isEqualToString:bundleIdentifier] &&
        ![@"org.pqrs.Karabiner-EventViewer" isEqualToString:bundleIdentifier]) {
      // Clear if text is huge.
      if (self.text.length > 4 * 1024) {
        [self.text setAttributedString:[[NSAttributedString alloc] initWithString:@""]];
      }

      NSString* bundleIdentifierLine = [NSString stringWithFormat:@"Bundle Identifier:  %@\n", bundleIdentifier];
      NSString* filePathLine = [NSString stringWithFormat:@"File Path:          %@\n\n", filePath];
      NSDictionary* attributes = @{
        NSForegroundColorAttributeName : NSColor.textColor,
        NSFontAttributeName : [NSFont fontWithName:@"Menlo" size:11],
      };

      [self.text appendAttributedString:[[NSAttributedString alloc] initWithString:bundleIdentifierLine
                                                                        attributes:attributes]];
      [self.text appendAttributedString:[[NSAttributedString alloc] initWithString:filePathLine
                                                                        attributes:attributes]];
    }

    // Update self.textView

    NSTextStorage* textStorage = self.textView.textStorage;
    [textStorage beginEditing];
    if (self.text.length == 0) {
      NSString* placeholder = @"Please switch to apps which you want to know Bundle Identifier.";
      [textStorage setAttributedString:[[NSAttributedString alloc] initWithString:placeholder]];
    } else {
      [textStorage setAttributedString:self.text];
    }
    [textStorage endEditing];

    [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
  });
}

@end
