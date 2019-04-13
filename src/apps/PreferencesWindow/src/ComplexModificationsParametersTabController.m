#import "ComplexModificationsParametersTabController.h"
#import "KarabinerKit/KarabinerKit.h"

@interface ComplexModificationsParametersTabController ()

@property(weak) IBOutlet NSStepper* basicToIfAloneTimeoutMillisecondsStepper;
@property(weak) IBOutlet NSTextField* basicToIfAloneTimeoutMillisecondsText;
@property(weak) IBOutlet NSStepper* basicToIfHeldDownThresholdMillisecondsStepper;
@property(weak) IBOutlet NSTextField* basicToIfHeldDownThresholdMillisecondsText;
@property(weak) IBOutlet NSStepper* basicToDelayedActionDelayMillisecondsStepper;
@property(weak) IBOutlet NSTextField* basicToDelayedActionDelayMillisecondsText;
@property(weak) IBOutlet NSStepper* basicSimultaneousThresholdMillisecondsStepper;
@property(weak) IBOutlet NSTextField* basicSimultaneousThresholdMillisecondsText;
@property(weak) IBOutlet NSStepper* mouseMotionToScrollSpeedStepper;
@property(weak) IBOutlet NSTextField* mouseMotionToScrollSpeedText;
@property id configurationLoadedObserver;

@end

@implementation ComplexModificationsParametersTabController

- (void)setup {
  self.configurationLoadedObserver = [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                                                       object:nil
                                                                                        queue:[NSOperationQueue mainQueue]
                                                                                   usingBlock:^(NSNotification* note) {
                                                                                     [self updateValues];
                                                                                   }];

  [self updateValues];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self.configurationLoadedObserver];
}

- (void)updateValues {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;

  {
    int value = [coreConfigurationModel getSelectedProfileComplexModificationsParameter:@"basic.to_if_alone_timeout_milliseconds"];
    self.basicToIfAloneTimeoutMillisecondsStepper.integerValue = value;
    self.basicToIfAloneTimeoutMillisecondsText.integerValue = value;
  }
  {
    int value = [coreConfigurationModel getSelectedProfileComplexModificationsParameter:@"basic.to_if_held_down_threshold_milliseconds"];
    self.basicToIfHeldDownThresholdMillisecondsStepper.integerValue = value;
    self.basicToIfHeldDownThresholdMillisecondsText.integerValue = value;
  }
  {
    int value = [coreConfigurationModel getSelectedProfileComplexModificationsParameter:@"basic.to_delayed_action_delay_milliseconds"];
    self.basicToDelayedActionDelayMillisecondsStepper.integerValue = value;
    self.basicToDelayedActionDelayMillisecondsText.integerValue = value;
  }
  {
    int value = [coreConfigurationModel getSelectedProfileComplexModificationsParameter:@"basic.simultaneous_threshold_milliseconds"];
    self.basicSimultaneousThresholdMillisecondsStepper.integerValue = value;
    self.basicSimultaneousThresholdMillisecondsText.integerValue = value;
  }
  {
    int value = [coreConfigurationModel getSelectedProfileComplexModificationsParameter:@"mouse_motion_to_scroll.speed"];
    self.mouseMotionToScrollSpeedStepper.integerValue = value;
    self.mouseMotionToScrollSpeedText.integerValue = value;
  }
}

- (IBAction)valueChanged:(id)sender {
  {
    NSStepper* stepper = self.basicToIfAloneTimeoutMillisecondsStepper;
    NSTextField* text = self.basicToIfAloneTimeoutMillisecondsText;
    if (sender == stepper) {
      text.integerValue = stepper.integerValue;
    }
    if (sender == text) {
      stepper.integerValue = text.integerValue;
    }
  }
  {
    NSStepper* stepper = self.basicToIfHeldDownThresholdMillisecondsStepper;
    NSTextField* text = self.basicToIfHeldDownThresholdMillisecondsText;
    if (sender == stepper) {
      text.integerValue = stepper.integerValue;
    }
    if (sender == text) {
      stepper.integerValue = text.integerValue;
    }
  }
  {
    NSStepper* stepper = self.basicToDelayedActionDelayMillisecondsStepper;
    NSTextField* text = self.basicToDelayedActionDelayMillisecondsText;
    if (sender == stepper) {
      text.integerValue = stepper.integerValue;
    }
    if (sender == text) {
      stepper.integerValue = text.integerValue;
    }
  }
  {
    NSStepper* stepper = self.basicSimultaneousThresholdMillisecondsStepper;
    NSTextField* text = self.basicSimultaneousThresholdMillisecondsText;
    if (sender == stepper) {
      text.integerValue = stepper.integerValue;
    }
    if (sender == text) {
      stepper.integerValue = text.integerValue;
    }
  }
  {
    NSStepper* stepper = self.mouseMotionToScrollSpeedStepper;
    NSTextField* text = self.mouseMotionToScrollSpeedText;
    if (sender == stepper) {
      text.integerValue = stepper.integerValue;
    }
    if (sender == text) {
      stepper.integerValue = text.integerValue;
    }
  }

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel setSelectedProfileComplexModificationsParameter:@"basic.to_if_alone_timeout_milliseconds"
                                                                    value:self.basicToIfAloneTimeoutMillisecondsText.intValue];
  [coreConfigurationModel setSelectedProfileComplexModificationsParameter:@"basic.to_if_held_down_threshold_milliseconds"
                                                                    value:self.basicToIfHeldDownThresholdMillisecondsText.intValue];
  [coreConfigurationModel setSelectedProfileComplexModificationsParameter:@"basic.to_delayed_action_delay_milliseconds"
                                                                    value:self.basicToDelayedActionDelayMillisecondsText.intValue];
  [coreConfigurationModel setSelectedProfileComplexModificationsParameter:@"basic.simultaneous_threshold_milliseconds"
                                                                    value:self.basicSimultaneousThresholdMillisecondsText.intValue];
  [coreConfigurationModel setSelectedProfileComplexModificationsParameter:@"mouse_motion_to_scroll.speed"
                                                                    value:self.mouseMotionToScrollSpeedText.intValue];
  [coreConfigurationModel save];
}

@end
