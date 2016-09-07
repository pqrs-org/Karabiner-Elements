/*
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
#ifndef _IOHIDUSAGETABLES_H
#define _IOHIDUSAGETABLES_H

/* ******************************************************************************************
 * HID Usage Tables
 *
 * The following constants are from the USB 'HID Usage Tables' specification, revision 1.1rc3
 * ****************************************************************************************** */


/* Usage Pages */
enum
{
    kHIDPage_Undefined              = 0x00,
    kHIDPage_GenericDesktop         = 0x01,
    kHIDPage_Simulation             = 0x02,
    kHIDPage_VR                     = 0x03,
    kHIDPage_Sport                  = 0x04,
    kHIDPage_Game                   = 0x05,
    kHIDPage_GenericDeviceControls  = 0x06,
    kHIDPage_KeyboardOrKeypad       = 0x07,    /* USB Device Class Definition for Human Interface Devices (HID). Note: the usage type for all key codes is Selector (Sel). */
    kHIDPage_LEDs                   = 0x08,
    kHIDPage_Button                 = 0x09,
    kHIDPage_Ordinal                = 0x0A,
    kHIDPage_Telephony              = 0x0B,
    kHIDPage_Consumer               = 0x0C,
    kHIDPage_Digitizer              = 0x0D,
    /* Reserved 0x0E */
    kHIDPage_PID                    = 0x0F,    /* USB Physical Interface Device definitions for force feedback and related devices. */
    kHIDPage_Unicode                = 0x10,
    /* Reserved 0x11 - 0x13 */
    kHIDPage_AlphanumericDisplay    = 0x14,
    /* Reserved 0x15 - 0x1F */
    kHIDPage_Sensor                 = 0x20,
    /* Reserved 0x21 - 0x7f */
    kHIDPage_Monitor                = 0x80,
    kHIDPage_MonitorEnumerated      = 0x81,
    kHIDPage_MonitorVirtual         = 0x82,
    kHIDPage_MonitorReserved        = 0x83,
    /* Power 0x84 - 0x87     USB Device Class Definition for Power Devices */
    kHIDPage_PowerDevice            = 0x84,                 /* Power Device Page */
    kHIDPage_BatterySystem          = 0x85,                 /* Battery System Page */
    kHIDPage_PowerReserved          = 0x86,
    kHIDPage_PowerReserved2         = 0x87,
    /* Reserved 0x88 - 0x8B */
    kHIDPage_BarCodeScanner         = 0x8C,    /* (Point of Sale) USB Device Class Definition for Bar Code Scanner Devices */
    kHIDPage_WeighingDevice         = 0x8D,    /* (Point of Sale) USB Device Class Definition for Weighing Devices */
    kHIDPage_Scale                  = 0x8D,    /* (Point of Sale) USB Device Class Definition for Scale Devices */
    kHIDPage_MagneticStripeReader   = 0x8E,
    /* ReservedPointofSalepages 0x8F */
    kHIDPage_CameraControl          = 0x90,    /* USB Device Class Definition for Image Class Devices */
    kHIDPage_Arcade                 = 0x91,    /* OAAF Definitions for arcade and coinop related Devices */
    /* Reserved 0x92 - 0xFEFF */
    /* VendorDefined 0xFF00 - 0xFFFF */
    kHIDPage_VendorDefinedStart     = 0xFF00
};

/* Undefined Usage for all usage pages */
enum
{
    kHIDUsage_Undefined    = 0x00
};

/* GenericDesktop Page (0x01) */
enum
{
    kHIDUsage_GD_Pointer    = 0x01,    /* Physical Collection */
    kHIDUsage_GD_Mouse    = 0x02,    /* Application Collection */
    /* 0x03 Reserved */
    kHIDUsage_GD_Joystick    = 0x04,    /* Application Collection */
    kHIDUsage_GD_GamePad    = 0x05,    /* Application Collection */
    kHIDUsage_GD_Keyboard    = 0x06,    /* Application Collection */
    kHIDUsage_GD_Keypad    = 0x07,    /* Application Collection */
    kHIDUsage_GD_MultiAxisController    = 0x08,    /* Application Collection */
    /* 0x09 - 0x2F Reserved */
    kHIDUsage_GD_X    = 0x30,    /* Dynamic Value */
    kHIDUsage_GD_Y    = 0x31,    /* Dynamic Value */
    kHIDUsage_GD_Z    = 0x32,    /* Dynamic Value */
    kHIDUsage_GD_Rx    = 0x33,    /* Dynamic Value */
    kHIDUsage_GD_Ry    = 0x34,    /* Dynamic Value */
    kHIDUsage_GD_Rz    = 0x35,    /* Dynamic Value */
    kHIDUsage_GD_Slider    = 0x36,    /* Dynamic Value */
    kHIDUsage_GD_Dial    = 0x37,    /* Dynamic Value */
    kHIDUsage_GD_Wheel    = 0x38,    /* Dynamic Value */
    kHIDUsage_GD_Hatswitch    = 0x39,    /* Dynamic Value */
    kHIDUsage_GD_CountedBuffer    = 0x3A,    /* Logical Collection */
    kHIDUsage_GD_ByteCount    = 0x3B,    /* Dynamic Value */
    kHIDUsage_GD_MotionWakeup    = 0x3C,    /* One-Shot Control */
    kHIDUsage_GD_Start    = 0x3D,    /* On/Off Control */
    kHIDUsage_GD_Select    = 0x3E,    /* On/Off Control */
    /* 0x3F Reserved */
    kHIDUsage_GD_Vx    = 0x40,    /* Dynamic Value */
    kHIDUsage_GD_Vy    = 0x41,    /* Dynamic Value */
    kHIDUsage_GD_Vz    = 0x42,    /* Dynamic Value */
    kHIDUsage_GD_Vbrx    = 0x43,    /* Dynamic Value */
    kHIDUsage_GD_Vbry    = 0x44,    /* Dynamic Value */
    kHIDUsage_GD_Vbrz    = 0x45,    /* Dynamic Value */
    kHIDUsage_GD_Vno    = 0x46,    /* Dynamic Value */
    /* 0x47 - 0x7F Reserved */
    kHIDUsage_GD_SystemControl    = 0x80,    /* Application Collection */
    kHIDUsage_GD_SystemPowerDown    = 0x81,    /* One-Shot Control */
    kHIDUsage_GD_SystemSleep    = 0x82,    /* One-Shot Control */
    kHIDUsage_GD_SystemWakeUp    = 0x83,    /* One-Shot Control */
    kHIDUsage_GD_SystemContextMenu    = 0x84,    /* One-Shot Control */
    kHIDUsage_GD_SystemMainMenu    = 0x85,    /* One-Shot Control */
    kHIDUsage_GD_SystemAppMenu    = 0x86,    /* One-Shot Control */
    kHIDUsage_GD_SystemMenuHelp    = 0x87,    /* One-Shot Control */
    kHIDUsage_GD_SystemMenuExit    = 0x88,    /* One-Shot Control */
    kHIDUsage_GD_SystemMenuSelect    = 0x89,    /* Selector */
    kHIDUsage_GD_SystemMenu    = kHIDUsage_GD_SystemMenuSelect,    /* Selector */
    kHIDUsage_GD_SystemMenuRight    = 0x8A,    /* Re-Trigger Control */
    kHIDUsage_GD_SystemMenuLeft    = 0x8B,    /* Re-Trigger Control */
    kHIDUsage_GD_SystemMenuUp    = 0x8C,    /* Re-Trigger Control */
    kHIDUsage_GD_SystemMenuDown    = 0x8D,    /* Re-Trigger Control */
    /* 0x8E - 0x8F Reserved */
    kHIDUsage_GD_DPadUp    = 0x90,    /* On/Off Control */
    kHIDUsage_GD_DPadDown    = 0x91,    /* On/Off Control */
    kHIDUsage_GD_DPadRight    = 0x92,    /* On/Off Control */
    kHIDUsage_GD_DPadLeft    = 0x93,    /* On/Off Control */
    /* 0x94 - 0xFFFF Reserved */
    kHIDUsage_GD_Reserved = 0xFFFF
};

/* Simulation Page (0x02) */
/* This section provides detailed descriptions of the usages employed by simulation devices. */
enum
{
    kHIDUsage_Sim_FlightSimulationDevice    = 0x01,    /* Application Collection */
    kHIDUsage_Sim_AutomobileSimulationDevice    = 0x02,    /* Application Collection */
    kHIDUsage_Sim_TankSimulationDevice    = 0x03,    /* Application Collection */
    kHIDUsage_Sim_SpaceshipSimulationDevice    = 0x04,    /* Application Collection */
    kHIDUsage_Sim_SubmarineSimulationDevice    = 0x05,    /* Application Collection */
    kHIDUsage_Sim_SailingSimulationDevice    = 0x06,    /* Application Collection */
    kHIDUsage_Sim_MotorcycleSimulationDevice    = 0x07,    /* Application Collection */
    kHIDUsage_Sim_SportsSimulationDevice    = 0x08,    /* Application Collection */
    kHIDUsage_Sim_AirplaneSimulationDevice    = 0x09,    /* Application Collection */
    kHIDUsage_Sim_HelicopterSimulationDevice    = 0x0A,    /* Application Collection */
    kHIDUsage_Sim_MagicCarpetSimulationDevice    = 0x0B,    /* Application Collection */
    kHIDUsage_Sim_BicycleSimulationDevice    = 0x0C,    /* Application Collection */
    /* 0x0D - 0x1F Reserved */
    kHIDUsage_Sim_FlightControlStick    = 0x20,    /* Application Collection */
    kHIDUsage_Sim_FlightStick    = 0x21,    /* Application Collection */
    kHIDUsage_Sim_CyclicControl    = 0x22,    /* Physical Collection */
    kHIDUsage_Sim_CyclicTrim    = 0x23,    /* Physical Collection */
    kHIDUsage_Sim_FlightYoke    = 0x24,    /* Application Collection */
    kHIDUsage_Sim_TrackControl    = 0x25,    /* Physical Collection */
    /* 0x26 - 0xAF Reserved */
    kHIDUsage_Sim_Aileron    = 0xB0,    /* Dynamic Value */
    kHIDUsage_Sim_AileronTrim    = 0xB1,    /* Dynamic Value */
    kHIDUsage_Sim_AntiTorqueControl    = 0xB2,    /* Dynamic Value */
    kHIDUsage_Sim_AutopilotEnable    = 0xB3,    /* On/Off Control */
    kHIDUsage_Sim_ChaffRelease    = 0xB4,    /* One-Shot Control */
    kHIDUsage_Sim_CollectiveControl    = 0xB5,    /* Dynamic Value */
    kHIDUsage_Sim_DiveBrake    = 0xB6,    /* Dynamic Value */
    kHIDUsage_Sim_ElectronicCountermeasures    = 0xB7,    /* On/Off Control */
    kHIDUsage_Sim_Elevator    = 0xB8,    /* Dynamic Value */
    kHIDUsage_Sim_ElevatorTrim    = 0xB9,    /* Dynamic Value */
    kHIDUsage_Sim_Rudder    = 0xBA,    /* Dynamic Value */
    kHIDUsage_Sim_Throttle    = 0xBB,    /* Dynamic Value */
    kHIDUsage_Sim_FlightCommunications    = 0xBC,    /* On/Off Control */
    kHIDUsage_Sim_FlareRelease    = 0xBD,    /* One-Shot Control */
    kHIDUsage_Sim_LandingGear    = 0xBE,    /* On/Off Control */
    kHIDUsage_Sim_ToeBrake    = 0xBF,    /* Dynamic Value */
    kHIDUsage_Sim_Trigger    = 0xC0,    /* Momentary Control */
    kHIDUsage_Sim_WeaponsArm    = 0xC1,    /* On/Off Control */
    kHIDUsage_Sim_Weapons    = 0xC2,    /* Selector */
    kHIDUsage_Sim_WingFlaps    = 0xC3,    /* Dynamic Value */
    kHIDUsage_Sim_Accelerator    = 0xC4,    /* Dynamic Value */
    kHIDUsage_Sim_Brake    = 0xC5,    /* Dynamic Value */
    kHIDUsage_Sim_Clutch    = 0xC6,    /* Dynamic Value */
    kHIDUsage_Sim_Shifter    = 0xC7,    /* Dynamic Value */
    kHIDUsage_Sim_Steering    = 0xC8,    /* Dynamic Value */
    kHIDUsage_Sim_TurretDirection    = 0xC9,    /* Dynamic Value */
    kHIDUsage_Sim_BarrelElevation    = 0xCA,    /* Dynamic Value */
    kHIDUsage_Sim_DivePlane    = 0xCB,    /* Dynamic Value */
    kHIDUsage_Sim_Ballast    = 0xCC,    /* Dynamic Value */
    kHIDUsage_Sim_BicycleCrank    = 0xCD,    /* Dynamic Value */
    kHIDUsage_Sim_HandleBars    = 0xCE,    /* Dynamic Value */
    kHIDUsage_Sim_FrontBrake    = 0xCF,    /* Dynamic Value */
    kHIDUsage_Sim_RearBrake    = 0xD0,    /* Dynamic Value */
    /* 0xD1 - 0xFFFF Reserved */
    kHIDUsage_Sim_Reserved = 0xFFFF
};

/* VR Page (0x03) */
/* Virtual Reality controls depend on designators to identify the individual controls. Most of the following are */
/* usages are applied to the collections of entities that comprise the actual device. */
enum
{
    kHIDUsage_VR_Belt    = 0x01,    /* Application Collection */
    kHIDUsage_VR_BodySuit    = 0x02,    /* Application Collection */
    kHIDUsage_VR_Flexor    = 0x03,    /* Physical Collection */
    kHIDUsage_VR_Glove    = 0x04,    /* Application Collection */
    kHIDUsage_VR_HeadTracker    = 0x05,    /* Physical Collection */
    kHIDUsage_VR_HeadMountedDisplay    = 0x06,    /* Application Collection */
    kHIDUsage_VR_HandTracker    = 0x07,    /* Application Collection */
    kHIDUsage_VR_Oculometer    = 0x08,    /* Application Collection */
    kHIDUsage_VR_Vest    = 0x09,    /* Application Collection */
    kHIDUsage_VR_AnimatronicDevice    = 0x0A,    /* Application Collection */
    /* 0x0B - 0x1F Reserved */
    kHIDUsage_VR_StereoEnable    = 0x20,    /* On/Off Control */
    kHIDUsage_VR_DisplayEnable    = 0x21,    /* On/Off Control */
    /* 0x22 - 0xFFFF Reserved */
    kHIDUsage_VR_Reserved = 0xFFFF
};

/* Sport Page (0x04) */
enum
{
    kHIDUsage_Sprt_BaseballBat    = 0x01,    /* Application Collection */
    kHIDUsage_Sprt_GolfClub    = 0x02,    /* Application Collection */
    kHIDUsage_Sprt_RowingMachine    = 0x03,    /* Application Collection */
    kHIDUsage_Sprt_Treadmill    = 0x04,    /* Application Collection */
    /* 0x05 - 0x2F Reserved */
    kHIDUsage_Sprt_Oar    = 0x30,    /* Dynamic Value */
    kHIDUsage_Sprt_Slope    = 0x31,    /* Dynamic Value */
    kHIDUsage_Sprt_Rate    = 0x32,    /* Dynamic Value */
    kHIDUsage_Sprt_StickSpeed    = 0x33,    /* Dynamic Value */
    kHIDUsage_Sprt_StickFaceAngle    = 0x34,    /* Dynamic Value */
    kHIDUsage_Sprt_StickHeelOrToe    = 0x35,    /* Dynamic Value */
    kHIDUsage_Sprt_StickFollowThrough    = 0x36,    /* Dynamic Value */
    kHIDUsage_Sprt_StickTempo    = 0x37,    /* Dynamic Value */
    kHIDUsage_Sprt_StickType    = 0x38,    /* Named Array */
    kHIDUsage_Sprt_StickHeight    = 0x39,    /* Dynamic Value */
    /* 0x3A - 0x4F Reserved */
    kHIDUsage_Sprt_Putter    = 0x50,    /* Selector */
    kHIDUsage_Sprt_1Iron    = 0x51,    /* Selector */
    kHIDUsage_Sprt_2Iron    = 0x52,    /* Selector */
    kHIDUsage_Sprt_3Iron    = 0x53,    /* Selector */
    kHIDUsage_Sprt_4Iron    = 0x54,    /* Selector */
    kHIDUsage_Sprt_5Iron    = 0x55,    /* Selector */
    kHIDUsage_Sprt_6Iron    = 0x56,    /* Selector */
    kHIDUsage_Sprt_7Iron    = 0x57,    /* Selector */
    kHIDUsage_Sprt_8Iron    = 0x58,    /* Selector */
    kHIDUsage_Sprt_9Iron    = 0x59,    /* Selector */
    kHIDUsage_Sprt_10Iron    = 0x5A,    /* Selector */
    kHIDUsage_Sprt_11Iron    = 0x5B,    /* Selector */
    kHIDUsage_Sprt_SandWedge    = 0x5C,    /* Selector */
    kHIDUsage_Sprt_LoftWedge    = 0x5D,    /* Selector */
    kHIDUsage_Sprt_PowerWedge    = 0x5E,    /* Selector */
    kHIDUsage_Sprt_1Wood    = 0x5F,    /* Selector */
    kHIDUsage_Sprt_3Wood    = 0x60,    /* Selector */
    kHIDUsage_Sprt_5Wood    = 0x61,    /* Selector */
    kHIDUsage_Sprt_7Wood    = 0x62,    /* Selector */
    kHIDUsage_Sprt_9Wood    = 0x63,    /* Selector */
    /* 0x64 - 0xFFFF Reserved */
    kHIDUsage_Sprt_Reserved = 0xFFFF
};

/* Game Page (0x05) */
enum
{
    kHIDUsage_Game_3DGameController    = 0x01,    /* Application Collection */
    kHIDUsage_Game_PinballDevice    = 0x02,    /* Application Collection */
    kHIDUsage_Game_GunDevice    = 0x03,    /* Application Collection */
    /* 0x04 - 0x1F Reserved */
    kHIDUsage_Game_PointofView    = 0x20,    /* Physical Collection */
    kHIDUsage_Game_TurnRightOrLeft    = 0x21,    /* Dynamic Value */
    kHIDUsage_Game_PitchUpOrDown    = 0x22,    /* Dynamic Value */
    kHIDUsage_Game_RollRightOrLeft    = 0x23,    /* Dynamic Value */
    kHIDUsage_Game_MoveRightOrLeft    = 0x24,    /* Dynamic Value */
    kHIDUsage_Game_MoveForwardOrBackward    = 0x25,    /* Dynamic Value */
    kHIDUsage_Game_MoveUpOrDown    = 0x26,    /* Dynamic Value */
    kHIDUsage_Game_LeanRightOrLeft    = 0x27,    /* Dynamic Value */
    kHIDUsage_Game_LeanForwardOrBackward    = 0x28,    /* Dynamic Value */
    kHIDUsage_Game_HeightOfPOV    = 0x29,    /* Dynamic Value */
    kHIDUsage_Game_Flipper    = 0x2A,    /* Momentary Control */
    kHIDUsage_Game_SecondaryFlipper    = 0x2B,    /* Momentary Control */
    kHIDUsage_Game_Bump    = 0x2C,    /* Momentary Control */
    kHIDUsage_Game_NewGame    = 0x2D,    /* One-Shot Control */
    kHIDUsage_Game_ShootBall    = 0x2E,    /* One-Shot Control */
    kHIDUsage_Game_Player    = 0x2F,    /* One-Shot Control */
    kHIDUsage_Game_GunBolt    = 0x30,    /* On/Off Control */
    kHIDUsage_Game_GunClip    = 0x31,    /* On/Off Control */
    kHIDUsage_Game_Gun    = 0x32,    /* Selector */
    kHIDUsage_Game_GunSingleShot    = 0x33,    /* Selector */
    kHIDUsage_Game_GunBurst    = 0x34,    /* Selector */
    kHIDUsage_Game_GunAutomatic    = 0x35,    /* Selector */
    kHIDUsage_Game_GunSafety    = 0x36,    /* On/Off Control */
    kHIDUsage_Game_GamepadFireOrJump    = 0x37,    /* Logical Collection */
    kHIDUsage_Game_GamepadTrigger    = 0x39,    /* Logical Collection */
    /* 0x3A - 0xFFFF Reserved */
    kHIDUsage_Game_Reserved = 0xFFFF
};

/* Generic Device Controls (0x0g) */
enum
{
    kHIDUsage_GenDevControls_BackgroundControls     = 0x01, /* Application Collection */
};

/* KeyboardOrKeypad Page (0x07) */
/* This section is the Usage Page for key codes to be used in implementing a USB keyboard. A Boot Keyboard (84-, 101- or 104-key) should at a minimum support all associated usage codes as indicated in the “Boot” */
/* column below. */
/* The usage type of all key codes is Selectors (Sel), except for the modifier keys Keyboard Left Control (0x224) to Keyboard Right GUI (0x231) which are Dynamic Flags (DV). */
/* Note: A general note on Usages and languages: Due to the variation of keyboards from language to language, it is not feasible to specify exact key mappings for every language. Where this list is not specific for a key function in a language, the closest equivalent key position should be used, so that a keyboard may be modified for a different language by simply printing different keycaps. One example is the Y key on a North American keyboard. In Germany this is typically Z. Rather than changing the keyboard firmware to put the Z Usage into that place in the descriptor list, the vendor should use the Y Usage on both the North American and German keyboards. This continues to be the existing practice in the industry, in order to minimize the number of changes to the electronics to accommodate otherlanguages. */
enum
{
    kHIDUsage_KeyboardErrorRollOver    = 0x01,    /* ErrorRollOver */
    kHIDUsage_KeyboardPOSTFail    = 0x02,    /* POSTFail */
    kHIDUsage_KeyboardErrorUndefined    = 0x03,    /* ErrorUndefined */
    kHIDUsage_KeyboardA    = 0x04,    /* a or A */
    kHIDUsage_KeyboardB    = 0x05,    /* b or B */
    kHIDUsage_KeyboardC    = 0x06,    /* c or C */
    kHIDUsage_KeyboardD    = 0x07,    /* d or D */
    kHIDUsage_KeyboardE    = 0x08,    /* e or E */
    kHIDUsage_KeyboardF    = 0x09,    /* f or F */
    kHIDUsage_KeyboardG    = 0x0A,    /* g or G */
    kHIDUsage_KeyboardH    = 0x0B,    /* h or H */
    kHIDUsage_KeyboardI    = 0x0C,    /* i or I */
    kHIDUsage_KeyboardJ    = 0x0D,    /* j or J */
    kHIDUsage_KeyboardK    = 0x0E,    /* k or K */
    kHIDUsage_KeyboardL    = 0x0F,    /* l or L */
    kHIDUsage_KeyboardM    = 0x10,    /* m or M */
    kHIDUsage_KeyboardN    = 0x11,    /* n or N */
    kHIDUsage_KeyboardO    = 0x12,    /* o or O */
    kHIDUsage_KeyboardP    = 0x13,    /* p or P */
    kHIDUsage_KeyboardQ    = 0x14,    /* q or Q */
    kHIDUsage_KeyboardR    = 0x15,    /* r or R */
    kHIDUsage_KeyboardS    = 0x16,    /* s or S */
    kHIDUsage_KeyboardT    = 0x17,    /* t or T */
    kHIDUsage_KeyboardU    = 0x18,    /* u or U */
    kHIDUsage_KeyboardV    = 0x19,    /* v or V */
    kHIDUsage_KeyboardW    = 0x1A,    /* w or W */
    kHIDUsage_KeyboardX    = 0x1B,    /* x or X */
    kHIDUsage_KeyboardY    = 0x1C,    /* y or Y */
    kHIDUsage_KeyboardZ    = 0x1D,    /* z or Z */
    kHIDUsage_Keyboard1    = 0x1E,    /* 1 or ! */
    kHIDUsage_Keyboard2    = 0x1F,    /* 2 or @ */
    kHIDUsage_Keyboard3    = 0x20,    /* 3 or # */
    kHIDUsage_Keyboard4    = 0x21,    /* 4 or $ */
    kHIDUsage_Keyboard5    = 0x22,    /* 5 or % */
    kHIDUsage_Keyboard6    = 0x23,    /* 6 or ^ */
    kHIDUsage_Keyboard7    = 0x24,    /* 7 or & */
    kHIDUsage_Keyboard8    = 0x25,    /* 8 or * */
    kHIDUsage_Keyboard9    = 0x26,    /* 9 or ( */
    kHIDUsage_Keyboard0    = 0x27,    /* 0 or ) */
    kHIDUsage_KeyboardReturnOrEnter    = 0x28,    /* Return (Enter) */
    kHIDUsage_KeyboardEscape    = 0x29,    /* Escape */
    kHIDUsage_KeyboardDeleteOrBackspace    = 0x2A,    /* Delete (Backspace) */
    kHIDUsage_KeyboardTab    = 0x2B,    /* Tab */
    kHIDUsage_KeyboardSpacebar    = 0x2C,    /* Spacebar */
    kHIDUsage_KeyboardHyphen    = 0x2D,    /* - or _ */
    kHIDUsage_KeyboardEqualSign    = 0x2E,    /* = or + */
    kHIDUsage_KeyboardOpenBracket    = 0x2F,    /* [ or { */
    kHIDUsage_KeyboardCloseBracket    = 0x30,    /* ] or } */
    kHIDUsage_KeyboardBackslash    = 0x31,    /* \ or | */
    kHIDUsage_KeyboardNonUSPound    = 0x32,    /* Non-US # or _ */
    kHIDUsage_KeyboardSemicolon    = 0x33,    /* ; or : */
    kHIDUsage_KeyboardQuote    = 0x34,    /* ' or " */
    kHIDUsage_KeyboardGraveAccentAndTilde    = 0x35,    /* Grave Accent and Tilde */
    kHIDUsage_KeyboardComma    = 0x36,    /* , or < */
    kHIDUsage_KeyboardPeriod    = 0x37,    /* . or > */
    kHIDUsage_KeyboardSlash    = 0x38,    /* / or ? */
    kHIDUsage_KeyboardCapsLock    = 0x39,    /* Caps Lock */
    kHIDUsage_KeyboardF1    = 0x3A,    /* F1 */
    kHIDUsage_KeyboardF2    = 0x3B,    /* F2 */
    kHIDUsage_KeyboardF3    = 0x3C,    /* F3 */
    kHIDUsage_KeyboardF4    = 0x3D,    /* F4 */
    kHIDUsage_KeyboardF5    = 0x3E,    /* F5 */
    kHIDUsage_KeyboardF6    = 0x3F,    /* F6 */
    kHIDUsage_KeyboardF7    = 0x40,    /* F7 */
    kHIDUsage_KeyboardF8    = 0x41,    /* F8 */
    kHIDUsage_KeyboardF9    = 0x42,    /* F9 */
    kHIDUsage_KeyboardF10    = 0x43,    /* F10 */
    kHIDUsage_KeyboardF11    = 0x44,    /* F11 */
    kHIDUsage_KeyboardF12    = 0x45,    /* F12 */
    kHIDUsage_KeyboardPrintScreen    = 0x46,    /* Print Screen */
    kHIDUsage_KeyboardScrollLock    = 0x47,    /* Scroll Lock */
    kHIDUsage_KeyboardPause    = 0x48,    /* Pause */
    kHIDUsage_KeyboardInsert    = 0x49,    /* Insert */
    kHIDUsage_KeyboardHome    = 0x4A,    /* Home */
    kHIDUsage_KeyboardPageUp    = 0x4B,    /* Page Up */
    kHIDUsage_KeyboardDeleteForward    = 0x4C,    /* Delete Forward */
    kHIDUsage_KeyboardEnd    = 0x4D,    /* End */
    kHIDUsage_KeyboardPageDown    = 0x4E,    /* Page Down */
    kHIDUsage_KeyboardRightArrow    = 0x4F,    /* Right Arrow */
    kHIDUsage_KeyboardLeftArrow    = 0x50,    /* Left Arrow */
    kHIDUsage_KeyboardDownArrow    = 0x51,    /* Down Arrow */
    kHIDUsage_KeyboardUpArrow    = 0x52,    /* Up Arrow */
    kHIDUsage_KeypadNumLock    = 0x53,    /* Keypad NumLock or Clear */
    kHIDUsage_KeypadSlash    = 0x54,    /* Keypad / */
    kHIDUsage_KeypadAsterisk    = 0x55,    /* Keypad * */
    kHIDUsage_KeypadHyphen    = 0x56,    /* Keypad - */
    kHIDUsage_KeypadPlus    = 0x57,    /* Keypad + */
    kHIDUsage_KeypadEnter    = 0x58,    /* Keypad Enter */
    kHIDUsage_Keypad1    = 0x59,    /* Keypad 1 or End */
    kHIDUsage_Keypad2    = 0x5A,    /* Keypad 2 or Down Arrow */
    kHIDUsage_Keypad3    = 0x5B,    /* Keypad 3 or Page Down */
    kHIDUsage_Keypad4    = 0x5C,    /* Keypad 4 or Left Arrow */
    kHIDUsage_Keypad5    = 0x5D,    /* Keypad 5 */
    kHIDUsage_Keypad6    = 0x5E,    /* Keypad 6 or Right Arrow */
    kHIDUsage_Keypad7    = 0x5F,    /* Keypad 7 or Home */
    kHIDUsage_Keypad8    = 0x60,    /* Keypad 8 or Up Arrow */
    kHIDUsage_Keypad9    = 0x61,    /* Keypad 9 or Page Up */
    kHIDUsage_Keypad0    = 0x62,    /* Keypad 0 or Insert */
    kHIDUsage_KeypadPeriod    = 0x63,    /* Keypad . or Delete */
    kHIDUsage_KeyboardNonUSBackslash    = 0x64,    /* Non-US \ or | */
    kHIDUsage_KeyboardApplication    = 0x65,    /* Application */
    kHIDUsage_KeyboardPower    = 0x66,    /* Power */
    kHIDUsage_KeypadEqualSign    = 0x67,    /* Keypad = */
    kHIDUsage_KeyboardF13    = 0x68,    /* F13 */
    kHIDUsage_KeyboardF14    = 0x69,    /* F14 */
    kHIDUsage_KeyboardF15    = 0x6A,    /* F15 */
    kHIDUsage_KeyboardF16    = 0x6B,    /* F16 */
    kHIDUsage_KeyboardF17    = 0x6C,    /* F17 */
    kHIDUsage_KeyboardF18    = 0x6D,    /* F18 */
    kHIDUsage_KeyboardF19    = 0x6E,    /* F19 */
    kHIDUsage_KeyboardF20    = 0x6F,    /* F20 */
    kHIDUsage_KeyboardF21    = 0x70,    /* F21 */
    kHIDUsage_KeyboardF22    = 0x71,    /* F22 */
    kHIDUsage_KeyboardF23    = 0x72,    /* F23 */
    kHIDUsage_KeyboardF24    = 0x73,    /* F24 */
    kHIDUsage_KeyboardExecute    = 0x74,    /* Execute */
    kHIDUsage_KeyboardHelp    = 0x75,    /* Help */
    kHIDUsage_KeyboardMenu    = 0x76,    /* Menu */
    kHIDUsage_KeyboardSelect    = 0x77,    /* Select */
    kHIDUsage_KeyboardStop    = 0x78,    /* Stop */
    kHIDUsage_KeyboardAgain    = 0x79,    /* Again */
    kHIDUsage_KeyboardUndo    = 0x7A,    /* Undo */
    kHIDUsage_KeyboardCut    = 0x7B,    /* Cut */
    kHIDUsage_KeyboardCopy    = 0x7C,    /* Copy */
    kHIDUsage_KeyboardPaste    = 0x7D,    /* Paste */
    kHIDUsage_KeyboardFind    = 0x7E,    /* Find */
    kHIDUsage_KeyboardMute    = 0x7F,    /* Mute */
    kHIDUsage_KeyboardVolumeUp    = 0x80,    /* Volume Up */
    kHIDUsage_KeyboardVolumeDown    = 0x81,    /* Volume Down */
    kHIDUsage_KeyboardLockingCapsLock    = 0x82,    /* Locking Caps Lock */
    kHIDUsage_KeyboardLockingNumLock    = 0x83,    /* Locking Num Lock */
    kHIDUsage_KeyboardLockingScrollLock    = 0x84,    /* Locking Scroll Lock */
    kHIDUsage_KeypadComma    = 0x85,    /* Keypad Comma */
    kHIDUsage_KeypadEqualSignAS400    = 0x86,    /* Keypad Equal Sign for AS/400 */
    kHIDUsage_KeyboardInternational1    = 0x87,    /* International1 */
    kHIDUsage_KeyboardInternational2    = 0x88,    /* International2 */
    kHIDUsage_KeyboardInternational3    = 0x89,    /* International3 */
    kHIDUsage_KeyboardInternational4    = 0x8A,    /* International4 */
    kHIDUsage_KeyboardInternational5    = 0x8B,    /* International5 */
    kHIDUsage_KeyboardInternational6    = 0x8C,    /* International6 */
    kHIDUsage_KeyboardInternational7    = 0x8D,    /* International7 */
    kHIDUsage_KeyboardInternational8    = 0x8E,    /* International8 */
    kHIDUsage_KeyboardInternational9    = 0x8F,    /* International9 */
    kHIDUsage_KeyboardLANG1    = 0x90,    /* LANG1 */
    kHIDUsage_KeyboardLANG2    = 0x91,    /* LANG2 */
    kHIDUsage_KeyboardLANG3    = 0x92,    /* LANG3 */
    kHIDUsage_KeyboardLANG4    = 0x93,    /* LANG4 */
    kHIDUsage_KeyboardLANG5    = 0x94,    /* LANG5 */
    kHIDUsage_KeyboardLANG6    = 0x95,    /* LANG6 */
    kHIDUsage_KeyboardLANG7    = 0x96,    /* LANG7 */
    kHIDUsage_KeyboardLANG8    = 0x97,    /* LANG8 */
    kHIDUsage_KeyboardLANG9    = 0x98,    /* LANG9 */
    kHIDUsage_KeyboardAlternateErase    = 0x99,    /* AlternateErase */
    kHIDUsage_KeyboardSysReqOrAttention    = 0x9A,    /* SysReq/Attention */
    kHIDUsage_KeyboardCancel    = 0x9B,    /* Cancel */
    kHIDUsage_KeyboardClear    = 0x9C,    /* Clear */
    kHIDUsage_KeyboardPrior    = 0x9D,    /* Prior */
    kHIDUsage_KeyboardReturn    = 0x9E,    /* Return */
    kHIDUsage_KeyboardSeparator    = 0x9F,    /* Separator */
    kHIDUsage_KeyboardOut    = 0xA0,    /* Out */
    kHIDUsage_KeyboardOper    = 0xA1,    /* Oper */
    kHIDUsage_KeyboardClearOrAgain    = 0xA2,    /* Clear/Again */
    kHIDUsage_KeyboardCrSelOrProps    = 0xA3,    /* CrSel/Props */
    kHIDUsage_KeyboardExSel    = 0xA4,    /* ExSel */
    /* 0xA5-0xDF Reserved */
    kHIDUsage_KeyboardLeftControl    = 0xE0,    /* Left Control */
    kHIDUsage_KeyboardLeftShift    = 0xE1,    /* Left Shift */
    kHIDUsage_KeyboardLeftAlt    = 0xE2,    /* Left Alt */
    kHIDUsage_KeyboardLeftGUI    = 0xE3,    /* Left GUI */
    kHIDUsage_KeyboardRightControl    = 0xE4,    /* Right Control */
    kHIDUsage_KeyboardRightShift    = 0xE5,    /* Right Shift */
    kHIDUsage_KeyboardRightAlt    = 0xE6,    /* Right Alt */
    kHIDUsage_KeyboardRightGUI    = 0xE7,    /* Right GUI */
    /* 0xE8-0xFFFF Reserved */
    kHIDUsage_Keyboard_Reserved = 0xFFFF
};

/* LEDs Page (0x08) */
/* An LED or indicator is implemented as an On/Off Control (OOF) using the “Single button toggle” mode, where a value of 1 will turn on the indicator, and a value of 0 will turn it off. The exceptions are described below. */
enum
{
    kHIDUsage_LED_NumLock    = 0x01,    /* On/Off Control */
    kHIDUsage_LED_CapsLock    = 0x02,    /* On/Off Control */
    kHIDUsage_LED_ScrollLock    = 0x03,    /* On/Off Control */
    kHIDUsage_LED_Compose    = 0x04,    /* On/Off Control */
    kHIDUsage_LED_Kana    = 0x05,    /* On/Off Control */
    kHIDUsage_LED_Power    = 0x06,    /* On/Off Control */
    kHIDUsage_LED_Shift    = 0x07,    /* On/Off Control */
    kHIDUsage_LED_DoNotDisturb    = 0x08,    /* On/Off Control */
    kHIDUsage_LED_Mute    = 0x09,    /* On/Off Control */
    kHIDUsage_LED_ToneEnable    = 0x0A,    /* On/Off Control */
    kHIDUsage_LED_HighCutFilter    = 0x0B,    /* On/Off Control */
    kHIDUsage_LED_LowCutFilter    = 0x0C,    /* On/Off Control */
    kHIDUsage_LED_EqualizerEnable    = 0x0D,    /* On/Off Control */
    kHIDUsage_LED_SoundFieldOn    = 0x0E,    /* On/Off Control */
    kHIDUsage_LED_SurroundOn    = 0x0F,    /* On/Off Control */
    kHIDUsage_LED_Repeat    = 0x10,    /* On/Off Control */
    kHIDUsage_LED_Stereo    = 0x11,    /* On/Off Control */
    kHIDUsage_LED_SamplingRateDetect    = 0x12,    /* On/Off Control */
    kHIDUsage_LED_Spinning    = 0x13,    /* On/Off Control */
    kHIDUsage_LED_CAV    = 0x14,    /* On/Off Control */
    kHIDUsage_LED_CLV    = 0x15,    /* On/Off Control */
    kHIDUsage_LED_RecordingFormatDetect    = 0x16,    /* On/Off Control */
    kHIDUsage_LED_OffHook    = 0x17,    /* On/Off Control */
    kHIDUsage_LED_Ring    = 0x18,    /* On/Off Control */
    kHIDUsage_LED_MessageWaiting    = 0x19,    /* On/Off Control */
    kHIDUsage_LED_DataMode    = 0x1A,    /* On/Off Control */
    kHIDUsage_LED_BatteryOperation    = 0x1B,    /* On/Off Control */
    kHIDUsage_LED_BatteryOK    = 0x1C,    /* On/Off Control */
    kHIDUsage_LED_BatteryLow    = 0x1D,    /* On/Off Control */
    kHIDUsage_LED_Speaker    = 0x1E,    /* On/Off Control */
    kHIDUsage_LED_HeadSet    = 0x1F,    /* On/Off Control */
    kHIDUsage_LED_Hold    = 0x20,    /* On/Off Control */
    kHIDUsage_LED_Microphone    = 0x21,    /* On/Off Control */
    kHIDUsage_LED_Coverage    = 0x22,    /* On/Off Control */
    kHIDUsage_LED_NightMode    = 0x23,    /* On/Off Control */
    kHIDUsage_LED_SendCalls    = 0x24,    /* On/Off Control */
    kHIDUsage_LED_CallPickup    = 0x25,    /* On/Off Control */
    kHIDUsage_LED_Conference    = 0x26,    /* On/Off Control */
    kHIDUsage_LED_StandBy    = 0x27,    /* On/Off Control */
    kHIDUsage_LED_CameraOn    = 0x28,    /* On/Off Control */
    kHIDUsage_LED_CameraOff    = 0x29,    /* On/Off Control */
    kHIDUsage_LED_OnLine    = 0x2A,    /* On/Off Control */
    kHIDUsage_LED_OffLine    = 0x2B,    /* On/Off Control */
    kHIDUsage_LED_Busy    = 0x2C,    /* On/Off Control */
    kHIDUsage_LED_Ready    = 0x2D,    /* On/Off Control */
    kHIDUsage_LED_PaperOut    = 0x2E,    /* On/Off Control */
    kHIDUsage_LED_PaperJam    = 0x2F,    /* On/Off Control */
    kHIDUsage_LED_Remote    = 0x30,    /* On/Off Control */
    kHIDUsage_LED_Forward    = 0x31,    /* On/Off Control */
    kHIDUsage_LED_Reverse    = 0x32,    /* On/Off Control */
    kHIDUsage_LED_Stop    = 0x33,    /* On/Off Control */
    kHIDUsage_LED_Rewind    = 0x34,    /* On/Off Control */
    kHIDUsage_LED_FastForward    = 0x35,    /* On/Off Control */
    kHIDUsage_LED_Play    = 0x36,    /* On/Off Control */
    kHIDUsage_LED_Pause    = 0x37,    /* On/Off Control */
    kHIDUsage_LED_Record    = 0x38,    /* On/Off Control */
    kHIDUsage_LED_Error    = 0x39,    /* On/Off Control */
    kHIDUsage_LED_Usage    = 0x3A,    /* Selector */
    kHIDUsage_LED_UsageInUseIndicator    = 0x3B,    /* Usage Switch */
    kHIDUsage_LED_UsageMultiModeIndicator    = 0x3C,    /* Usage Modifier */
    kHIDUsage_LED_IndicatorOn    = 0x3D,    /* Selector */
    kHIDUsage_LED_IndicatorFlash    = 0x3E,    /* Selector */
    kHIDUsage_LED_IndicatorSlowBlink    = 0x3F,    /* Selector */
    kHIDUsage_LED_IndicatorFastBlink    = 0x40,    /* Selector */
    kHIDUsage_LED_IndicatorOff    = 0x41,    /* Selector */
    kHIDUsage_LED_FlashOnTime    = 0x42,    /* Dynamic Value */
    kHIDUsage_LED_SlowBlinkOnTime    = 0x43,    /* Dynamic Value */
    kHIDUsage_LED_SlowBlinkOffTime    = 0x44,    /* Dynamic Value */
    kHIDUsage_LED_FastBlinkOnTime    = 0x45,    /* Dynamic Value */
    kHIDUsage_LED_FastBlinkOffTime    = 0x46,    /* Dynamic Value */
    kHIDUsage_LED_UsageIndicatorColor    = 0x47,    /* Usage Modifier */
    kHIDUsage_LED_IndicatorRed    = 0x48,    /* Selector */
    kHIDUsage_LED_IndicatorGreen    = 0x49,    /* Selector */
    kHIDUsage_LED_IndicatorAmber    = 0x4A,    /* Selector */
    kHIDUsage_LED_GenericIndicator    = 0x4B,    /* On/Off Control */
    kHIDUsage_LED_SystemSuspend    = 0x4C,    /* On/Off Control */
    kHIDUsage_LED_ExternalPowerConnected    = 0x4D,    /* On/Off Control */
    /* 0x4E - 0xFFFF Reserved */
    kHIDUsage_LED_Reserved = 0xFFFF
};

/* Button Page (0x09) */
/* The Button page is the first place an application should look for user selection controls. System graphical user interfaces typically employ a pointer and a set of hierarchical selectors to select, move and otherwise manipulate their environment. For these purposes the following assignment of significance can be applied to the Button usages: */
/* • Button 1, Primary Button. Used for object selecting, dragging, and double click activation. On MacOS, this is the only button. Microsoft operating systems call this a logical left button, because it */
/* is not necessarily physically located on the left of the pointing device. */
/* • Button 2, Secondary Button. Used by newer graphical user interfaces to browse object properties. Exposed by systems to applications that typically assign application-specific functionality. */
/* • Button 3, Tertiary Button. Optional control. Exposed to applications, but seldom assigned functionality due to prevalence of two- and one-button devices. */
/* • Buttons 4 -55. As the button number increases, its significance as a selector decreases. */
/* In many ways the assignment of button numbers is similar to the assignment of Effort in Physical descriptors. Button 1 would be used to define the button a finger rests on when the hand is in the “at rest” position, that is, virtually no effort is required by the user to activate the button. Button values increment as the finger has to stretch to reach a control. See Section 6.2.3, “Physical Descriptors,” in the HID Specification for methods of further qualifying buttons. */
enum
{
    kHIDUsage_Button_1    = 0x01,    /* (primary/trigger) */
    kHIDUsage_Button_2    = 0x02,    /* (secondary) */
    kHIDUsage_Button_3    = 0x03,    /* (tertiary) */
    kHIDUsage_Button_4    = 0x04, /* 4th button */
    /* ... */
    kHIDUsage_Button_65535    = 0xFFFF
};

/* Ordinal Page (0x0A) */
/* The Ordinal page allows multiple instances of a control or sets of controls to be declared without requiring individual enumeration in the native usage page. For example, it is not necessary to declare usages of Pointer 1, Pointer 2, and so forth on the Generic Desktop page. When parsed, the ordinal instance number is, in essence, concatenated to the usages attached to the encompassing collection to create Pointer 1, Pointer 2, and so forth. */
/* For an example, see Section A.5, “Multiple Instances of a Control,” in Appendix A, “Usage Examples.” By convention, an Ordinal collection is placed inside the collection for which it is declaring multiple instances. */
/* Instances do not have to be identical. */
enum
{
    /* 0x00 Reserved */
    kHIDUsage_Ord_Instance1    = 0x01,    /* Usage Modifier */
    kHIDUsage_Ord_Instance2    = 0x02,    /* Usage Modifier */
    kHIDUsage_Ord_Instance3    = 0x03,    /* Usage Modifier */
    kHIDUsage_Ord_Instance4    = 0x04,    /* Usage Modifier */
    kHIDUsage_Ord_Instance65535    = 0xFFFF    /* Usage Modifier */
};

/* Telephony Page (0x0B) */
/* This usage page defines the keytop and control usages for telephony devices. */
/* Indicators on a phone are handled by wrapping them in LED: Usage In Use Indicator and LED: Usage Selected Indicator usages. For example, a message-indicator LED would be identified by a Telephony: Message usage declared as a Feature or Output in a LED: Usage In Use Indicator collection. */
/* See Section 14, “Consumer Page (0x0C),” for audio volume and tone controls. */
enum
{
    kHIDUsage_Tfon_Phone    = 0x01,    /* Application Collection */
    kHIDUsage_Tfon_AnsweringMachine    = 0x02,    /* Application Collection */
    kHIDUsage_Tfon_MessageControls    = 0x03,    /* Logical Collection */
    kHIDUsage_Tfon_Handset    = 0x04,    /* Logical Collection */
    kHIDUsage_Tfon_Headset    = 0x05,    /* Logical Collection */
    kHIDUsage_Tfon_TelephonyKeyPad    = 0x06,    /* Named Array */
    kHIDUsage_Tfon_ProgrammableButton    = 0x07,    /* Named Array */
    /* 0x08 - 0x1F Reserved */
    kHIDUsage_Tfon_HookSwitch    = 0x20,    /* On/Off Control */
    kHIDUsage_Tfon_Flash    = 0x21,    /* Momentary Control */
    kHIDUsage_Tfon_Feature    = 0x22,    /* One-Shot Control */
    kHIDUsage_Tfon_Hold    = 0x23,    /* On/Off Control */
    kHIDUsage_Tfon_Redial    = 0x24,    /* One-Shot Control */
    kHIDUsage_Tfon_Transfer    = 0x25,    /* One-Shot Control */
    kHIDUsage_Tfon_Drop    = 0x26,    /* One-Shot Control */
    kHIDUsage_Tfon_Park    = 0x27,    /* On/Off Control */
    kHIDUsage_Tfon_ForwardCalls    = 0x28,    /* On/Off Control */
    kHIDUsage_Tfon_AlternateFunction    = 0x29,    /* Momentary Control */
    kHIDUsage_Tfon_Line    = 0x2A,    /* One-Shot Control */
    kHIDUsage_Tfon_SpeakerPhone    = 0x2B,    /* On/Off Control */
    kHIDUsage_Tfon_Conference    = 0x2C,    /* On/Off Control */
    kHIDUsage_Tfon_RingEnable    = 0x2D,    /* On/Off Control */
    kHIDUsage_Tfon_Ring    = 0x2E,    /* Selector */
    kHIDUsage_Tfon_PhoneMute    = 0x2F,    /* On/Off Control */
    kHIDUsage_Tfon_CallerID    = 0x30,    /* Momentary Control */
    /* 0x31 - 0x4F Reserved */
    kHIDUsage_Tfon_SpeedDial    = 0x50,    /* One-Shot Control */
    kHIDUsage_Tfon_StoreNumber    = 0x51,    /* One-Shot Control */
    kHIDUsage_Tfon_RecallNumber    = 0x52,    /* One-Shot Control */
    kHIDUsage_Tfon_PhoneDirectory    = 0x53,    /* On/Off Control */
    /* 0x54 - 0x6F Reserved */
    kHIDUsage_Tfon_VoiceMail    = 0x70,    /* On/Off Control */
    kHIDUsage_Tfon_ScreenCalls    = 0x71,    /* On/Off Control */
    kHIDUsage_Tfon_DoNotDisturb    = 0x72,    /* On/Off Control */
    kHIDUsage_Tfon_Message    = 0x73,    /* One-Shot Control */
    kHIDUsage_Tfon_AnswerOnOrOff    = 0x74,    /* On/Off Control */
    /* 0x75 - 0x8F Reserved */
    kHIDUsage_Tfon_InsideDialTone    = 0x90,    /* Momentary Control */
    kHIDUsage_Tfon_OutsideDialTone    = 0x91,    /* Momentary Control */
    kHIDUsage_Tfon_InsideRingTone    = 0x92,    /* Momentary Control */
    kHIDUsage_Tfon_OutsideRingTone    = 0x93,    /* Momentary Control */
    kHIDUsage_Tfon_PriorityRingTone    = 0x94,    /* Momentary Control */
    kHIDUsage_Tfon_InsideRingback    = 0x95,    /* Momentary Control */
    kHIDUsage_Tfon_PriorityRingback    = 0x96,    /* Momentary Control */
    kHIDUsage_Tfon_LineBusyTone    = 0x97,    /* Momentary Control */
    kHIDUsage_Tfon_ReorderTone    = 0x98,    /* Momentary Control */
    kHIDUsage_Tfon_CallWaitingTone    = 0x99,    /* Momentary Control */
    kHIDUsage_Tfon_ConfirmationTone1    = 0x9A,    /* Momentary Control */
    kHIDUsage_Tfon_ConfirmationTone2    = 0x9B,    /* Momentary Control */
    kHIDUsage_Tfon_TonesOff    = 0x9C,    /* On/Off Control */
    kHIDUsage_Tfon_OutsideRingback    = 0x9D,    /* Momentary Control */
    /* 0x9E - 0xAF Reserved */
    kHIDUsage_Tfon_PhoneKey0    = 0xB0,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey1    = 0xB1,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey2    = 0xB2,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey3    = 0xB3,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey4    = 0xB4,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey5    = 0xB5,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey6    = 0xB6,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey7    = 0xB7,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey8    = 0xB8,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKey9    = 0xB9,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKeyStar    = 0xBA,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKeyPound    = 0xBB,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKeyA    = 0xBC,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKeyB    = 0xBD,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKeyC    = 0xBE,    /* Selector/One-Shot Control */
    kHIDUsage_Tfon_PhoneKeyD    = 0xBF,    /* Selector/One-Shot Control */
    /* 0xC0 - 0xFFFF Reserved */
    kHIDUsage_TFon_Reserved = 0xFFFF
};

/* Consumer Page (0x0C) */
/* All controls on the Consumer page are application-specific. That is, they affect a specific device, not the system as a whole. */
enum
{
    kHIDUsage_Csmr_ConsumerControl    = 0x01,    /* Application Collection */
    kHIDUsage_Csmr_NumericKeyPad    = 0x02,    /* Named Array */
    kHIDUsage_Csmr_ProgrammableButtons    = 0x03,    /* Named Array */
    kHIDUsage_Csmr_Microphone   = 0x04,    /* Application Collection */
    kHIDUsage_Csmr_Headphone   = 0x05,    /* Application Collection */
    kHIDUsage_Csmr_GraphicEqualizer   = 0x06,    /* Application Collection */
    /* 0x07 - 0x1F Reserved */
    kHIDUsage_Csmr_Plus10    = 0x20,    /* One-Shot Control */
    kHIDUsage_Csmr_Plus100    = 0x21,    /* One-Shot Control */
    kHIDUsage_Csmr_AMOrPM    = 0x22,    /* One-Shot Control */
    /* 0x23 - 0x3F Reserved */
    kHIDUsage_Csmr_Power    = 0x30,    /* On/Off Control */
    kHIDUsage_Csmr_Reset    = 0x31,    /* One-Shot Control */
    kHIDUsage_Csmr_Sleep    = 0x32,    /* One-Shot Control */
    kHIDUsage_Csmr_SleepAfter    = 0x33,    /* One-Shot Control */
    kHIDUsage_Csmr_SleepMode    = 0x34,    /* Re-Trigger Control */
    kHIDUsage_Csmr_Illumination    = 0x35,    /* On/Off Control */
    kHIDUsage_Csmr_FunctionButtons    = 0x36,    /* Named Array */
    /* 0x37 - 0x3F Reserved */
    kHIDUsage_Csmr_Menu    = 0x40,    /* On/Off Control */
    kHIDUsage_Csmr_MenuPick    = 0x41,    /* One-Shot Control */
    kHIDUsage_Csmr_MenuUp    = 0x42,    /* One-Shot Control */
    kHIDUsage_Csmr_MenuDown    = 0x43,    /* One-Shot Control */
    kHIDUsage_Csmr_MenuLeft    = 0x44,    /* One-Shot Control */
    kHIDUsage_Csmr_MenuRight    = 0x45,    /* One-Shot Control */
    kHIDUsage_Csmr_MenuEscape    = 0x46,    /* One-Shot Control */
    kHIDUsage_Csmr_MenuValueIncrease    = 0x47,    /* One-Shot Control */
    kHIDUsage_Csmr_MenuValueDecrease    = 0x48,    /* One-Shot Control */
    /* 0x49 - 0x5F Reserved */
    kHIDUsage_Csmr_DataOnScreen    = 0x60,    /* On/Off Control */
    kHIDUsage_Csmr_ClosedCaption    = 0x61,    /* On/Off Control */
    kHIDUsage_Csmr_ClosedCaptionSelect    = 0x62,    /* Selector */
    kHIDUsage_Csmr_VCROrTV    = 0x63,    /* On/Off Control */
    kHIDUsage_Csmr_BroadcastMode    = 0x64,    /* One-Shot Control */
    kHIDUsage_Csmr_Snapshot    = 0x65,    /* One-Shot Control */
    kHIDUsage_Csmr_Still    = 0x66,    /* One-Shot Control */
    kHIDUsage_Csmr_PictureInPictureToggle = 0x67, /* One-Shot Control */
    kHIDUsage_Csmr_PictureInPictureSwap = 0x68, /* One-Shot Control */
    kHIDUsage_Csmr_RedMenuButton = 0x69, /* Momentary Control */
    kHIDUsage_Csmr_GreenMenuButton = 0x6A, /* Momentary Control */
    kHIDUsage_Csmr_BlueMenuButton = 0x6B, /* Momentary Control */
    kHIDUsage_Csmr_YellowMenuButton = 0x6C, /* Momentary Control */
    kHIDUsage_Csmr_Aspect = 0x6D, /* One-shot Control */
    kHIDUsage_Csmr_3DModeSelect = 0x6E, /* One-shot Control */
    kHIDUsage_Csmr_DisplayBrightnessIncrement = 0x6F, /* Re-Trigger Control */
    kHIDUsage_Csmr_DisplayBrightnessDecrement = 0x70, /* Re-Trigger Control */
    kHIDUsage_Csmr_DisplayBrightness = 0x71, /* Logical Collection */
    kHIDUsage_Csmr_DisplayBacklightToggle = 0x72, /* One Shot Control */
    kHIDUsage_Csmr_DisplayBrightnessMinimum = 0x73, /* One Shot Control */
    kHIDUsage_Csmr_DisplayBrightnessMaximum = 0x74, /* One Shot Control */
    kHIDUsage_Csmr_DisplayBrightnessSetAutoBrightness = 0x75, /* On Off Control */
    /* 0x76 - 0x7F Reserved */
    kHIDUsage_Csmr_Selection    = 0x80,    /* Named Array */
    kHIDUsage_Csmr_Assign    = 0x81,    /* Selector */
    kHIDUsage_Csmr_ModeStep    = 0x82,    /* One-Shot Control */
    kHIDUsage_Csmr_RecallLast    = 0x83,    /* One-Shot Control */
    kHIDUsage_Csmr_EnterChannel    = 0x84,    /* One-Shot Control */
    kHIDUsage_Csmr_OrderMovie    = 0x85,    /* One-Shot Control */
    kHIDUsage_Csmr_Channel    = 0x86,    /* Linear Control */
    kHIDUsage_Csmr_MediaSelection    = 0x87,    /* Selector */
    kHIDUsage_Csmr_MediaSelectComputer    = 0x88,    /* Selector */
    kHIDUsage_Csmr_MediaSelectTV    = 0x89,    /* Selector */
    kHIDUsage_Csmr_MediaSelectWWW    = 0x8A,    /* Selector */
    kHIDUsage_Csmr_MediaSelectDVD    = 0x8B,    /* Selector */
    kHIDUsage_Csmr_MediaSelectTelephone    = 0x8C,    /* Selector */
    kHIDUsage_Csmr_MediaSelectProgramGuide    = 0x8D,    /* Selector */
    kHIDUsage_Csmr_MediaSelectVideoPhone    = 0x8E,    /* Selector */
    kHIDUsage_Csmr_MediaSelectGames    = 0x8F,    /* Selector */
    kHIDUsage_Csmr_MediaSelectMessages    = 0x90,    /* Selector */
    kHIDUsage_Csmr_MediaSelectCD    = 0x91,    /* Selector */
    kHIDUsage_Csmr_MediaSelectVCR    = 0x92,    /* Selector */
    kHIDUsage_Csmr_MediaSelectTuner    = 0x93,    /* Selector */
    kHIDUsage_Csmr_Quit    = 0x94,    /* One-Shot Control */
    kHIDUsage_Csmr_Help    = 0x95,    /* On/Off Control */
    kHIDUsage_Csmr_MediaSelectTape    = 0x96,    /* Selector */
    kHIDUsage_Csmr_MediaSelectCable    = 0x97,    /* Selector */
    kHIDUsage_Csmr_MediaSelectSatellite    = 0x98,    /* Selector */
    kHIDUsage_Csmr_MediaSelectSecurity    = 0x99,    /* Selector */
    kHIDUsage_Csmr_MediaSelectHome    = 0x9A,    /* Selector */
    kHIDUsage_Csmr_MediaSelectCall    = 0x9B,    /* Selector */
    kHIDUsage_Csmr_ChannelIncrement    = 0x9C,    /* One-Shot Control */
    kHIDUsage_Csmr_ChannelDecrement    = 0x9D,    /* One-Shot Control */
    kHIDUsage_Csmr_Media    = 0x9E,    /* Selector */
    /* 0x9F Reserved */
    kHIDUsage_Csmr_VCRPlus    = 0xA0,    /* One-Shot Control */
    kHIDUsage_Csmr_Once    = 0xA1,    /* One-Shot Control */
    kHIDUsage_Csmr_Daily    = 0xA2,    /* One-Shot Control */
    kHIDUsage_Csmr_Weekly    = 0xA3,    /* One-Shot Control */
    kHIDUsage_Csmr_Monthly    = 0xA4,    /* One-Shot Control */
    /* 0xA5 - 0xAF Reserved */
    kHIDUsage_Csmr_Play    = 0xB0,    /* On/Off Control */
    kHIDUsage_Csmr_Pause    = 0xB1,    /* On/Off Control */
    kHIDUsage_Csmr_Record    = 0xB2,    /* On/Off Control */
    kHIDUsage_Csmr_FastForward    = 0xB3,    /* On/Off Control */
    kHIDUsage_Csmr_Rewind    = 0xB4,    /* On/Off Control */
    kHIDUsage_Csmr_ScanNextTrack    = 0xB5,    /* One-Shot Control */
    kHIDUsage_Csmr_ScanPreviousTrack    = 0xB6,    /* One-Shot Control */
    kHIDUsage_Csmr_Stop    = 0xB7,    /* One-Shot Control */
    kHIDUsage_Csmr_Eject    = 0xB8,    /* One-Shot Control */
    kHIDUsage_Csmr_RandomPlay    = 0xB9,    /* On/Off Control */
    kHIDUsage_Csmr_SelectDisc    = 0xBA,    /* Named Array */
    kHIDUsage_Csmr_EnterDisc    = 0xBB,    /* Momentary Control */
    kHIDUsage_Csmr_Repeat    = 0xBC,    /* One-Shot Control */
    kHIDUsage_Csmr_Tracking    = 0xBD,    /* Linear Control */
    kHIDUsage_Csmr_TrackNormal    = 0xBE,    /* One-Shot Control */
    kHIDUsage_Csmr_SlowTracking    = 0xBF,    /* Linear Control */
    kHIDUsage_Csmr_FrameForward    = 0xC0,    /* Re-Trigger Control */
    kHIDUsage_Csmr_FrameBack    = 0xC1,    /* Re-Trigger Control */
    kHIDUsage_Csmr_Mark    = 0xC2,    /* One-Shot Control */
    kHIDUsage_Csmr_ClearMark    = 0xC3,    /* One-Shot Control */
    kHIDUsage_Csmr_RepeatFromMark    = 0xC4,    /* On/Off Control */
    kHIDUsage_Csmr_ReturnToMark    = 0xC5,    /* One-Shot Control */
    kHIDUsage_Csmr_SearchMarkForward    = 0xC6,    /* One-Shot Control */
    kHIDUsage_Csmr_SearchMarkBackwards    = 0xC7,    /* One-Shot Control */
    kHIDUsage_Csmr_CounterReset    = 0xC8,    /* One-Shot Control */
    kHIDUsage_Csmr_ShowCounter    = 0xC9,    /* One-Shot Control */
    kHIDUsage_Csmr_TrackingIncrement    = 0xCA,    /* Re-Trigger Control */
    kHIDUsage_Csmr_TrackingDecrement    = 0xCB,    /* Re-Trigger Control */
    kHIDUsage_Csmr_StopOrEject    = 0xCC,    /* One-Shot Control */
    kHIDUsage_Csmr_PlayOrPause    = 0xCD,    /* One-Shot Control */
    kHIDUsage_Csmr_PlayOrSkip    = 0xCE,    /* One-Shot Control */
    kHIDUsage_Csmr_VoiceCommand    = 0xCF, /* One-Shot Control */
    /* 0xCF - 0xDF Reserved */
    kHIDUsage_Csmr_Volume    = 0xE0,    /* Linear Control */
    kHIDUsage_Csmr_Balance    = 0xE1,    /* Linear Control */
    kHIDUsage_Csmr_Mute    = 0xE2,    /* On/Off Control */
    kHIDUsage_Csmr_Bass    = 0xE3,    /* Linear Control */
    kHIDUsage_Csmr_Treble    = 0xE4,    /* Linear Control */
    kHIDUsage_Csmr_BassBoost    = 0xE5,    /* On/Off Control */
    kHIDUsage_Csmr_SurroundMode    = 0xE6,    /* One-Shot Control */
    kHIDUsage_Csmr_Loudness    = 0xE7,    /* On/Off Control */
    kHIDUsage_Csmr_MPX    = 0xE8,    /* On/Off Control */
    kHIDUsage_Csmr_VolumeIncrement    = 0xE9,    /* Re-Trigger Control */
    kHIDUsage_Csmr_VolumeDecrement    = 0xEA,    /* Re-Trigger Control */
    /* 0xEB - 0xEF Reserved */
    kHIDUsage_Csmr_Speed    = 0xF0,    /* Selector */
    kHIDUsage_Csmr_PlaybackSpeed    = 0xF1,    /* Named Array */
    kHIDUsage_Csmr_StandardPlay    = 0xF2,    /* Selector */
    kHIDUsage_Csmr_LongPlay    = 0xF3,    /* Selector */
    kHIDUsage_Csmr_ExtendedPlay    = 0xF4,    /* Selector */
    kHIDUsage_Csmr_Slow    = 0xF5,    /* One-Shot Control */
    /* 0xF6 - 0xFF Reserved */
    kHIDUsage_Csmr_FanEnable    = 0x100,    /* On/Off Control */
    kHIDUsage_Csmr_FanSpeed    = 0x101,    /* Linear Control */
    kHIDUsage_Csmr_LightEnable    = 0x102,    /* On/Off Control */
    kHIDUsage_Csmr_LightIlluminationLevel    = 0x103,    /* Linear Control */
    kHIDUsage_Csmr_ClimateControlEnable    = 0x104,    /* On/Off Control */
    kHIDUsage_Csmr_RoomTemperature    = 0x105,    /* Linear Control */
    kHIDUsage_Csmr_SecurityEnable    = 0x106,    /* On/Off Control */
    kHIDUsage_Csmr_FireAlarm    = 0x107,    /* One-Shot Control */
    kHIDUsage_Csmr_PoliceAlarm    = 0x108,    /* One-Shot Control */
    kHIDUsage_Csmr_Proximity    = 0x109,    /* Linear Control */
    kHIDUsage_Csmr_Motion    = 0x10A,    /* One-Shot Control */
    kHIDUsage_Csmr_DuressAlarm    = 0x10B,    /* One-Shot Control */
    kHIDUsage_Csmr_HoldupAlarm    = 0x10C,    /* One-Shot Control */
    kHIDUsage_Csmr_MedicalAlarm    = 0x10D,    /* One-Shot Control */
    /* 0x10E - 0x14F Reserved */
    kHIDUsage_Csmr_BalanceRight    = 0x150,    /* Re-Trigger Control */
    kHIDUsage_Csmr_BalanceLeft    = 0x151,    /* Re-Trigger Control */
    kHIDUsage_Csmr_BassIncrement    = 0x152,    /* Re-Trigger Control */
    kHIDUsage_Csmr_BassDecrement    = 0x153,    /* Re-Trigger Control */
    kHIDUsage_Csmr_TrebleIncrement    = 0x154,    /* Re-Trigger Control */
    kHIDUsage_Csmr_TrebleDecrement    = 0x155,    /* Re-Trigger Control */
    /* 0x156 - 0x15F Reserved */
    kHIDUsage_Csmr_SpeakerSystem    = 0x160,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelLeft    = 0x161,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelRight    = 0x162,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelCenter    = 0x163,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelFront    = 0x164,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelCenterFront    = 0x165,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelSide    = 0x166,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelSurround    = 0x167,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelLowFrequencyEnhancement    = 0x168,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelTop    = 0x169,    /* Logical Collection */
    kHIDUsage_Csmr_ChannelUnknown    = 0x16A,    /* Logical Collection */
    /* 0x16B - 0x16F Reserved */
    kHIDUsage_Csmr_SubChannel    = 0x170,    /* Linear Control */
    kHIDUsage_Csmr_SubChannelIncrement    = 0x171,    /* One-Shot Control */
    kHIDUsage_Csmr_SubChannelDecrement    = 0x172,    /* One-Shot Control */
    kHIDUsage_Csmr_AlternateAudioIncrement    = 0x173,    /* One-Shot Control */
    kHIDUsage_Csmr_AlternateAudioDecrement    = 0x174,    /* One-Shot Control */
    /* 0x175 - 0x17F Reserved */
    kHIDUsage_Csmr_ApplicationLaunchButtons    = 0x180,    /* Named Array */
    kHIDUsage_Csmr_ALLaunchButtonConfigurationTool    = 0x181,    /* Selector */
    kHIDUsage_Csmr_ALProgrammableButtonConfiguration    = 0x182,    /* Selector */
    kHIDUsage_Csmr_ALConsumerControlConfiguration    = 0x183,    /* Selector */
    kHIDUsage_Csmr_ALWordProcessor    = 0x184,    /* Selector */
    kHIDUsage_Csmr_ALTextEditor    = 0x185,    /* Selector */
    kHIDUsage_Csmr_ALSpreadsheet    = 0x186,    /* Selector */
    kHIDUsage_Csmr_ALGraphicsEditor    = 0x187,    /* Selector */
    kHIDUsage_Csmr_ALPresentationApp    = 0x188,    /* Selector */
    kHIDUsage_Csmr_ALDatabaseApp    = 0x189,    /* Selector */
    kHIDUsage_Csmr_ALEmailReader    = 0x18A,    /* Selector */
    kHIDUsage_Csmr_ALNewsreader    = 0x18B,    /* Selector */
    kHIDUsage_Csmr_ALVoicemail    = 0x18C,    /* Selector */
    kHIDUsage_Csmr_ALContactsOrAddressBook    = 0x18D,    /* Selector */
    kHIDUsage_Csmr_ALCalendarOrSchedule    = 0x18E,    /* Selector */
    kHIDUsage_Csmr_ALTaskOrProjectManager    = 0x18F,    /* Selector */
    kHIDUsage_Csmr_ALLogOrJournalOrTimecard    = 0x190,    /* Selector */
    kHIDUsage_Csmr_ALCheckbookOrFinance    = 0x191,    /* Selector */
    kHIDUsage_Csmr_ALCalculator    = 0x192,    /* Selector */
    kHIDUsage_Csmr_ALAOrVCaptureOrPlayback    = 0x193,    /* Selector */
    kHIDUsage_Csmr_ALLocalMachineBrowser    = 0x194,    /* Selector */
    kHIDUsage_Csmr_ALLANOrWANBrowser    = 0x195,    /* Selector */
    kHIDUsage_Csmr_ALInternetBrowser    = 0x196,    /* Selector */
    kHIDUsage_Csmr_ALRemoteNetworkingOrISPConnect    = 0x197,    /* Selector */
    kHIDUsage_Csmr_ALNetworkConference    = 0x198,    /* Selector */
    kHIDUsage_Csmr_ALNetworkChat    = 0x199,    /* Selector */
    kHIDUsage_Csmr_ALTelephonyOrDialer    = 0x19A,    /* Selector */
    kHIDUsage_Csmr_ALLogon    = 0x19B,    /* Selector */
    kHIDUsage_Csmr_ALLogoff    = 0x19C,    /* Selector */
    kHIDUsage_Csmr_ALLogonOrLogoff    = 0x19D,    /* Selector */
    kHIDUsage_Csmr_ALTerminalLockOrScreensaver    = 0x19E,    /* Selector */
    kHIDUsage_Csmr_ALControlPanel    = 0x19F,    /* Selector */
    kHIDUsage_Csmr_ALCommandLineProcessorOrRun    = 0x1A0,    /* Selector */
    kHIDUsage_Csmr_ALProcessOrTaskManager    = 0x1A1,    /* Selector */
    kHIDUsage_Csmr_AL    = 0x1A2,    /* Selector */
    kHIDUsage_Csmr_ALNextTaskOrApplication    = 0x1A3,    /* Selector */
    kHIDUsage_Csmr_ALPreviousTaskOrApplication    = 0x1A4,    /* Selector */
    kHIDUsage_Csmr_ALPreemptiveHaltTaskOrApplication    = 0x1A5,    /* Selector */
    kHIDUsage_Csmr_ALIntegratedHelpCenter   = 0x1A6, /* Selector */
    kHIDUsage_Csmr_ALDocuments   = 0x1A7, /* Selector */
    kHIDUsage_Csmr_ALThesaurus   = 0x1A8, /* Selector */
    kHIDUsage_Csmr_ALDictionary   = 0x1A9, /* Selector */
    kHIDUsage_Csmr_ALDesktop   = 0x1AA, /* Selector */
    kHIDUsage_Csmr_ALSpellCheck   = 0x1AB, /* Selector */
    kHIDUsage_Csmr_ALGrammerCheck   = 0x1AC, /* Selector */
    kHIDUsage_Csmr_ALWirelessStatus   = 0x1AD, /* Selector */
    kHIDUsage_Csmr_ALKeyboardLayout   = 0x1AE, /* Selector */
    kHIDUsage_Csmr_ALVirusProtection   = 0x1AF, /* Selector */
    kHIDUsage_Csmr_ALEncryption   = 0x1B0, /* Selector */
    kHIDUsage_Csmr_ALScreenSaver   = 0x1B1, /* Selector */
    kHIDUsage_Csmr_ALAlarms   = 0x1B2, /* Selector */
    kHIDUsage_Csmr_ALClock   = 0x1B3, /* Selector */
    kHIDUsage_Csmr_ALFileBrowser   = 0x1B4, /* Selector */
    kHIDUsage_Csmr_ALPowerStatus   = 0x1B5, /* Selector */
    kHIDUsage_Csmr_ALImageBrowser   = 0x1B6, /* Selector */
    kHIDUsage_Csmr_ALAudioBrowser   = 0x1B7, /* Selector */
    kHIDUsage_Csmr_ALMovieBrowser   = 0x1B8, /* Selector */
    kHIDUsage_Csmr_ALDigitalRightsManager   = 0x1B9, /* Selector */
    kHIDUsage_Csmr_ALDigitalWallet   = 0x1BA, /* Selector */
    /* 0x1BB Reserved */
    kHIDUsage_Csmr_ALInstantMessaging   = 0x1BC, /* Selector */
    kHIDUsage_Csmr_ALOEMFeatureBrowser  = 0x1BD, /* Selector */
    kHIDUsage_Csmr_ALOEMHelp    = 0x1BE, /* Selector */
    kHIDUsage_Csmr_ALOnlineCommunity    = 0x1BF, /* Selector */
    kHIDUsage_Csmr_ALEntertainmentContentBrowser    = 0x1C0, /* Selector */
    kHIDUsage_Csmr_ALOnlineShoppingBrowswer = 0x1C1, /* Selector */
    kHIDUsage_Csmr_ALSmartCardInformationOrHelp = 0x1C2, /* Selector */
    kHIDUsage_Csmr_ALMarketMonitorOrFinanceBrowser  = 0x1C3, /* Selector */
    kHIDUsage_Csmr_ALCustomizedCorporateNewsBrowser = 0x1C4, /* Selector */
    kHIDUsage_Csmr_ALOnlineActivityBrowswer= 0x1C5, /* Selector */
    kHIDUsage_Csmr_ALResearchOrSearchBrowswer   = 0x1C6, /* Selector */
    kHIDUsage_Csmr_ALAudioPlayer= 0x1C7, /* Selector */
    /* 0x1C8 - 0x1FF Reserved */
    kHIDUsage_Csmr_GenericGUIApplicationControls    = 0x200,    /* Named Array */
    kHIDUsage_Csmr_ACNew    = 0x201,    /* Selector */
    kHIDUsage_Csmr_ACOpen    = 0x202,    /* Selector */
    kHIDUsage_Csmr_ACClose    = 0x203,    /* Selector */
    kHIDUsage_Csmr_ACExit    = 0x204,    /* Selector */
    kHIDUsage_Csmr_ACMaximize    = 0x205,    /* Selector */
    kHIDUsage_Csmr_ACMinimize    = 0x206,    /* Selector */
    kHIDUsage_Csmr_ACSave    = 0x207,    /* Selector */
    kHIDUsage_Csmr_ACPrint    = 0x208,    /* Selector */
    kHIDUsage_Csmr_ACProperties    = 0x209,    /* Selector */
    kHIDUsage_Csmr_ACUndo    = 0x21A,    /* Selector */
    kHIDUsage_Csmr_ACCopy    = 0x21B,    /* Selector */
    kHIDUsage_Csmr_ACCut    = 0x21C,    /* Selector */
    kHIDUsage_Csmr_ACPaste    = 0x21D,    /* Selector */
    kHIDUsage_Csmr_AC    = 0x21E,    /* Selector */
    kHIDUsage_Csmr_ACFind    = 0x21F,    /* Selector */
    kHIDUsage_Csmr_ACFindandReplace    = 0x220,    /* Selector */
    kHIDUsage_Csmr_ACSearch    = 0x221,    /* Selector */
    kHIDUsage_Csmr_ACGoTo    = 0x222,    /* Selector */
    kHIDUsage_Csmr_ACHome    = 0x223,    /* Selector */
    kHIDUsage_Csmr_ACBack    = 0x224,    /* Selector */
    kHIDUsage_Csmr_ACForward    = 0x225,    /* Selector */
    kHIDUsage_Csmr_ACStop    = 0x226,    /* Selector */
    kHIDUsage_Csmr_ACRefresh    = 0x227,    /* Selector */
    kHIDUsage_Csmr_ACPreviousLink    = 0x228,    /* Selector */
    kHIDUsage_Csmr_ACNextLink    = 0x229,    /* Selector */
    kHIDUsage_Csmr_ACBookmarks    = 0x22A,    /* Selector */
    kHIDUsage_Csmr_ACHistory    = 0x22B,    /* Selector */
    kHIDUsage_Csmr_ACSubscriptions    = 0x22C,    /* Selector */
    kHIDUsage_Csmr_ACZoomIn    = 0x22D,    /* Selector */
    kHIDUsage_Csmr_ACZoomOut    = 0x22E,    /* Selector */
    kHIDUsage_Csmr_ACZoom    = 0x22F,    /* Selector */
    kHIDUsage_Csmr_ACFullScreenView    = 0x230,    /* Selector */
    kHIDUsage_Csmr_ACNormalView    = 0x231,    /* Selector */
    kHIDUsage_Csmr_ACViewToggle    = 0x232,    /* Selector */
    kHIDUsage_Csmr_ACScrollUp    = 0x233,    /* Selector */
    kHIDUsage_Csmr_ACScrollDown    = 0x234,    /* Selector */
    kHIDUsage_Csmr_ACScroll    = 0x235,    /* Selector */
    kHIDUsage_Csmr_ACPanLeft    = 0x236,    /* Selector */
    kHIDUsage_Csmr_ACPanRight    = 0x237,    /* Selector */
    kHIDUsage_Csmr_ACPan    = 0x238,    /* Selector */
    kHIDUsage_Csmr_ACNewWindow    = 0x239,    /* Selector */
    kHIDUsage_Csmr_ACTileHorizontally    = 0x23A,    /* Selector */
    kHIDUsage_Csmr_ACTileVertically    = 0x23B,    /* Selector */
    kHIDUsage_Csmr_ACFormat    = 0x23C,    /* Selector */
    kHIDUsage_Csmr_ACEdit   = 0x23D,    /* Selector */
    kHIDUsage_Csmr_ACBold   = 0x23E,    /* Selector */
    kHIDUsage_Csmr_ACItalics    = 0x23F,    /* Selector */
    kHIDUsage_Csmr_ACUnderline  = 0x240,    /* Selector */
    kHIDUsage_Csmr_ACStrikethrough  = 0x241,    /* Selector */
    kHIDUsage_Csmr_ACSubscript  = 0x242,    /* Selector */
    kHIDUsage_Csmr_ACSuperscript    = 0x243,    /* Selector */
    kHIDUsage_Csmr_ACAllCaps    = 0x244,    /* Selector */
    kHIDUsage_Csmr_ACRotate = 0x245,    /* Selector */
    kHIDUsage_Csmr_ACResize = 0x246,    /* Selector */
    kHIDUsage_Csmr_ACFlipHorizontal = 0x247,    /* Selector */
    kHIDUsage_Csmr_ACFlipVertical   = 0x248,    /* Selector */
    kHIDUsage_Csmr_ACMirrorHorizontal   = 0x249,    /* Selector */
    kHIDUsage_Csmr_ACMirrorVertical = 0x24A,    /* Selector */
    kHIDUsage_Csmr_ACFontSelect = 0x24B,    /* Selector */
    kHIDUsage_Csmr_ACFontColor  = 0x24C,    /* Selector */
    kHIDUsage_Csmr_ACFontSize   = 0x24D,    /* Selector */
    kHIDUsage_Csmr_ACJustifyLeft    = 0x24E,    /* Selector */
    kHIDUsage_Csmr_ACJustifyCenterH = 0x24F,    /* Selector */
    kHIDUsage_Csmr_ACJustifyRight   = 0x250,    /* Selector */
    kHIDUsage_Csmr_ACJustifyBlockH  = 0x251,    /* Selector */
    kHIDUsage_Csmr_ACJustifyTop     = 0x252,    /* Selector */
    kHIDUsage_Csmr_ACJustifyCenterV = 0x253,    /* Selector */
    kHIDUsage_Csmr_ACJustifyBottom  = 0x254,    /* Selector */
    kHIDUsage_Csmr_ACJustifyBlockV  = 0x255,    /* Selector */
    kHIDUsage_Csmr_ACIndentyDecrease    = 0x256,    /* Selector */
    kHIDUsage_Csmr_ACIndentyIncrease    = 0x257,    /* Selector */
    kHIDUsage_Csmr_ACNumberedList   = 0x258,    /* Selector */
    kHIDUsage_Csmr_ACRestartNumbering   = 0x259,    /* Selector */
    kHIDUsage_Csmr_ACBulletedList   = 0x25A,    /* Selector */
    kHIDUsage_Csmr_ACPromote    = 0x25B,    /* Selector */
    kHIDUsage_Csmr_ACDemote = 0x25C,    /* Selector */
    kHIDUsage_Csmr_ACYes    = 0x25D,    /* Selector */
    kHIDUsage_Csmr_ACNo = 0x25E,    /* Selector */
    kHIDUsage_Csmr_ACCancel = 0x25F,    /* Selector */
    kHIDUsage_Csmr_ACCatalog    = 0x260,    /* Selector */
    kHIDUsage_Csmr_ACBuyOrCheckout  = 0x261,    /* Selector */
    kHIDUsage_Csmr_ACAddToCart  = 0x262,    /* Selector */
    kHIDUsage_Csmr_ACExpand = 0x263,    /* Selector */
    kHIDUsage_Csmr_ACExpandAll  = 0x264,    /* Selector */
    kHIDUsage_Csmr_ACCollapse   = 0x265,    /* Selector */
    kHIDUsage_Csmr_ACCollapseAll    = 0x266,    /* Selector */
    kHIDUsage_Csmr_ACPrintPreview   = 0x267,    /* Selector */
    kHIDUsage_Csmr_ACPasteSpecial   = 0x268,    /* Selector */
    kHIDUsage_Csmr_ACInsertMode = 0x269,    /* Selector */
    kHIDUsage_Csmr_ACDelete = 0x26A,    /* Selector */
    kHIDUsage_Csmr_ACLock   = 0x26B,    /* Selector */
    kHIDUsage_Csmr_ACUnlock = 0x26C,    /* Selector */
    kHIDUsage_Csmr_ACProtect    = 0x26D,    /* Selector */
    kHIDUsage_Csmr_ACUnprotect  = 0x26E,    /* Selector */
    kHIDUsage_Csmr_ACAttachComment  = 0x26F,    /* Selector */
    kHIDUsage_Csmr_ACDetachComment  = 0x270,    /* Selector */
    kHIDUsage_Csmr_ACViewComment    = 0x271,    /* Selector */
    kHIDUsage_Csmr_ACSelectWord = 0x272,    /* Selector */
    kHIDUsage_Csmr_ACSelectSentence = 0x273,    /* Selector */
    kHIDUsage_Csmr_ACSelectParagraph    = 0x274,    /* Selector */
    kHIDUsage_Csmr_ACSelectColumn   = 0x275,    /* Selector */
    kHIDUsage_Csmr_ACSelectRow  = 0x276,    /* Selector */
    kHIDUsage_Csmr_ACSelectTable    = 0x277,    /* Selector */
    kHIDUsage_Csmr_ACSelectObject   = 0x278,    /* Selector */
    kHIDUsage_Csmr_ACRedoOrRepeat   = 0x279,    /* Selector */
    kHIDUsage_Csmr_ACSort   = 0x27A,    /* Selector */
    kHIDUsage_Csmr_ACSortAscending  = 0x27B,    /* Selector */
    kHIDUsage_Csmr_ACSortDescending = 0x27C,    /* Selector */
    kHIDUsage_Csmr_ACFilter = 0x27D,    /* Selector */
    kHIDUsage_Csmr_ACSetClock   = 0x27E,    /* Selector */
    kHIDUsage_Csmr_ACViewClock  = 0x27F,    /* Selector */
    kHIDUsage_Csmr_ACSelectTimeZone = 0x280,    /* Selector */
    kHIDUsage_Csmr_ACEditTimeZones  = 0x281,    /* Selector */
    kHIDUsage_Csmr_ACSetAlarm   = 0x282,    /* Selector */
    kHIDUsage_Csmr_ACClearAlarm = 0x283,    /* Selector */
    kHIDUsage_Csmr_ACSnoozeAlarm    = 0x284,    /* Selector */
    kHIDUsage_Csmr_ACResetAlarm = 0x285,    /* Selector */
    kHIDUsage_Csmr_ACSynchronize    = 0x286,    /* Selector */
    kHIDUsage_Csmr_ACSendOrReceive  = 0x287,    /* Selector */
    kHIDUsage_Csmr_ACSendTo = 0x288,    /* Selector */
    kHIDUsage_Csmr_ACReply  = 0x289,    /* Selector */
    kHIDUsage_Csmr_ACReplyAll   = 0x28A,    /* Selector */
    kHIDUsage_Csmr_ACForwardMessage = 0x28B,    /* Selector */
    kHIDUsage_Csmr_ACSend   = 0x28C,    /* Selector */
    kHIDUsage_Csmr_ACAttachFile = 0x28D,    /* Selector */
    kHIDUsage_Csmr_ACUpload = 0x28E,    /* Selector */
    kHIDUsage_Csmr_ACDownload   = 0x28F,    /* Selector */
    kHIDUsage_Csmr_ACSetBorders = 0x290,    /* Selector */
    kHIDUsage_Csmr_ACInsertRow  = 0x291,    /* Selector */
    kHIDUsage_Csmr_ACInsertColumn   = 0x292,    /* Selector */
    kHIDUsage_Csmr_ACInsertFile = 0x293,    /* Selector */
    kHIDUsage_Csmr_ACInsertPicture  = 0x294,    /* Selector */
    kHIDUsage_Csmr_ACInsertObject   = 0x295,    /* Selector */
    kHIDUsage_Csmr_ACInsertSymbol   = 0x296,    /* Selector */
    kHIDUsage_Csmr_ACSaveAndClose   = 0x297,    /* Selector */
    kHIDUsage_Csmr_ACRename = 0x298,    /* Selector */
    kHIDUsage_Csmr_ACMerge  = 0x299,    /* Selector */
    kHIDUsage_Csmr_ACSplit  = 0x29A,    /* Selector */
    kHIDUsage_Csmr_ACDistributeH    = 0x29B,    /* Selector */
    kHIDUsage_Csmr_ACDistributeV    = 0x29C,    /* Selector */
    /* 0x29D - 0xFFFF Reserved */
    kHIDUsage_Csmr_Reserved = 0xFFFF
};

/* Digitizer Page (0x0D) */
/* This section provides detailed descriptions of the usages employed by Digitizer Devices. */
enum
{
    kHIDUsage_Dig_Digitizer                         = 0x01,     /* Application Collection */
    kHIDUsage_Dig_Pen                               = 0x02,     /* Application Collection */
    kHIDUsage_Dig_LightPen                          = 0x03,     /* Application Collection */
    kHIDUsage_Dig_TouchScreen                       = 0x04,     /* Application Collection */
    kHIDUsage_Dig_TouchPad                          = 0x05,     /* Application Collection */
    kHIDUsage_Dig_WhiteBoard                        = 0x06,     /* Application Collection */
    kHIDUsage_Dig_CoordinateMeasuringMachine        = 0x07,     /* Application Collection */
    kHIDUsage_Dig_3DDigitizer                       = 0x08,     /* Application Collection */
    kHIDUsage_Dig_StereoPlotter                     = 0x09,     /* Application Collection */
    kHIDUsage_Dig_ArticulatedArm                    = 0x0A,     /* Application Collection */
    kHIDUsage_Dig_Armature                          = 0x0B,     /* Application Collection */
    kHIDUsage_Dig_MultiplePointDigitizer            = 0x0C,     /* Application Collection */
    kHIDUsage_Dig_FreeSpaceWand                     = 0x0D,     /* Application Collection */
    kHIDUsage_Dig_DeviceConfiguration               = 0x0E,     /* Application Collection */
    /* 0x0F - 0x1F Reserved */
    kHIDUsage_Dig_Stylus                            = 0x20,     /* Logical Collection */
    kHIDUsage_Dig_Puck                              = 0x21,     /* Logical Collection */
    kHIDUsage_Dig_Finger                            = 0x22,     /* Logical Collection */
    kHIDUsage_Dig_DeviceSettings                    = 0x23,     /* Logical Collection */
    kHIDUsage_Dig_GestureCharacter                  = 0x24,     /* Logical Collection */
    /* 0x25 - 0x2F Reserved */
    kHIDUsage_Dig_TipPressure                       = 0x30,     /* Dynamic Value */
    kHIDUsage_Dig_BarrelPressure                    = 0x31,     /* Dynamic Value */
    kHIDUsage_Dig_InRange                           = 0x32,     /* Momentary Control */
    kHIDUsage_Dig_Touch                             = 0x33,     /* Momentary Control */
    kHIDUsage_Dig_Untouch                           = 0x34,     /* One-Shot Control */
    kHIDUsage_Dig_Tap                               = 0x35,     /* One-Shot Control */
    kHIDUsage_Dig_Quality                           = 0x36,     /* Dynamic Value */
    kHIDUsage_Dig_DataValid                         = 0x37,     /* Momentary Control */
    kHIDUsage_Dig_TransducerIndex                   = 0x38,     /* Dynamic Value */
    kHIDUsage_Dig_TabletFunctionKeys                = 0x39,     /* Logical Collection */
    kHIDUsage_Dig_ProgramChangeKeys                 = 0x3A,     /* Logical Collection */
    kHIDUsage_Dig_BatteryStrength                   = 0x3B,     /* Dynamic Value */
    kHIDUsage_Dig_Invert                            = 0x3C,     /* Momentary Control */
    kHIDUsage_Dig_XTilt                             = 0x3D,     /* Dynamic Value */
    kHIDUsage_Dig_YTilt                             = 0x3E,     /* Dynamic Value */
    kHIDUsage_Dig_Azimuth                           = 0x3F,     /* Dynamic Value */
    kHIDUsage_Dig_Altitude                          = 0x40,     /* Dynamic Value */
    kHIDUsage_Dig_Twist                             = 0x41,     /* Dynamic Value */
    kHIDUsage_Dig_TipSwitch                         = 0x42,     /* Momentary Control */
    kHIDUsage_Dig_SecondaryTipSwitch                = 0x43,     /* Momentary Control */
    kHIDUsage_Dig_BarrelSwitch                      = 0x44,     /* Momentary Control */
    kHIDUsage_Dig_Eraser                            = 0x45,     /* Momentary Control */
    kHIDUsage_Dig_TabletPick                        = 0x46,     /* Momentary Control */
    kHIDUsage_Dig_TouchValid                        = 0x47,     /* Momentary Control */
    kHIDUsage_Dig_Width                             = 0x48,     /* Dynamic Value */
    kHIDUsage_Dig_Height                            = 0x49,     /* Dynamic Value */
    /* 0x4A - 0x50 Reserved */
    kHIDUsage_Dig_ContactIdentifier                 = 0x51,     /* Dynamic Value */
    kHIDUsage_Dig_DeviceMode                        = 0x52,     /* Dynamic Value */
    kHIDUsage_Dig_DeviceIdentifier                  = 0x53,     /* Dynamic Value */
    kHIDUsage_Dig_ContactCount                      = 0x54,     /* Dynamic Value */
    kHIDUsage_Dig_ContactCountMaximum               = 0x55,     /* Static Value */
    
    /* 0x56 - 0x5F Reserved */
    kHIDUsage_Dig_GestureCharacterEnable            = 0x60,     /* Dynamic Flag */
    kHIDUsage_Dig_GestureCharacterQuality           = 0x61,     /* Dynamic Value */
    kHIDUsage_Dig_GestureCharacterDataLength        = 0x62,     /* Dynamic Value */
    kHIDUsage_Dig_GestureCharacterData              = 0x63,     /* Buffered Bytes */
    kHIDUsage_Dig_GestureCharacterEncoding          = 0x64,     /* Named Array */
    kHIDUsage_Dig_GestureCharacterEncodingUTF8      = 0x65,     /* Selector */
    kHIDUsage_Dig_GestureCharacterEncodingUTF16LE   = 0x66,     /* Selector */
    kHIDUsage_Dig_GestureCharacterEncodingUTF16BE   = 0x67,     /* Selector */
    kHIDUsage_Dig_GestureCharacterEncodingUTF32LE   = 0x68,     /* Selector */
    kHIDUsage_Dig_GestureCharacterEncodingUTF32BE   = 0x69,     /* Selector */
    
    /* 0x70 - 0xFFFF Reserved */
    kHIDUsage_Dig_Reserved                          = 0xFFFF
};

/* Physical Interface Device Page (0x0F) */
/* This section provides detailed descriptions of the usages employed by Power Devices. */
enum
{
    kHIDUsage_PID_PhysicalInterfaceDevice = 0x01,        /* CA - A collection of PID usages */
    /* 0x02 - 0x1F Reserved */
    kHIDUsage_PID_Normal = 0x20,                        /* DV - A force applied perpendicular to the surface of an object */
    kHIDUsage_PID_SetEffectReport = 0x21,                /* XXX */
    kHIDUsage_PID_EffectBlockIndex = 0x22,                /* XXX */
    kHIDUsage_PID_ParamBlockOffset = 0x23,                /* XXX */
    kHIDUsage_PID_ROM_Flag = 0x24,                        /* XXX */
    kHIDUsage_PID_EffectType = 0x25,                    /* XXX */
    kHIDUsage_PID_ET_ConstantForce = 0x26,                /* XXX */
    kHIDUsage_PID_ET_Ramp = 0x27,                        /* XXX */
    kHIDUsage_PID_ET_CustomForceData = 0x28,            /* XXX */
    /* 0x29 - 0x2F Reserved */
    kHIDUsage_PID_ET_Square = 0x30,                        /* XXX */
    kHIDUsage_PID_ET_Sine = 0x31,                        /* XXX */
    kHIDUsage_PID_ET_Triangle = 0x32,                    /* XXX */
    kHIDUsage_PID_ET_SawtoothUp = 0x33,                    /* XXX */
    kHIDUsage_PID_ET_SawtoothDown = 0x34,                /* XXX */
    /* 0x35 - 0x3F Reserved */
    kHIDUsage_PID_ET_Spring = 0x40,                        /* XXX */
    kHIDUsage_PID_ET_Damper = 0x41,                        /* XXX */
    kHIDUsage_PID_ET_Inertia = 0x42,                    /* XXX */
    kHIDUsage_PID_ET_Friction = 0x43,                    /* XXX */
    /* 0x44 - 0x4F Reserved */
    kHIDUsage_PID_Duration = 0x50,                        /* XXX */
    kHIDUsage_PID_SamplePeriod = 0x51,                    /* XXX */
    kHIDUsage_PID_Gain = 0x52,                            /* XXX */
    kHIDUsage_PID_TriggerButton = 0x53,                    /* XXX */
    kHIDUsage_PID_TriggerRepeatInterval = 0x54,            /* XXX */
    kHIDUsage_PID_AxesEnable = 0x55,                    /* XXX */
    kHIDUsage_PID_DirectionEnable = 0x56,                /* XXX */
    kHIDUsage_PID_Direction = 0x57,                        /* XXX */
    kHIDUsage_PID_TypeSpecificBlockOffset = 0x58,        /* XXX */
    kHIDUsage_PID_BlockType = 0x59,                        /* XXX */
    kHIDUsage_PID_SetEnvelopeReport = 0x5A,                /* XXX */
    kHIDUsage_PID_AttackLevel = 0x5B,                    /* XXX */
    kHIDUsage_PID_AttackTime = 0x5C,                    /* XXX */
    kHIDUsage_PID_FadeLevel = 0x5D,                        /* XXX */
    kHIDUsage_PID_FadeTime = 0x5E,                        /* XXX */
    kHIDUsage_PID_SetConditionReport = 0x5F,            /* XXX */
    
    kHIDUsage_PID_CP_Offset = 0x60,                        /* XXX */
    kHIDUsage_PID_PositiveCoefficient = 0x61,            /* XXX */
    kHIDUsage_PID_NegativeCoefficient = 0x62,            /* XXX */
    kHIDUsage_PID_PositiveSaturation = 0x63,            /* XXX */
    kHIDUsage_PID_NegativeSaturation = 0x64,            /* XXX */
    kHIDUsage_PID_DeadBand = 0x65,                        /* XXX */
    kHIDUsage_PID_DownloadForceSample = 0x66,            /* XXX */
    kHIDUsage_PID_IsochCustomForceEnable = 0x67,        /* XXX */
    kHIDUsage_PID_CustomForceDataReport = 0x68,            /* XXX */
    kHIDUsage_PID_CustomForceData = 0x69,                /* XXX */
    kHIDUsage_PID_CustomForceVendorDefinedData = 0x6A,    /* XXX */
    kHIDUsage_PID_SetCustomForceReport = 0x6B,            /* XXX */
    kHIDUsage_PID_CustomForceDataOffset = 0x6C,            /* XXX */
    kHIDUsage_PID_SampleCount = 0x6D,                    /* XXX */
    kHIDUsage_PID_SetPeriodicReport = 0x6E,                /* XXX */
    kHIDUsage_PID_Offset = 0x6F,                        /* XXX */
    
    kHIDUsage_PID_Magnitude = 0x70,                        /* XXX */
    kHIDUsage_PID_Phase = 0x71,                            /* XXX */
    kHIDUsage_PID_Period = 0x72,                        /* XXX */
    kHIDUsage_PID_SetConstantForceReport = 0x73,        /* XXX */
    kHIDUsage_PID_SetRampForceReport = 0x74,            /* XXX */
    kHIDUsage_PID_RampStart = 0x75,                        /* XXX */
    kHIDUsage_PID_RampEnd = 0x76,                        /* XXX */
    kHIDUsage_PID_EffectOperationReport = 0x77,            /* XXX */
    kHIDUsage_PID_EffectOperation = 0x78,                /* XXX */
    kHIDUsage_PID_OpEffectStart = 0x79,                    /* XXX */
    kHIDUsage_PID_OpEffectStartSolo = 0x7A,                /* XXX */
    kHIDUsage_PID_OpEffectStop = 0x7B,                    /* XXX */
    kHIDUsage_PID_LoopCount = 0x7C,                        /* XXX */
    kHIDUsage_PID_DeviceGainReport = 0x7D,                /* XXX */
    kHIDUsage_PID_DeviceGain = 0x7E,                    /* XXX */
    kHIDUsage_PID_PoolReport = 0x7F,                    /* XXX */
    
    kHIDUsage_PID_RAM_PoolSize = 0x80,                    /* XXX */
    kHIDUsage_PID_ROM_PoolSize = 0x81,                    /* XXX */
    kHIDUsage_PID_ROM_EffectBlockCount = 0x82,            /* XXX */
    kHIDUsage_PID_SimultaneousEffectsMax = 0x83,        /* XXX */
    kHIDUsage_PID_PoolAlignment = 0x84,                    /* XXX */
    kHIDUsage_PID_PoolMoveReport = 0x85,                /* XXX */
    kHIDUsage_PID_MoveSource = 0x86,                    /* XXX */
    kHIDUsage_PID_MoveDestination = 0x87,                /* XXX */
    kHIDUsage_PID_MoveLength = 0x88,                    /* XXX */
    kHIDUsage_PID_BlockLoadReport = 0x89,                /* XXX */
    /* 0x8A Reserved */
    kHIDUsage_PID_BlockLoadStatus = 0x8B,                /* XXX */
    kHIDUsage_PID_BlockLoadSuccess = 0x8C,                /* XXX */
    kHIDUsage_PID_BlockLoadFull = 0x8D,                    /* XXX */
    kHIDUsage_PID_BlockLoadError = 0x8E,                /* XXX */
    kHIDUsage_PID_BlockHandle = 0x8F,                    /* XXX */
    
    kHIDUsage_PID_BlockFreeReport = 0x90,                /* XXX */
    kHIDUsage_PID_TypeSpecificBlockHandle = 0x91,        /* XXX */
    kHIDUsage_PID_StateReport = 0x92,                    /* XXX */
    /* 0x93 Reserved */
    kHIDUsage_PID_EffectPlaying = 0x94,                    /* XXX */
    kHIDUsage_PID_DeviceControlReport = 0x95,            /* XXX */
    kHIDUsage_PID_DeviceControl = 0x96,                    /* XXX */
    kHIDUsage_PID_DC_EnableActuators = 0x97,            /* XXX */
    kHIDUsage_PID_DC_DisableActuators = 0x98,            /* XXX */
    kHIDUsage_PID_DC_StopAllEffects = 0x99,                /* XXX */
    kHIDUsage_PID_DC_DeviceReset = 0x9A,                /* XXX */
    kHIDUsage_PID_DC_DevicePause = 0x9B,                /* XXX */
    kHIDUsage_PID_DC_DeviceContinue = 0x9C,                /* XXX */
    /* 0x9d - 0x9E Reserved */
    kHIDUsage_PID_DevicePaused = 0x9F,                    /* XXX */
    
    kHIDUsage_PID_ActuatorsEnabled = 0xA0,                /* XXX */
    /* 0xA1 - 0xA3 Reserved */
    kHIDUsage_PID_SafetySwitch = 0xA4,                    /* XXX */
    kHIDUsage_PID_ActuatorOverrideSwitch = 0xA5,        /* XXX */
    kHIDUsage_PID_ActuatorPower = 0xA6,                    /* XXX */
    kHIDUsage_PID_StartDelay = 0xA7,                    /* XXX */
    kHIDUsage_PID_ParameterBlockSize = 0xA8,            /* XXX */
    kHIDUsage_PID_DeviceManagedPool = 0xA9,                /* XXX */
    kHIDUsage_PID_SharedParameterBlocks = 0xAA,            /* XXX */
    kHIDUsage_PID_CreateNewEffectReport = 0xAB,            /* XXX */
    kHIDUsage_PID_RAM_PoolAvailable = 0xAC,                /* XXX */
    /* 0xAD - 0xFFFF Reserved */
    kHIDUsage_PID_Reserved = 0xFFFF
};

/* AlphanumericDisplay Page (0x14) */
/* The Alphanumeric Display page is intended for use by simple alphanumeric displays that are used on consumer devices. */
enum
{
    kHIDUsage_AD_AlphanumericDisplay    = 0x01,    /* Application Collection */
    /* 0x02 - 0x1F Reserved */
    kHIDUsage_AD_DisplayAttributesReport    = 0x20,    /* Logical Collection */
    kHIDUsage_AD_ASCIICharacterSet    = 0x21,    /* Static Flag */
    kHIDUsage_AD_DataReadBack    = 0x22,    /* Static Flag */
    kHIDUsage_AD_FontReadBack    = 0x23,    /* Static Flag */
    kHIDUsage_AD_DisplayControlReport    = 0x24,    /* Logical Collection */
    kHIDUsage_AD_ClearDisplay    = 0x25,    /* Dynamic Flag */
    kHIDUsage_AD_DisplayEnable    = 0x26,    /* Dynamic Flag */
    kHIDUsage_AD_ScreenSaverDelay    = 0x27,    /* Static Value */
    kHIDUsage_AD_ScreenSaverEnable    = 0x28,    /* Dynamic Flag */
    kHIDUsage_AD_VerticalScroll    = 0x29,    /* Static Flag */
    kHIDUsage_AD_HorizontalScroll    = 0x2A,    /* Static Flag */
    kHIDUsage_AD_CharacterReport    = 0x2B,    /* Logical Collection */
    kHIDUsage_AD_DisplayData    = 0x2C,    /* Dynamic Value */
    kHIDUsage_AD_DisplayStatus    = 0x2D,    /* Logical Collection */
    kHIDUsage_AD_StatNotReady    = 0x2E,    /* Selector */
    kHIDUsage_AD_StatReady    = 0x2F,    /* Selector */
    kHIDUsage_AD_ErrNotaloadablecharacter    = 0x30,    /* Selector */
    kHIDUsage_AD_ErrFontdatacannotberead    = 0x31,    /* Selector */
    kHIDUsage_AD_CursorPositionReport    = 0x32,    /* Logical Collection */
    kHIDUsage_AD_Row    = 0x33,    /* Dynamic Value */
    kHIDUsage_AD_Column    = 0x34,    /* Dynamic Value */
    kHIDUsage_AD_Rows    = 0x35,    /* Static Value */
    kHIDUsage_AD_Columns    = 0x36,    /* Static Value */
    kHIDUsage_AD_CursorPixelPositioning    = 0x37,    /* Static Flag */
    kHIDUsage_AD_CursorMode    = 0x38,    /* Dynamic Flag */
    kHIDUsage_AD_CursorEnable    = 0x39,    /* Dynamic Flag */
    kHIDUsage_AD_CursorBlink    = 0x3A,    /* Dynamic Flag */
    kHIDUsage_AD_FontReport    = 0x3B,    /* Logical Collection */
    kHIDUsage_AD_FontData    = 0x3C,    /* Buffered Byte */
    kHIDUsage_AD_CharacterWidth    = 0x3D,    /* Static Value */
    kHIDUsage_AD_CharacterHeight    = 0x3E,    /* Static Value */
    kHIDUsage_AD_CharacterSpacingHorizontal    = 0x3F,    /* Static Value */
    kHIDUsage_AD_CharacterSpacingVertical    = 0x40,    /* Static Value */
    kHIDUsage_AD_UnicodeCharacterSet    = 0x41,    /* Static Flag */
    /* 0x42 - 0xFFFF Reserved */
    kHIDUsage_AD_Reserved = 0xFFFF
};

/* Sensor Page (0x14) */
/* The Sensor page is intended for use by devices that capture biometric, electrical, environmental, light, location, mechanical, motion, orientation, scanner, etc events. */
enum
{
    kHIDUsage_Snsr_Undefined                            = 0x00,
    kHIDUsage_Snsr_Sensor                               = 0x01,     /* Application/Physical Collection */
    /* 0x02 - 0x0F Reserved */
    kHIDUsage_Snsr_Biometric                            = 0x10,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Biometric_HumanPresence              = 0x11,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Biometric_HumanProximity             = 0x12,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Biometric_HumanTouch                 = 0x13,     /* Application/Physical Collection */
    /* 0x14 - 0x1F Reserved */
    kHIDUsage_Snsr_Electrical                           = 0x20,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Capacitance               = 0x21,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Current                   = 0x22,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Power                     = 0x23,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Inductance                = 0x24,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Resistance                = 0x25,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Voltage                   = 0x26,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Potentiometer             = 0x27,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Frequency                 = 0x28,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Electrical_Period                    = 0x29,     /* Application/Physical Collection */
    /* 0x2A - 0x2F Reserved */
    kHIDUsage_Snsr_Environmental                        = 0x30,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Environmental_AtmosphericPressure    = 0x31,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Environmental_Humidity               = 0x32,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Environmental_Temperature            = 0x33,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Environmental_WindDirection          = 0x34,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Environmental_WindSpeed              = 0x35,     /* Application/Physical Collection */
    /* 0x36 - 0x3F Reserved */
    kHIDUsage_Snsr_Light                                = 0x40,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Light_AmbientLight                   = 0x41,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Light_ConsumerInfrared               = 0x42,     /* Application/Physical Collection */
    /* 0x43 - 0x4F Reserved */
    kHIDUsage_Snsr_Location                             = 0x50,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Location_Broadcast                   = 0x51,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Location_DeadReckoning               = 0x52,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Location_GPS                         = 0x53,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Location_Lookup                      = 0x54,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Location_Other                       = 0x55,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Location_Static                      = 0x56,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Location_Triangulation               = 0x57,     /* Application/Physical Collection */
    /* 0x58 - 0x5F Reserved */
    kHIDUsage_Snsr_Mechanical                           = 0x60,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_BooleanSwitch             = 0x61,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_BooleanSwitchArray        = 0x62,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_MultivalueSwitch          = 0x63,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_Force                     = 0x64,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_Pressure                  = 0x65,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_Strain                    = 0x66,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_Weight                    = 0x67,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_HapticVibrator            = 0x68,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Mechanical_HallEffectSwitch          = 0x69,     /* Application/Physical Collection */
    /* 0x6A - 0x6F Reserved */
    kHIDUsage_Snsr_Motion                               = 0x70,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Accelerometer1D               = 0x71,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Accelerometer2D               = 0x72,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Accelerometer3D               = 0x73,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Gyrometer1D                   = 0x74,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Gyrometer2D                   = 0x75,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Gyrometer3D                   = 0x76,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_MotionDetector                = 0x77,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Speedometer                   = 0x78,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Accelerometer                 = 0x79,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Motion_Gyrometer                     = 0x7A,     /* Application/Physical Collection */
    /* 0x7B - 0x7F Reserved */
    kHIDUsage_Snsr_Orientation                          = 0x80,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Compass1D                = 0x81,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Compass2D                = 0x82,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Compass3D                = 0x83,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Inclinometer1D           = 0x84,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Inclinometer2D           = 0x85,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Inclinometer3D           = 0x86,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Distance1D               = 0x87,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Distance2D               = 0x88,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_Distance3D               = 0x89,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_DeviceOrientation        = 0x8A,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_CompassD                 = 0x8B,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_InclinometerD            = 0x8C,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Orientation_DistanceD                = 0x8D,     /* Application/Physical Collection */
    /* 0x8E - 0x8F Reserved */
    kHIDUsage_Snsr_Scanner                              = 0x90,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Scanner_Barcode                      = 0x91,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Scanner_RFID                         = 0x92,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Scanner_NFC                          = 0x93,     /* Application/Physical Collection */
    /* 0x94 - 0x9F Reserved */
    kHIDUsage_Snsr_Time                                 = 0xA0,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Time_AlarmTimer                      = 0xA1,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Time_RealTimeClock                   = 0xA2,     /* Application/Physical Collection */
    /* 0xA3 - 0xDF Reserved */
    kHIDUsage_Snsr_Other                                = 0xE0,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Other_Custom                         = 0xE1,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Other_Generic                        = 0xE2,     /* Application/Physical Collection */
    kHIDUsage_Snsr_Other_GenericEnumerator              = 0xE3,     /* Application/Physical Collection */
    /* 0xE4 - 0xEF Reserved */
    /* 0xF0 - 0xFF Vendor Reserved */
    
    /* Common Sensor Type Data Fields */

    /* Usage Switches used in conjunction with other Data Usages. The value of the modifier is OR-ed in to the upper nibble of the 16bit Data Usage. */
    kHIDUsage_Snsr_Modifier_None                                = 0x0,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_ChangeSensitivityAbsolute           = 0x1,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_Max                                 = 0x2,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_Min                                 = 0x3,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_Accuracy                            = 0x4,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_Resolution                          = 0x5,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_ThresholdHigh                       = 0x6,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_ThresholdLow                        = 0x7,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_CalibrationOffset                   = 0x8,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_CalibrationMultiplier               = 0x9,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_ReportInterval                      = 0xA,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_FrequencyMax                        = 0xB,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_PeriodMax                           = 0xC,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_ChangeSensitivityPercentRange       = 0xD,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_ChangeSensitivityPercentRelative    = 0xE,      /* Data Field Usage Switch */
    kHIDUsage_Snsr_Modifier_VendorDefined                       = 0xF,      /* Data Field Usage Switch */

    /* Event Usages */
    kHIDUsage_Snsr_Event                                        = 0x0200,
    kHIDUsage_Snsr_Event_SensorState                            = 0x0201,
    kHIDUsage_Snsr_Event_SensorEvent                            = 0x0202,
    /* 0x0203 - 0x02FF Event Reserved */
    
    kHIDUsage_Snsr_Event_SensorState_Undefined                  = 0x0800,
    kHIDUsage_Snsr_Event_SensorState_Ready                      = 0x0801,
    kHIDUsage_Snsr_Event_SensorState_NotAvailable               = 0x0802,
    kHIDUsage_Snsr_Event_SensorState_NoData                     = 0x0803,
    kHIDUsage_Snsr_Event_SensorState_Initializing               = 0x0804,
    kHIDUsage_Snsr_Event_SensorState_AccessDenied               = 0x0805,
    kHIDUsage_Snsr_Event_SensorState_Error                      = 0x0806,
    /* 0x0807 - 0x080F Reserved */
    
    kHIDUsage_Snsr_Event_SensorEvent_Unknown                    = 0x0810,
    kHIDUsage_Snsr_Event_SensorEvent_StateChanged               = 0x0811,
    kHIDUsage_Snsr_Event_SensorEvent_PropertyChanged            = 0x0812,
    kHIDUsage_Snsr_Event_SensorEvent_DataUpdated                = 0x0813,
    kHIDUsage_Snsr_Event_SensorEvent_PollResponse               = 0x0814,
    kHIDUsage_Snsr_Event_SensorEvent_ChangeSensitivity          = 0x0815,
    kHIDUsage_Snsr_Event_SensorEvent_RangeMaxReached            = 0x0816,
    kHIDUsage_Snsr_Event_SensorEvent_RangeMinReached            = 0x0817,
    kHIDUsage_Snsr_Event_SensorEvent_HighThresholdCrossUp       = 0x0818,
    kHIDUsage_Snsr_Event_SensorEvent_HighThresholdCrossDown     = 0x0819,
    kHIDUsage_Snsr_Event_SensorEvent_LowThresholdCrossUp        = 0x081A,
    kHIDUsage_Snsr_Event_SensorEvent_LowThresholdCrossDown      = 0x081B,
    kHIDUsage_Snsr_Event_SensorEvent_ZeroThresholdCrossUp       = 0x081C,
    kHIDUsage_Snsr_Event_SensorEvent_ZeroThresholdCrossDown     = 0x081D,
    kHIDUsage_Snsr_Event_SensorEvent_PeriodExceeded             = 0x081E,
    kHIDUsage_Snsr_Event_SensorEvent_FrequencyExceeded          = 0x081F,
    kHIDUsage_Snsr_Event_SensorEvent_ComplexTrigger             = 0x0820,
    /* 0x0821 - 0x082F Reserved */

    /* Property Usages */
    kHIDUsage_Snsr_Property                                     = 0x0300,
    kHIDUsage_Snsr_Property_FriendlyName                        = 0x0301,
    kHIDUsage_Snsr_Property_PersistentUniqueID                  = 0x0302,
    kHIDUsage_Snsr_Property_SensorStatus                        = 0x0303,
    kHIDUsage_Snsr_Property_MinimumReportInterval               = 0x0304,
    kHIDUsage_Snsr_Property_Manufacturer                        = 0x0305,
    kHIDUsage_Snsr_Property_Model                               = 0x0306,
    kHIDUsage_Snsr_Property_SerialNumber                        = 0x0307,
    kHIDUsage_Snsr_Property_Description                         = 0x0308,
    kHIDUsage_Snsr_Property_ConnectionType                      = 0x0309,
    kHIDUsage_Snsr_Property_DevicePath                          = 0x030A,
    kHIDUsage_Snsr_Property_HardwareRevision                    = 0x030B,
    kHIDUsage_Snsr_Property_FirmwareVersion                     = 0x030C,
    kHIDUsage_Snsr_Property_ReleaseData                         = 0x030D,
    kHIDUsage_Snsr_Property_ReportInterval                      = 0x030E,
    kHIDUsage_Snsr_Property_ChangeSensitivityAbsolute           = 0x030F,
    kHIDUsage_Snsr_Property_ChangeSensitivityPercentRange       = 0x0310,
    kHIDUsage_Snsr_Property_ChangeSensitivityPercentRelative    = 0x0311,
    kHIDUsage_Snsr_Property_Accuracy                            = 0x0312,
    kHIDUsage_Snsr_Property_Resolution                          = 0x0313,
    kHIDUsage_Snsr_Property_Maximum                             = 0x0314,
    kHIDUsage_Snsr_Property_Minimum                             = 0x0315,
    kHIDUsage_Snsr_Property_ReportingState                      = 0x0316,
    kHIDUsage_Snsr_Property_SamplingRate                        = 0x0317,
    kHIDUsage_Snsr_Property_ResponseCurve                       = 0x0318,
    kHIDUsage_Snsr_Property_PowerState                          = 0x0319,
    /* 0x031A - 0x03FF Reserved */
    
    kHIDUsage_Snsr_Property_ConnectionType_Integrated           = 0x0830,
    kHIDUsage_Snsr_Property_ConnectionType_Attached             = 0x0831,
    kHIDUsage_Snsr_Property_ConnectionType_External             = 0x0832,
    /* 0x0833 - 0x083F Reserved */
    kHIDUsage_Snsr_Property_ReportingState_NoEvents             = 0x0840,
    kHIDUsage_Snsr_Property_ReportingState_AllEvents            = 0x0841,
    kHIDUsage_Snsr_Property_ReportingState_ThresholdEvents      = 0x0842,
    kHIDUsage_Snsr_Property_ReportingState_WakeNoEvents         = 0x0843,
    kHIDUsage_Snsr_Property_ReportingState_WakeAllEvents        = 0x0844,
    kHIDUsage_Snsr_Property_ReportingState_WakeThresholdEvents  = 0x0845,
    /* 0x0846 - 0x084F Reserved */
    kHIDUsage_Snsr_Property_PowerState_Undefined                = 0x0850,
    kHIDUsage_Snsr_Property_PowerState_D0_FullPower             = 0x0851,
    kHIDUsage_Snsr_Property_PowerState_D1_LowPower              = 0x0852,
    kHIDUsage_Snsr_Property_PowerState_D2_Standby               = 0x0853,
    kHIDUsage_Snsr_Property_PowerState_D3_Sleep                 = 0x0854,
    kHIDUsage_Snsr_Property_PowerState_D4_PowerOff              = 0x0855,
    /* 0x0855 - 0x085F Reserved */
    
    /* Specific Sensor Type Data Fields */
    /*** TODO ***/
};

/* Power Device Page (0x84) */
/* This section provides detailed descriptions of the usages employed by Power Devices. */
enum
{
    
    kHIDUsage_PD_Undefined = 0x00,                /* Power Device Undefined Usage */
    kHIDUsage_PD_iName = 0x01,                    /* CL- Power Device Name Index */
    kHIDUsage_PD_PresentStatus = 0x02,            /* CL- Power Device Present Status */
    kHIDUsage_PD_ChangedStatus = 0x03,            /* CA- Power Device Changed Status */
    kHIDUsage_PD_UPS = 0x04,                    /* CA- Uninterruptible Power Supply */
    kHIDUsage_PD_PowerSupply = 0x05,            /* CA- Power Supply */
    /* Reserved 0x06 - 0x0F */
    kHIDUsage_PD_BatterySystem = 0x10,            /* CP- Battery System power module */
    kHIDUsage_PD_BatterySystemID = 0x11,        /* SV IF- Battery System ID */
    kHIDUsage_PD_Battery = 0x12,                /* CP- Battery */
    kHIDUsage_PD_BatteryID = 0x13,                /* SV IF- Battery ID */
    kHIDUsage_PD_Charger = 0x14,                /* CP- Charger */
    kHIDUsage_PD_ChargerID = 0x15,                /* SV IF- Charger ID */
    kHIDUsage_PD_PowerConverter = 0x16,            /* CP- Power Converter power module */
    kHIDUsage_PD_PowerConverterID = 0x17,        /* SV IF- Power Converter ID */
    kHIDUsage_PD_OutletSystem = 0x18,            /* CP- Outlet System power module */
    kHIDUsage_PD_OutletSystemID = 0x19,            /* SV IF-Outlet System ID */
    kHIDUsage_PD_Input = 0x1A,                    /* CP- Power Device Input */
    kHIDUsage_PD_InputID = 0x1B,                /* SV IF- Power Device Input ID */
    kHIDUsage_PD_Output = 0x1C,                    /* CP- Power Device Output */
    kHIDUsage_PD_OutputID = 0x1D,                /* SV IF- Power Device Output ID */
    kHIDUsage_PD_Flow = 0x1E,                    /* CP- Power Device Flow */
    kHIDUsage_PD_FlowID = 0x1F,                    /* Item IF- Power Device Flow ID */
    kHIDUsage_PD_Outlet = 0x20,                    /* CP- Power Device Outlet */
    kHIDUsage_PD_OutletID = 0x21,                /* SV IF- Power Device Outlet ID */
    kHIDUsage_PD_Gang = 0x22,                    /* CL/CP- Power Device Gang */
    kHIDUsage_PD_GangID = 0x23,                    /* SV IF- Power Device Gang ID */
    kHIDUsage_PD_PowerSummary = 0x24,            /* CL/CP- Power Device Power Summary */
    kHIDUsage_PD_PowerSummaryID = 0x25,            /* SV IF- Power Device Power Summary ID */
    /* Reserved 0x26 - 0x2F */
    kHIDUsage_PD_Voltage = 0x30,                /* DV IF- Power Device Voltage */
    kHIDUsage_PD_Current = 0x31,                /* DV IF- Power Device Current */
    kHIDUsage_PD_Frequency = 0x32,                /* DV IF- Power Device Frequency */
    kHIDUsage_PD_ApparentPower = 0x33,            /* DV IF- Power Device Apparent Power */
    kHIDUsage_PD_ActivePower = 0x34,            /* DV IF- Power Device RMS Power */
    kHIDUsage_PD_PercentLoad = 0x35,            /* DV IF- Power Device Percent Load */
    kHIDUsage_PD_Temperature = 0x36,            /* DV IF- Power Device Temperature */
    kHIDUsage_PD_Humidity = 0x37,                /* DV IF- Power Device Humidity */
    kHIDUsage_PD_BadCount = 0x38,                /* DV IF- Power Device Bad Condition Count */
    /* Reserved 0x39 - 0x3F */
    kHIDUsage_PD_ConfigVoltage = 0x40,            /* SV/DV F- Power Device Nominal Voltage */
    kHIDUsage_PD_ConfigCurrent = 0x41,            /* SV/DV F- Power Device Nominal Current */
    kHIDUsage_PD_ConfigFrequency = 0x42,        /* SV/DV F- Power Device Nominal Frequency */
    kHIDUsage_PD_ConfigApparentPower = 0x43,    /* SV/DV F- Power Device Nominal Apparent Power */
    kHIDUsage_PD_ConfigActivePower = 0x44,        /* SV/DV F- Power Device Nominal RMS Power */
    kHIDUsage_PD_ConfigPercentLoad = 0x45,        /* SV/DV F- Power Device Nominal Percent Load */
    kHIDUsage_PD_ConfigTemperature = 0x46,        /* SV/DV F- Power Device Nominal Temperature */
    kHIDUsage_PD_ConfigHumidity = 0x47,            /* SV/DV F- Power Device Nominal Humidity */
    /* Reserved 0x48 - 0x4F */
    kHIDUsage_PD_SwitchOnControl = 0x50,        /* DV F- Power Device Switch On Control */
    kHIDUsage_PD_SwitchOffControl = 0x51,        /* DV F- Power Device Switch Off Control */
    kHIDUsage_PD_ToggleControl = 0x52,            /* DV F- Power Device Toogle Sequence Control */
    kHIDUsage_PD_LowVoltageTransfer = 0x53,        /* DV F- Power Device Min Transfer Voltage */
    kHIDUsage_PD_HighVoltageTransfer = 0x54,    /* DV F- Power Device Max Transfer Voltage */
    kHIDUsage_PD_DelayBeforeReboot = 0x55,        /* DV F- Power Device Delay Before Reboot */
    kHIDUsage_PD_DelayBeforeStartup = 0x56,        /* DV F- Power Device Delay Before Startup */
    kHIDUsage_PD_DelayBeforeShutdown = 0x57,    /* DV F- Power Device Delay Before Shutdown */
    kHIDUsage_PD_Test = 0x58,                    /* DV F- Power Device Test Request/Result */
    kHIDUsage_PD_ModuleReset = 0x59,            /* DV F- Power Device Reset Request/Result */
    kHIDUsage_PD_AudibleAlarmControl = 0x5A,    /* DV F- Power Device Audible Alarm Control */
    /* Reserved 0x5B - 0x5F */
    kHIDUsage_PD_Present = 0x60,                /* DV IOF- Power Device Present */
    kHIDUsage_PD_Good = 0x61,                    /* DV IOF- Power Device Good */
    kHIDUsage_PD_InternalFailure = 0x62,        /* DV IOF- Power Device Internal Failure */
    kHIDUsage_PD_VoltageOutOfRange = 0x63,        /* DV IOF- Power Device Voltage Out Of Range */
    kHIDUsage_PD_FrequencyOutOfRange = 0x64,    /* DV IOF- Power Device Frequency Out Of Range */
    kHIDUsage_PD_Overload = 0x65,                /* DV IOF- Power Device Overload */
    kHIDUsage_PD_OverCharged = 0x66,            /* DV IOF- Power Device Over Charged */
    kHIDUsage_PD_OverTemperature = 0x67,        /* DV IOF- Power Device Over Temperature */
    kHIDUsage_PD_ShutdownRequested = 0x68,        /* DV IOF- Power Device Shutdown Requested */
    kHIDUsage_PD_ShutdownImminent = 0x69,        /* DV IOF- Power Device Shutdown Imminent */
    /* Reserved 0x6A */
    kHIDUsage_PD_SwitchOnOff = 0x6B,            /* DV IOF- Power Device On/Off Switch Status */
    kHIDUsage_PD_Switchable = 0x6C,                /* DV IOF- Power Device Switchable */
    kHIDUsage_PD_Used = 0x6D,                    /* DV IOF- Power Device Used */
    kHIDUsage_PD_Boost = 0x6E,                    /* DV IOF- Power Device Boosted */
    kHIDUsage_PD_Buck = 0x6F,                    /* DV IOF- Power Device Bucked */
    kHIDUsage_PD_Initialized = 0x70,            /* DV IOF- Power Device Initialized */
    kHIDUsage_PD_Tested = 0x71,                    /* DV IOF- Power Device Tested */
    kHIDUsage_PD_AwaitingPower = 0x72,            /* DV IOF- Power Device Awaiting Power */
    kHIDUsage_PD_CommunicationLost = 0x73,        /* DV IOF- Power Device Communication Lost */
    /* Reserved 0x74 - 0xFC */
    kHIDUsage_PD_iManufacturer = 0xFD,            /* SV F- Power Device Manufacturer String Index */
    kHIDUsage_PD_iProduct = 0xFE,                /* SV F- Power Device Product String Index */
    kHIDUsage_PD_iserialNumber = 0xFF            /* SV F- Power Device Serial Number String Index */
};

/* Battery System Page (x85) */
/* This section provides detailed descriptions of the usages employed by Battery Systems. */    
enum
{
    kHIDUsage_BS_Undefined = 0x00,                /* Battery System Undefined */
    kHIDUsage_BS_SMBBatteryMode = 0x01,            /* CL - SMB Mode */
    kHIDUsage_BS_SMBBatteryStatus = 0x02,        /* CL - SMB Status */
    kHIDUsage_BS_SMBAlarmWarning = 0x03,        /* CL - SMB Alarm Warning */
    kHIDUsage_BS_SMBChargerMode = 0x04,            /* CL - SMB Charger Mode */
    kHIDUsage_BS_SMBChargerStatus = 0x05,        /* CL - SMB Charger Status */
    kHIDUsage_BS_SMBChargerSpecInfo = 0x06,        /* CL - SMB Charger Extended Status */
    kHIDUsage_BS_SMBSelectorState = 0x07,        /* CL - SMB Selector State */
    kHIDUsage_BS_SMBSelectorPresets = 0x08,        /* CL - SMB Selector Presets */
    kHIDUsage_BS_SMBSelectorInfo = 0x09,        /* CL - SMB Selector Info */
    /* Reserved 0x0A - 0x0F */
    kHIDUsage_BS_OptionalMfgFunction1 = 0x10,    /* DV F - Battery System Optional SMB Mfg Function 1 */
    kHIDUsage_BS_OptionalMfgFunction2 = 0x11,    /* DV F - Battery System Optional SMB Mfg Function 2 */
    kHIDUsage_BS_OptionalMfgFunction3 = 0x12,    /* DV F - Battery System Optional SMB Mfg Function 3 */
    kHIDUsage_BS_OptionalMfgFunction4 = 0x13,    /* DV F - Battery System Optional SMB Mfg Function 4 */
    kHIDUsage_BS_OptionalMfgFunction5 = 0x14,    /* DV F - Battery System Optional SMB Mfg Function 5 */
    kHIDUsage_BS_ConnectionToSMBus = 0x15,        /* DF F - Battery System Connection To System Management Bus */
    kHIDUsage_BS_OutputConnection = 0x16,        /* DF F - Battery System Output Connection Status */
    kHIDUsage_BS_ChargerConnection = 0x17,        /* DF F - Battery System Charger Connection */
    kHIDUsage_BS_BatteryInsertion = 0x18,        /* DF F - Battery System Battery Insertion */
    kHIDUsage_BS_Usenext = 0x19,                /* DF F - Battery System Use Next */
    kHIDUsage_BS_OKToUse = 0x1A,                /* DF F - Battery System OK To Use */
    kHIDUsage_BS_BatterySupported = 0x1B,        /* DF F - Battery System Battery Supported */
    kHIDUsage_BS_SelectorRevision = 0x1C,        /* DF F - Battery System Selector Revision */
    kHIDUsage_BS_ChargingIndicator = 0x1D,        /* DF F - Battery System Charging Indicator */
    /* Reserved 0x1E - 0x27 */
    kHIDUsage_BS_ManufacturerAccess = 0x28,        /* DV F - Battery System Manufacturer Access */
    kHIDUsage_BS_RemainingCapacityLimit = 0x29,    /* DV F - Battery System Remaining Capacity Limit */
    kHIDUsage_BS_RemainingTimeLimit = 0x2A,        /* DV F - Battery System Remaining Time Limit */
    kHIDUsage_BS_AtRate = 0x2B,                    /* DV F - Battery System At Rate... */
    kHIDUsage_BS_CapacityMode = 0x2C,            /* DV F - Battery System Capacity Mode */
    kHIDUsage_BS_BroadcastToCharger = 0x2D,        /* DV F - Battery System Broadcast To Charger */
    kHIDUsage_BS_PrimaryBattery = 0x2E,            /* DV F - Battery System Primary Battery */
    kHIDUsage_BS_ChargeController = 0x2F,        /* DV F - Battery System Charge Controller */
    /* Reserved 0x30 - 0x3F */
    kHIDUsage_BS_TerminateCharge = 0x40,        /* DF IOF - Battery System Terminate Charge */
    kHIDUsage_BS_TerminateDischarge = 0x41,        /* DF IOF - Battery System Terminate Discharge */
    kHIDUsage_BS_BelowRemainingCapacityLimit = 0x42, /* DF IOF - Battery System Below Remaining Capacity Limit */
    kHIDUsage_BS_RemainingTimeLimitExpired = 0x43, /* DF IOF - Battery System Remaining Time Limit Expired */
    kHIDUsage_BS_Charging = 0x44,                /* DF IOF - Battery System Charging */
    kHIDUsage_BS_Discharging = 0x45,            /* DV IOF - Battery System Discharging */
    kHIDUsage_BS_FullyCharged = 0x46,            /* DF IOF - Battery System Fully Charged */
    kHIDUsage_BS_FullyDischarged = 0x47,        /* DV IOF - Battery System Fully Discharged */
    kHIDUsage_BS_ConditioningFlag = 0x48,        /* DV IOF - Battery System Conditioning Flag */
    kHIDUsage_BS_AtRateOK = 0x49,                /* DV IOF - Battery System At Rate OK */
    kHIDUsage_BS_SMBErrorCode = 0x4A,            /* DF IOF - Battery System SMB Error Code */
    kHIDUsage_BS_NeedReplacement = 0x4B,        /* DF IOF - Battery System Need Replacement */
    /* Reserved 0x4C - 0x5F */
    kHIDUsage_BS_AtRateTimeToFull = 0x60,        /* DV IF - Battery System At Rate Time To Full */
    kHIDUsage_BS_AtRateTimeToEmpty = 0x61,        /* DV IF - Battery System At Rate Time To Empty */
    kHIDUsage_BS_AverageCurrent = 0x62,            /* DV IF - Battery System Average Current */
    kHIDUsage_BS_Maxerror = 0x63,                /* DV IF - Battery System Max Error */
    kHIDUsage_BS_RelativeStateOfCharge = 0x64,    /* DV IF - Battery System Relative State Of Charge */
    kHIDUsage_BS_AbsoluteStateOfCharge = 0x65,    /* DV IF - Battery System Absolute State Of Charge */
    kHIDUsage_BS_RemainingCapacity = 0x66,        /* DV IF - Battery System Remaining Capacity */
    kHIDUsage_BS_FullChargeCapacity = 0x67,        /* DV IF - Battery System Full Charge Capacity */
    kHIDUsage_BS_RunTimeToEmpty = 0x68,            /* DV IF - Battery System Run Time To Empty */
    kHIDUsage_BS_AverageTimeToEmpty = 0x69,        /* DV IF - Battery System Average Time To Empty */
    kHIDUsage_BS_AverageTimeToFull = 0x6A,        /* DV IF - Battery System Average Time To Full */
    kHIDUsage_BS_CycleCount = 0x6B,                /* DV IF - Battery System Cycle Count */
    /* Reserved 0x6C - 0x7F */
    kHIDUsage_BS_BattPackModelLevel = 0x80,        /* SV F - Battery System Batt Pack Model Level */
    kHIDUsage_BS_InternalChargeController = 0x81, /* SF F - Battery System Internal Charge Controller */
    kHIDUsage_BS_PrimaryBatterySupport = 0x82,    /* SF F - Battery System Primary Battery Support */
    kHIDUsage_BS_DesignCapacity = 0x83,            /* SV F - Battery System Design Capacity */
    kHIDUsage_BS_SpecificationInfo = 0x84,        /* SV F - Battery System Specification Info */
    kHIDUsage_BS_ManufacturerDate = 0x85,        /* SV F - Battery System Manufacturer Date */
    kHIDUsage_BS_SerialNumber = 0x86,            /* SV F - Battery System Serial Number */
    kHIDUsage_BS_iManufacturerName = 0x87,        /* SV F - Battery System Manufacturer Name Index */
    kHIDUsage_BS_iDevicename = 0x88,            /* SV F - Battery System Device Name Index */
    kHIDUsage_BS_iDeviceChemistry = 0x89,        /* SV F - Battery System Device Chemistry Index */
    kHIDUsage_BS_ManufacturerData = 0x8A,        /* SV F - Battery System Manufacturer Data */
    kHIDUsage_BS_Rechargable = 0x8B,            /* SV F - Battery System Rechargable */
    kHIDUsage_BS_WarningCapacityLimit = 0x8C,    /* SV F - Battery System Warning Capacity Limit */
    kHIDUsage_BS_CapacityGranularity1 = 0x8D,    /* SV F - Battery System Capacity Granularity 1 */
    kHIDUsage_BS_CapacityGranularity2 = 0x8E,    /* SV F - Battery System Capacity Granularity 2 */
    kHIDUsage_BS_iOEMInformation = 0x8F,        /* SV F - Battery System OEM Information Index */
    /* Reserved 0x90 - 0xBF */
    kHIDUsage_BS_InhibitCharge = 0xC0,            /* DF IOF - Battery System Inhibit Charge */
    kHIDUsage_BS_EnablePolling = 0xC1,            /* DF IOF - Battery System Enable Polling */
    kHIDUsage_BS_ResetToZero = 0xC2,            /* DF IOF - Battery System Reset To Zero */
    /* Reserved 0xC3 - 0xCF */
    kHIDUsage_BS_ACPresent = 0xD0,                /* DF IOF - Battery System AC Present */
    kHIDUsage_BS_BatteryPresent = 0xD1,            /* DF IOF - Battery System Battery Present */
    kHIDUsage_BS_PowerFail = 0xD2,                /* DF IOF - Battery System Power Fail */
    kHIDUsage_BS_AlarmInhibited = 0xD3,            /* DF IOF - Battery System Alarm Inhibited */
    kHIDUsage_BS_ThermistorUnderRange = 0xD4,    /* DF IOF - Battery System Thermistor Under Range */
    kHIDUsage_BS_ThermistorHot = 0xD5,            /* DF IOF - Battery System Thermistor Hot */
    kHIDUsage_BS_ThermistorCold = 0xD6,            /* DF IOF - Battery System Thermistor Cold */
    kHIDUsage_BS_ThermistorOverRange = 0xD7,    /* DF IOF - Battery System Thermistor Over Range */
    kHIDUsage_BS_VoltageOutOfRange = 0xD8,        /* DF IOF - Battery System Voltage Out Of Range */
    kHIDUsage_BS_CurrentOutOfRange = 0xD9,        /* DF IOF - Battery System Current Out Of Range */
    kHIDUsage_BS_CurrentNotRegulated = 0xDA,    /* DF IOF - Battery System Current Not Regulated */
    kHIDUsage_BS_VoltageNotRegulated = 0xDB,    /* DF IOF - Battery System Voltage Not Regulated */
    kHIDUsage_BS_MasterMode = 0xDC,                /* DF IOF - Battery System Master Mode */
    /* Reserved 0xDD - 0xEF */
    kHIDUsage_BS_ChargerSelectorSupport = 0xF0,    /* SF F- Battery System Charger Support Selector */
    kHIDUsage_BS_ChargerSpec = 0xF1,            /* SF F- Battery System Charger Specification */
    kHIDUsage_BS_Level2 = 0xF2,                    /* SF F- Battery System Charger Level 2 */
    kHIDUsage_BS_Level3 = 0xF3                    /* SF F- Battery System Charger Level 3 */
    /* Reserved 0xF2 - 0xFF */
};

/* Bar Code Scanner Page (0x8C) */
/* This section provides detailed descriptions of the usages employed by Bar Code Scanner Devices. */
enum
{
    kHIDUsage_BCS_Undefined = 0x00,                     /* Bar Code Scanner Undefined Usage */
    kHIDUsage_BCS_BadgeReader = 0x01,                   /* CA - Bar Code Badge Reader */
    kHIDUsage_BCS_BarCodeScanner = 0x02,                /* CA -Bar Code Scanner */
    kHIDUsage_BCS_DumbBarCodeScanner = 0x03,            /* CA -Dumb Bar Code Scanner  Usage */
    kHIDUsage_BCS_CordlessScannerBase = 0x04,           /* CA -Cordless Base Usage */
    kHIDUsage_BCS_BarCodeScannerCradle = 0x05,          /* CA -Bar Code Scanner Cradle Usage */
    /* Reserved 0x06 - 0x0F */
    kHIDUsage_BCS_AttributeReport = 0x10,               /* CL - Attribute Report */
    kHIDUsage_BCS_SettingsReport = 0x11,                /* CL - Settings Report */
    kHIDUsage_BCS_ScannedDataReport = 0x12,             /* CL - Scanned Data Report */
    kHIDUsage_BCS_RawScannedDataReport = 0x13,          /* CL - Raw Scanned Data Report */
    kHIDUsage_BCS_TriggerReport = 0x14,                 /* CL - Trigger Report */
    kHIDUsage_BCS_StatusReport = 0x15,                  /* CL - Status Report */
    kHIDUsage_BCS_UPC_EANControlReport = 0x16,          /* CL - UPC/EAN Control Report */
    kHIDUsage_BCS_EAN2_3LabelControlReport = 0x17,      /* CL - EAN 2/3 Label Control Report */
    kHIDUsage_BCS_Code39ControlReport = 0x18,           /* CL - Code 39 Control Report */
    kHIDUsage_BCS_Interleaved2of5ControlReport = 0x19,  /* CL - Interleaved 2 of 5 Control Report */
    kHIDUsage_BCS_Standard2of5ControlReport = 0x1A,     /* CL - Standard 2 of 5 Control Report */
    kHIDUsage_BCS_MSIPlesseyControlReport = 0x1B,       /* CL - MSI Plessey Control Report */
    kHIDUsage_BCS_CodabarControlReport = 0x1C,          /* CL - Codabar Control Report */
    kHIDUsage_BCS_Code128ControlReport = 0x1D,          /* CL - Code 128 Control Report */
    kHIDUsage_BCS_Misc1DControlReport = 0x1E,           /* CL - Misc 1D Control Report */
    kHIDUsage_BCS_2DControlReport = 0x1F,               /* CL - 2D Control Report */
    /* Reserved 0x20 - 0x2F */
    kHIDUsage_BCS_Aiming_PointerMide = 0x30,            /* SF - Aiming Pointer Mode */
    kHIDUsage_BCS_BarCodePresentSensor = 0x31,          /* SF - Bar Code Present Sensor */
    kHIDUsage_BCS_Class1ALaser = 0x32,                  /* SF - Class 1A Laser */
    kHIDUsage_BCS_Class2Laser = 0x33,                   /* SF - Class 2 Laser */
    kHIDUsage_BCS_HeaterPresent = 0x34,                 /* SF - Heater Present */
    kHIDUsage_BCS_ContactScanner = 0x35,                /* SF - Contact Scanner */
    kHIDUsage_BCS_ElectronicArticleSurveillanceNotification = 0x36, /* SF - Electronic Article Surveillance Notification */
    kHIDUsage_BCS_ConstantElectronicArticleSurveillance = 0x37, /* SF - Constant Electronic Article Surveillance */
    kHIDUsage_BCS_ErrorIndication = 0x38,               /* SF - Error Indication */
    kHIDUsage_BCS_FixedBeeper = 0x39,                   /* SF - Fixed Beeper */
    kHIDUsage_BCS_GoodDecodeIndication = 0x3A,          /* SF - Good Decode Indication */
    kHIDUsage_BCS_HandsFreeScanning = 0x3B,             /* SF - Hands Free Scanning */
    kHIDUsage_BCS_IntrinsicallySafe = 0x3C,             /* SF - Intrinsically Safe */
    kHIDUsage_BCS_KlasseEinsLaser = 0x3D,               /* SF - Klasse Eins Laser */
    kHIDUsage_BCS_LongRangeScanner = 0x3E,              /* SF - Long Range Scanner */
    kHIDUsage_BCS_MirrorSpeedControl = 0x3F,            /* SF - Mirror Speed Control */
    kHIDUsage_BCS_NotOnFileIndication = 0x40,           /* SF - Not On File Indication */
    kHIDUsage_BCS_ProgrammableBeeper = 0x41,            /* SF - Programmable Beeper */
    kHIDUsage_BCS_Triggerless = 0x42,                   /* SF - Triggerless */
    kHIDUsage_BCS_Wand = 0x43,                          /* SF - Wand */
    kHIDUsage_BCS_WaterResistant = 0x44,                /* SF - Water Resistant */
    kHIDUsage_BCS_MultiRangeScanner = 0x45,             /* SF - Multi-Range Scanner */
    kHIDUsage_BCS_ProximitySensor = 0x46,               /* SF - Proximity Sensor */
    /* Reserved 0x47 - 0x4C */
    kHIDUsage_BCS_FragmentDecoding = 0x4D,              /* DF - Fragment Decoding */
    kHIDUsage_BCS_ScannerReadConfidence = 0x4E,         /* DV - Scanner Read Confidence */
    kHIDUsage_BCS_DataPrefix = 0x4F,                    /* NAry - Data Prefix  */
    kHIDUsage_BCS_PrefixAIMI = 0x50,                    /* SEL - Prefix AIMI */
    kHIDUsage_BCS_PrefixNone = 0x51,                    /* SEL - Prefix None */
    kHIDUsage_BCS_PrefixProprietary = 0x52,             /* SEL - Prefix Proprietary */
    /* Reserved 0x53 - 0x54 */
    kHIDUsage_BCS_ActiveTime = 0x55,                    /* DV - Active Time */
    kHIDUsage_BCS_AimingLaserPattern = 0x56,            /* DF - Aiming Laser Pattern */
    kHIDUsage_BCS_BarCodePresent = 0x57,                /* OOC - Bar Code Present */
    kHIDUsage_BCS_BeeperState = 0x58,                   /* OOC - Beeper State */
    kHIDUsage_BCS_LaserOnTime = 0x59,                   /* DV - Laser On Time */
    kHIDUsage_BCS_LaserState = 0x5A,                    /* OOC - Laser State */
    kHIDUsage_BCS_LockoutTime = 0x5B,                   /* DV - Lockout Time */
    kHIDUsage_BCS_MotorState = 0x5C,                    /* OOC - Motor State */
    kHIDUsage_BCS_MotorTimeout = 0x5D,                  /* DV - Motor Timeout */
    kHIDUsage_BCS_PowerOnResetScanner = 0x5E,           /* DF - Power On Reset Scanner */
    kHIDUsage_BCS_PreventReadOfBarcodes = 0x5F,         /* DF - Prevent Read of Barcodes */
    kHIDUsage_BCS_InitiateBarcodeRead = 0x60,           /* DF - Initiate Barcode Read */
    kHIDUsage_BCS_TriggerState = 0x61,                  /* DF - Trigger State */
    kHIDUsage_BCS_TriggerMode = 0x62,                   /* NAry - Trigger Mode */
    kHIDUsage_BCS_TriggerModeBlinkingLaserOn = 0x63,    /* SEL - Trigger Mode Blinking Laser On */
    kHIDUsage_BCS_TriggerModeContinuousLaserOn = 0x64,  /* SEL - Trigger Mode Continuous Laser On */
    kHIDUsage_BCS_TriggerModeLaserOnWhilePulled = 0x65, /* SEL - Trigger Mode Laser on while Pulled */
    kHIDUsage_BCS_TriggerModeLaserStaysOnAfterTriggerRelease = 0x66, /* SEL - Trigger Mode Laser stays on after Trigger Release */
    /* Reserved 0x67 - 0x6C */
    kHIDUsage_BCS_CommitParametersToNVM = 0x6D,         /* DF - Commit Parameters to NVM */
    kHIDUsage_BCS_ParameterScanning = 0x6E,             /* DF - Parameter Scanning */
    kHIDUsage_BCS_ParametersChanged = 0x6F,             /* OOC - Parameters Changed */
    kHIDUsage_BCS_SetParameterDefaultValues = 0x70,     /* DF - Set parameter default values */
    /* Reserved 0x71 - 0x74 */
    kHIDUsage_BCS_ScannerInCradle = 0x75,               /* OOC - Scanner In Cradle */
    kHIDUsage_BCS_ScannerInRange = 0x76,                /* OOC - Scanner In Range */
    /* Reserved 0x77 - 0x79 */
    kHIDUsage_BCS_AimDuration = 0x7A,                   /* DV - Aim Duration */
    kHIDUsage_BCS_GoodReadLampDuration = 0x7B,          /* DV - Good Read Lamp Duration */
    kHIDUsage_BCS_GoodReadLampIntensity = 0x7C,         /* DV - Good Read Lamp Intensity */
    kHIDUsage_BCS_GoodReadLED = 0x7D,                   /* DF - Good Read LED */
    kHIDUsage_BCS_GoodReadToneFrequency = 0x7E,         /* DV - Good Read Tone Frequency*/
    kHIDUsage_BCS_GoodReadToneLength = 0x7F,            /* DV - Good Read Tone Length */
    kHIDUsage_BCS_GoodReadToneVolume = 0x80,            /* DV - Good Read Tone Volume */
    /* Reserved 0x81 */
    kHIDUsage_BCS_NoReadMessage = 0x82,                 /* DF - No Read Message */
    kHIDUsage_BCS_NotOnFileVolume = 0x83,               /* DV - Not on File Volume */
    kHIDUsage_BCS_PowerupBeep = 0x84,                   /* DF - Powerup Beep */
    kHIDUsage_BCS_SoundErrorBeep = 0x85,               /* DF - Sound Error Beep */
    kHIDUsage_BCS_SoundGoodReadBeep = 0x86,             /* DF - Sound Good Read Beep */
    kHIDUsage_BCS_SoundNotOnFileBeep = 0x87,            /* DF - Sound Not On File Beep */
    kHIDUsage_BCS_GoodReadWhenToWrite = 0x88,           /* NArry - Good Read When to Write */
    kHIDUsage_BCS_GRWTIAfterDecode = 0x89,              /* SEL - GRWTI After Decode */
    kHIDUsage_BCS_GRWTIBeep_LampAfterTransmit = 0x8A,   /* SEL - GRWTI Beep/Lamp after transmit */
    kHIDUsage_BCS_GRWTINoBeep_LampUseAtAll = 0x8B,      /* SEL - GRWTI No Beep/Lamp at all */
    /* Reserved 0x8C - 0x90 */
    kHIDUsage_BCS_BooklandEAN = 0x91,                   /* DF - Bookland EAN */
    kHIDUsage_BCS_ConvertEAN8To13Type = 0x92,           /* DF - Convert EAN 8 to 13 Type */
    kHIDUsage_BCS_ConvertUPCAToEAN_13 = 0x93,           /* DF - Convert UPC A to EAN-13 */
    kHIDUsage_BCS_ConvertUPC_EToA = 0x94,               /* DF - Convert UPC-E to A */
    kHIDUsage_BCS_EAN_13 = 0x95,                        /* DF - EAN-13 */
    kHIDUsage_BCS_EAN_8 = 0x96,                         /* DF - EAN_8 */
    kHIDUsage_BCS_EAN_99_128_Mandatory  = 0x97,         /* DF - EAN-99 128_Mandatory  */
    kHIDUsage_BCS_EAN_99_P5_128_Optional = 0x98,        /* DF - EAN-99 P5/128_Optional */
    /* Reserved 0x99 */
    kHIDUsage_BCS_UPC_EAN = 0x9A,                       /* DF - UPC/EAN */
    kHIDUsage_BCS_UPC_EANCouponCode = 0x9B,             /* DF - UPC/EAN Coupon Code */
    kHIDUsage_BCS_UPC_EANPeriodicals = 0x9C,            /* DV - UPC/EAN Periodicals */
    kHIDUsage_BCS_UPC_A = 0x9D,                         /* DF - UPC-A */
    kHIDUsage_BCS_UPC_AWith128Mandatory = 0x9E,         /* DF - UPC-A with 128 Mandatory */
    kHIDUsage_BCS_UPC_AWith128Optical = 0x9F,           /* DF - UPC-A with 128 Optical */
    kHIDUsage_BCS_UPC_AWithP5Optional = 0xA0,           /* DF - UPC-A with P5 Optional */
    kHIDUsage_BCS_UPC_E = 0xA1,                         /* DF - UPC-E */
    kHIDUsage_BCS_UPC_E1 = 0xA2,                        /* DF - UPC-E1 */
    /* Reserved 0xA3 - 0xA8 */
    kHIDUsage_BCS_Periodical = 0xA9,                    /* NArry - Periodical */
    kHIDUsage_BCS_PeriodicalAutoDiscriminatePlus2 = 0xAA,/* SEL - Periodical Auto-Discriminate + 2 */
    kHIDUsage_BCS_PeriodicalOnlyDecodeWithPlus2 = 0xAB, /* SEL - Periodical Only Decode with + 2 */
    kHIDUsage_BCS_PeriodicalIgnorePlus2 = 0xAC,         /* SEL - Periodical Ignore + 2 */
    kHIDUsage_BCS_PeriodicalAutoDiscriminatePlus5 = 0xAD,/* SEL - Periodical Auto-Discriminate + 5 */
    kHIDUsage_BCS_PeriodicalOnlyDecodeWithPlus5 = 0xAE, /* SEL - Periodical Only Decode with + 5 */
    kHIDUsage_BCS_PeriodicalIgnorePlus5 = 0xAF,         /* SEL - Periodical Ignore + 5 */
    kHIDUsage_BCS_Check = 0xB0,                         /* NArry - Check */
    kHIDUsage_BCS_CheckDisablePrice = 0xB1,             /* SEL - Check Disable Price */
    kHIDUsage_BCS_CheckEnable4DigitPrice = 0xB2,        /* SEL - Check Enable 4 digit Price */
    kHIDUsage_BCS_CheckEnable5DigitPrice = 0xB3,        /* SEL - Check Enable 5 digit Price */
    kHIDUsage_BCS_CheckEnableEuropean4DigitPrice = 0xB4,/* SEL - Check Enable European 4 digit Price */
    kHIDUsage_BCS_CheckEnableEuropean5DigitPrice = 0xB5,/* SEL - Check Enable European 5 digit Price */
    /* Reserved 0xB6 */
    kHIDUsage_BCS_EANTwoLabel = 0xB7,                   /* DF - EAN Two Label  */
    kHIDUsage_BCS_EANThreeLabel  = 0xB8,                /* DF - EAN Thread Label */
    kHIDUsage_BCS_EAN8FlagDigit1 = 0xB9,                /* DV - EAN 8 Flag Digit 1 */
    kHIDUsage_BCS_EAN8FlagDigit2 = 0xBA,                /* DV - EAN 8 Flag Digit 2 */
    kHIDUsage_BCS_EAN8FlagDigit3 = 0xBB,                /* DV - EAN 8 Flag Digit 3 */
    kHIDUsage_BCS_EAN13FlagDigit1 = 0xBC,               /* DV - EAN 13 Flag Digit 1 */
    kHIDUsage_BCS_EAN13FlagDigit2 = 0xBD,               /* DV - EAN 13 Flag Digit 2 */
    kHIDUsage_BCS_EAN13FlagDigit3 = 0xBE,               /* DV - EAN 13 Flag Digit 3 */
    kHIDUsage_BCS_AddEAN2_3LabelDefinition = 0xBF,      /* DF - Add EAN 2/3 Label Definition */
    kHIDUsage_BCS_ClearAllEAN2_3LabelDefinitions = 0xC0,/* DF - Clear all EAN 2/3 Label Definitions */
    /* Reserved 0xC1 - 0xC2 */
    kHIDUsage_BCS_Codabar = 0xC3,                       /* DF - Codabar */
    kHIDUsage_BCS_Code128 = 0xC4,                       /* DF - Code 128 */
    /* Reserved 0xC5 - 0xC6 */
    kHIDUsage_BCS_Code39 = 0xC7,                        /* DF - Code 39 */
    kHIDUsage_BCS_Code93 = 0xC8,                        /* DF - Code 93 */
    kHIDUsage_BCS_FullASCIIConversion = 0xC9,           /* DF - Full ASCII Conversion */
    kHIDUsage_BCS_Interleaved2of5 = 0xCA,               /* DF - Interleaved 2 of 5 */
    kHIDUsage_BCS_ItalianPharmacyCode = 0xCB,           /* DF - Italian Pharmacy Code */
    kHIDUsage_BCS_MSI_Plessey = 0xCC,                   /* DF - MSI/Plessey */
    kHIDUsage_BCS_Standard2of5IATA = 0xCD,              /* DF - Standard 2 of 5 IATA */
    kHIDUsage_BCS_Standard2of5 = 0xCE,                  /* DF - Standard 2 of 5 */
    /* Reserved 0xCF - 0xD2 */
    kHIDUsage_BCS_TransmitStart_Stop = 0xD3,            /* DF - Transmit Start/Stop */
    kHIDUsage_BCS_TriOptic = 0xD4,                      /* DF - Tri-Optic  */
    kHIDUsage_BCS_UCC_EAN_128 = 0xD5,                   /* DF - UCC/EAN-128  */
    kHIDUsage_BCS_CheckDigit = 0xD6,                    /* NArry - Check Digit */
    kHIDUsage_BCS_CheckDigitDisable = 0xD7,             /* SEL - Check Digit Disable */
    kHIDUsage_BCS_CheckDigitEnableInterleaved2of5OPCC = 0xD8, /* SEL - Check Digit Enable Interleaved 2 of 5 OPCC */
    kHIDUsage_BCS_CheckDigitEnableInterleaved2of5USS = 0xD9, /* SEL - Check Digit Enable Interleaved 2 of 5 USS */
    kHIDUsage_BCS_CheckDigitEnableStandard2of5OPCC = 0xD8, /* SEL - Check Digit Enable Standard 2 of 5 OPCC */
    kHIDUsage_BCS_CheckDigitEnableStandard2of5USS = 0xD9, /* SEL - Check Digit Enable Standard 2 of 5 USS */
    kHIDUsage_BCS_CheckDigitEnableOneMSIPlessey = 0xDC, /* SEL - Check Digit Enable One MSI Plessey */
    kHIDUsage_BCS_CheckDigitEnableTwoMSIPlessey = 0xDD, /* SEL - Check Digit Enable Two MSI Plessey */
    kHIDUsage_BCS_CheckDigitCodabarEnable = 0xDE,       /* SEL - Check Digit Codabar Enable */
    kHIDUsage_BCS_CheckDigitCode99Enable = 0xDF,        /* SEL - Check Digit Code 99 Enable */
    /* Reserved 0xE0 - 0xEF */
    kHIDUsage_BCS_TransmitCheckDigit = 0xF0,            /* NArry - Transmit Check Digit */
    kHIDUsage_BCS_DisableCheckDigitTransmit = 0xF1,     /* SEL - Disable Check Digit Transmit */
    kHIDUsage_BCS_EnableCheckDigitTransmit = 0xF2,      /* SEL - Enable Check Digit Transmit */
    /* Reserved 0xF3 - 0xFA */
    kHIDUsage_BCS_SymbologyIdentifier1 = 0xFB,          /* DV - Symbology Identifier 1 */
    kHIDUsage_BCS_SymbologyIdentifier2 = 0xFC,          /* DV - Symbology Identifier 2 */
    kHIDUsage_BCS_SymbologyIdentifier3 = 0xFD,          /* DV - Symbology Identifier 3 */
    kHIDUsage_BCS_DecodedData = 0xFE,                   /* DV - Decoded Data */
    kHIDUsage_BCS_DecodeDataContinued = 0xFF,           /* DF - Decode Data Continued */
    kHIDUsage_BCS_BarSpaceData = 0x100,                 /* DV - Bar Space Data */
    kHIDUsage_BCS_ScannerDataAccuracy = 0x101,          /* DV - Scanner Data Accuracy */
    kHIDUsage_BCS_RawDataPolarity = 0x102,              /* NArry - Raw Data Polarity */
    kHIDUsage_BCS_PolarityInvertedBarCode = 0x103,      /* SEL - Polarity Inverted Bar Code */
    kHIDUsage_BCS_PolarityNormalBarCode = 0x103,        /* SEL - Polarity Normal Bar Code */
    /* Reserved 0x105 */
    kHIDUsage_BCS_MinimumLengthToDecode = 0x106,        /* DV - Minimum Length to Decode */
    kHIDUsage_BCS_MaximumLengthToDecode = 0x107,        /* DV - Maximum Length to Decode */
    kHIDUsage_BCS_FirstDiscreteLengthToDecode = 0x108,  /* DV - First Discrete Length to Decode */
    kHIDUsage_BCS_SecondDiscreteLengthToDecode = 0x109, /* DV - Second Discrete Length to Decode */
    kHIDUsage_BCS_DataLengthMethod = 0x10A,             /* NArry - Data Length Method */
    kHIDUsage_BCS_DLMethodReadAny = 0x10B,              /* DF - DL Method Read any */
    kHIDUsage_BCS_DLMethodCheckInRange = 0x10C,         /* DF - DL Method Check in Range */
    kHIDUsage_BCS_DLMethodCheckForDiscrete = 0x10D,     /* DF - DL Method Check for Discrete */
    /* Reserved 0x10E - 0x10F */
    kHIDUsage_BCS_AztecCode = 0x110,                    /* DF - Aztec Code */
    kHIDUsage_BCS_BC412 = 0x111,                        /* DF - BC412 */
    kHIDUsage_BCS_ChannelCode = 0x112,                  /* DF - Channel Code */
    kHIDUsage_BCS_Code16 = 0x113,                       /* DF - Code 16 */
    kHIDUsage_BCS_Code32 = 0x114,                       /* DF - Code 32 */
    kHIDUsage_BCS_Code49 = 0x115,                       /* DF - Code 49 */
    kHIDUsage_BCS_CodeOne = 0x116,                      /* DF - Code One */
    kHIDUsage_BCS_Colorcode = 0x117,                    /* DF - Colorcode */
    kHIDUsage_BCS_DataMatrix = 0x118,                   /* DF - Data Matrix */
    kHIDUsage_BCS_MaxiCode = 0x119,                     /* DF - MaxiCode */
    kHIDUsage_BCS_MicroPDF = 0x11A,                     /* DF - MicroPDF */
    kHIDUsage_BCS_PDF_417 = 0x11B,                      /* DF - PDF-417 */
    kHIDUsage_BCS_PosiCode = 0x11C,                     /* DF - PosiCode */
    kHIDUsage_BCS_QRCode = 0x11D,                       /* DF - QR Code */
    kHIDUsage_BCS_SuperCode = 0x11E,                    /* DF - SuperCode */
    kHIDUsage_BCS_UltraCode = 0x11F,                    /* DF - UltraCode */
    kHIDUsage_BCS_USB_5_SlugCode = 0x120,               /* DF - USD-5 (Slug Code) */
    kHIDUsage_BCS_VeriCode = 0x121                      /* DF - VeriCode */
    /* Reserved 0x122 - 0xFFFF */
};

/* Weighing Devices Page (0x8D) */
/* This section provides detailed descriptions of the usages employed by Weighing Devices. */
enum
{
    kHIDUsage_WD_Undefined = 0x00,                      /* Weighing Device Undefined Usage */
    kHIDUsage_WD_WeighingDevice = 0x01,                 /* CA - Weighing Device */
    /* Reserved 0x02 - 0x1F */
    kHIDUsage_WD_ScaleScaleDevice = 0x20,               /* CL - Scale Device */
    kHIDUsage_WD_ScaleScaleClassIMetricCL = 0x21,       /* CL - Scale Class I Metric */
    kHIDUsage_WD_ScaleScaleClassIMetric = 0x22,         /* SEL - Scale Class I Metric */
    kHIDUsage_WD_ScaleScaleClassIIMetric = 0x23,        /* SEL - Scale Class II Metric */
    kHIDUsage_WD_ScaleScaleClassIIIMetric = 0x24,       /* SEL - Scale Class III Metric */
    kHIDUsage_WD_ScaleScaleClassIIILMetric = 0x25,      /* SEL - Scale Class IIIL Metric */
    kHIDUsage_WD_ScaleScaleClassIVMetric = 0x26,        /* SEL - Scale Class IV Metric */
    kHIDUsage_WD_ScaleScaleClassIIIEnglish = 0x27,      /* SEL - Scale Class III English */
    kHIDUsage_WD_ScaleScaleClassIIILEnglish = 0x28,     /* SEL - Scale Class IIIL English */
    kHIDUsage_WD_ScaleScaleClassIVEnglish = 0x29,       /* SEL - Scale Class IV English */
    kHIDUsage_WD_ScaleScaleClassGeneric = 0x2A,         /* SEL - Scale Class Generic */
    /* Reserved 0x2B - 0x2F */
    kHIDUsage_WD_ScaleAtrributeReport = 0x30,           /* CL - Scale Attribute Report */
    kHIDUsage_WD_ScaleControlReport = 0x31,             /* CL - Scale Control Report */
    kHIDUsage_WD_ScaleDataReport = 0x32,                /* CL - Scale Data Report */
    kHIDUsage_WD_ScaleStatusReport = 0x33,              /* CL - Scale Status Report */
    kHIDUsage_WD_ScaleWeightLimitReport = 0x34,         /* CL - Scale Weight Limit Report */
    kHIDUsage_WD_ScaleStatisticsReport = 0x35,          /* CL - Scale Statistics Report */
    /* Reserved 0x36 - 0x3F */
    kHIDUsage_WD_DataWeight = 0x40,                     /* DV - Data Weight */
    kHIDUsage_WD_DataScaling = 0x41,                    /* DV - Data Scaling */
    /* Reserved 0x42 - 0x4F */
    kHIDUsage_WD_WeightUnit = 0x50,                     /* CL - Weight Unit */
    kHIDUsage_WD_WeightUnitMilligram = 0x51,            /* SEL - Weight Unit Milligram */
    kHIDUsage_WD_WeightUnitGram = 0x52,                 /* SEL - Weight Unit Gram */
    kHIDUsage_WD_WeightUnitKilogram = 0x53,             /* SEL - Weight Unit Kilogram */
    kHIDUsage_WD_WeightUnitCarats = 0x54,               /* SEL - Weight Unit Carats */
    kHIDUsage_WD_WeightUnitTaels = 0x55,                /* SEL - Weight Unit Taels */
    kHIDUsage_WD_WeightUnitGrains = 0x56,               /* SEL - Weight Unit Grains */
    kHIDUsage_WD_WeightUnitPennyweights = 0x57,         /* SEL - Weight Unit Pennyweights */
    kHIDUsage_WD_WeightUnitMetricTon = 0x58,            /* SEL - Weight Unit Metric Ton */
    kHIDUsage_WD_WeightUnitAvoirTon = 0x59,             /* SEL - Weight Unit Avoir Ton */
    kHIDUsage_WD_WeightUnitTroyOunce = 0x5A,            /* SEL - Weight Unit Troy Ounce */
    kHIDUsage_WD_WeightUnitOunce = 0x5B,                /* SEL - Weight Unit Ounce */
    kHIDUsage_WD_WeightUnitPound = 0x5C,                /* SEL - Weight Unit Pound */
    /* Reserved 0x5D - 0x5F */
    kHIDUsage_WD_CalibrationCount = 0x60,               /* DV - Calibration Count */
    kHIDUsage_WD_RezeroCount = 0x61,                    /* DV - Re-Zero Count*/
    /* Reserved 0x62 - 0x6F */
    kHIDUsage_WD_ScaleStatus = 0x70,                    /* CL - Scale Status */
    kHIDUsage_WD_ScaleStatusFault = 0x71,               /* SEL - Scale Status Fault */
    kHIDUsage_WD_ScaleStatusStableAtZero = 0x72,        /* SEL - Scale Status Stable at Center of  Zero */
    kHIDUsage_WD_ScaleStatusInMotion = 0x73,            /* SEL - Scale Status In Motion */
    kHIDUsage_WD_ScaleStatusWeightStable = 0x74,        /* SEL - Scale Status Weight Stable */
    kHIDUsage_WD_ScaleStatusUnderZero = 0x75,           /* SEL - Scale Status Under Zero */
    kHIDUsage_WD_ScaleStatusOverWeightLimit = 0x76,     /* SEL - Scale Status Over Weight Limit */
    kHIDUsage_WD_ScaleStatusRequiresCalibration = 0x77, /* SEL - Scale Status Requires Calibration */
    kHIDUsage_WD_ScaleStatusRequiresRezeroing = 0x78,   /* SEL - Scale Status Requires Re-zeroing */
    /* Reserved 0x79 - 0x7F */
    kHIDUsage_WD_ZeroScale = 0x80,                      /* OOC - Zero Scale */
    kHIDUsage_WD_EnforcedZeroReturn = 0x81              /* OOC - Enforced Zero Return */
    /* Reserved 0x82 - 0xFFFF */
    
};

/* Magnetic Stripe Reader Page (0x8E) */
/* This section provides detailed descriptions of the usages employed by Magnetic Stripe Reader Devices. */
enum 
{
    kHIDUsage_MSR_Undefined = 0x00,                     /* MagStripe Undefined Usage */
    kHIDUsage_MSR_DeviceReadOnly = 0x01,                /* CA - MSR Device Read-Only */
    /* Reserved 0x02 - 0x10 */
    kHIDUsage_MSR_Track1Length = 0x11,                     /* SF, DF, SEL - Track 1 Length */
    kHIDUsage_MSR_Track2Length = 0x12,                     /* SF, DF, SEL - Track 2 Length */
    kHIDUsage_MSR_Track3Length = 0x13,                     /* SF, DF, SEL - Track 3 Length */
    kHIDUsage_MSR_TrackJISLength = 0x14,                   /* SF, DF, SEL - Track JIS Length */
    /* Reserved 0x15 - 0x1F */
    kHIDUsage_MSR_TrackData = 0x20,                        /* SF, DF, SEL - Track Data */
    kHIDUsage_MSR_Track1Data = 0x21,                       /* SF, DF, SEL - Track 1 Data */
    kHIDUsage_MSR_Track2Data = 0x22,                       /* SF, DF, SEL - Track 2 Data */
    kHIDUsage_MSR_Track3Data = 0x23,                       /* SF, DF, SEL - Track 3 Data */
    kHIDUsage_MSR_TrackJISData = 0x24                      /* SF, DF, SEL - Track JIS Data */
    /* Reserved 0x25 - 0xFFFF */
};

#endif /* _IOHIDUSAGETABLES_H */
