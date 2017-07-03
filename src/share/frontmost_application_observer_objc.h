#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void krbn_frontmost_application_observer_objc;
typedef void (*krbn_frontmost_application_observer_callback)(const char* bundle_identifier,
                                                             const char* file_path,
                                                             void* context);
void krbn_frontmost_application_observer_initialize(krbn_frontmost_application_observer_objc** observer,
                                                    krbn_frontmost_application_observer_callback callback,
                                                    void* context);
void krbn_frontmost_application_observer_terminate(krbn_frontmost_application_observer_objc** observer);

#ifdef __cplusplus
}
#endif
