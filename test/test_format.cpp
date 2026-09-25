#include "doctest.h"
#include "format.h"

#include <string>

using fmt::Units;

static std::string s(const fmt::Label& label) { return label.c_str(); }

TEST_CASE("thousands adds commas") {
  CHECK(s(fmt::thousands(0)) == "0");
  CHECK(s(fmt::thousands(950)) == "950");
  CHECK(s(fmt::thousands(35000)) == "35,000");
  CHECK(s(fmt::thousands(1234567)) == "1,234,567");
  CHECK(s(fmt::thousands(-1200)) == "-1,200");
}

TEST_CASE("altitude text in aviation and metric units") {
  CHECK(s(fmt::altitude(35000, false, Units::Aviation)) == "35,000 ft");
  CHECK(s(fmt::altitude(35000, false, Units::Metric)) == "10,668 m");
  CHECK(s(fmt::altitude(0, true, Units::Aviation)) == "GROUND");
}

TEST_CASE("shortAltitude uses flight levels up high, hundreds of feet down low") {
  CHECK(s(fmt::shortAltitude(35000, false)) == "FL350");
  CHECK(s(fmt::shortAltitude(18000, false)) == "FL180");
  CHECK(s(fmt::shortAltitude(4550, false)) == "4,600");
  CHECK(s(fmt::shortAltitude(900, false)) == "900");
  CHECK(s(fmt::shortAltitude(0, true)) == "GND");
}

TEST_CASE("speed text") {
  CHECK(s(fmt::speed(444.9f, Units::Aviation)) == "445 kt");
  CHECK(s(fmt::speed(444.9f, Units::Metric)) == "824 km/h");
}

TEST_CASE("distance text keeps one decimal below 100") {
  CHECK(s(fmt::distance(6.748f, Units::Aviation)) == "6.7 nm");
  CHECK(s(fmt::distance(120.4f, Units::Aviation)) == "120 nm");
  CHECK(s(fmt::distance(10.0f, Units::Metric)) == "18.5 km");
}

TEST_CASE("vertical rate text") {
  CHECK(s(fmt::verticalRate(1216, Units::Aviation)) == "+1,216 fpm");
  CHECK(s(fmt::verticalRate(-640, Units::Aviation)) == "-640 fpm");
  CHECK(s(fmt::verticalRate(64, Units::Aviation)) == "LEVEL");
  CHECK(s(fmt::verticalRate(1000, Units::Metric)) == "+5.1 m/s");
}

TEST_CASE("heading text pads to three digits") {
  CHECK(s(fmt::heading(28.75f)) == "029\xC2\xB0");
  CHECK(s(fmt::heading(359.7f)) == "000\xC2\xB0");
  CHECK(s(fmt::heading(270)) == "270\xC2\xB0");
}

TEST_CASE("trimmed callsign removes the API's trailing spaces") {
  CHECK(s(fmt::trimmed("AAL1699 ")) == "AAL1699");
  CHECK(s(fmt::trimmed("  N21HL  ")) == "N21HL");
  CHECK(s(fmt::trimmed("")) == "");
  CHECK(s(fmt::trimmed(nullptr)) == "");
}

TEST_CASE("clock text in 24h and 12h") {
  CHECK(s(fmt::clock(9, 5, true)) == "09:05");
  CHECK(s(fmt::clock(21, 30, true)) == "21:30");
  CHECK(s(fmt::clock(21, 30, false)) == "9:30 PM");
  CHECK(s(fmt::clock(0, 1, false)) == "12:01 AM");
  CHECK(s(fmt::clock(12, 0, false)) == "12:00 PM");
}

TEST_CASE("altitudeBand groups altitudes for coloring") {
  CHECK(fmt::altitudeBand(0, true) == 0);
  CHECK(fmt::altitudeBand(500, false) == 1);
  CHECK(fmt::altitudeBand(3000, false) == 2);
  CHECK(fmt::altitudeBand(9000, false) == 3);
  CHECK(fmt::altitudeBand(20000, false) == 4);
  CHECK(fmt::altitudeBand(33000, false) == 5);
  CHECK(fmt::altitudeBand(45000, false) == 5);
}

TEST_CASE("verticalTrend ignores small wobbles") {
  CHECK(fmt::verticalTrend(1200) == 1);
  CHECK(fmt::verticalTrend(-800) == -1);
  CHECK(fmt::verticalTrend(-150) == 0);
  CHECK(fmt::verticalTrend(250) == 0);
}

TEST_CASE("emergency squawk codes are recognised") {
  CHECK(fmt::isEmergencySquawk("7700"));
  CHECK(fmt::isEmergencySquawk("7600"));
  CHECK(fmt::isEmergencySquawk("7500"));
  CHECK_FALSE(fmt::isEmergencySquawk("1317"));
  CHECK_FALSE(fmt::isEmergencySquawk(""));
  CHECK_FALSE(fmt::isEmergencySquawk(nullptr));
}

TEST_CASE("Label never overflows") {
  fmt::Label label = fmt::Label::printf("%s", "this text is far too long to fit inside the tiny label buffer");
  CHECK(std::string(label.c_str()).size() == fmt::Label::kCapacity - 1);
}
