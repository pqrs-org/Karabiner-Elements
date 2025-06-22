#include <AppKit/AppKit.h>

typedef struct {
  float x;
  float y;
} mtPoint;

typedef struct {
  mtPoint position;
  mtPoint velocity;
} mtReadout;

typedef struct {
  int frame;
  double timestamp;
  int identifier;
  int state;
  int fingerId;
  int handId;
  mtReadout normalized;
  float size;
  int pressure;
  float angle;
  float majorAxis;
  float minorAxis;
  mtReadout absoluteVector;
  int unknown1[2];
  float zDensity;
} Finger;

typedef struct CF_BRIDGED_TYPE(id) MTDevice *MTDeviceRef;
typedef int (*MTContactCallbackFunction)(MTDeviceRef, Finger *, int, double, int);

CFMutableArrayRef MTDeviceCreateList(void);
io_service_t MTDeviceGetService(MTDeviceRef);
void MTRegisterContactFrameCallback(MTDeviceRef, MTContactCallbackFunction);
void MTUnregisterContactFrameCallback(MTDeviceRef, MTContactCallbackFunction);
void MTDeviceStart(MTDeviceRef, int);
void MTDeviceStop(MTDeviceRef, int);
