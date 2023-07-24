// Copyright 2023 Google LLC
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

/// Module for shared functionality between all Bluetooth platforms.
mod adapter;
mod address;
mod advertisement;
mod device;
mod error;

pub use adapter::*;
pub use address::*;
pub use advertisement::*;
pub use device::*;
pub use error::*;
