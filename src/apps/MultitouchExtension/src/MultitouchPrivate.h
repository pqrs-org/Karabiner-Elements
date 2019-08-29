// -*- mode: objective-c -*-

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
  int unknown1;
  int unknown2;
  mtReadout normalized;
  float size;
  int unknown3;
  float angle;
  float majorAxis;
  float minorAxis;
  mtReadout unknown4;
  int unknown5[2];
  float unknown6;
} Finger;

typedef void* MTDeviceRef;
typedef int (*MTContactCallbackFunction)(MTDeviceRef, Finger*, int, double, int);

CFMutableArrayRef MTDeviceCreateList(void);
void MTRegisterContactFrameCallback(MTDeviceRef, MTContactCallbackFunction);
void MTUnregisterContactFrameCallback(MTDeviceRef, MTContactCallbackFunction);
void MTDeviceStart(MTDeviceRef, int);
void MTDeviceStop(MTDeviceRef, int);
