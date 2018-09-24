#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void krbn_frontmost_application_monitor_objc;
typedef void (*krbn_frontmost_application_monitor_callback)(const char* bundle_identifier,
                                                            const char* file_path,
                                                            void* context);
void krbn_frontmost_application_monitor_initialize(krbn_frontmost_application_monitor_objc** monitor,
                                                   krbn_frontmost_application_monitor_callback callback,
                                                   void* context);
void krbn_frontmost_application_monitor_terminate(krbn_frontmost_application_monitor_objc** monitor);

#ifdef __cplusplus
}
#endif
