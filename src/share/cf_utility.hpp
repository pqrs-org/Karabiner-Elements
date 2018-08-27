#pragma once

// `krbn::cf_utility::run_loop_thread` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "logger.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <boost/optional.hpp>
#include <mutex>
#include <string>
#include <thread>

namespace krbn {
class cf_utility final {
public:
  template <typename T>
  class deleter final {
  public:
    using pointer = T;
    void operator()(T _Nullable ref) {
      if (ref) {
        CFRelease(ref);
      }
    }
  };

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

  // ========================================
  // CFRunLoop
  // ========================================

  /**
   * Create a thread and then run CFRunLoop on the thread.
   */
  class run_loop_thread final {
  public:
    run_loop_thread(void) : run_loop_(nullptr),
                            running_(false) {
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
                           kCFRunLoopDefaultMode);

        // Run

        CFRunLoopPerformBlock(run_loop_,
                              kCFRunLoopDefaultMode,
                              ^{
                                {
                                  std::lock_guard<std::mutex> lock(running_mutex_);

                                  running_ = true;
                                }
                                running_cv_.notify_one();
                              });

        CFRunLoopRun();

        // Remove source

        CFRunLoopRemoveSource(run_loop_,
                              source,
                              kCFRunLoopDefaultMode);

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

    void enqueue(void (^_Nonnull block)(void)) const {
      // Do not call `CFRunLoopPerformBlock` until `CFRunLoopRun` is called.
      // A segmentation fault occurs if we call `CFRunLoopPerformBlock` while `CFRunLoopRun' is processing.

      wait_until_running();

      CFRunLoopPerformBlock(run_loop_,
                            kCFRunLoopDefaultMode,
                            block);

      CFRunLoopWakeUp(run_loop_);
    }

  private:
    void wait_until_running(void) const {
      std::unique_lock<std::mutex> lock(running_mutex_);

      running_cv_.wait(lock, [this] {
        return running_;
      });
    }

    static void perform(void* _Nullable info) {
    }

    std::thread thread_;

    CFRunLoopRef _Nullable run_loop_;

    bool running_;
    mutable std::mutex running_mutex_;
    mutable std::condition_variable running_cv_;
  };
};
} // namespace krbn
