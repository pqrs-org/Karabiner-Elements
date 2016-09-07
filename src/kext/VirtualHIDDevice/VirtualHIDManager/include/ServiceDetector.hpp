#pragma once

class ServiceDetector final {
public:
  ServiceDetector(const ServiceDetector&) = delete;

  ServiceDetector(void) : matchedNotifier_(nullptr),
                          terminatedNotifier_(nullptr),
                          service_(nullptr) {}

  void setNotifier(const char* serviceName) {
    unsetNotifier();

    matchedNotifier_ = addMatchingNotification(gIOMatchedNotification,
                                               serviceMatching(serviceName),
                                               matchedCallback,
                                               this, nullptr, 0);

    terminatedNotifier_ = addMatchingNotification(gIOTerminatedNotification,
                                                  serviceMatching(serviceName),
                                                  terminatedCallback,
                                                  this, nullptr, 0);
  }

  void unsetNotifier(void) {
    if (matchedNotifier_) {
      matchedNotifier_->remove();
      matchedNotifier_ = nullptr;
    }

    if (terminatedNotifier_) {
      terminatedNotifier_->remove();
      terminatedNotifier_ = nullptr;
    }

    if (service_) {
      service_->release();
      service_ = nullptr;
    }
  }

  IOService* getService(void) { return service_; }

private:
  static bool matchedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier) {
    auto self = static_cast<ServiceDetector*>(target);
    if (self) {
      if (newService) {
        auto name = newService->getName();
        if (name) {
          IOLog("org_pqrs_driver_VirtualHIDManager: %s is found.\n", name);
        }

        if (self->service_) {
          self->service_->release();
        }

        newService->retain();
        self->service_ = newService;
      }
    }
    return true;
  }

  static bool terminatedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier) {
    auto self = static_cast<ServiceDetector*>(target);
    if (self) {
      if (self->service_) {
        self->service_->release();
        self->service_ = nullptr;
      }
    }
    return true;
  }

  IONotifier* matchedNotifier_;
  IONotifier* terminatedNotifier_;
  IOService* service_;
};
