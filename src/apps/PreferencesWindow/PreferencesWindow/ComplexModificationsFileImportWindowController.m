#import "ComplexModificationsFileImportWindowController.h"

@interface ComplexModificationsFileImportWindowController ()

@property(weak) IBOutlet NSWindow* preferencesWindow;
@property(weak) IBOutlet NSTextField* urlLabel;
@property(unsafe_unretained) IBOutlet NSTextView* detailTextView;

@end

@implementation ComplexModificationsFileImportWindowController

- (void)setup:(NSString*)url {
  self.urlLabel.stringValue = url;

  NSURLSession* session = [NSURLSession sharedSession];
  NSURLSessionDataTask* task = [session dataTaskWithURL:[NSURL URLWithString:url]
                                      completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
                                        if (error) {

                                        } else {
                                          NSError* e;
                                          NSDictionary* jsonObject = [NSJSONSerialization JSONObjectWithData:data
                                                                                                     options:0
                                                                                                       error:&e];
                                          dispatch_async(dispatch_get_main_queue(), ^{
                                            NSTextStorage* textStorage = self.detailTextView.textStorage;
                                            [textStorage beginEditing];
                                            [textStorage setAttributedString:[[NSAttributedString alloc] initWithString:@""]];

                                            // Title
                                            {
                                              NSString* title = jsonObject[@"title"];
                                              if (title) {
                                                NSString* s = [NSString stringWithFormat:@"%@\n", title];
                                                [textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:s]];
                                              }
                                            }

                                            // Descriptions
                                            {
                                              for (NSDictionary* rule in jsonObject[@"rules"]) {
                                                NSString* description = rule[@"description"];
                                                if (description) {
                                                  NSString* s = [NSString stringWithFormat:@"- %@\n", description];
                                                  [textStorage appendAttributedString:[[NSAttributedString alloc] initWithString:s]];
                                                }
                                              }
                                            }

                                            [textStorage endEditing];
                                          });
                                        }
                                      }];
  [task resume];
}

- (void)show {
  [self.preferencesWindow beginSheet:self.window
                   completionHandler:^(NSModalResponse returnCode){}];
}

- (IBAction)closePanel:(id)sender {
  [self.preferencesWindow endSheet:self.window];
}

@end
