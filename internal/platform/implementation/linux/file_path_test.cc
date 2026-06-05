// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "internal/platform/implementation/linux/file_path.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "internal/platform/implementation/linux/device_info.h"
#include "internal/platform/implementation/linux/utils.h"
#include "gtest/gtest.h"

namespace nearby {
namespace linux {

namespace {

const wchar_t* kFileName(L"increment_file_test.txt");
const wchar_t* kFirstIterationFileName(L"/increment_file_test (1).txt");
const wchar_t* kSecondIterationFileName(L"/increment_file_test (2).txt");
const wchar_t* kThirdIterationFileName(L"/increment_file_test (3).txt");
const wchar_t* kNoDotsFileName(L"incrementfiletesttxt");
const wchar_t* kOneIterationNoDotsFileName(L"/incrementfiletesttxt (1)");
const wchar_t* kMultipleDotsFileName(L"increment.file.test.txt");
const wchar_t* kOneIterationMultipleDotsFileName(
    L"/increment.file.test (1).txt");
const wchar_t* kImmediateEscape(L"../");
const wchar_t* kLongEscapeBackSlash(L"..\\test\\..\\..\\test");
const wchar_t* kTwoLevelFolder(L"/test/test");
const wchar_t* kLongEscapeSlash(L"../test/../../test");
const wchar_t* kLongEscapeMixedSlash(L"../test\\..\\../test");
const wchar_t* kLongEscapeEndingEscape(L"../test/../../test/..");
const wchar_t* kLongEscapeEndingEscapeWithSlash(
    L"../test/../../test/../../../");
}  // namespace

// Can't run on google 3, I presume the SHGetKnownFolderPath
// fails.
class FilePathTests : public testing::Test {
 protected:
  // You can define per-test set-up logic as usual.
  FilePathTests() {
    default_download_path_ =
        string_to_wstring(std::string(getenv("HOME")).append("/Downloads"));
  }
  std::wstring default_download_path_;
};

TEST_F(FilePathTests,
       GetDownloadPathWithEmptyStringArgumentsShouldReturnBaseDownloadPath) {
  std::wstring parent_folder(L"");
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_);
}  // NOLINT false lint error here

TEST_F(
    FilePathTests,
    GetDownloadPathWithSlashParentFolderArgumentsShouldReturnBaseDownloadPath) {
  std::wstring parent_folder(L"/");
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_);
}  // NOLINT false lint error here

TEST_F(
    FilePathTests,
    GetDownloadPathWithBackslashParentFolderArgumentsShouldReturnBaseDownloadPath) {
  std::wstring parent_folder(L"\\");
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_);
}  // NOLINT false lint error here

TEST_F(
    FilePathTests,
    GetDownloadPathWithAttemptToEscapeUsersDownloadFolderShouldReturnDownloadPathNotEscapingUsersDownloadFolder) {
  std::wstring parent_folder(kImmediateEscape);
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithMultipleAttemptsToEscapeUsersDownloadFolderWithBackslashShouldReturnDownloadPathNotEscapingUsersDownloadFolder) {
  std::wstring parent_folder(kLongEscapeBackSlash);
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_ + kTwoLevelFolder);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithMultipleAttemptsToEscapeUsersDownloadFolderShouldReturnDownloadPathNotEscapingUsersDownloadFolder) {
  std::wstring parent_folder(kLongEscapeSlash);
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_ + kTwoLevelFolder);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithMultipleAttemptsToEscapeUsersDownloadFolderWithMixedSlashShouldReturnDownloadPathNotEscapingUsersDownloadFolder) {
  std::wstring parent_folder(kLongEscapeMixedSlash);
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_ + kTwoLevelFolder);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithMultipleAttemptsToEscapeUsersDownloadFolderWithEndingEscapeShouldReturnDownloadPathNotEscapingUsersDownloadFolder) {
  std::wstring parent_folder(kLongEscapeEndingEscape);
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_ + kTwoLevelFolder);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithMultipleAttemptsToEscapeUsersDownloadFolderWithEndingSlashShouldReturnDownloadPathNotEscapingUsersDownloadFolder) {
  std::wstring parent_folder(kLongEscapeEndingEscapeWithSlash);
  std::wstring file_name(L"");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_ + kTwoLevelFolder);
}

TEST_F(FilePathTests,
       GetDownloadPathWithSlashFileNameArgumentsShouldReturnBaseDownloadPath) {
  std::wstring parent_folder(L"");
  std::wstring file_name(L"/");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, default_download_path_);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithBackslashFileNameArgumentsShouldReturnBaseDownloadPath) {
  std::wstring parent_folder(L"");
  std::wstring file_name(L"\\");

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  auto result_size = actual.size();
  auto default_size = default_download_path_.size();

  EXPECT_EQ(actual, default_download_path_);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithParentFolderShouldReturnParentFolderAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"test_parent_folder");
  std::wstring file_name(L"");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_parent_folder";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithParentFolderStartingWithSlashArgumentsShouldReturnParentFolderAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"/test_parent_folder");
  std::wstring file_name(L"");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_parent_folder";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithParentFolderStartingWithBackslashArgumentsShouldReturnParentFolderAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"\\test_parent_folder");
  std::wstring file_name(L"");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_parent_folder";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithParentFolderEndingWithSlashArgumentsShouldReturnParentFolderAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"test_parent_folder/");
  std::wstring file_name(L"");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_parent_folder";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithParentFolderEndingWithBackslashArgumentsShouldReturnParentFolderAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"test_parent_folder\\");
  std::wstring file_name(L"");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_parent_folder";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithFileNameBeginningWithSlashArgumentsShouldReturnFileNameAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"");
  std::wstring file_name(L"/test_file_name.name");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_file_name.name";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithFileNameBeginningWithBackslashArgumentsShouldReturnFileNameAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"");
  std::wstring file_name(L"\\test_file_name.name");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_file_name.name";

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, path.str().c_str());
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithFileNameEndingWithSlashArgumentsShouldReturnFileNameAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"");
  std::wstring file_name(L"test_file_name.name/");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_file_name.name";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithFileNameEndingWithBackslashArgumentsShouldReturnFileNameAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"");
  std::wstring file_name(L"test_file_name.name\\");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_file_name.name";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithParentFolderAndFileNameArgumentsShouldReturnParentFolderAndFileNameAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"test_parent_folder");
  std::wstring file_name(L"test_file_name.name");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_parent_folder"
       << "/"
       << "test_file_name.name";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithParentFolderEndingWithBackslashAndFileNameArgumentsShouldReturnParentFolderAndFileNameAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"test_parent_folder\\");
  std::wstring file_name(L"test_file_name.name");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_parent_folder"
       << "/"
       << "test_file_name.name";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPathWithFileNameStartingWithBackslashAndParentFolderArgumentsShouldReturnParentFolderAndFileNameAppendedToBaseDownloadPath) {
  std::wstring parent_folder(L"test_parent_folder");
  std::wstring file_name(L"\\test_file_name.name");

  std::wstringstream path(L"");
  path << default_download_path_ << L"/" << "test_parent_folder"
       << "/"
       << "test_file_name.name";

  std::wstring expected = path.str();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_IllegalFileNameCharactersReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x05,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test\x5Test");
  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_LowestIllegalFileNameCharacterReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x01,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test\x1Test");

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_HighestIllegalFileNameCharacterReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x1f,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test\x1fTest");

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_IllegalFileNameCharacterQuestionMarkReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2f,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test?Test");

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_IllegalFileNameCharacterAsteriskReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2f,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test*Test");

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_IllegalFileNameCharacterLessThanReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2f,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test<Test");

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_IllegalFileNameCharacterGreaterThanReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2f,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test>Test");

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_IllegalFileNameCharacterVerticalBarReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2f,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test|Test");

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_IllegalFileNameCharacterColonReturnsFileNameWithUnderbarSubstituted) {
  // char illegal_character_sequence[]{ 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x2f,
  // 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0 };
  auto illegal_character_sequence(L"Test:Test");

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/Test_Test");

  auto actual(FilePath::GetDownloadPath(
      parent_folder, std::wstring(illegal_character_sequence)));

  EXPECT_EQ(actual, expected);
}

TEST_F(FilePathTests,
       GetDownloadPath_FileDoesntExistReturnsFileWithPassedName) {
  std::wstring file_name(kFileName);
  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(L"/");
  expected.append(file_name);

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);
}

TEST_F(FilePathTests,
       GetDownloadPath_FileExistsReturnsFileWithIncrementedName) {
  std::wstring file_name(kFileName);
  std::wstring renamed_file_name(kFirstIterationFileName);
  std::wstring parent_folder(L"");

  std::wstring output_file_path(default_download_path_);
  output_file_path.append(L"/");
  output_file_path.append(file_name);

  std::wstring expected(default_download_path_);
  expected += renamed_file_name;

  std::wifstream input_file;
  std::wofstream output_file;

  output_file.open(wstring_to_string(output_file_path),
                   std::ofstream::binary | std::ofstream::out);

  ASSERT_TRUE(output_file.rdstate() == std::ofstream::goodbit);

  output_file.close();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(actual, expected);

  // Remove the file and check that it is removed
  // File 1
  std::filesystem::remove(output_file_path.c_str());

  input_file.open(wstring_to_string(output_file_path),
                  std::ifstream::binary | std::ifstream::in);

  ASSERT_FALSE(input_file.rdstate() == std::ifstream::goodbit);
}

TEST_F(FilePathTests,
       GetDownloadPath_MultipleFilesExistReturnsNextIncrementedFileName) {
  std::ofstream output_file;
  std::ifstream input_file;

  std::wstring file_name(kFileName);
  std::wstring first_renamed_file_name(kFirstIterationFileName);
  std::wstring second_renamed_file_name(kSecondIterationFileName);

  std::wstring parent_folder(L"");

  std::wstring expected(default_download_path_);
  expected.append(second_renamed_file_name.c_str());

  std::wstring output_file1_path(default_download_path_);
  output_file1_path.append(L"/");
  output_file1_path.append(file_name);

  std::wstring output_file2_path(default_download_path_);
  output_file2_path.append(first_renamed_file_name);

  // Create the test files
  output_file.open(wstring_to_string(output_file1_path),
                   std::ofstream::binary | std::ofstream::out);
  ASSERT_TRUE(output_file.rdstate() == std::ofstream::goodbit);
  output_file.close();
  output_file.clear();

  output_file.open(wstring_to_string(output_file2_path),
                   std::ofstream::binary | std::ofstream::out);
  ASSERT_TRUE(output_file.rdstate() == std::ofstream::goodbit);
  output_file.close();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(expected, actual);

  // Remove the test files and check that it is removed
  // File 1
  std::filesystem::remove(wstring_to_string(output_file1_path).c_str());
  input_file.open(wstring_to_string(output_file1_path),
                  std::ifstream::binary | std::ifstream::in);

  ASSERT_FALSE(input_file.rdstate() == std::ifstream::goodbit);

  // File 2
  std::filesystem::remove(wstring_to_string(output_file2_path).c_str());

  input_file.clear();
  input_file.open(wstring_to_string(output_file2_path),
                  std::ifstream::binary | std::ifstream::in);

  ASSERT_FALSE(input_file.rdstate() == std::ifstream::goodbit);
}

TEST_F(
    FilePathTests,
    GetDownloadPath_FileNameContainsMultipleDotsReturnsIncrementBeforeFirstDot) {
  std::ifstream input_file;
  std::ofstream output_file;

  std::wstring file_name(kMultipleDotsFileName);
  std::wstring renamed_file_name(kOneIterationMultipleDotsFileName);

  std::wstring parent_folder(L"");

  std::wstring output_file1_path(default_download_path_);
  output_file1_path.append(L"/");
  output_file1_path.append(file_name);

  std::wstring output_file2_path(default_download_path_);
  output_file2_path.append(renamed_file_name);

  std::wstring expected(default_download_path_);
  expected.append(renamed_file_name);

  output_file.open(wstring_to_string(output_file1_path),
                   std::ofstream::binary | std::ofstream::out);
  ASSERT_TRUE(output_file.rdstate() == std::ofstream::goodbit);
  output_file.close();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(expected, actual);

  std::filesystem::remove(wstring_to_string(output_file1_path).c_str());
  input_file.open(wstring_to_string(output_file1_path),
                  std::ifstream::binary | std::ifstream::in);

  ASSERT_FALSE(input_file.rdstate() == std::ifstream::goodbit);
}

TEST_F(FilePathTests,
       GetDownloadPath_FileNameContainsNoDotsReturnsWithIncrementAtEnd) {
  std::ifstream input_file;
  std::ofstream output_file;

  std::wstring file_name(kNoDotsFileName);
  std::wstring renamed_file_name(kOneIterationNoDotsFileName);

  std::wstring parent_folder(L"");

  std::wstring output_file1_path(default_download_path_);
  output_file1_path.append(L"/");
  output_file1_path.append(file_name);

  std::wstring output_file2_path(default_download_path_);
  output_file2_path.append(L"/");
  output_file2_path.append(renamed_file_name);

  std::wstring expected(default_download_path_);
  expected.append(renamed_file_name);

  output_file.open(wstring_to_string(output_file1_path),
                   std::ofstream::binary | std::ofstream::out);
  ASSERT_TRUE(output_file.rdstate() == std::ofstream::goodbit);
  output_file.close();

  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(expected, actual);

  std::filesystem::remove(wstring_to_string(output_file1_path).c_str());
  input_file.open(wstring_to_string(output_file1_path),
                  std::ifstream::binary | std::ifstream::in);

  ASSERT_FALSE(input_file.rdstate() == std::ifstream::goodbit);
}

TEST_F(FilePathTests,
       GetDownloadPath_FileNameExistsWithAHoleBetweenRenamedFiles) {
  std::ifstream input_file;
  std::ofstream output_file;

  std::wstring file_name(kFileName);
  std::wstring file_name1(kFirstIterationFileName);
  std::wstring file_name2(kSecondIterationFileName);
  std::wstring file_name3(kThirdIterationFileName);

  std::wstring parent_folder(L"");

  // Create the path for the original file name
  std::wstring output_file_path(default_download_path_);
  output_file_path.append(L"/");
  output_file_path.append(
      file_name);  // Original file name example: "increment_file_test.txt"

  // Create the path for the first iteration of the original file name
  std::wstring output_file1_path(default_download_path_);
  output_file1_path.append(file_name1);  // First iteration on original file
                                         // name example:
                                         // "increment_file_test (1).txt"

  // Create the path for the third iteration of the original file name
  std::wstring output_file3_path(default_download_path_);
  output_file3_path.append(
      file_name3);  // Third iteration on original file
                    // name example: "increment_file_test (3).txt"

  // Create the expected result which is the second iteration of the original
  // file name
  std::wstring expected(default_download_path_);
  expected.append(file_name2);  // Second iteration on original file name
                                // example: "increment_file_test (2).txt"

  // Create the original file
  output_file.open(wstring_to_string(output_file_path),
                   std::ofstream::binary | std::ofstream::out);
  ASSERT_TRUE(output_file.rdstate() == std::ofstream::goodbit);
  output_file.close();

  // Create the first iteration of the original file
  output_file.clear();
  output_file.open(wstring_to_string(output_file1_path),
                   std::ofstream::binary | std::ofstream::out);
  ASSERT_TRUE(output_file.rdstate() == std::ofstream::goodbit);
  output_file.close();

  // Create the third iteration of the original file
  output_file.clear();
  output_file.open(wstring_to_string(output_file3_path),
                   std::ofstream::binary | std::ofstream::out);
  ASSERT_TRUE(output_file.rdstate() == std::ofstream::goodbit);
  output_file.close();

  // This should return the second iteration of the original file
  auto actual(FilePath::GetDownloadPath(parent_folder, file_name));

  EXPECT_EQ(expected, actual);

  // Delete the original file
  std::filesystem::remove(wstring_to_string(output_file_path).c_str());
  input_file.open(wstring_to_string(output_file_path),
                  std::ifstream::binary | std::ifstream::in);
  ASSERT_FALSE(input_file.rdstate() == std::ifstream::goodbit);

  // Delete the first iteration of the original file
  input_file.clear();  // Reset the input_file state
  std::filesystem::remove(wstring_to_string(output_file1_path).c_str());
  input_file.open(wstring_to_string(output_file1_path),
                  std::ifstream::binary | std::ifstream::in);
  ASSERT_FALSE(input_file.rdstate() == std::ifstream::goodbit);

  // Delete the third iteration of the original file
  input_file.clear();  // Reset the input_file state
  std::filesystem::remove(wstring_to_string(output_file3_path).c_str());
  input_file.open(wstring_to_string(output_file3_path),
                  std::ifstream::binary | std::ifstream::in);
  ASSERT_FALSE(input_file.rdstate() == std::ifstream::goodbit);
}
}  // namespace linux
}  // namespace nearby
