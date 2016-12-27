#import "EventQueue.h"
#import "JsonUtility.h"
#import "PreferencesKeys.h"
#include "libkrbn.h"

@interface EventQueue ()

@property NSMutableArray* queue;
@property NSDictionary* hidSystemKeyNames;
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

    _hidSystemKeyNames = @{
      @(0x0) : @"a",
      @(0xb) : @"b",
      @(0x8) : @"c",
      @(0x2) : @"d",
      @(0xe) : @"e",
      @(0x3) : @"f",
      @(0x5) : @"g",
      @(0x4) : @"h",
      @(0x22) : @"i",
      @(0x26) : @"j",
      @(0x28) : @"k",
      @(0x25) : @"l",
      @(0x2e) : @"m",
      @(0x2d) : @"n",
      @(0x1f) : @"o",
      @(0x23) : @"p",
      @(0xc) : @"q",
      @(0xf) : @"r",
      @(0x1) : @"s",
      @(0x11) : @"t",
      @(0x20) : @"u",
      @(0x9) : @"v",
      @(0xd) : @"w",
      @(0x7) : @"x",
      @(0x10) : @"y",
      @(0x6) : @"z",

      @(0x12) : @"1",
      @(0x13) : @"2",
      @(0x14) : @"3",
      @(0x15) : @"4",
      @(0x17) : @"5",
      @(0x16) : @"6",
      @(0x1a) : @"7",
      @(0x1c) : @"8",
      @(0x19) : @"9",
      @(0x1d) : @"0",

      @(0x24) : @"return_or_enter",
      @(0x35) : @"escape",
      @(0x33) : @"delete_or_backspace",
      @(0x30) : @"tab",
      @(0x31) : @"spacebar",
      @(0x1b) : @"hyphen",
      @(0x18) : @"equal_sign",
      @(0x21) : @"open_bracket",
      @(0x1e) : @"close_bracket",
      @(0x2a) : @"backslash",
      @(0x29) : @"semicolon",
      @(0x27) : @"quote",
      @(0x32) : @"grave_accent_and_tilde",
      @(0x2b) : @"comma",
      @(0x2f) : @"period",
      @(0x2c) : @"slash",
      @(0x39) : @"caps_lock",

      @(0x7a) : @"f1",
      @(0x78) : @"f2",
      @(0x63) : @"f3",
      @(0x76) : @"f4",
      @(0x60) : @"f5",
      @(0x61) : @"f6",
      @(0x62) : @"f7",
      @(0x64) : @"f8",
      @(0x65) : @"f9",
      @(0x6d) : @"f10",
      @(0x67) : @"f11",
      @(0x6f) : @"f12",
      @(0x69) : @"f13",
      @(0x6b) : @"f14",
      @(0x71) : @"f15",
      @(0x6a) : @"f16",
      @(0x40) : @"f17",
      @(0x4f) : @"f18",
      @(0x50) : @"f19",
      @(0x5a) : @"f20",

      @(0x72) : @"help",
      @(0x73) : @"home",
      @(0x74) : @"page_up",
      @(0x75) : @"delete_forward",
      @(0x77) : @"end",
      @(0x79) : @"page_down",
      @(0x7c) : @"right_arrow",
      @(0x7b) : @"left_arrow",
      @(0x7d) : @"down_arrow",
      @(0x7e) : @"up_arrow",

      @(0x47) : @"keypad_num_lock",
      @(0x4b) : @"keypad_slash",
      @(0x43) : @"keypad_asterisk",
      @(0x4e) : @"keypad_hyphen",
      @(0x45) : @"keypad_plus",
      @(0x4c) : @"keypad_enter",
      @(0x53) : @"keypad_1",
      @(0x54) : @"keypad_2",
      @(0x55) : @"keypad_3",
      @(0x56) : @"keypad_4",
      @(0x57) : @"keypad_5",
      @(0x58) : @"keypad_6",
      @(0x59) : @"keypad_7",
      @(0x5b) : @"keypad_8",
      @(0x5c) : @"keypad_9",
      @(0x52) : @"keypad_0",
      @(0x41) : @"keypad_period",

      @(0xa) : @"non_us_backslash",
      @(0x6e) : @"application",
      @(0x51) : @"keypad_equal_sign",

      @(0x5f) : @"keypad_comma",

      @(0x5e) : @"international1",
      @(0x5d) : @"international3",

      @(0x68) : @"lang1",
      @(0x66) : @"lang2",

      @(0x3b) : @"left_control",
      @(0x38) : @"left_shift",
      @(0x3a) : @"left_option",
      @(0x37) : @"left_command",
      @(0x3e) : @"right_control",
      @(0x3c) : @"right_shift",
      @(0x3d) : @"right_option",
      @(0x36) : @"right_command",

      @(0x3f) : @"fn",
      @(0x82) : @"dashboard",
      @(0x83) : @"launchpad",
      @(0xa0) : @"mission_control",
    };

    _hidSystemAuxControlButtonNames = @{
      @(NX_POWER_KEY) : @"power",
      @(NX_KEYTYPE_MUTE) : @"mute",
      @(NX_KEYTYPE_SOUND_UP) : @"volume_up",
      @(NX_KEYTYPE_SOUND_DOWN) : @"volume_down",
      @(NX_KEYTYPE_BRIGHTNESS_DOWN) : @"display_brightness_decrement",
      @(NX_KEYTYPE_BRIGHTNESS_UP) : @"display_brightness_increment",
      @(NX_KEYTYPE_ILLUMINATION_DOWN) : @"illumination_decrement",
      @(NX_KEYTYPE_ILLUMINATION_UP) : @"illumination_increment",
      @(NX_KEYTYPE_FAST) : @"fastforward",
      @(NX_KEYTYPE_PLAY) : @"play_or_pause",
      @(NX_KEYTYPE_REWIND) : @"rewind",
    };
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
