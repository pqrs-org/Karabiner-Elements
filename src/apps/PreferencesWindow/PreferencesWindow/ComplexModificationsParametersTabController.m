#import "ComplexModificationsParametersTabController.h"
#import "KarabinerKit/KarabinerKit.h"

@interface ComplexModificationsParametersTabController ()

@property(weak) IBOutlet NSStepper* basicToIfAloneTimeoutMillisecondsStepper;
@property(weak) IBOutlet NSTextField* basicToIfAloneTimeoutMillisecondsText;

@end

@implementation ComplexModificationsParametersTabController

- (void)setup {
  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  [self updateValues];
                                                }];

  [self updateValues];
}

- (void)updateValues {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;

  {
    int value = [coreConfigurationModel getSelectedProfileComplexModificationsParameter:@"basic.to_if_alone_timeout_milliseconds"];
    self.basicToIfAloneTimeoutMillisecondsStepper.integerValue = value;
    self.basicToIfAloneTimeoutMillisecondsText.integerValue = value;
  }
}

- (IBAction)valueChanged:(id)sender {
  if (sender == self.basicToIfAloneTimeoutMillisecondsStepper) {
    self.basicToIfAloneTimeoutMillisecondsText.integerValue = self.basicToIfAloneTimeoutMillisecondsStepper.integerValue;
  }
  if (sender == self.basicToIfAloneTimeoutMillisecondsText) {
    self.basicToIfAloneTimeoutMillisecondsStepper.integerValue = self.basicToIfAloneTimeoutMillisecondsText.integerValue;
  }

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel setSelectedProfileComplexModificationsParameter:@"basic.to_if_alone_timeout_milliseconds"
                                                                    value:self.basicToIfAloneTimeoutMillisecondsText.intValue];
  [coreConfigurationModel save];
}

@end
