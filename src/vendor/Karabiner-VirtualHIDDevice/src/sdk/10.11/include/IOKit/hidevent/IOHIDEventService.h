/*
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Copyright (c) 1999-2009 Apple Computer, Inc.  All Rights Reserved.
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

#ifndef _IOKIT_HID_IOHIDEVENTSERVICE_H
#define _IOKIT_HID_IOHIDEVENTSERVICE_H

#include <TargetConditionals.h>

#include <IOKit/IOService.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/hidsystem/IOHIDTypes.h>
#include <IOKit/hid/IOHIDInterface.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDKeys.h>

#if TARGET_OS_EMBEDDED
    #include <IOKit/hid/IOHIDEvent.h>
#endif

enum 
{
    kHIDDispatchOptionPointerNoAcceleration         = 0x01,
    kHIDDispatchOptionPointerDisplayIntegrated      = 0x02,
    kHIDDispatchOptionPointerAffixToScreen          = kHIDDispatchOptionPointerDisplayIntegrated,
    kHIDDispatchOptionPointerAbsolutToRelative      = 0x04
};

enum
{
    kHIDDispatchOptionScrollNoAcceleration              = (1<<0),

    kHIDDispatchOptionScrollMomentumAny                 = 0x00e,
    kHIDDispatchOptionScrollMomentumContinue            = (1<<1),
    kHIDDispatchOptionScrollMomentumStart               = (1<<2),
    kHIDDispatchOptionScrollMomentumEnd                 = (1<<3),
    
    kHIDDispatchOptionPhaseAny                          = 0xff0,
    kHIDDispatchOptionPhaseBegan                        = (1<<4),
    kHIDDispatchOptionPhaseChanged                      = (1<<5),
    kHIDDispatchOptionPhaseEnded                        = (1<<6),
    kHIDDispatchOptionPhaseCanceled                     = (1<<7),
    kHIDDispatchOptionPhaseMayBegin                     = (1<<11),
    
    kHIDDispatchOptionDeliveryNotificationForce         = (1<<30),
    kHIDDispatchOptionDeliveryNotificationSuppress      = (1<<31)
};

enum 
{
    kHIDDispatchOptionKeyboardNoRepeat                  = (1<<0)
};

class   IOHIDPointing;
class   IOHIDKeyboard;
class   IOHIDConsumer;
struct  TransducerData;

/*! @class IOHIDEventService : public IOService
 @abstract
 @discussion
 */
class IOHIDEventService: public IOService
{
    OSDeclareAbstractStructors( IOHIDEventService )
    
    friend class IOHIDPointing;
    friend class IOHIDKeyboard;
    friend class IOHIDConsumer;
    friend class AppleEmbeddedKeyboard;
    friend class IOHIDEventServiceUserClient;

private:
    IOHIDKeyboard *         _keyboardNub;
    IOHIDPointing *         _pointingNub;
    IOHIDConsumer *         _consumerNub;

    IONotifier *            _publishNotify;
    IORecursiveLock *       _nubLock;
    
    void *                  _reserved0;
    
    bool                    _readyForInputReports;
    
    struct ExpansionData {
        IOService *             provider;
        IOWorkLoop *            workLoop;
        OSArray *               deviceUsagePairs;
        IOCommandGate *         commandGate;
        
#if TARGET_OS_EMBEDDED
        OSDictionary *          clientDict;
#endif

        struct {
            UInt32                  deviceID;
            bool                    range;
            bool                    touch;
            SInt32                  x;
            SInt32                  y;
            SInt32                  z;
        } digitizer;

        struct {
            struct {
                UInt32                  delayMS;
                IOTimerEventSource *    timer;
                UInt32                  state;
                IOOptionBits            options;
            } eject;
            struct {
                UInt32                  delayMS;
                IOTimerEventSource *    timer;
                UInt32                  state;
                IOOptionBits            options;
            } caps;
            
#if TARGET_OS_EMBEDDED
            struct {
                UInt32                  startMask;
                UInt32                  mask;
                UInt32                  nmiMask;
                UInt32                  nmiDelay;
                IOTimerEventSource *    nmiTimer;
            } debug;

            bool                    swapISO;
#endif
            bool                    appleVendorSupported;
        } keyboard;
        
        struct {
            IOFixed                 x;
            IOFixed                 y;
            IOFixed                 z;
            IOFixed                 rX;
            IOFixed                 rY;
            IOFixed                 rZ;
            UInt32                  buttonState;
            IOOptionBits            options;
            IOTimerEventSource *    timer;
        } multiAxis;
        
        struct {
            UInt32                  buttonState;
        } relativePointer;


    };
    ExpansionData *         _reserved;
    
    IOHIDPointing *         newPointingShim (
                                UInt32          buttonCount         = 1,
                                IOFixed         pointerResolution   = (400 << 16),
                                IOFixed         scrollResolution    = 0,
                                IOOptionBits    options             = 0 );
                                
    IOHIDKeyboard *         newKeyboardShim (
                                UInt32          supportedModifiers  = 0,
                                IOOptionBits    options             = 0 );

    IOHIDConsumer *         newConsumerShim ( IOOptionBits options = 0 );
    
    void                    parseSupportedElements ( 
                                OSArray *                   elementArray, 
                                UInt32                      bootProtocol );
                                
    void                    processTabletElement ( IOHIDElement * element );

    IOFixed                 determineResolution ( IOHIDElement * element );
                                    
    static bool             _publishMatchingNotificationHandler(void * target, void * ref, IOService * newService, IONotifier * notifier);

    void                    ejectTimerCallback(IOTimerEventSource *sender);

    void                    capsTimerCallback(IOTimerEventSource *sender);
    
#if TARGET_OS_EMBEDDED
    void                    debuggerTimerCallback(IOTimerEventSource *sender);

    void                    stackshotTimerCallback(IOTimerEventSource *sender);
#endif
    
    void                    multiAxisTimerCallback(IOTimerEventSource *sender);

    void                    calculateCapsLockDelay();
    
    void                    calculateStandardType();

protected:

    virtual void            free();
        
/*! @function handleOpen
    @abstract Handle a client open on the interface.
    @discussion This method is called by IOService::open() with the
    arbitration lock held, and must return true to accept the client open.
    This method will in turn call handleClientOpen() to qualify the client
    requesting the open.
    @param client The client object that requested the open.
    @param options Options passed to IOService::open().
    @param argument Argument passed to IOService::open().
    @result true to accept the client open, false otherwise. */

    virtual bool handleOpen(IOService *  client,
                            IOOptionBits options,
                            void *       argument);

/*! @function handleClose
    @abstract Handle a client close on the interface.
    @discussion This method is called by IOService::close() with the
    arbitration lock held. This method will in turn call handleClientClose()
    to notify interested subclasses about the client close. If this represents
    the last close, then the interface will also close the controller before
    this method returns. The controllerWillClose() method will be called before
    closing the controller. Subclasses should not override this method.
    @param client The client object that requested the close.
    @param options Options passed to IOService::close(). */

    virtual void handleClose(IOService * client, IOOptionBits options);

/*! @function handleIsOpen
    @abstract Query whether a client has an open on the interface.
    @discussion This method is always called by IOService with the
    arbitration lock held. Subclasses should not override this method.
    @result true if the specified client, or any client if none (0) is
    specified, presently has an open on this object. */

    virtual bool handleIsOpen(const IOService * client) const;

/*! @function handleStart
    @abstract Prepare the hardware and driver to support I/O operations.
    @discussion IOHIDEventService will call this method from start() before
    any I/O operations are issued to the concrete subclass. Methods
    such as getReportElements() are only called after handleStart()
    has returned true. A subclass that overrides this method should
    begin its implementation by calling the version in super, and
    then check the return value.
    @param provider The provider argument passed to start().
    @result True on success, or false otherwise. Returning false will
    cause start() to fail and return false. */

    virtual bool            handleStart( IOService * provider );

/*! @function handleStop
    @abstract Quiesce the hardware and stop the driver.
    @discussion IOHIDEventService will call this method from stop() to
    signal that the hardware should be quiesced and the driver stopped.
    A subclass that overrides this method should end its implementation
    by calling the version in super.
    @param provider The provider argument passed to stop(). */

    virtual void            handleStop(  IOService * provider );
    
    virtual OSString *      getTransport ();
    virtual UInt32          getLocationID ();
    virtual UInt32          getVendorID ();
    virtual UInt32          getVendorIDSource ();
    virtual UInt32          getProductID ();
    virtual UInt32          getVersion ();
    virtual UInt32          getCountryCode ();
    virtual OSString *      getManufacturer ();
    virtual OSString *      getProduct ();
    virtual OSString *      getSerialNumber ();
    
    virtual OSArray *       getReportElements();

    virtual void            setElementValue (
                                UInt32                      usagePage,
                                UInt32                      usage,
                                UInt32                      value );
    
    virtual UInt32          getElementValue ( 
                                UInt32                      usagePage,
                                UInt32                      usage );
                                
    virtual void            dispatchKeyboardEvent(
                                AbsoluteTime                timeStamp,
                                UInt32                      usagePage,
                                UInt32                      usage,
                                UInt32                      value,
                                IOOptionBits                options = 0 );

    virtual void            dispatchRelativePointerEvent(
                                AbsoluteTime                timeStamp,
                                SInt32                      dx,
                                SInt32                      dy,
                                UInt32                      buttonState,
                                IOOptionBits                options = 0 );
    
    virtual void            dispatchAbsolutePointerEvent(
                                AbsoluteTime                timeStamp,
                                SInt32                      x,
                                SInt32                      y,
                                IOGBounds *                 bounds,
                                UInt32                      buttonState,
                                bool                        inRange,
                                SInt32                      tipPressure,
                                SInt32                      tipPressureMin,
                                SInt32                      tipPressureMax,
                                IOOptionBits                options = 0 );

    virtual void            dispatchScrollWheelEvent(
                                AbsoluteTime                timeStamp,
                                SInt32                      deltaAxis1,
                                SInt32                      deltaAxis2,
                                SInt32                      deltaAxis3,
                                IOOptionBits                options = 0 );

    virtual void            dispatchTabletPointerEvent(
                                AbsoluteTime                timeStamp,
                                UInt32                      transducerID,
                                SInt32                      x,
                                SInt32                      y,
                                SInt32                      z,
                                IOGBounds *                 bounds,
                                UInt32                      buttonState,
                                SInt32                      tipPressure,
                                SInt32                      tipPressureMin,
                                SInt32                      tipPressureMax,
                                SInt32                      barrelPressure,
                                SInt32                      barrelPressureMin,
                                SInt32                      barrelPressureMax,
                                SInt32                      tiltX,
                                SInt32                      tiltY,
                                UInt32                      twist,
                                IOOptionBits                options = 0 );

    virtual void            dispatchTabletProximityEvent(
                                AbsoluteTime                timeStamp,
                                UInt32                      transducerID,
                                bool                        inRange,
                                bool                        invert,
                                UInt32                      vendorTransducerUniqueID        = 0,
                                UInt32                      vendorTransducerSerialNumber    = 0,
                                IOOptionBits                options                         = 0 );

public:
    bool                    readyForReports();

    virtual bool            init(OSDictionary * properties = 0);

    virtual bool            start( IOService * provider );
    
    virtual void            stop( IOService * provider );

    virtual bool            matchPropertyTable(OSDictionary * table, SInt32 * score);    
    
    virtual IOReturn        setSystemProperties( OSDictionary * properties );
    
    virtual IOReturn        setProperties( OSObject * properties );
    
protected:
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  0);
    virtual OSArray *       getDeviceUsagePairs();
    
        
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  1);
    virtual UInt32          getReportInterval();

    OSMetaClassDeclareReservedUsed(IOHIDEventService,  2);
    enum {
        kMultiAxisOptionRotationForTranslation  = (1<<0),
        kMultiAxisOptionZForScroll              = (1<<1)
    };
/*!
    @function dispatchMultiAxisPointerEvent
    @abstract Dispatch multi-axis pointer event
    @discussion This is meant to be used with joysticks or multi-axis pointer devices such as those with
                with 6 degrees of freedom.  This function will generate related relative pointer and scroll
                event associated with movement.
    @param timeStamp    AbsoluteTime representing origination of event
    @param buttonState  Button mask where bit0 is the primary button, bit1 secondary and so forth
    @param x            Absolute location of pointer along the x-axis from -1.0 to 1.0 in 16:16 fixed point.
    @param y            Absolute location of pointer along the y-axis from -1.0 to 1.0 in 16:16 fixed point.
    @param z            Absolute location of pointer along the z-axis from -1.0 to 1.0 in 16:16 fixed point.
    @param rX           Absolute rotation of pointer around the x-axis from -1.0 to 1.0 in 16:16 fixed point.
    @param rY           Absolute rotation of pointer around the y-axis from -1.0 to 1.0 in 16:16 fixed point.
    @param rZ           Absolute rotation of pointer around the z-axis from -1.0 to 1.0 in 16:16 fixed point.
    @param options      Additional options to be used when dispatching event such as leveraging rotational
                        axis for translation or using the z axis for vertical scrolling.
*/
    virtual void            dispatchMultiAxisPointerEvent(
                                AbsoluteTime                timeStamp,
                                UInt32                      buttonState,
                                IOFixed                     x,
                                IOFixed                     y,
                                IOFixed                     z,
                                IOFixed                     rX      = 0,
                                IOFixed                     rY      = 0,
                                IOFixed                     rZ      = 0,
                                IOOptionBits                options = 0 );

    enum {
        kDigitizerInvert = (1<<0),
        
        kDigitizerCapabilityButtons             = (1<<16),
        kDigitizerCapabilityPressure            = (1<<16),
        kDigitizerCapabilityTangentialPressure  = (1<<16),
        kDigitizerCapabilityZ                   = (1<<16),
        kDigitizerCapabilityTiltX               = (1<<16),
        kDigitizerCapabilityTiltY               = (1<<16),
        kDigitizerCapabilityTwist               = (1<<16),
    };
    
    enum {
        kDigitizerTransducerTypeStylus = 0,
        kDigitizerTransducerTypePuck,
        kDigitizerTransducerTypeFinger,
        kDigitizerTransducerTypeHand
    };
    typedef UInt32 DigitizerTransducerType;
    
/*!
    @function dispatchDigitizerEvent
    @abstract Dispatch tablet events without orientation
    @discussion This is meant to be used with transducers without any orientation.
    @param timeStamp    AbsoluteTime representing origination of event
    @param ID           ID of the transducer generating the event
    @param type         Type of the transducer generating the event
    @param inRange      Details whether the transducer is in promitity to digitizer surface
    @param buttonState  Button mask where bit0 is the primary button, bit1 secondary and so forth
    @param x            Absolute location of transducer along the x-axis from 0.0 to 1.0 in
                        16:16 fixed point.
    @param y            Absolute location of transducer along the y-axis from 0.0 to 1.0 in
                        16:16 fixed point.
    @param z            Absolute location of transducer along the z-axis from 0.0 to 1.0 in
                        16:16 fixed point. This is typically used to determine the distance
                        between the transducer and surface
    @param tipPressure  Absolute pressure exerted on surface by tip from 0.0 to 1.0 in 16:16
                        fixed point.
    @param auxPressure  Absolute pressure exerted on transducer from 0.0 to 1.0 in 16:16 fixed point.
    @param options      Additional options to be used when dispatching event.
*/
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  3);
    virtual void            dispatchDigitizerEvent(
                                AbsoluteTime                    timeStamp,
                                UInt32                          transducerID,
                                DigitizerTransducerType         type,
                                bool                            inRange,
                                UInt32                          buttonState,
                                IOFixed                         x,
                                IOFixed                         y,
                                IOFixed                         z               = 0,
                                IOFixed                         tipPressure     = 0,
                                IOFixed                         auxPressure     = 0,
                                IOFixed                         twist           = 0,
                                IOOptionBits                    options         = 0 );

/*! 
    @function dispatchDigitizerEventWithTiltOrientation
    @abstract Dispatch tablet events with tilt orientation
    @discussion This is meant to be used with transducers that leverage tilt orientation
    @param timeStamp    AbsoluteTime representing origination of event
    @param ID           ID of the transducer generating the event 
    @param type         Type of the transducer generating the event
    @param inRange      Details whether the transducer is in promitity to digitizer surface
    @param buttonState  Button mask where bit0 is the primary button, bit1 secondary and so forth
    @param x            Absolute location of transducer along the x-axis from 0.0 to 1.0 in 
                        16:16 fixed point.
    @param y            Absolute location of transducer along the y-axis from 0.0 to 1.0 in
                        16:16 fixed point.
    @param z            Absolute location of transducer along the z-axis from 0.0 to 1.0 in
                        16:16 fixed point. This is typically used to determine the distance 
                        between the transducer and surface
    @param tipPressure  Absolute pressure exerted on surface by tip from 0.0 to 1.0 in 16:16 
                        fixed point.
    @param auxPressure  Absolute pressure exerted on transducer from 0.0 to 1.0 in 16:16 fixed point.
    @param twist        Absolute clockwise rotation along the transducer's major axis from 0.0 to 
                        360.0 degrees in 16:16 fixed point.
    @param tiltX        Absolute plane angle between the Y-Z plane and the plane containing the
                        transducer axis and the Y axis.  A positive X tilt is to the right.  Value is
                        represented in degrees from -90.0 to 90.0 in 16:16 fixed point.
    @param tiltY        Absolute plane angle between the X-Z plane and the plane containing the
                        transducer axis and the X axis.  A positive Y tilt is towards the user.  Value
                        is represented in degrees from -90.0 to 90.0 in 16:16 fixed point.
    @param options      Additional options to be used when dispatching event. 
*/
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  4);
    virtual void            dispatchDigitizerEventWithTiltOrientation(
                                AbsoluteTime                    timeStamp,
                                UInt32                          ID,
                                DigitizerTransducerType         type,
                                bool                            inRange,
                                UInt32                          buttonState,
                                IOFixed                         x,
                                IOFixed                         y,
                                IOFixed                         z               = 0,
                                IOFixed                         tipPressure     = 0, // 0.0-1.0 in 16:16 fixed
                                IOFixed                         auxPressure     = 0, // 0.0-1.0 in 16:16 fixed
                                IOFixed                         twist           = 0,
                                IOFixed                         tiltX           = 0,
                                IOFixed                         tiltY           = 0,
                                IOOptionBits                    options         = 0 );

/*!
    @function dispatchDigitizerEventWithPolarOrientation
    @abstract Dispatch tablet events with polar orientation
    @discussion This is meant to be used with transducers that leverage polar orientation
    @param timeStamp    AbsoluteTime representing origination of event
    @param ID           ID of the transducer generating the event
    @param type         Type of the transducer generating the event
    @param inRange      Details whether the transducer is in promitity to digitizer surface
    @param buttonState  Button mask where bit0 is the primary button, bit1 secondary and so forth
    @param x            Absolute location of transducer along the x-axis from 0.0 to 1.0 in
                        16:16 fixed point.
    @param y            Absolute location of transducer along the y-axis from 0.0 to 1.0 in
                        16:16 fixed point.
    @param z            Absolute location of transducer along the z-axis from 0.0 to 1.0 in
                        16:16 fixed point. This is typically used to determine the distance
                        between the transducer and surface
    @param tipPressure  Absolute pressure exerted on surface by tip from 0.0 to 1.0 in 16:16
                        fixed point.
    @param auxPressure  Absolute pressure exerted on transducer from 0.0 to 1.0 in 16:16 fixed point.
    @param twist        Absolute clockwise rotation along the transducer's major axis from 0.0 to
                        360.0 degrees in 16:16 fixed point.
    @param altitude     Specifies angle with the X-Y plane thorugh a signed, semicircular range.
                        Positive values specify an angle downward and toward the positive Z axis.
                        Value is represented in degrees from -180.0 to 180.0 in 16:16 fixed point.
    @param azimuth      Counter clockwise rotation of the cursor around the Z-axis through a full
                        circular range.  Value is represented in degrees from 0.0 to 360.0 in 16:16 
                        fixed point.
    @param options      Additional options to be used when dispatching event.
*/
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  5);
    virtual void            dispatchDigitizerEventWithPolarOrientation(
                                AbsoluteTime                    timeStamp,
                                UInt32                          transducerID,
                                DigitizerTransducerType         type,
                                bool                            inRange,
                                UInt32                          buttonState,
                                IOFixed                         x,
                                IOFixed                         y,
                                IOFixed                         z               = 0,
                                IOFixed                         tipPressure     = 0, // 0.0-1.0 in 16:16 fixed
                                IOFixed                         tanPressure     = 0, // 0.0-1.0 in 16:16 fixed
                                IOFixed                         twist           = 0,
                                IOFixed                         altitude        = 0,
                                IOFixed                         azimuth         = 0,
                                IOOptionBits                    options         = 0 );
    
    /*!
     @function dispatchUnicodeEvent
     @abstract Dispatch unicode events
     @discussion The HID specificiation provides a means to dispatch unicode characters from HID
     compliant devices.  The original method was to leverage the unicode page to deliver UTF-16 LE
     characters by way of a usage page selector.
     @param timeStamp   AbsoluteTime representing origination of event
     @param length      Length of unicode payload
     @param payload     character payload
     @param quality     A fixed point value from 0.0 to 1.0 that represents that quality/confidence of the event.
     @param options     Additional options to be used when dispatching event.
     */
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  6);
    enum {
        kUnicodeEncodingTypeUTF8    = 0,
        kUnicodeEncodingTypeUTF16LE,
        kUnicodeEncodingTypeUTF16BE,
        kUnicodeEncodingTypeUTF32LE,
        kUnicodeEncodingTypeUTF32BE,
    };
    typedef UInt32 UnicodeEncodingType;

    virtual void            dispatchUnicodeEvent(
                                                 AbsoluteTime               timeStamp,
                                                 UInt8 *                    payload,
                                                 UInt32                     length,
                                                 UnicodeEncodingType        encoding    = kUnicodeEncodingTypeUTF16LE,
                                                 IOFixed                    quality     = (1<<16),
                                                 IOOptionBits               options     = 0);

    
private:
    enum {
        kDigitizerOrientationTypeTilt = 0,
        kDigitizerOrientationTypePolar,
        kDigitizerOrientationTypeQuality
    };
    typedef UInt32 DigitizerOrientationType;
    
    void            dispatchDigitizerEventWithOrientation(
                                AbsoluteTime                    timeStamp,
                                UInt32                          transducerID,
                                DigitizerTransducerType         type,
                                bool                            inRange,
                                UInt32                          buttonState,
                                IOFixed                         x,
                                IOFixed                         y,
                                IOFixed                         z                       = 0,
                                IOFixed                         tipPressure             = 0,
                                IOFixed                         auxPressure             = 0,
                                IOFixed                         twist                   = 0,
                                DigitizerOrientationType        orientationType         = kDigitizerOrientationTypeTilt,
                                IOFixed *                       orientationParams       = NULL,
                                UInt32                          orientationParamCount   = 0,
                                IOOptionBits                    options                 = 0 );
    



#if TARGET_OS_EMBEDDED
public:
    typedef void (*Action)(OSObject *target, OSObject * sender, void *context, OSObject *event, IOOptionBits options);

    OSMetaClassDeclareReservedUsed(IOHIDEventService,  7);
    virtual bool            open(
                                IOService *                 client,
                                IOOptionBits                options,
                                void *                      context,
                                Action                      action);
                                
protected:    
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  8);
    virtual void            dispatchEvent(IOHIDEvent * event, IOOptionBits options=0);

    OSMetaClassDeclareReservedUsed(IOHIDEventService,  9);
    virtual UInt32          getPrimaryUsagePage();
    
    OSMetaClassDeclareReservedUsed(IOHIDEventService, 10);
    virtual UInt32          getPrimaryUsage();
    
public:
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  11);
    virtual IOHIDEvent *    copyEvent(
                                IOHIDEventType              type, 
                                IOHIDEvent *                matching = 0,
                                IOOptionBits                options = 0);
    
protected:
    /*!
     @function dispatchStandardGameControllerEvent
     @abstract Dispatch standard game controller event
     @discussion This is meant to dispatch a conforming standard game controller event that includes the
     following: Direction Pad, Face Buttons, and Left and Right Shoulder Buttons.
     @param timeStamp   AbsoluteTime representing origination of event
     @param dpadUp      Direction pad up with a fixed value between 0.0 and 1.0
     @param dpadDown    Direction pad down with a fixed value between 0.0 and 1.0
     @param dpadLeft    Direction pad left with a fixed value between 0.0 and 1.0
     @param dpadRight   Direction pad right with a fixed value between 0.0 and 1.0
     @param faceX       Face button X with a fixed value between 0.0 and 1.0
     @param faceY       Face button Y with a fixed value between 0.0 and 1.0
     @param faceA       Face button A with a fixed value between 0.0 and 1.0
     @param faceB       Face button B with a fixed value between 0.0 and 1.0
     @param shoulderL   Left shoulder button with a fixed value between 0.0 and 1.0
     @param shoulderR   Right shoulder button with a fixed value between 0.0 and 1.0
     @param options     Additional options to be defined.
     */
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  12);
    virtual void            dispatchStandardGameControllerEvent(
                                                                AbsoluteTime                    timeStamp,
                                                                IOFixed                         dpadUp,
                                                                IOFixed                         dpadDown,
                                                                IOFixed                         dpadLeft,
                                                                IOFixed                         dpadRight,
                                                                IOFixed                         faceX,
                                                                IOFixed                         faceY,
                                                                IOFixed                         faceA,
                                                                IOFixed                         faceB,
                                                                IOFixed                         shoulderL,
                                                                IOFixed                         shoulderR,
                                                                IOOptionBits                    options         = 0 );
    
    /*!
     @function dispatchExtendedGameControllerEvent
     @abstract Dispatch extended game controller event
     @discussion This is meant to dispatch a conforming extended game controller event that includes the
     following: Direction Pad, Face Buttons, Left and Right Joysticks and 2 Left and 2 Right Shoulder Buttons.
     @param timeStamp   AbsoluteTime representing origination of event
     @param dpadUp      Direction pad up with a fixed value between 0.0 and 1.0
     @param dpadDown    Direction pad down with a fixed value between 0.0 and 1.0
     @param dpadLeft    Direction pad left with a fixed value between 0.0 and 1.0
     @param dpadRight   Direction pad right with a fixed value between 0.0 and 1.0
     @param faceX       Face button X with a fixed value between 0.0 and 1.0
     @param faceY       Face button Y with a fixed value between 0.0 and 1.0
     @param faceA       Face button A with a fixed value between 0.0 and 1.0
     @param faceB       Face button B with a fixed value between 0.0 and 1.0
     @param shoulderL1  Top left shoulder button with a fixed value between 0.0 and 1.0
     @param shoulderR1  Top right shoulder button with a fixed value between 0.0 and 1.0
     @param shoulderL2  Bottom left shoulder button with a fixed value between 0.0 and 1.0
     @param shoulderR2  Bottom right shoulder button with a fixed value between 0.0 and 1.0
     @param joystickX   Joystick X axis with a fixed value between -1.0 and 1.0
     @param joystickY   Joystick Y axis with a fixed value between -1.0 and 1.0
     @param joystickZ   Joystick Z axis with a fixed value between -1.0 and 1.0
     @param joystickRz  Joystick Rz axis with a fixed value between -1.0 and 1.0
     @param options     Additional options to be defined.
     */
    OSMetaClassDeclareReservedUsed(IOHIDEventService,  13);
    virtual void            dispatchExtendedGameControllerEvent(
                                                                AbsoluteTime                    timeStamp,
                                                                IOFixed                         dpadUp,
                                                                IOFixed                         dpadDown,
                                                                IOFixed                         dpadLeft,
                                                                IOFixed                         dpadRight,
                                                                IOFixed                         faceX,
                                                                IOFixed                         faceY,
                                                                IOFixed                         faceA,
                                                                IOFixed                         faceB,
                                                                IOFixed                         shoulderL1,
                                                                IOFixed                         shoulderR1,
                                                                IOFixed                         shoulderL2,
                                                                IOFixed                         shoulderR2,
                                                                IOFixed                         joystickX,
                                                                IOFixed                         joystickY,
                                                                IOFixed                         joystickZ,
                                                                IOFixed                         joystickRz,
                                                                IOOptionBits                    options         = 0 );
    

#else
    OSMetaClassDeclareReservedUnused(IOHIDEventService,  7);
    OSMetaClassDeclareReservedUnused(IOHIDEventService,  8);
    OSMetaClassDeclareReservedUnused(IOHIDEventService,  9);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 10);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 11);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 12);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 13);
#endif
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 14);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 15);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 16);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 17);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 18);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 19);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 20);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 21);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 22);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 23);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 24);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 25);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 26);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 27);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 28);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 29);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 30);
    OSMetaClassDeclareReservedUnused(IOHIDEventService, 31);
    
#if TARGET_OS_EMBEDDED
public:
    virtual void            close( IOService * forClient, IOOptionBits options = 0 );
    
private:
    bool                    openGated( IOService *client, IOOptionBits *pOptions, void *context, Action action);
    void                    closeGated( IOService * forClient, IOOptionBits *pOptions);
#endif

};

#endif /* !_IOKIT_HID_IOHIDEVENTSERVICE_H */
