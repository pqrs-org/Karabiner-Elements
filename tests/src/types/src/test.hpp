#include <boost/ut.hpp>
#include <pqrs/json.hpp>

namespace {
using namespace boost::ut;
using namespace boost::ut::literals;

template <typename T>
void json_unmarshal_error_test(const nlohmann::json& json,
                               const std::string_view& what) {
  try {
    json.get<T>();
    expect(false);
  } catch (pqrs::json::unmarshal_error& ex) {
    expect(what == ex.what());
  } catch (...) {
    expect(false);
  }
}
} // namespace
