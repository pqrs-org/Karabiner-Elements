#import "EventQueue.h"
#import "KarabinerKit/KarabinerKit.h"
#import "PreferencesKeys.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface EventQueue ()

@property NSMutableArray* queue;
@property NSMutableDictionary<NSNumber*, NSMutableSet<NSString*>*>* modifierFlags;
@property(copy) NSString* simpleModificationJsonString;
@property(weak) IBOutlet NSTableView* view;
@property(weak) IBOutlet NSButton* addSimpleModificationButton;

- (void)pushKeyEvent:(NSString*)code
                name:(NSString*)name
           eventType:(NSString*)eventType
                misc:(NSString*)misc;
- (void)updateAddSimpleModificationButton:(NSString*)title;

@end

static void hid_value_observer_callback(uint64_t device_id,
                                        enum libkrbn_hid_value_type type,
                                        uint32_t value,
                                        enum libkrbn_hid_value_event_type event_type,
                                        void* refcon) {
  EventQueue* queue = (__bridge EventQueue*)(refcon);
  if (!queue) {
    return;
  }

  @weakify(queue);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(queue);
    if (!queue) {
      return;
    }

    NSNumber* deviceId = @(device_id);

    NSString* code = nil;
    if ([[NSUserDefaults standardUserDefaults] boolForKey:kShowHex]) {
      code = [NSString stringWithFormat:@"0x%02x", value];
    } else {
      code = [NSString stringWithFormat:@"%d", value];
    }
    NSString* keyType = @"";
    NSMutableDictionary* simpleModificationJson = [NSMutableDictionary new];

    char buffer[256];
    buffer[0] = '\0';
    NSString* name = @"";

    switch (type) {
      case libkrbn_hid_value_type_key_code: {
        keyType = @"key";
        libkrbn_get_key_code_name(buffer, sizeof(buffer), value);
        name = [NSString stringWithUTF8String:buffer];

        NSNumber* unnamedNumber = nil;
        {
          uint32_t unnamedKeyCodeNumber = 0;
          if (libkrbn_find_unnamed_key_code_number(buffer, &unnamedKeyCodeNumber)) {
            unnamedNumber = [NSNumber numberWithInteger:unnamedKeyCodeNumber];
          }
        }

        if (libkrbn_is_modifier_flag(value)) {
          NSMutableSet* set = queue.modifierFlags[deviceId];
          if (!set) {
            set = [NSMutableSet new];
            queue.modifierFlags[deviceId] = set;
          }

          if (event_type == libkrbn_hid_value_event_type_key_down) {
            [set addObject:name];
          } else {
            [set removeObject:name];
          }
        }

        simpleModificationJson[@"from"] = [NSMutableDictionary new];
        simpleModificationJson[@"from"][@"key_code"] = unnamedNumber ? unnamedNumber : name;
        break;
      }

      case libkrbn_hid_value_type_consumer_key_code: {
        keyType = @"consumer_key";
        libkrbn_get_consumer_key_code_name(buffer, sizeof(buffer), value);
        name = [NSString stringWithUTF8String:buffer];

        NSNumber* unnamedNumber = nil;
        {
          uint32_t unnamedConsumerKeyCodeNumber = 0;
          if (libkrbn_find_unnamed_consumer_key_code_number(buffer, &unnamedConsumerKeyCodeNumber)) {
            unnamedNumber = [NSNumber numberWithInteger:unnamedConsumerKeyCodeNumber];
          }
        }

        simpleModificationJson[@"from"] = [NSMutableDictionary new];
        simpleModificationJson[@"from"][@"consumer_key_code"] = unnamedNumber ? unnamedNumber : name;
        break;
      }
    }

    NSString* eventType = @"";
    switch (event_type) {
      case libkrbn_hid_value_event_type_key_down:
        eventType = [NSString stringWithFormat:@"%@_down", keyType];
        break;
      case libkrbn_hid_value_event_type_key_up:
        eventType = [NSString stringWithFormat:@"%@_up", keyType];
        break;
      case libkrbn_hid_value_event_type_single:
        eventType = @"";
        break;
    }

    NSString* misc = @"";
    {
      NSMutableSet* set = queue.modifierFlags[deviceId];
      if (set && set.count > 0) {
        NSArray* flags = [[set allObjects] sortedArrayUsingSelector:@selector(compare:)];
        misc = [misc stringByAppendingString:[NSString stringWithFormat:@"flags: %@ ", [flags componentsJoinedByString:@","]]];
      }
    }

    [queue pushKeyEvent:code name:name eventType:eventType misc:misc];

    if (simpleModificationJson.count > 0) {
      queue.simpleModificationJsonString = [KarabinerKitJsonUtility createJsonString:simpleModificationJson];
      [queue updateAddSimpleModificationButton:[NSString stringWithFormat:@"Add `%@` to Karabiner-Elements", name]];
    }
  });
}

@implementation EventQueue

enum {
  MAXNUM = 256,
};

- (instancetype)init {
  self = [super init];

  if (self) {
    _queue = [NSMutableArray new];
    _modifierFlags = [NSMutableDictionary new];
  }

  return self;
}

- (void)dealloc {
  [self terminateHidValueObserver];

  [[NSDistributedNotificationCenter defaultCenter] removeObserver:self
                                                             name:nil
                                                           object:nil];
}

- (void)setup {
  [self initializeHidValueObserver];
  [self updateAddSimpleModificationButton:nil];

  {
    NSString* name = [NSString stringWithUTF8String:libkrbn_get_distributed_notification_device_grabbing_state_is_changed()];
    NSString* object = [NSString stringWithUTF8String:libkrbn_get_distributed_notification_observed_object()];

    [[NSDistributedNotificationCenter defaultCenter] addObserver:self
                                                        selector:@selector(deviceGrabbingStateIsChangedCallback)
                                                            name:name
                                                          object:object
                                              suspensionBehavior:NSNotificationSuspensionBehaviorDeliverImmediately];
  }
}

- (void)initializeHidValueObserver {
  libkrbn_enable_hid_value_monitor(hid_value_observer_callback,
                                   (__bridge void*)(self));
}

- (void)terminateHidValueObserver {
  libkrbn_disable_hid_value_monitor();
}

- (void)updateAddSimpleModificationButton:(NSString*)title {
  if (title) {
    self.addSimpleModificationButton.hidden = NO;
  } else {
    self.addSimpleModificationButton.hidden = YES;
  }
  self.addSimpleModificationButton.title = title;
}

- (void)deviceGrabbingStateIsChangedCallback {
  [self initializeHidValueObserver];
}

- (BOOL)observed {
  return libkrbn_hid_value_monitor_observed();
}

- (NSInteger)numberOfRowsInTableView:(NSTableView*)aTableView {
  return [self.queue count];
}

- (id)tableView:(NSTableView*)aTableView objectValueForTableColumn:(NSTableColumn*)aTableColumn row:(NSInteger)rowIndex {
  id identifier = [aTableColumn identifier];

  NSDictionary* dict = self.queue[([self.queue count] - 1 - rowIndex)];
  return dict[identifier];
}

- (void)refresh {
  [self.view reloadData];
  [self.view scrollRowToVisible:([self.queue count] - 1)];
}

- (NSString*)modifierFlagsToString:(NSUInteger)flags {
  NSMutableArray* names = [NSMutableArray new];
  if (flags & NSEventModifierFlagCapsLock) {
    [names addObject:@"caps"];
  }
  if (flags & NSEventModifierFlagShift) {
    [names addObject:@"shift"];
  }
  if (flags & NSEventModifierFlagControl) {
    [names addObject:@"ctrl"];
  }
  if (flags & NSEventModifierFlagOption) {
    [names addObject:@"opt"];
  }
  if (flags & NSEventModifierFlagCommand) {
    [names addObject:@"cmd"];
  }
  if (flags & NSEventModifierFlagNumericPad) {
    [names addObject:@"numpad"];
  }
  if (flags & NSEventModifierFlagHelp) {
    [names addObject:@"help"];
  }
  if (flags & NSEventModifierFlagFunction) {
    [names addObject:@"fn"];
  }

  return [names componentsJoinedByString:@","];
}

- (NSString*)buttonToString:(NSEvent*)event {
  NSInteger number = [event buttonNumber];
  return [NSString stringWithFormat:@"button%d", (int)(number + 1)];
}

- (void)pushKeyEvent:(NSString*)code
                name:(NSString*)name
           eventType:(NSString*)eventType
                misc:(NSString*)misc {
  [self push:eventType
        code:code
        name:name
        misc:misc];
}

- (void)pushMouseEvent:(NSEvent*)event eventType:(NSString*)eventType {
  NSString* flags = [self modifierFlagsToString:[event modifierFlags]];
  [self push:eventType
        code:[NSString stringWithFormat:@"%d", (int)([event buttonNumber])]
        name:[self buttonToString:event]
        misc:[NSString stringWithFormat:@"{x:%d,y:%d} click_count:%d %@",
                                        (int)([event locationInWindow].x), (int)([event locationInWindow].y),
                                        (int)([event clickCount]),
                                        [flags length] > 0 ? [NSString stringWithFormat:@"flags:%@", flags] : @""]];
}

- (void)pushScrollWheelEvent:(NSEvent*)event eventType:(NSString*)eventType {
  [self push:eventType
        code:@""
        name:@""
        misc:[NSString stringWithFormat:@"dx:%.03f dy:%.03f dz:%.03f", [event deltaX], [event deltaY], [event deltaZ]]];
}

- (void)pushMouseEvent:(NSEvent*)event {
  switch ([event type]) {
    case NSEventTypeLeftMouseDown:
    case NSEventTypeRightMouseDown:
    case NSEventTypeOtherMouseDown:
      [self pushMouseEvent:event eventType:@"button_down"];
      break;

    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseUp:
    case NSEventTypeOtherMouseUp:
      [self pushMouseEvent:event eventType:@"button_up"];
      break;

    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
      [self pushMouseEvent:event eventType:@"mouse_dragged"];
      break;

    case NSEventTypeScrollWheel:
      [self pushScrollWheelEvent:event eventType:@"scroll_wheel"];
      break;

    default:
      // Do nothing
      break;
  }
}

- (void)push:(NSString*)eventType code:(NSString*)code name:(NSString*)name misc:(NSString*)misc {
  NSDictionary* dict = @{@"eventType" : eventType,
                         @"code" : code,
                         @"name" : name,
                         @"misc" : misc};

  [self.queue insertObject:dict atIndex:0];
  if ([self.queue count] > MAXNUM) {
    [self.queue removeLastObject];
  }
  [self refresh];
}

- (IBAction)clear:(id)sender {
  [self.queue removeAllObjects];
  [self updateAddSimpleModificationButton:nil];
  [self refresh];
}

- (IBAction)copy:(id)sender {
  NSPasteboard* pboard = [NSPasteboard generalPasteboard];
  NSMutableString* string = [NSMutableString new];

  for (NSUInteger i = 0; i < [self.queue count]; ++i) {
    NSDictionary* dict = self.queue[([self.queue count] - 1 - i)];

    NSString* eventType = [NSString stringWithFormat:@"type:%@", dict[@"eventType"]];
    NSString* code = [NSString stringWithFormat:@"code:%@", dict[@"code"]];
    NSString* name = [NSString stringWithFormat:@"name:%@", dict[@"name"]];
    NSString* misc = [NSString stringWithFormat:@"misc:%@", dict[@"misc"]];

    [string appendFormat:@"%@ %@ %@ %@\n",
                         [eventType stringByPaddingToLength:20
                                                 withString:@" "
                                            startingAtIndex:0],
                         [code stringByPaddingToLength:15
                                            withString:@" "
                                       startingAtIndex:0],
                         [name stringByPaddingToLength:20
                                            withString:@" "
                                       startingAtIndex:0],
                         misc];
  }

  if ([string length] > 0) {
    [pboard clearContents];
    [pboard writeObjects:@[ string ]];
  }
}

- (IBAction)addSimpleModification:(id)sender {
  NSString* escapedJsonString = [self.simpleModificationJsonString stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet.URLQueryAllowedCharacterSet];
  NSString* url = [NSString stringWithFormat:@"karabiner://karabiner/simple_modifications/new?json=%@", escapedJsonString];
  NSLog(@"url %@", url);
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:url]];
}

@end
