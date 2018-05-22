#import "VariablesController.h"
#import "libkrbn.h"

@interface VariablesController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;
@property libkrbn_file_monitor* libkrbn_file_monitor;

- (void)callback;

@end

static void staticCallback(void* context) {
  VariablesController* p = (__bridge VariablesController*)(context);
  [p callback];
}

@implementation VariablesController

- (void)setup {
  libkrbn_file_monitor* p = nil;
  libkrbn_file_monitor_initialize(&p,
                                  libkrbn_get_manipulator_environment_json_file_path(),
                                  staticCallback,
                                  (__bridge void*)(self));
  self.libkrbn_file_monitor = p;
}

- (void)dealloc {
  libkrbn_file_monitor* p = self.libkrbn_file_monitor;
  libkrbn_file_monitor_terminate(&p);
  self.libkrbn_file_monitor = nil;
}

- (void)callback {
  NSString* string = [NSString stringWithContentsOfFile:[NSString stringWithUTF8String:libkrbn_get_manipulator_environment_json_file_path()]
                                               encoding:NSUTF8StringEncoding
                                                  error:nil];
  if (string) {
    NSTextStorage* textStorage = self.textView.textStorage;
    NSFont* font = [NSFont fontWithName:@"Menlo" size:11];

    [textStorage beginEditing];
    [textStorage setAttributedString:[[NSAttributedString alloc] initWithString:string
                                                                     attributes:@{NSFontAttributeName : font}]];
    [textStorage endEditing];
  }
}

@end
