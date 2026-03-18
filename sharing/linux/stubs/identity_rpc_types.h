// Copyright 2026 Google LLC
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

#ifndef THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_IDENTITY_RPC_TYPES_H_
#define THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_IDENTITY_RPC_TYPES_H_

#include <cstdint>

#include <string>
#include <utility>
#include <vector>

#include "google/protobuf/timestamp.pb.h"

namespace google::nearby::identity::v1 {

class SharedCredential {
 public:
  enum DataType {
    DATA_TYPE_UNKNOWN = 0,
    DATA_TYPE_PUBLIC_CERTIFICATE = 1,
  };

  void set_id(uint64_t id) { id_ = id; }
  uint64_t id() const { return id_; }

  void set_data(std::string data) { data_ = std::move(data); }
  const std::string& data() const { return data_; }

  void set_data_type(DataType data_type) { data_type_ = data_type; }
  DataType data_type() const { return data_type_; }

  google::protobuf::Timestamp* mutable_expiration_time() {
    return &expiration_time_;
  }
  const google::protobuf::Timestamp& expiration_time() const {
    return expiration_time_;
  }

 private:
  uint64_t id_ = 0;
  std::string data_;
  DataType data_type_ = DATA_TYPE_UNKNOWN;
  google::protobuf::Timestamp expiration_time_;
};

class PerVisibilitySharedCredentials {
 public:
  enum Visibility {
    VISIBILITY_UNKNOWN = 0,
    VISIBILITY_SELF = 1,
    VISIBILITY_CONTACTS = 2,
  };

  void set_visibility(Visibility visibility) { visibility_ = visibility; }
  Visibility visibility() const { return visibility_; }

  SharedCredential* add_shared_credentials() {
    shared_credentials_.emplace_back();
    return &shared_credentials_.back();
  }
  const std::vector<SharedCredential>& shared_credentials() const {
    return shared_credentials_;
  }

 private:
  Visibility visibility_ = VISIBILITY_UNKNOWN;
  std::vector<SharedCredential> shared_credentials_;
};

class Device {
 public:
  enum Contact {
    CONTACT_UNKNOWN = 0,
    CONTACT_GOOGLE_CONTACT = 1,
    CONTACT_GOOGLE_CONTACT_LATEST = 2,
  };

  void set_name(std::string name) { name_ = std::move(name); }
  const std::string& name() const { return name_; }

  void set_display_name(std::string display_name) {
    display_name_ = std::move(display_name);
  }
  const std::string& display_name() const { return display_name_; }

  void set_contact(Contact contact) { contact_ = contact; }
  Contact contact() const { return contact_; }

  PerVisibilitySharedCredentials* add_per_visibility_shared_credentials() {
    per_visibility_shared_credentials_.emplace_back();
    return &per_visibility_shared_credentials_.back();
  }
  const std::vector<PerVisibilitySharedCredentials>&
  per_visibility_shared_credentials() const {
    return per_visibility_shared_credentials_;
  }

 private:
  std::string name_;
  std::string display_name_;
  Contact contact_ = CONTACT_UNKNOWN;
  std::vector<PerVisibilitySharedCredentials> per_visibility_shared_credentials_;
};

class QuerySharedCredentialsRequest {
 public:
  void set_name(std::string name) { name_ = std::move(name); }
  const std::string& name() const { return name_; }

  void set_page_token(std::string page_token) {
    page_token_ = std::move(page_token);
  }
  const std::string& page_token() const { return page_token_; }

 private:
  std::string name_;
  std::string page_token_;
};

class QuerySharedCredentialsResponse {
 public:
  SharedCredential* add_shared_credentials() {
    shared_credentials_.emplace_back();
    return &shared_credentials_.back();
  }
  const std::vector<SharedCredential>& shared_credentials() const {
    return shared_credentials_;
  }

  void set_next_page_token(std::string next_page_token) {
    next_page_token_ = std::move(next_page_token);
  }
  const std::string& next_page_token() const { return next_page_token_; }

 private:
  std::vector<SharedCredential> shared_credentials_;
  std::string next_page_token_;
};

class PublishDeviceRequest {
 public:
  Device* mutable_device() { return &device_; }
  const Device& device() const { return device_; }

 private:
  Device device_;
};

class PublishDeviceResponse {
 public:
  enum ContactUpdate {
    CONTACT_UPDATE_UNKNOWN = 0,
    CONTACT_UPDATE_REMOVED = 1,
  };

  void add_contact_updates(ContactUpdate contact_update) {
    contact_updates_.push_back(contact_update);
  }
  const std::vector<ContactUpdate>& contact_updates() const {
    return contact_updates_;
  }

 private:
  std::vector<ContactUpdate> contact_updates_;
};

class GetAccountInfoRequest {};

class AccountInfo {
 public:
  enum Capability {
    CAPABILITY_UNKNOWN = 0,
    CAPABILITY_TITANIUM = 1,
  };

  void add_capabilities(Capability capability) {
    capabilities_.push_back(capability);
  }
  const std::vector<Capability>& capabilities() const { return capabilities_; }

 private:
  std::vector<Capability> capabilities_;
};

class GetAccountInfoResponse {
 public:
  AccountInfo* mutable_account_info() { return &account_info_; }
  const AccountInfo& account_info() const { return account_info_; }

 private:
  AccountInfo account_info_;
};

}  // namespace google::nearby::identity::v1

#endif  // THIRD_PARTY_NEARBY_SHARING_LINUX_STUBS_IDENTITY_RPC_TYPES_H_
