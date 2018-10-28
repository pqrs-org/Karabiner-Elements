#pragma once

// `krbn::cf_utility::run_loop_thread` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "dispatcher.hpp"
#include "logger.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <boost/optional.hpp>
#include <mutex>
#include <string>
#include <thread>

namespace krbn {
class cf_utility final {
public:
  // ========================================
  // Converts
  // ========================================

  static boost::optional<std::string> to_string(CFTypeRef _Nullable value) {
    if (!value) {
      return boost::none;
    }

    if (CFStringGetTypeID() != CFGetTypeID(value)) {
      return boost::none;
    }

    auto cfstring = static_cast<CFStringRef>(value);

    std::string string;
    if (auto p = CFStringGetCStringPtr(cfstring, kCFStringEncodingUTF8)) {
      string = p;
    } else {
      auto length = CFStringGetLength(cfstring);
      auto max_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
      char* buffer = new char[max_size];
      if (CFStringGetCString(cfstring, buffer, max_size, kCFStringEncodingUTF8)) {
        string = buffer;
      }
      delete[] buffer;
    }

    return string;
  }

  static CFStringRef _Nullable create_cfstring(const std::string& string) {
    return CFStringCreateWithCString(kCFAllocatorDefault,
                                     string.c_str(),
                                     kCFStringEncodingUTF8);
  }

  static boost::optional<int64_t> to_int64_t(CFTypeRef _Nullable value) {
    if (!value) {
      return boost::none;
    }

    if (CFNumberGetTypeID() != CFGetTypeID(value)) {
      return boost::none;
    }

    auto cfnumber = static_cast<CFNumberRef>(value);

    int64_t result;
    if (CFNumberGetValue(cfnumber, kCFNumberSInt64Type, &result)) {
      return result;
    }

    return boost::none;
  }

  // ========================================
  // CFArray, CFMutableArray
  // ========================================

  static CFArrayRef _Nonnull create_empty_cfarray(void) {
    return CFArrayCreate(nullptr, nullptr, 0, &kCFTypeArrayCallBacks);
  }

  static CFMutableArrayRef _Nonnull create_cfmutablearray(CFIndex capacity = 0) {
    return CFArrayCreateMutable(nullptr, capacity, &kCFTypeArrayCallBacks);
  }

  template <typename T>
  static T _Nullable get_value(CFArrayRef _Nonnull array, CFIndex index) {
    if (array && index < CFArrayGetCount(array)) {
      return static_cast<T>(const_cast<void*>(CFArrayGetValueAtIndex(array, index)));
    }
    return nullptr;
  }

  template <typename T>
  static bool exists(CFArrayRef _Nonnull array, T _Nonnull value) {
    if (array) {
      CFRange range = {0, CFArrayGetCount(array)};
      if (CFArrayContainsValue(array, range, value)) {
        return true;
      }
    }
    return false;
  }

  // ========================================
  // CFDictionary, CFMutableDictionary
  // ========================================

  static CFMutableDictionaryRef _Nonnull create_cfmutabledictionary(CFIndex capacity = 0) {
    return CFDictionaryCreateMutable(nullptr,
                                     capacity,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks);
  }

  static void set_cfmutabledictionary_value(CFMutableDictionaryRef _Nonnull dictionary,
                                            CFStringRef _Nonnull key,
                                            int64_t value) {
    if (auto number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &value)) {
      CFDictionarySetValue(dictionary, key, number);
      CFRelease(number);
    }
  }

  // ========================================
  // CFRunLoop
  // ========================================

  /**
   * Create a thread and then run CFRunLoop on the thread.
   */
  class run_loop_thread final {
  public:
    run_loop_thread(void) : run_loop_(nullptr) {
      running_wait_ = pqrs::dispatcher::make_wait();

      thread_ = std::thread([this] {
        run_loop_ = CFRunLoopGetCurrent();
        CFRetain(run_loop_);

        // Append empty source to prevent immediately quitting of `CFRunLoopRun`.

        auto context = CFRunLoopSourceContext();
        context.perform = perform;
        auto source = CFRunLoopSourceCreate(kCFAllocatorDefault,
                                            0,
                                            &context);

        CFRunLoopAddSource(run_loop_,
                           source,
                           kCFRunLoopCommonModes);

        // Run

        CFRunLoopPerformBlock(run_loop_,
                              kCFRunLoopCommonModes,
                              ^{
                                running_wait_->notify();
                              });

        CFRunLoopRun();

        // Remove source

        CFRunLoopRemoveSource(run_loop_,
                              source,
                              kCFRunLoopCommonModes);

        CFRelease(source);
      });
    }

    ~run_loop_thread(void) {
      if (thread_.joinable()) {
        logger::get_logger().error("Call `cf_utility::run_loop_thread::terminate` before destroy `cf_utility::run_loop_thread`");
        terminate();
      }

      if (run_loop_) {
        CFRelease(run_loop_);
      }
    }

    void terminate(void) {
      enqueue(^{
        CFRunLoopStop(run_loop_);
      });

      if (thread_.joinable()) {
        thread_.join();
      }
    }

    CFRunLoopRef _Nonnull get_run_loop(void) const {
      // We wait until running to avoid a segmentation fault which is described in `enqueue`.

      wait_until_running();

      return run_loop_;
    }

    void wake(void) const {
      // Do not touch run_loop_ until `CFRunLoopRun` is called.
      // A segmentation fault occurs if we touch `run_loop_` while `CFRunLoopRun' is processing.

      wait_until_running();

      CFRunLoopWakeUp(run_loop_);
    }

    void enqueue_and_wait(const std::function<void(void)>& function) const {
      auto wait = pqrs::dispatcher::make_wait();

      enqueue(^{
        function();

        wait->notify();
      });

      wait->wait_notice();
    }

  private:
    void enqueue(void (^_Nonnull block)(void)) const {
      // Do not call `CFRunLoopPerformBlock` until `CFRunLoopRun` is called.
      // A segmentation fault occurs if we call `CFRunLoopPerformBlock` while `CFRunLoopRun' is processing.

      wait_until_running();

      CFRunLoopPerformBlock(run_loop_,
                            kCFRunLoopCommonModes,
                            block);

      CFRunLoopWakeUp(run_loop_);
    }

    void wait_until_running(void) const {
      running_wait_->wait_notice();
    }

    static void perform(void* _Nullable info) {
    }

    std::thread thread_;

    CFRunLoopRef _Nullable run_loop_;
    std::shared_ptr<pqrs::dispatcher::wait> running_wait_;
  };
};
} // namespace krbn
