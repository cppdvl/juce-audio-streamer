#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

PluginProcessor testPlugin;
TEST_CASE("one is equal to one", "[dummy]")
{
  REQUIRE(1 == 1);
}


TEST_CASE("Plugin instance name", "[name]")
{
  CHECK_THAT(testPlugin.getName().toStdString(),
             Catch::Matchers::Equals("DAWn AUDIO'S AudioStream Plugin"));
}

