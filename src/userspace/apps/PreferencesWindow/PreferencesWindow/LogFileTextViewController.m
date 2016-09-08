#import "LogFileTextViewController.h"
#import "weakify.h"

@interface LogFileTextViewController ()

@property(unsafe_unretained) IBOutlet NSTextView* textView;
@property dispatch_source_t timer;
@property NSDate* fileModificationDate;

@end

@implementation LogFileTextViewController

- (void)monitor:(NSString*)filePath {
  self.textView.editable = NO;
  self.textView.richText = NO;

  self.timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
  if (self.timer) {
    dispatch_source_set_timer(self.timer, DISPATCH_TIME_NOW, 0.1 * NSEC_PER_SEC, 0);

    @weakify(self);
    dispatch_source_set_event_handler(self.timer, ^{
      @strongify(self);
      if (!self) return;

      NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:filePath error:nil];
      NSDate* fileModificationDate = [attributes fileModificationDate];
      if (!fileModificationDate) {
        return;
      }

      if (self.fileModificationDate == nil ||
          ![self.fileModificationDate isEqualToDate:fileModificationDate]) {
        NSLog(@"%@ updated", filePath);
        self.fileModificationDate = fileModificationDate;
        self.textView.string = [NSString stringWithContentsOfFile:filePath
                                                         encoding:NSUTF8StringEncoding
                                                            error:nil];
        [self.textView scrollRangeToVisible:NSMakeRange(self.textView.string.length, 0)];
      }
    });
    dispatch_resume(self.timer);
  }
}

@end
