#pragma once

class ServiceDetector {
public:
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

    service_ = nullptr;
  }

  IOService* getService(void) { return service_; }

private:
  static bool matchedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier) {
    auto self = static_cast<ServiceDetector*>(target);
    if (self) {
      self->service_ = newService;
    }
    return true;
  }

  static bool terminatedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier) {
    auto self = static_cast<ServiceDetector*>(target);
    if (self) {
      self->service_ = nullptr;
    }
    return true;
  }

  IONotifier* matchedNotifier_;
  IONotifier* terminatedNotifier_;
  IOService* service_;
};
