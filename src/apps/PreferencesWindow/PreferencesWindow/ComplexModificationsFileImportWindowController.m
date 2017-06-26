#import "ComplexModificationsFileImportWindowController.h"
#import "ComplexModificationsRulesTableViewController.h"
#import "libkrbn.h"

@interface ComplexModificationsFileImportWindowController ()

@property(weak) IBOutlet ComplexModificationsRulesTableViewController* complexModificationsRulesTableViewController;
@property(weak) IBOutlet NSButton* importButton;
@property(weak) IBOutlet NSProgressIndicator* progressIndicator;
@property(weak) IBOutlet NSTextField* urlLabel;
@property(weak) IBOutlet NSWindow* preferencesWindow;
@property(unsafe_unretained) IBOutlet NSTextView* detailTextView;
@property NSData* jsonData;

@end

@implementation ComplexModificationsFileImportWindowController

- (void)setup:(NSString*)url {
  self.urlLabel.stringValue = url;
  self.importButton.enabled = NO;
  self.progressIndicator.hidden = NO;
  [self.progressIndicator startAnimation:nil];

  NSURLSession* session = [NSURLSession sharedSession];
  NSURLSessionDataTask* task = [session dataTaskWithURL:[NSURL URLWithString:url]
                                      completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
                                        dispatch_sync(dispatch_get_main_queue(), ^{
                                          self.jsonData = data;
                                          self.importButton.enabled = NO;
                                          self.progressIndicator.hidden = YES;
                                          [self.progressIndicator stopAnimation:nil];

                                          NSTextStorage* textStorage = self.detailTextView.textStorage;
                                          [textStorage beginEditing];

                                          if (error) {
                                            [textStorage setAttributedString:[[NSAttributedString alloc] initWithString:[error localizedDescription]]];

                                          } else {
                                            NSError* e;
                                            NSDictionary* jsonObject = [NSJSONSerialization JSONObjectWithData:data
                                                                                                       options:0
                                                                                                         error:&e];
                                            if (e) {
                                              [textStorage setAttributedString:[[NSAttributedString alloc] initWithString:[e localizedDescription]]];

                                            } else {
                                              self.importButton.enabled = YES;

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
                                            }
                                          }

                                          [textStorage endEditing];
                                        });
                                      }];
  [task resume];
}

- (void)show {
  [self.preferencesWindow beginSheet:self.window
                   completionHandler:^(NSModalResponse returnCode){}];
}

- (IBAction)importFile:(id)sender {
  NSString* filePath = [NSString stringWithFormat:@"%s/%ld.json", libkrbn_get_user_complex_modifications_assets_directory(), time(NULL)];
  [self.jsonData writeToFile:filePath atomically:NO];
  [self closePanel:nil];

  NSAlert* alert = [NSAlert new];
  alert.messageText = @"Imported";
  alert.informativeText = @"This file was imported successfully.";
  [alert addButtonWithTitle:@"OK"];

  [alert beginSheetModalForWindow:self.preferencesWindow
                completionHandler:^(NSModalResponse returnCode){}];
}

- (IBAction)closePanel:(id)sender {
  [self.preferencesWindow endSheet:self.window];
}

@end
