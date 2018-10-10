#import "LogFileTextViewController.h"
#import "libkrbn.h"
#import "weakify.h"

typedef enum {
  LogLevelInfo,
  LogLevelWarn,
  LogLevelErr,
} LogLevel;

@interface LogFileTextViewController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;
@property(weak) IBOutlet NSTextField* currentTime;
@property(weak) IBOutlet NSTabViewItem* logTabViewItem;
@property dispatch_source_t timer;
@property NSDate* fileModificationDate;
@property libkrbn_log_monitor* libkrbn_log_monitor;
@property NSInteger errorLogCount;

- (void)appendLogLine:(const char*)line;

@end

static void log_updated_callback(const char* line, void* refcon) {
  LogFileTextViewController* controller = (__bridge LogFileTextViewController*)(refcon);
  if (!controller) {
    return;
  }

  NSString* lineString = [controller logLineString:line];

  @weakify(controller);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(controller);
    if (!controller) {
      return;
    }

    [controller appendLogLine:lineString];
  });
}

@implementation LogFileTextViewController

- (void)monitor {
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
      NSString* line = [self logLineString:libkrbn_log_monitor_initial_line(self.libkrbn_log_monitor, i)];
      LogLevel level = [self getLogLevel:line];
      if (line) {
        [self.textView.textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:[self logLineString:line]
                                                                                          attributes:[self stringAttributes:level]]];
        [self.textView.textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:@"\n"
                                                                                          attributes:[self stringAttributes:level]]];
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

- (void)resetErrorLogCount {
  self.errorLogCount = 0;
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

- (void)appendLogLine:(NSString*)line {
  LogLevel level = [self getLogLevel:line];

  if (level == LogLevelErr) {
    ++(self.errorLogCount);
  }

  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    [self.textView.textStorage beginEditing];

    // Clear if text is huge.
    if (self.textView.textStorage.length > 1024 * 1024) {
      [self.textView.textStorage setAttributedString:[[NSAttributedString alloc] initWithString:@""]];
    }

    [self.textView.textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:line
                                                                                      attributes:[self stringAttributes:level]]];
    [self.textView.textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:@"\n"
                                                                                      attributes:[self stringAttributes:level]]];
    [self.textView.textStorage endEditing];

    [self scrollToBottom];
    [self updateTabLabel];
  });
}

- (void)updateTabLabel {
  if (self.logTabViewItem.tabState == NSSelectedTab) {
    [self resetErrorLogCount];
  } else {
    if (self.errorLogCount > 0) {
      self.logTabViewItem.label = [NSString stringWithFormat:@"Log \u2666%ld", self.errorLogCount];
      return;
    }
  }

  self.logTabViewItem.label = @"Log";
}

- (LogLevel)getLogLevel:(NSString*)line {
  if (libkrbn_is_warn_log([line UTF8String])) {
    return LogLevelWarn;
  }
  if (libkrbn_is_err_log([line UTF8String])) {
    return LogLevelErr;
  }
  return LogLevelInfo;
}

- (NSColor*)getBackgroundColor:(LogLevel)level {
  if (level == LogLevelWarn) {
    // #fcf8e3
    return [NSColor colorWithRed:(CGFloat)(0xfc) / 255
                           green:(CGFloat)(0xf8) / 255
                            blue:(CGFloat)(0xe3) / 255
                           alpha:1.0];
  }
  if (level == LogLevelErr) {
    // #f2dede
    return [NSColor colorWithRed:(CGFloat)(0xf2) / 255
                           green:(CGFloat)(0xde) / 255
                            blue:(CGFloat)(0xde) / 255
                           alpha:1.0];
  }
  return [NSColor textBackgroundColor];
}

- (NSColor*)getForegroundColor:(LogLevel)level {
  if (level == LogLevelWarn) {
    // #8a6d3b
    return [NSColor colorWithRed:(CGFloat)(0x8a) / 255
                           green:(CGFloat)(0x6d) / 255
                            blue:(CGFloat)(0x3b) / 255
                           alpha:1.0];
  }
  if (level == LogLevelErr) {
    // #a94442
    return [NSColor colorWithRed:(CGFloat)(0xa9) / 255
                           green:(CGFloat)(0x44) / 255
                            blue:(CGFloat)(0x42) / 255
                           alpha:1.0];
  }
  return [NSColor textColor];
}

- (NSDictionary*)stringAttributes:(LogLevel)level {
  return @{
    NSFontAttributeName : [NSFont fontWithName:@"Menlo" size:11],
    NSBackgroundColorAttributeName : [self getBackgroundColor:level],
    NSForegroundColorAttributeName : [self getForegroundColor:level],
  };
}

- (void)scrollToBottom {
  [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
}

@end
