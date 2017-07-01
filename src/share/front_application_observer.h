#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*krbn_front_application_observer_callback)(const char* bundle_identifier,
                                                         const char* file_path);
void krbn_front_application_observer_initialize(krbn_front_application_observer_callback callback);
void krbn_front_application_observer_terminate(void);

#ifdef __cplusplus
}
#endif
