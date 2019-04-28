#include <catch2/catch.hpp>
#include <iostream>

#include "../../share/json_helper.hpp"
#include "manipulator/manipulators/mouse_motion_to_scroll/counter.hpp"

namespace mouse_motion_to_scroll = krbn::manipulator::manipulators::mouse_motion_to_scroll;

class counter_test final : pqrs::dispatcher::extra::dispatcher_client {
public:
  counter_test(std::shared_ptr<pqrs::dispatcher::pseudo_time_source> time_source,
               std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
               const krbn::core_configuration::details::complex_modifications_parameters& parameters,
               const mouse_motion_to_scroll::options& options) : dispatcher_client(weak_dispatcher),
                                                                 time_source_(time_source),
                                                                 counter_(weak_dispatcher,
                                                                          parameters,
                                                                          options),
                                                                 last_ms_(0) {
    result_ = nlohmann::json::array();

    counter_.scroll_event_arrived.connect([this](auto&& pointing_motion) {
      auto t = time_source_->now() - pqrs::dispatcher::time_point(pqrs::dispatcher::duration(0));
      auto json = nlohmann::json::object({
          {"time", t.count() - first_ms_},
          {"wh", pointing_motion.get_horizontal_wheel()},
          {"wv", pointing_motion.get_vertical_wheel()},
      });
#if 0
      std::cout << json << "," << std::endl;
#endif
      result_.push_back(json);
    });
  }

  ~counter_test(void) {
    detach_from_dispatcher();
  }

  const nlohmann::json get_result(void) const {
    return result_;
  }

  void update(int x, int y, pqrs::dispatcher::time_point time_point) {
    counter_.update(krbn::pointing_motion(x, y, 0, 0), time_point);
  }

  void set_now(int ms) {
    if (last_ms_ == 0) {
      first_ms_ = ms;
      last_ms_ = ms - 10;
    }

    while (last_ms_ < ms) {
      last_ms_ += 10;
#if 0
      std::cout << "now:" << last_ms_ << std::endl;
#endif
      auto now = pqrs::dispatcher::time_point(std::chrono::milliseconds(last_ms_));
      auto wait = pqrs::make_thread_wait();

      enqueue_to_dispatcher(
          [wait] {
            wait->notify();
          },
          now);

      time_source_->set_now(now);

      wait->wait_notice();
    }
  }

private:
  std::shared_ptr<pqrs::dispatcher::pseudo_time_source> time_source_;
  mouse_motion_to_scroll::counter counter_;
  int first_ms_;
  int last_ms_;
  nlohmann::json result_;
};

TEST_CASE("json/input") {
  {
    auto input_files_json = krbn::unit_testing::json_helper::load_jsonc("json/files.jsonc");
    for (const auto& file_json : input_files_json) {
      auto input_file_path = "json/" + file_json.at("input").get<std::string>();
      auto expected_file_path = "json/" + file_json.at("expected").get<std::string>();
      auto input_json = krbn::unit_testing::json_helper::load_jsonc(input_file_path);
      auto expected_json = krbn::unit_testing::json_helper::load_jsonc(expected_file_path);

      std::cout << input_file_path << std::endl;

      auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
      auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

      {
        krbn::core_configuration::details::complex_modifications_parameters parameters(input_json.at("parameters"));
        mouse_motion_to_scroll::options options;
        options.update(input_json.at("options"));
        counter_test counter_test(time_source,
                                  dispatcher,
                                  parameters,
                                  options);

        std::chrono::milliseconds first_time_stamp(0);
        std::chrono::milliseconds last_time_stamp(0);

        for (const auto& j : input_json.at("input")) {
          auto time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::nanoseconds(j.at("time_stamp").get<uint64_t>()));

          if (first_time_stamp == std::chrono::milliseconds(0)) {
            first_time_stamp = time_stamp;
          }
          last_time_stamp = time_stamp;

          auto x = j.at("pointing_motion").at("x").get<int>();
          auto y = j.at("pointing_motion").at("y").get<int>();

          counter_test.update(x,
                              y,
                              pqrs::dispatcher::time_point(time_stamp));
        }

#if 0
          std::cout << "t:" << (time_stamp - first_time_stamp).count()
                    << " x,y:" << x << "," << y << std::endl;
#endif

        counter_test.set_now(first_time_stamp.count());
        counter_test.set_now(last_time_stamp.count() + 1000);

        REQUIRE(counter_test.get_result() == expected_json);
      }

      dispatcher->terminate();
    }
  }
}
