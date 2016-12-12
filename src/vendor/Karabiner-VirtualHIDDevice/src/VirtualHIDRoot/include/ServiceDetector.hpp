#pragma once

class ServiceDetector final {
public:
  typedef bool (*IsTargetServiceCallback)(IOService* service, IOService* refCon);

  ServiceDetector(const ServiceDetector&) = delete;

  ServiceDetector(void) : matchedNotifier_(nullptr),
                          terminatedNotifier_(nullptr),
                          isTargetServiceCallback_(nullptr),
                          isTargetServiceCallbackRefCon_(nullptr) {
    services_ = OSArray::withCapacity(8);
  }

  ~ServiceDetector(void) {
    unsetNotifier();
    OSSafeReleaseNULL(services_);
  }

  void setIsTargetServiceCallback(IsTargetServiceCallback isTargetServiceCallback, IOService* isTargetServiceCallbackRefCon) {
    isTargetServiceCallback_ = isTargetServiceCallback;
    isTargetServiceCallbackRefCon_ = isTargetServiceCallbackRefCon;

    if (isTargetServiceCallbackRefCon_) {
      isTargetServiceCallbackRefCon_->retain();
    }
  }

  void unsetIsTargetServiceCallback(void) {
    if (isTargetServiceCallbackRefCon_) {
      isTargetServiceCallbackRefCon_->release();
      isTargetServiceCallbackRefCon_ = nullptr;
    }

    isTargetServiceCallback_ = nullptr;
  }

  void setNotifier(const char* serviceName) {
    unsetNotifier();

    matchedNotifier_ = addMatchingNotification(gIOMatchedNotification,
                                               serviceMatching(serviceName),
                                               staticMatchedCallback,
                                               this, nullptr, 0);

    terminatedNotifier_ = addMatchingNotification(gIOTerminatedNotification,
                                                  serviceMatching(serviceName),
                                                  staticTerminatedCallback,
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

    if (services_) {
      services_->flushCollection();
    }
  }

  IOService* getService(void) {
    if (services_ && services_->getCount() > 0) {
      return OSDynamicCast(IOService, services_->getObject(0));
    }
    return nullptr;
  }

private:
  static bool staticMatchedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier) {
    auto self = static_cast<ServiceDetector*>(target);
    if (self) {
      return self->matchedCallback(refCon, newService, notifier);
    }
    return false;
  }

  bool matchedCallback(void* refCon, IOService* newService, IONotifier* notifier) {
    if (!newService) {
      return false;
    }

    bool isTargetService = true;

    if (isTargetServiceCallback_) {
      isTargetService = isTargetServiceCallback_(newService, isTargetServiceCallbackRefCon_);
    }

    if (isTargetService) {
      if (services_) {
        auto index = services_->getNextIndexOfObject(newService, 0);
        if (index == static_cast<unsigned int>(-1)) {
          services_->setObject(newService);
        }
      }
    }

    return true;
  }

  static bool staticTerminatedCallback(void* target, void* refCon, IOService* newService, IONotifier* notifier) {
    auto self = static_cast<ServiceDetector*>(target);
    if (self) {
      return self->terminatedCallback(refCon, newService, notifier);
    }

    return false;
  }

  bool terminatedCallback(void* refCon, IOService* newService, IONotifier* notifier) {
    if (!newService) {
      return false;
    }

    if (services_) {
      auto index = services_->getNextIndexOfObject(newService, 0);
      if (index != static_cast<unsigned int>(-1)) {
        services_->removeObject(index);
      }
    }

    return true;
  }

  OSArray* services_;
  IONotifier* matchedNotifier_;
  IONotifier* terminatedNotifier_;

  IsTargetServiceCallback isTargetServiceCallback_;
  IOService* isTargetServiceCallbackRefCon_;
};
