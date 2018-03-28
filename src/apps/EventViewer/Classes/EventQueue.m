#import "EventQueue.h"
#import "KarabinerKit/KarabinerKit.h"
#import "PreferencesKeys.h"
#include "libkrbn.h"

@interface EventQueue ()

@property libkrbn_hid_value_observer* libkrbn_hid_value_observer;
@property NSMutableArray* queue;
@property(copy) NSString* simpleModificationJsonString;
@property(weak) IBOutlet NSTableView* view;
@property(weak) IBOutlet NSButton* addSimpleModificationButton;

- (void)pushKeyEvent:(NSString*)code name:(NSString*)name eventType:(NSString*)eventType;
- (void)updateAddSimpleModificationButton:(NSString*)title;

@end

static void hid_value_observer_callback(enum libkrbn_hid_value_type type,
                                        uint32_t value,
                                        enum libkrbn_hid_value_event_type event_type,
                                        void* refcon) {
  EventQueue* queue = (__bridge EventQueue*)(refcon);
  if (queue) {
    NSString* code = [NSString stringWithFormat:@"0x%x", value];
    NSMutableDictionary* simpleModificationJson = [NSMutableDictionary new];

    char buffer[256];
    buffer[0] = '\0';
    switch (type) {
      case libkrbn_hid_value_type_key_code:
        libkrbn_get_key_code_name(buffer, sizeof(buffer), value);
        simpleModificationJson[@"from"] = [NSMutableDictionary new];
        simpleModificationJson[@"from"][@"key_code"] = [NSString stringWithUTF8String:buffer];
        break;

      case libkrbn_hid_value_type_consumer_key_code:
        libkrbn_get_consumer_key_code_name(buffer, sizeof(buffer), value);
        simpleModificationJson[@"from"] = [NSMutableDictionary new];
        simpleModificationJson[@"from"][@"consumer_key_code"] = [NSString stringWithUTF8String:buffer];
        break;
    }
    NSString* name = [NSString stringWithUTF8String:buffer];

    NSString* eventType = @"";
    switch (event_type) {
      case libkrbn_hid_value_event_type_key_down:
        eventType = @"key_down";
        break;
      case libkrbn_hid_value_event_type_key_up:
        eventType = @"key_up";
        break;
      case libkrbn_hid_value_event_type_single:
        eventType = @"";
        break;
    }

    [queue pushKeyEvent:code name:name eventType:eventType];

    if (simpleModificationJson.count > 0) {
      queue.simpleModificationJsonString = [KarabinerKitJsonUtility createJsonString:simpleModificationJson];
      [queue updateAddSimpleModificationButton:[NSString stringWithFormat:@"Add `%@` to Karabiner-Elements", name]];
    }
  }
}

@implementation EventQueue

enum {
  MAXNUM = 50,
};

- (instancetype)init {
  self = [super init];

  if (self) {
    libkrbn_hid_value_observer* p = NULL;
    if (libkrbn_hid_value_observer_initialize(&p,
                                              hid_value_observer_callback,
                                              (__bridge void*)(self))) {
      self.libkrbn_hid_value_observer = p;
    }

    _queue = [NSMutableArray new];
  }

  return self;
}

- (void)dealloc {
  if (self.libkrbn_hid_value_observer) {
    libkrbn_hid_value_observer* p = self.libkrbn_hid_value_observer;
    libkrbn_hid_value_observer_terminate(&p);
  }
}

- (void)setup {
  [self updateAddSimpleModificationButton:nil];
}

- (void)updateAddSimpleModificationButton:(NSString*)title {
  if (title) {
    self.addSimpleModificationButton.hidden = NO;
  } else {
    self.addSimpleModificationButton.hidden = YES;
  }
  self.addSimpleModificationButton.title = title;
}

- (NSInteger)observedDeviceCount {
  if (self.libkrbn_hid_value_observer) {
    libkrbn_hid_value_observer* p = self.libkrbn_hid_value_observer;
    return libkrbn_hid_value_observer_calculate_observed_device_count(p);
  }
  return 0;
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

- (void)pushKeyEvent:(NSString*)code name:(NSString*)name eventType:(NSString*)eventType {
  [self push:eventType
        code:code
        name:name
        misc:@""];
}

- (void)pushMouseEvent:(NSEvent*)event eventType:(NSString*)eventType {
  NSString* flags = [self modifierFlagsToString:[event modifierFlags]];
  [self push:eventType
        code:[NSString stringWithFormat:@"0x%x", (int)([event buttonNumber])]
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
      [self pushMouseEvent:event eventType:@"MouseDown"];
      break;

    case NSEventTypeLeftMouseUp:
    case NSEventTypeRightMouseUp:
    case NSEventTypeOtherMouseUp:
      [self pushMouseEvent:event eventType:@"MouseUp"];
      break;

    case NSEventTypeLeftMouseDragged:
    case NSEventTypeRightMouseDragged:
    case NSEventTypeOtherMouseDragged:
      [self pushMouseEvent:event eventType:@"MouseDragged"];
      break;

    case NSEventTypeScrollWheel:
      [self pushScrollWheelEvent:event eventType:@"ScrollWheel"];
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

    NSString* eventType = [NSString stringWithFormat:@"eventType:%@", dict[@"eventType"]];
    NSString* code = [NSString stringWithFormat:@"code:%@", dict[@"code"]];
    NSString* name = [NSString stringWithFormat:@"name:%@", dict[@"name"]];
    NSString* misc = [NSString stringWithFormat:@"misc:%@", dict[@"misc"]];

    [string appendFormat:@"%@ %@ %@ %@\n",
                         [eventType stringByPaddingToLength:25
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
