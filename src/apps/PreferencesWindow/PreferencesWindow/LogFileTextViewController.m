#import "LogFileTextViewController.h"
#include "libkrbn.h"
#import "weakify.h"

@interface LogFileTextViewController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;
@property(weak) IBOutlet NSTextField* currentTime;
@property dispatch_source_t timer;
@property NSDate* fileModificationDate;
@property libkrbn_log_monitor* libkrbn_log_monitor;

- (void)appendLogLine:(const char*)line;

@end

static void log_updated_callback(const char* line, void* refcon) {
  LogFileTextViewController* controller = (__bridge LogFileTextViewController*)(refcon);
  [controller appendLogLine:line];
}

@implementation LogFileTextViewController

- (void)monitor {
  self.textView.editable = NO;
  self.textView.richText = NO;

  // ----------------------------------------
  // setup libkrbn_log_monitor

  libkrbn_log_monitor* p = NULL;
  if (!libkrbn_log_monitor_initialize(&p, log_updated_callback, (__bridge void*)(self))) {
    return;
  }
  self.libkrbn_log_monitor = p;

  // update textView

  [self.textView.textStorage beginEditing];
  {
    size_t size = libkrbn_log_monitor_initial_lines_size(self.libkrbn_log_monitor);
    for (size_t i = 0; i < size; ++i) {
      const char* line = libkrbn_log_monitor_initial_line(self.libkrbn_log_monitor, i);
      if (line) {
        [self.textView.textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:[self logLineString:line]]];
        [self.textView.textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:@"\n"]];
      }
    }
  }
  [self.textView.textStorage endEditing];
  [self scrollToBottom];

  libkrbn_log_monitor_start(self.libkrbn_log_monitor);

  // ----------------------------------------
  // setup timer_.

  self.timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
  if (self.timer) {
    dispatch_source_set_timer(self.timer, DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC, 0);

    NSDateFormatter* formatter = [NSDateFormatter new];
    formatter.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
    formatter.dateFormat = @"[yyyy-MM-dd HH:mm:ss]";

    @weakify(self);
    dispatch_source_set_event_handler(self.timer, ^{
      @strongify(self);
      if (!self) return;

      self.currentTime.stringValue = [formatter stringFromDate:[NSDate date]];
    });
    dispatch_resume(self.timer);
  }
}

- (void)dealloc {
  if (self.libkrbn_log_monitor) {
    libkrbn_log_monitor* p = self.libkrbn_log_monitor;
    libkrbn_log_monitor_terminate(&p);
  }
}

- (NSString*)logLineString:(const char*)line {
  NSString* lineString = [NSString stringWithUTF8String:line];
  if (!lineString) {
    lineString = @"(This line contains corrupted characters.)";
  }
  return lineString;
}

- (void)appendLogLine:(const char*)line {
  NSString* lineString = [self logLineString:line];

  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) return;

    [self.textView.textStorage beginEditing];
    [self.textView.textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:lineString]];
    [self.textView.textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:@"\n"]];
    [self.textView.textStorage endEditing];

    [self scrollToBottom];
  });
}

- (void)scrollToBottom {
  [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
}

@end
