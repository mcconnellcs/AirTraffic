#include "doctest.h"
#include "app_settings.h"

TEST_CASE("parseCoordinate accepts numbers in range") {
  double v = 0;
  CHECK(settings::parseCoordinate("32.7763", -90, 90, &v));
  CHECK(v == doctest::Approx(32.7763));
  CHECK(settings::parseCoordinate("-79.93 ", -180, 180, &v));
  CHECK(v == doctest::Approx(-79.93));
}

TEST_CASE("parseCoordinate rejects junk and leaves the value alone") {
  double v = 1.5;
  CHECK_FALSE(settings::parseCoordinate("", -90, 90, &v));
  CHECK_FALSE(settings::parseCoordinate(nullptr, -90, 90, &v));
  CHECK_FALSE(settings::parseCoordinate("north", -90, 90, &v));
  CHECK_FALSE(settings::parseCoordinate("12abc", -90, 90, &v));
  CHECK_FALSE(settings::parseCoordinate("91", -90, 90, &v));
  CHECK_FALSE(settings::parseCoordinate("-181", -180, 180, &v));
  CHECK(v == doctest::Approx(1.5));
}

TEST_CASE("nextRange cycles through the range choices") {
  CHECK(settings::nextRange(10) == 25);
  CHECK(settings::nextRange(25) == 50);
  CHECK(settings::nextRange(50) == 100);
  CHECK(settings::nextRange(100) == 10);
  CHECK(settings::nextRange(33) == kDefaultRangeNm);
}
