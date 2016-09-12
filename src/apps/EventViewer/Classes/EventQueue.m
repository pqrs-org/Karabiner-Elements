#import "EventQueue.h"
#import "PreferencesKeys.h"

@interface EventQueue ()

@property NSMutableArray* queue;
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

  if (keycode == 0x35) return @"Escape";
  if (keycode == 0x36) return @"Command_R";
  if (keycode == 0x37) return @"Command_L";
  if (keycode == 0x38) return @"Shift_L";
  if (keycode == 0x39) return @"CapsLock";
  if (keycode == 0x3a) return @"Option_L";
  if (keycode == 0x3b) return @"Control_L";
  if (keycode == 0x3c) return @"Shift_R";
  if (keycode == 0x3d) return @"Option_R";
  if (keycode == 0x3e) return @"Control_R";
  if (keycode == 0x3f) return @"Fn";

  if (keycode == 0x24) return @"Return";
  if (keycode == 0x30) return @"Tab";
  if (keycode == 0x31) return @"Space";
  if (keycode == 0x33) return @"Delete";
  if (keycode == 0x4c) return @"Enter";
  if (keycode == 0x66) return @"LANG2";
  if (keycode == 0x68) return @"LANG1";
  if (keycode == 0x6e) return @"Application";

  // ------------------------------------------------------------
  @try {
    NSString* characters = [event characters];
    if ([characters length] > 0) {
      unichar unichar = [characters characterAtIndex:0];

      if (unichar == NSUpArrowFunctionKey) { return @"Up"; }
      if (unichar == NSDownArrowFunctionKey) { return @"Down"; }
      if (unichar == NSLeftArrowFunctionKey) { return @"Left"; }
      if (unichar == NSRightArrowFunctionKey) { return @"Right"; }

      if (unichar == NSF1FunctionKey) { return @"F1"; }
      if (unichar == NSF2FunctionKey) { return @"F2"; }
      if (unichar == NSF3FunctionKey) { return @"F3"; }
      if (unichar == NSF4FunctionKey) { return @"F4"; }
      if (unichar == NSF5FunctionKey) { return @"F5"; }
      if (unichar == NSF6FunctionKey) { return @"F6"; }
      if (unichar == NSF7FunctionKey) { return @"F7"; }
      if (unichar == NSF8FunctionKey) { return @"F8"; }
      if (unichar == NSF9FunctionKey) { return @"F9"; }
      if (unichar == NSF10FunctionKey) { return @"F10"; }
      if (unichar == NSF11FunctionKey) { return @"F11"; }
      if (unichar == NSF12FunctionKey) { return @"F12"; }
      if (unichar == NSF13FunctionKey) { return @"F13"; }
      if (unichar == NSF14FunctionKey) { return @"F14"; }
      if (unichar == NSF15FunctionKey) { return @"F15"; }
      if (unichar == NSF16FunctionKey) { return @"F16"; }
      if (unichar == NSF17FunctionKey) { return @"F17"; }
      if (unichar == NSF18FunctionKey) { return @"F18"; }
      if (unichar == NSF19FunctionKey) { return @"F19"; }
      if (unichar == NSF20FunctionKey) { return @"F20"; }
      if (unichar == NSF21FunctionKey) { return @"F21"; }
      if (unichar == NSF22FunctionKey) { return @"F22"; }
      if (unichar == NSF23FunctionKey) { return @"F23"; }
      if (unichar == NSF24FunctionKey) { return @"F24"; }
      if (unichar == NSF25FunctionKey) { return @"F25"; }
      if (unichar == NSF26FunctionKey) { return @"F26"; }
      if (unichar == NSF27FunctionKey) { return @"F27"; }
      if (unichar == NSF28FunctionKey) { return @"F28"; }
      if (unichar == NSF29FunctionKey) { return @"F29"; }
      if (unichar == NSF30FunctionKey) { return @"F30"; }
      if (unichar == NSF31FunctionKey) { return @"F31"; }
      if (unichar == NSF32FunctionKey) { return @"F32"; }
      if (unichar == NSF33FunctionKey) { return @"F33"; }
      if (unichar == NSF34FunctionKey) { return @"F34"; }
      if (unichar == NSF35FunctionKey) { return @"F35"; }

      if (unichar == NSInsertFunctionKey) { return @"Insert"; }
      if (unichar == NSDeleteFunctionKey) { return @"Forward Delete"; }
      if (unichar == NSHomeFunctionKey) { return @"Home"; }
      if (unichar == NSBeginFunctionKey) { return @"Begin"; }
      if (unichar == NSEndFunctionKey) { return @"End"; }
      if (unichar == NSPageUpFunctionKey) { return @"Page Up"; }
      if (unichar == NSPageDownFunctionKey) { return @"Page Down"; }
      if (unichar == NSPrintScreenFunctionKey) { return @"Print Screen"; }
      if (unichar == NSScrollLockFunctionKey) { return @"Scroll Lock"; }
      if (unichar == NSPauseFunctionKey) { return @"Pause"; }
      if (unichar == NSSysReqFunctionKey) { return @"System Request"; }
      if (unichar == NSBreakFunctionKey) { return @"Break"; }
      if (unichar == NSResetFunctionKey) { return @"Reset"; }
      if (unichar == NSStopFunctionKey) { return @"Stop"; }
      if (unichar == NSMenuFunctionKey) { return @"Menu"; }
      if (unichar == NSUserFunctionKey) { return @"User"; }
      if (unichar == NSSystemFunctionKey) { return @"System"; }
      if (unichar == NSPrintFunctionKey) { return @"Print"; }
      if (unichar == NSClearLineFunctionKey) { return @"Clear/Num Lock"; }
      if (unichar == NSClearDisplayFunctionKey) { return @"Clear Display"; }
      if (unichar == NSInsertLineFunctionKey) { return @"Insert Line"; }
      if (unichar == NSDeleteLineFunctionKey) { return @"Delete Line"; }
      if (unichar == NSInsertCharFunctionKey) { return @"Insert Character"; }
      if (unichar == NSDeleteCharFunctionKey) { return @"Delete Character"; }
      if (unichar == NSPrevFunctionKey) { return @"Previous"; }
      if (unichar == NSNextFunctionKey) { return @"Next"; }
      if (unichar == NSSelectFunctionKey) { return @"Select"; }
      if (unichar == NSExecuteFunctionKey) { return @"Execute"; }
      if (unichar == NSUndoFunctionKey) { return @"Undo"; }
      if (unichar == NSRedoFunctionKey) { return @"Redo"; }
      if (unichar == NSFindFunctionKey) { return @"Find"; }
      if (unichar == NSHelpFunctionKey) { return @"Help"; }
      if (unichar == NSModeSwitchFunctionKey) { return @"Mode Switch"; }
    }
  }
  @catch (NSException* exception) {
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

    [self push:eventType
          code:[NSString stringWithFormat:@"0x%x", keyCode]
          name:@""
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
