#import "VariablesController.h"
#import "libkrbn.h"

@interface VariablesController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;
@property libkrbn_manipulator_environment_monitor* libkrbn_manipulator_environment_monitor;

- (void)callback;

@end

static void staticCallback(void* context) {
  VariablesController* p = (__bridge VariablesController*)(context);
  [p callback];
}

@implementation VariablesController

- (void)setup {
  libkrbn_manipulator_environment_monitor* p = nil;
  libkrbn_manipulator_environment_monitor_initialize(&p,
                                                     staticCallback,
                                                     (__bridge void*)(self));
  self.libkrbn_manipulator_environment_monitor = p;
}

- (void)dealloc {
  libkrbn_manipulator_environment_monitor* p = self.libkrbn_manipulator_environment_monitor;
  libkrbn_manipulator_environment_monitor_terminate(&p);
  self.libkrbn_manipulator_environment_monitor = nil;
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
