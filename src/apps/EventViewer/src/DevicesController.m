#import "DevicesController.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface DevicesController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;

- (void)callback:(NSString*)filePath;

@end

static void staticCallback(const char* filePath,
                           void* context) {
  DevicesController* p = (__bridge DevicesController*)(context);
  if (p && filePath) {
    [p callback:[NSString stringWithUTF8String:filePath]];
  }
}

@implementation DevicesController

- (void)setup {
  libkrbn_enable_device_details_json_file_monitor(staticCallback,
                                                  (__bridge void*)(self));
}

- (void)dealloc {
  libkrbn_disable_device_details_json_file_monitor();
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
