#import "LogFileTextViewController.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

typedef enum {
  LogLevelInfo,
  LogLevelWarn,
  LogLevelError,
} LogLevel;

@interface LogFileTextViewController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;
@property(weak) IBOutlet NSTextField* currentTime;
@property(weak) IBOutlet NSTabViewItem* logTabViewItem;
@property dispatch_source_t timer;
@property NSDate* fileModificationDate;

- (NSMutableAttributedString*)logLinesString:(libkrbn_log_lines*)logLines;
- (void)updateLogLines:(NSAttributedString*)logLinesString;

@end

static void log_updated_callback(libkrbn_log_lines* logLines, void* refcon) {
  if (logLines) {
    LogFileTextViewController* controller = (__bridge LogFileTextViewController*)(refcon);
    if (controller) {
      NSMutableAttributedString* logLinesString = [controller logLinesString:logLines];
      [controller updateLogLines:logLinesString];
    }
  }
}

@implementation LogFileTextViewController

- (void)monitor {
  // ----------------------------------------
  // setup libkrbn_log_monitor

  libkrbn_enable_log_monitor(log_updated_callback,
                             (__bridge void*)(self));

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
  libkrbn_disable_log_monitor();
}

- (void)updateLogLines:(NSAttributedString*)logLinesString {
  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    [self.textView.textStorage beginEditing];
    [self.textView.textStorage setAttributedString:logLinesString];
    [self.textView.textStorage endEditing];

    [self scrollToBottom];
  });
}

- (LogLevel)getLogLevel:(const char*)line {
  if (libkrbn_log_lines_is_warn_line(line)) {
    return LogLevelWarn;
  }
  if (libkrbn_log_lines_is_error_line(line)) {
    return LogLevelError;
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
  if (level == LogLevelError) {
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
  if (level == LogLevelError) {
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

- (NSMutableAttributedString*)logLinesString:(libkrbn_log_lines*)logLines {
  NSMutableAttributedString* result = [NSMutableAttributedString new];

  if (logLines) {
    size_t size = libkrbn_log_lines_get_size(logLines);
    for (size_t i = 0; i < size; ++i) {
      const char* line = libkrbn_log_lines_get_line(logLines, i);
      if (line) {
        LogLevel level = [self getLogLevel:line];
        NSDictionary* attributes = [self stringAttributes:level];

        NSString* lineString = [NSString stringWithCString:line
                                                  encoding:NSUTF8StringEncoding];
        if (lineString) {
          [result appendAttributedString:[[NSAttributedString alloc] initWithString:lineString
                                                                         attributes:attributes]];
          [result appendAttributedString:[[NSAttributedString alloc] initWithString:@"\n"
                                                                         attributes:attributes]];
        }
      }
    }
  }

  return result;
}

- (void)scrollToBottom {
  [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
}

@end
