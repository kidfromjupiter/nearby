#include "connections/cli/command_line.h"

#include <cstdint>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace nearby::connections::cli {
namespace {

TEST(CommandLineTest, TokenizesQuotesAndEscapes) {
  std::vector<std::string> tokens;
  std::string error;
  ASSERT_TRUE(Tokenize(R"(send-bytes E123 "hello world" 'and more' a\ b)",
                       &tokens, &error))
      << error;
  EXPECT_EQ(tokens,
            (std::vector<std::string>{"send-bytes", "E123", "hello world",
                                      "and more", "a b"}));
}

TEST(CommandLineTest, ParsesOptionsAndNegatedFlags) {
  CommandLine command;
  std::string error;
  ASSERT_TRUE(ParseCommand(
      "advertise --service-id=chat --mediums ble,wifi-lan --low-power "
      "--no-auto-upgrade",
      &command, &error))
      << error;
  EXPECT_EQ(command.command, "advertise");
  EXPECT_EQ(command.options.at("service-id"), "chat");
  EXPECT_EQ(command.options.at("mediums"), "ble,wifi-lan");
  EXPECT_EQ(command.options.at("low-power"), "true");
  EXPECT_EQ(command.options.at("auto-upgrade"), "false");
}

TEST(CommandLineTest, RejectsDuplicateOptions) {
  CommandLine command;
  std::string error;
  EXPECT_FALSE(
      ParseCommand("discover --low-power --low-power=false", &command, &error));
  EXPECT_NE(error.find("more than once"), std::string::npos);
}

TEST(CommandLineTest, ParsesTypedValues) {
  CommandLine command;
  std::string error;
  ASSERT_TRUE(ParseCommand("request --keep-alive-interval=1500 --low-power=no",
                           &command, &error));
  std::int64_t interval;
  bool low_power;
  EXPECT_TRUE(GetInt64(command, "keep-alive-interval", 0, &interval, &error));
  EXPECT_EQ(interval, 1500);
  EXPECT_TRUE(GetBool(command, "low-power", true, &low_power, &error));
  EXPECT_FALSE(low_power);
}

TEST(CommandLineTest, ReportsMalformedInput) {
  CommandLine command;
  std::string error;
  EXPECT_FALSE(
      ParseCommand("send-bytes E123 \"unterminated", &command, &error));
  EXPECT_NE(error.find("unterminated"), std::string::npos);
}

}  // namespace
}  // namespace nearby::connections::cli
