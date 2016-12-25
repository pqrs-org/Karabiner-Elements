#import "EventQueue.h"
#import "JsonUtility.h"
#import "PreferencesKeys.h"
#include "libkrbn.h"

@interface EventQueue ()

@property NSMutableArray* queue;
@property NSMutableDictionary* hidSystemKeyNames;
@property NSDictionary* hidSystemAuxControlButtonNames;
@property(weak) IBOutlet NSTableView* view;

@end

@implementation EventQueue

enum {
  MAXNUM = 50,
};

- (instancetype)init {
  self = [super init];

  if (self) {
    _queue = [NSMutableArray new];
    _hidSystemKeyNames = [NSMutableDictionary new];
    _hidSystemAuxControlButtonNames = @{
      @(NX_POWER_KEY) : @"power",
      @(NX_KEYTYPE_MUTE) : @"mute",
      @(NX_KEYTYPE_SOUND_UP) : @"volume_up",
      @(NX_KEYTYPE_SOUND_DOWN) : @"volume_down",
      @(NX_KEYTYPE_BRIGHTNESS_DOWN) : @"vk_consumer_brightness_down",
      @(NX_KEYTYPE_BRIGHTNESS_UP) : @"vk_consumer_brightness_up",
      @(NX_KEYTYPE_ILLUMINATION_DOWN) : @"vk_consumer_illumination_down",
      @(NX_KEYTYPE_ILLUMINATION_UP) : @"vk_consumer_illumination_up",
      @(NX_KEYTYPE_FAST) : @"vk_consumer_next",
      @(NX_KEYTYPE_PLAY) : @"vk_consumer_play",
      @(NX_KEYTYPE_REWIND) : @"vk_consumer_previous",
    };

    NSString* jsonFilePath = [[NSBundle mainBundle] pathForResource:@"simple_modifications" ofType:@"json"];
    if (jsonFilePath) {
      NSArray* jsonObject = [JsonUtility loadFile:jsonFilePath];
      if (jsonObject) {
        for (NSDictionary* dict in jsonObject) {
          NSString* name = dict[@"name"];
          if (name) {
            uint8_t hid_system_key = 0;
            if (libkrbn_get_hid_system_key(&hid_system_key, [name UTF8String])) {
              if (!_hidSystemKeyNames[@(hid_system_key)]) {
                _hidSystemKeyNames[@(hid_system_key)] = name;
              }
            }
          }
        }
      }
    }
  }

  return self;
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
  return [NSString stringWithFormat:@"%s%s%s%s%s%s%s%s",
                                    ((flags & NSAlphaShiftKeyMask) ? "Caps " : ""),
                                    ((flags & NSShiftKeyMask) ? "Shift " : ""),
                                    ((flags & NSControlKeyMask) ? "Ctrl " : ""),
                                    ((flags & NSAlternateKeyMask) ? "Opt " : ""),
                                    ((flags & NSCommandKeyMask) ? "Cmd " : ""),
                                    ((flags & NSNumericPadKeyMask) ? "NumPad " : ""),
                                    ((flags & NSHelpKeyMask) ? "Help " : ""),
                                    ((flags & NSFunctionKeyMask) ? "Fn " : "")];
}

- (NSString*)specialKeycodeToString:(NSEvent*)event {
  unsigned short keycode = [event keyCode];

  NSString* name = self.hidSystemKeyNames[@(keycode)];
  if (name) {
    return name;
  }

  return nil;
}

- (NSString*)keycodeToString:(NSEvent*)event {
  NSString* string = [self specialKeycodeToString:event];
  if (string) return string;

  // --------------------
  // Difference of "characters" and "charactersIgnoringModifiers".
  //
  // [NSEvent characters]
  //   Option+z => Ω
  //
  // [NSEvent charactersIgnoringModifiers]
  //   Option+z => z
  //
  // We prefer "Shift+Option+z" style than "Shift+Ω".
  // Therefore we use charactersIgnoringModifiers here.
  //
  // --------------------
  // However, there is a problem.
  // When we use "Dvorak - Qwerty ⌘" as Input Source,
  // charactersIgnoringModifiers returns ';'.
  // It's wrong. Input Source does not change Command+z.
  // So, we need to use 'characters' in this case.
  //
  // [NSEvent characters]
  //    Command+z => z
  // [NSEvent charactersIgnoringModifierss]
  //    Command+z => ;
  //
  //
  // --------------------
  // But, we cannot use these properly without information about current Input Source.
  // And this information cannot get by program.
  //
  // So, we use charactersIgnoringModifierss here and
  // display the result of 'characters' in the 'misc' field.

  @try {
    return [event charactersIgnoringModifiers];
  }
  @catch (NSException* exception) {
  }
  return @"";
}

- (NSString*)charactersToString:(NSEvent*)event {
  // We ignore special characters.
  NSString* string = [self specialKeycodeToString:event];
  if (string) return @"";

  @try {
    return [event characters];
  }
  @catch (NSException* exception) {
  }
  return @"";
}

- (NSString*)buttonToString:(NSEvent*)event {
  NSInteger number = [event buttonNumber];
  if (number == 0) return @"left";
  if (number == 1) return @"right";
  if (number == 2) return @"middle";

  return [NSString stringWithFormat:@"button%d", (int)(number + 1)];
}

- (int)buttonToKernelValue:(NSEvent*)event {
  NSInteger number = [event buttonNumber];
  switch (number) {
  case 0:
    return 0x00000004;
  case 1:
    return 0x00000001;
  case 2:
    return 0x00000002;
  default:
    return (1 << number);
  }
}

- (void)pushKeyEvent:(NSEvent*)event eventType:(NSString*)eventType {
  // An invalid event will be sent when we press command-tab and switch the current app to EventViewer.
  // (keyMod and keyCode == 0).
  // So, we ignore it.
  if ([[NSUserDefaults standardUserDefaults] boolForKey:kHideIgnorableEvents]) {
    if ([eventType isEqualToString:@"FlagsChanged"] && [event keyCode] == 0) {
      return;
    }
  }

  // Show `characters` at last because `characters` might be newline. (== newline truncates message.)
  NSString* misc = [NSString stringWithFormat:@"characters:%@",
                                              [[self charactersToString:event] stringByPaddingToLength:4
                                                                                            withString:@" "
                                                                                       startingAtIndex:0]];

  [self push:eventType
        code:[NSString stringWithFormat:@"0x%x", (int)([event keyCode])]
        name:[self keycodeToString:event]
       flags:[self modifierFlagsToString:[event modifierFlags]]
        misc:misc];
}

- (void)pushSystemDefinedEvent:(NSEvent*)event {
  if ([event subtype] == NX_SUBTYPE_AUX_CONTROL_BUTTONS) {
    int keyCode = (([event data1] & 0xFFFF0000) >> 16);
    int keyFlags = ([event data1] & 0x0000FFFF);
    int keyState = ((keyFlags & 0xFF00) >> 8);

    NSString* eventType = nil;
    switch (keyState) {
    case NX_KEYDOWN:
      eventType = @"SysKeyDown";
      break;
    case NX_KEYUP:
      eventType = @"SysKeyUp";
      break;
    }
    if (!eventType) {
      return;
    }

    // Hide Help (0x5) and NumLock (0xa)
    if ([[NSUserDefaults standardUserDefaults] boolForKey:kHideIgnorableEvents]) {
      if (keyCode == 0x5 || keyCode == 0xa) {
        return;
      }
    }

    NSString* name = self.hidSystemAuxControlButtonNames[@(keyCode)];
    if (!name) {
      name = @"";
    }

    [self push:eventType
          code:[NSString stringWithFormat:@"0x%x", keyCode]
          name:name
         flags:[self modifierFlagsToString:[event modifierFlags]]
          misc:@""];
  }
}

- (void)pushMouseEvent:(NSEvent*)event eventType:(NSString*)eventType {
  [self push:eventType
        code:[NSString stringWithFormat:@"0x%x", (int)([event buttonNumber])]
        name:[self buttonToString:event]
       flags:[self modifierFlagsToString:[event modifierFlags]]
        misc:[NSString stringWithFormat:@"{%d,%d} %d",
                                        (int)([event locationInWindow].x), (int)([event locationInWindow].y),
                                        (int)([event clickCount])]];
}

- (void)pushScrollWheelEvent:(NSEvent*)event eventType:(NSString*)eventType {
  [self push:eventType
        code:@""
        name:@""
       flags:[self modifierFlagsToString:[event modifierFlags]]
        misc:[NSString stringWithFormat:@"dx:%.03f dy:%.03f dz:%.03f", [event deltaX], [event deltaY], [event deltaZ]]];
}

- (void)pushFromNSApplication:(NSEvent*)event {
  switch ([event type]) {
  case NSKeyDown:
    [self pushKeyEvent:event eventType:@"KeyDown"];
    break;

  case NSKeyUp:
    [self pushKeyEvent:event eventType:@"KeyUp"];
    break;

  case NSFlagsChanged:
    [self pushKeyEvent:event eventType:@"FlagsChanged"];
    break;

  case NSSystemDefined:
    [self pushSystemDefinedEvent:event];
    break;

  default:
    // Do nothing
    break;
  }
}

- (void)pushMouseEvent:(NSEvent*)event {
  switch ([event type]) {
  case NSLeftMouseDown:
  case NSRightMouseDown:
  case NSOtherMouseDown:
    [self pushMouseEvent:event eventType:@"MouseDown"];
    break;

  case NSLeftMouseUp:
  case NSRightMouseUp:
  case NSOtherMouseUp:
    [self pushMouseEvent:event eventType:@"MouseUp"];
    break;

  case NSLeftMouseDragged:
  case NSRightMouseDragged:
  case NSOtherMouseDragged:
    [self pushMouseEvent:event eventType:@"MouseDragged"];
    break;

  case NSScrollWheel:
    [self pushScrollWheelEvent:event eventType:@"ScrollWheel"];
    break;

  default:
    // Do nothing
    break;
  }
}

- (void)push:(NSString*)eventType code:(NSString*)code name:(NSString*)name flags:(NSString*)flags misc:(NSString*)misc {
  NSDictionary* dict = @{ @"eventType" : eventType,
                          @"code" : code,
                          @"name" : name,
                          @"flags" : flags,
                          @"misc" : misc };

  [self.queue insertObject:dict atIndex:0];
  if ([self.queue count] > MAXNUM) {
    [self.queue removeLastObject];
  }
  [self refresh];
}

- (IBAction)clear:(id)sender {
  [self.queue removeAllObjects];
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
    NSString* flags = [NSString stringWithFormat:@"flags:%@", dict[@"flags"]];
    NSString* misc = [NSString stringWithFormat:@"misc:%@", dict[@"misc"]];

    [string appendFormat:@"%@ %@ %@ %@ %@\n",
                         [eventType stringByPaddingToLength:25
                                                 withString:@" "
                                            startingAtIndex:0],
                         [code stringByPaddingToLength:15
                                            withString:@" "
                                       startingAtIndex:0],
                         [name stringByPaddingToLength:20
                                            withString:@" "
                                       startingAtIndex:0],
                         [flags stringByPaddingToLength:40
                                             withString:@" "
                                        startingAtIndex:0],
                         misc];
  }

  if ([string length] > 0) {
    [pboard clearContents];
    [pboard writeObjects:@[ string ]];
  }
}

@end
