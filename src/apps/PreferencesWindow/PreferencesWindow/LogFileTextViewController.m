#import "LogFileTextViewController.h"
#import "weakify.h"

@interface LogFileTextViewController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;
@property(weak) IBOutlet NSTextField* currentTime;
@property dispatch_source_t timer;
@property NSDate* fileModificationDate;

@end

@implementation LogFileTextViewController

- (void)monitor:(NSString*)filePath {
  self.textView.editable = NO;
  self.textView.richText = NO;

  self.timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
  if (self.timer) {
    dispatch_source_set_timer(self.timer, DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC, 0);

    NSDateFormatter* formatter = [NSDateFormatter new];
    formatter.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
    formatter.dateFormat = @"yyyy-MM-dd HH:mm:ss";

    @weakify(self);
    dispatch_source_set_event_handler(self.timer, ^{
      @strongify(self);
      if (!self) return;

      self.currentTime.stringValue = [formatter stringFromDate:[NSDate date]];

      if ([[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
        NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:filePath error:nil];
        if (!attributes) {
          return;
        }

        NSDate* fileModificationDate = [attributes fileModificationDate];
        if (!fileModificationDate) {
          return;
        }

        if (self.fileModificationDate == nil ||
            ![self.fileModificationDate isEqualToDate:fileModificationDate]) {
          self.fileModificationDate = fileModificationDate;
          NSString* contents = [NSString stringWithContentsOfFile:filePath
                                                         encoding:NSUTF8StringEncoding
                                                            error:nil];
          if (!contents) {
            return;
          }

          self.textView.string = contents;
          [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
        }
      }
    });
    dispatch_resume(self.timer);
  }
}

@end
