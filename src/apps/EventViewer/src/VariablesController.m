#import "VariablesController.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface VariablesController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;

- (void)callback:(NSString*)filePath;

@end

static void staticCallback(const char* filePath,
                           void* context) {
  VariablesController* p = (__bridge VariablesController*)(context);
  if (p && filePath) {
    [p callback:[NSString stringWithUTF8String:filePath]];
  }
}

@implementation VariablesController

- (void)setup {
  libkrbn_enable_manipulator_environment_json_file_monitor(
      staticCallback,
      (__bridge void*)(self));
}

- (void)dealloc {
  libkrbn_disable_manipulator_environment_json_file_monitor();
}

- (void)callback:(NSString*)filePath {
  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    NSString* string = [NSString stringWithContentsOfFile:filePath
                                                 encoding:NSUTF8StringEncoding
                                                    error:nil];
    if (string) {
      NSTextStorage* textStorage = self.textView.textStorage;
      NSFont* font = [NSFont fontWithName:@"Menlo" size:11];

      [textStorage beginEditing];
      [textStorage setAttributedString:[[NSAttributedString alloc] initWithString:string
                                                                       attributes:@{
                                                                         NSForegroundColorAttributeName : NSColor.textColor,
                                                                         NSFontAttributeName : font,
                                                                       }]];
      [textStorage endEditing];
    }
  });
}

@end
