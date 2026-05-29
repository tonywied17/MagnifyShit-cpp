#include <doctest.h>

#include "../src/util/Result.hpp"

using namespace magshit;

TEST_CASE("Result<int> holds value")
{
    Result<int> r{42};
    REQUIRE(r.ok());
    CHECK(static_cast<bool>(r));
    CHECK(r.value() == 42);
}

TEST_CASE("Result<int> holds error")
{
    Result<int> r{makeError("nope", -7)};
    REQUIRE_FALSE(r.ok());
    CHECK_FALSE(static_cast<bool>(r));
    CHECK(r.error().message == "nope");
    CHECK(r.error().code == -7);
}

TEST_CASE("Result<void> default-constructed is ok")
{
    Result<void> r;
    CHECK(r.ok());
}

TEST_CASE("Result<void> from error is not ok")
{
    Result<void> r{makeError("bad")};
    CHECK_FALSE(r.ok());
    CHECK(r.error().message == "bad");
}
