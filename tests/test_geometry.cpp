#include <doctest.h>

#include "../src/util/Geometry.hpp"

using namespace magshit;

TEST_CASE("Rect helpers")
{
    Rect r{10, 20, 100, 50};
    CHECK(r.right()  == 110);
    CHECK(r.bottom() == 70);
    CHECK(r.center().x == 60);
    CHECK(r.center().y == 45);
}

TEST_CASE("clampToBounds fits inside")
{
    Rect bounds{0, 0, 1920, 1080};

    SUBCASE("fully inside is unchanged")
    {
        Rect r{100, 100, 200, 100};
        const Rect c = clampToBounds(r, bounds);
        CHECK(c.x == 100);
        CHECK(c.y == 100);
        CHECK(c.w == 200);
        CHECK(c.h == 100);
    }

    SUBCASE("shifted off the left edge gets pulled in")
    {
        Rect r{-50, 100, 200, 100};
        const Rect c = clampToBounds(r, bounds);
        CHECK(c.x == 0);
    }

    SUBCASE("shifted off the right edge gets pulled in")
    {
        Rect r{1900, 100, 200, 100};
        const Rect c = clampToBounds(r, bounds);
        CHECK(c.right() == bounds.right());
    }

    SUBCASE("oversized rect is clamped to bounds size")
    {
        Rect r{0, 0, 5000, 5000};
        const Rect c = clampToBounds(r, bounds);
        CHECK(c.w == bounds.w);
        CHECK(c.h == bounds.h);
    }
}

TEST_CASE("clampToBounds with non-zero origin")
{
    Rect bounds{1000, 500, 800, 600};
    Rect r{900, 400, 200, 200};
    const Rect c = clampToBounds(r, bounds);
    CHECK(c.x >= bounds.x);
    CHECK(c.y >= bounds.y);
    CHECK(c.right()  <= bounds.right());
    CHECK(c.bottom() <= bounds.bottom());
}
