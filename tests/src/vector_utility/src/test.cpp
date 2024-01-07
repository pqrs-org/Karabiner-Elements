#include "vector_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;
  using namespace std::literals;

  "vector_utility::move_element"_test = [] {
    //
    // source_index == 0
    //

    // 0 -> 0
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 0, 0);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }
    // 0 -> 1
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 0, 1);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }
    // 0 -> 2
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 0, 2);
      expect(std::vector<int>({2, 1, 3, 4, 5}) == v);
    }
    // 0 -> 3
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 0, 3);
      expect(std::vector<int>({2, 3, 1, 4, 5}) == v);
    }
    // 0 -> 4
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 0, 4);
      expect(std::vector<int>({2, 3, 4, 1, 5}) == v);
    }
    // 0 -> 5
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 0, 5);
      expect(std::vector<int>({2, 3, 4, 5, 1}) == v);
    }
    // 0 -> 6
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 0, 6);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }
    // 0 -> 999
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 0, 999);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }

    //
    // source_index == 2
    //

    // 2 -> 0
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 2, 0);
      expect(std::vector<int>({3, 1, 2, 4, 5}) == v);
    }
    // 2 -> 1
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 2, 1);
      expect(std::vector<int>({1, 3, 2, 4, 5}) == v);
    }
    // 2 -> 2
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 2, 2);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }
    // 2 -> 3
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 2, 3);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }
    // 2 -> 4
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 2, 4);
      expect(std::vector<int>({1, 2, 4, 3, 5}) == v);
    }
    // 2 -> 5
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 2, 5);
      expect(std::vector<int>({1, 2, 4, 5, 3}) == v);
    }
    // 2 -> 6
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 2, 6);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }

    // 4 -> 0
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 4, 0);
      expect(std::vector<int>({5, 1, 2, 3, 4}) == v);
    }
    // 4 -> 5
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 4, 5);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }

    // 5 -> 0
    {
      std::vector<int> v{1, 2, 3, 4, 5};
      krbn::vector_utility::move_element(v, 5, 0);
      expect(std::vector<int>({1, 2, 3, 4, 5}) == v);
    }
  };

  return 0;
}
