<a id="top"></a>
# Test fixtures

## Defining test fixtures

Although Catch allows you to group tests together as sections within a test case, it can still be convenient, sometimes, to group them using a more traditional test fixture. Catch fully supports this too. You define the test fixture as a simple structure:

```c++
class UniqueTestsFixture {
  private:
   static int uniqueID;
  protected:
   DBConnection conn;
  public:
   UniqueTestsFixture() : conn(DBConnection::createConnection("myDB")) {
   }
  protected:
   int getID() {
     return ++uniqueID;
   }
 };

 int UniqueTestsFixture::uniqueID = 0;

 TEST_CASE_METHOD(UniqueTestsFixture, "Create Employee/No Name", "[create]") {
   REQUIRE_THROWS(conn.executeSQL("INSERT INTO employee (id, name) VALUES (?, ?)", getID(), ""));
 }
 TEST_CASE_METHOD(UniqueTestsFixture, "Create Employee/Normal", "[create]") {
   REQUIRE(conn.executeSQL("INSERT INTO employee (id, name) VALUES (?, ?)", getID(), "Joe Bloggs"));
 }
```

The two test cases here will create uniquely-named derived classes of UniqueTestsFixture and thus can access the `getID()` protected method and `conn` member variables. This ensures that both the test cases are able to create a DBConnection using the same method (DRY principle) and that any ID's created are unique such that the order that tests are executed does not matter.


Catch2 also provides `TEMPLATE_TEST_CASE_METHOD` and
`TEMPLATE_PRODUCT_TEST_CASE_METHOD` that can be used together
with templated fixtures and templated template fixtures to perform
tests for multiple different types. Unlike `TEST_CASE_METHOD`,
`TEMPLATE_TEST_CASE_METHOD` and `TEMPLATE_PRODUCT_TEST_CASE_METHOD` do
require the tag specification to be non-empty, as it is followed by
further macro arguments.

Also note that, because of limitations of the C++ preprocessor, if you
want to specify a type with multiple template parameters, you need to
enclose it in parentheses, e.g. `std::map<int, std::string>` needs to be
passed as `(std::map<int, std::string>)`.
In the case of `TEMPLATE_PRODUCT_TEST_CASE_METHOD`, if a member of the
type list should consist of more than single type, it needs to be enclosed
in another pair of parentheses, e.g. `(std::map, std::pair)` and
`((int, float), (char, double))`.

Example:
```cpp
template< typename T >
struct Template_Fixture {
    Template_Fixture(): m_a(1) {}

    T m_a;
};

TEMPLATE_TEST_CASE_METHOD(Template_Fixture,"A TEMPLATE_TEST_CASE_METHOD based test run that succeeds", "[class][template]", int, float, double) {
    REQUIRE( Template_Fixture<TestType>::m_a == 1 );
}

template<typename T>
struct Template_Template_Fixture {
    Template_Template_Fixture() {}

    T m_a;
};

template<typename T>
struct Foo_class {
    size_t size() {
        return 0;
    }
};

TEMPLATE_PRODUCT_TEST_CASE_METHOD(Template_Template_Fixture, "A TEMPLATE_PRODUCT_TEST_CASE_METHOD based test succeeds", "[class][template]", (Foo_class, std::vector), int) {
    REQUIRE( Template_Template_Fixture<TestType>::m_a.size() == 0 );
}
```

_While there is an upper limit on the number of types you can specify
in single `TEMPLATE_TEST_CASE_METHOD` or `TEMPLATE_PRODUCT_TEST_CASE_METHOD`,
the limit is very high and should not be encountered in practice._

## Signature-based parametrised test fixtures

> [Introduced](https://github.com/catchorg/Catch2/issues/1609) in Catch 2.8.0.

Catch2 also provides `TEMPLATE_TEST_CASE_METHOD_SIG` and `TEMPLATE_PRODUCT_TEST_CASE_METHOD_SIG` to support
fixtures using non-type template parameters. These test cases work similar to `TEMPLATE_TEST_CASE_METHOD` and `TEMPLATE_PRODUCT_TEST_CASE_METHOD`,
with additional positional argument for [signature](test-cases-and-sections.md#signature-based-parametrised-test-cases).

Example:
```cpp
template <int V>
struct Nttp_Fixture{
    int value = V;
};

TEMPLATE_TEST_CASE_METHOD_SIG(Nttp_Fixture, "A TEMPLATE_TEST_CASE_METHOD_SIG based test run that succeeds", "[class][template][nttp]",((int V), V), 1, 3, 6) {
    REQUIRE(Nttp_Fixture<V>::value > 0);
}

template<typename T>
struct Template_Fixture_2 {
    Template_Fixture_2() {}

    T m_a;
};

template< typename T, size_t V>
struct Template_Foo_2 {
    size_t size() { return V; }
};

TEMPLATE_PRODUCT_TEST_CASE_METHOD_SIG(Template_Fixture_2, "A TEMPLATE_PRODUCT_TEST_CASE_METHOD_SIG based test run that succeeds", "[class][template][product][nttp]", ((typename T, size_t S), T, S),(std::array, Template_Foo_2), ((int,2), (float,6)))
{
    REQUIRE(Template_Fixture_2<TestType>{}.m_a.size() >= 2);
}
```

## Template fixtures with types specified in template type lists

Catch2 also provides `TEMPLATE_LIST_TEST_CASE_METHOD` to support template fixtures with types specified in
template type lists like `std::tuple`, `boost::mpl::list` or `boost::mp11::mp_list`. This test case works the same as `TEMPLATE_TEST_CASE_METHOD`,
only difference is the source of types. This allows you to reuse the template type list in multiple test cases.

Example:
```cpp
using MyTypes = std::tuple<int, char, double>;
TEMPLATE_LIST_TEST_CASE_METHOD(Template_Fixture, "Template test case method with test types specified inside std::tuple", "[class][template][list]", MyTypes)
{
    REQUIRE( Template_Fixture<TestType>::m_a == 1 );
}
```

---

[Home](Readme.md#top)
