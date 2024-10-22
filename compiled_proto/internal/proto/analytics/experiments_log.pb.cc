// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: internal/proto/analytics/experiments_log.proto

#include "internal/proto/analytics/experiments_log.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG
namespace nearby {
namespace experiments {
constexpr ExperimentsLog::ExperimentsLog(
  ::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized)
  : experiment_token_(&::PROTOBUF_NAMESPACE_ID::internal::fixed_address_empty_string)
  , zwieback_cookie_(&::PROTOBUF_NAMESPACE_ID::internal::fixed_address_empty_string){}
struct ExperimentsLogDefaultTypeInternal {
  constexpr ExperimentsLogDefaultTypeInternal()
    : _instance(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized{}) {}
  ~ExperimentsLogDefaultTypeInternal() {}
  union {
    ExperimentsLog _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ExperimentsLogDefaultTypeInternal _ExperimentsLog_default_instance_;
}  // namespace experiments
}  // namespace nearby
namespace nearby {
namespace experiments {

// ===================================================================

class ExperimentsLog::_Internal {
 public:
  using HasBits = decltype(std::declval<ExperimentsLog>()._has_bits_);
  static void set_has_experiment_token(HasBits* has_bits) {
    (*has_bits)[0] |= 1u;
  }
  static void set_has_zwieback_cookie(HasBits* has_bits) {
    (*has_bits)[0] |= 2u;
  }
};

ExperimentsLog::ExperimentsLog(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::MessageLite(arena, is_message_owned) {
  SharedCtor();
  if (!is_message_owned) {
    RegisterArenaDtor(arena);
  }
  // @@protoc_insertion_point(arena_constructor:nearby.experiments.ExperimentsLog)
}
ExperimentsLog::ExperimentsLog(const ExperimentsLog& from)
  : ::PROTOBUF_NAMESPACE_ID::MessageLite(),
      _has_bits_(from._has_bits_) {
  _internal_metadata_.MergeFrom<std::string>(from._internal_metadata_);
  experiment_token_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    experiment_token_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (from._internal_has_experiment_token()) {
    experiment_token_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, from._internal_experiment_token(), 
      GetArenaForAllocation());
  }
  zwieback_cookie_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    zwieback_cookie_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (from._internal_has_zwieback_cookie()) {
    zwieback_cookie_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, from._internal_zwieback_cookie(), 
      GetArenaForAllocation());
  }
  // @@protoc_insertion_point(copy_constructor:nearby.experiments.ExperimentsLog)
}

inline void ExperimentsLog::SharedCtor() {
experiment_token_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  experiment_token_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
zwieback_cookie_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  zwieback_cookie_.Set(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), "", GetArenaForAllocation());
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

ExperimentsLog::~ExperimentsLog() {
  // @@protoc_insertion_point(destructor:nearby.experiments.ExperimentsLog)
  if (GetArenaForAllocation() != nullptr) return;
  SharedDtor();
  _internal_metadata_.Delete<std::string>();
}

inline void ExperimentsLog::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  experiment_token_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  zwieback_cookie_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

void ExperimentsLog::ArenaDtor(void* object) {
  ExperimentsLog* _this = reinterpret_cast< ExperimentsLog* >(object);
  (void)_this;
}
void ExperimentsLog::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void ExperimentsLog::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}

void ExperimentsLog::Clear() {
// @@protoc_insertion_point(message_clear_start:nearby.experiments.ExperimentsLog)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  if (cached_has_bits & 0x00000003u) {
    if (cached_has_bits & 0x00000001u) {
      experiment_token_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000002u) {
      zwieback_cookie_.ClearNonDefaultToEmpty();
    }
  }
  _has_bits_.Clear();
  _internal_metadata_.Clear<std::string>();
}

const char* ExperimentsLog::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  _Internal::HasBits has_bits{};
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // optional bytes experiment_token = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_experiment_token();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // optional string zwieback_cookie = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          auto str = _internal_mutable_zwieback_cookie();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<std::string>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  _has_bits_.Or(has_bits);
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* ExperimentsLog::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:nearby.experiments.ExperimentsLog)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  // optional bytes experiment_token = 1;
  if (cached_has_bits & 0x00000001u) {
    target = stream->WriteBytesMaybeAliased(
        1, this->_internal_experiment_token(), target);
  }

  // optional string zwieback_cookie = 2;
  if (cached_has_bits & 0x00000002u) {
    target = stream->WriteStringMaybeAliased(
        2, this->_internal_zwieback_cookie(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = stream->WriteRaw(_internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).data(),
        static_cast<int>(_internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).size()), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:nearby.experiments.ExperimentsLog)
  return target;
}

size_t ExperimentsLog::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:nearby.experiments.ExperimentsLog)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _has_bits_[0];
  if (cached_has_bits & 0x00000003u) {
    // optional bytes experiment_token = 1;
    if (cached_has_bits & 0x00000001u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::BytesSize(
          this->_internal_experiment_token());
    }

    // optional string zwieback_cookie = 2;
    if (cached_has_bits & 0x00000002u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
          this->_internal_zwieback_cookie());
    }

  }
  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    total_size += _internal_metadata_.unknown_fields<std::string>(::PROTOBUF_NAMESPACE_ID::internal::GetEmptyString).size();
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void ExperimentsLog::CheckTypeAndMergeFrom(
    const ::PROTOBUF_NAMESPACE_ID::MessageLite& from) {
  MergeFrom(*::PROTOBUF_NAMESPACE_ID::internal::DownCast<const ExperimentsLog*>(
      &from));
}

void ExperimentsLog::MergeFrom(const ExperimentsLog& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:nearby.experiments.ExperimentsLog)
  GOOGLE_DCHECK_NE(&from, this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._has_bits_[0];
  if (cached_has_bits & 0x00000003u) {
    if (cached_has_bits & 0x00000001u) {
      _internal_set_experiment_token(from._internal_experiment_token());
    }
    if (cached_has_bits & 0x00000002u) {
      _internal_set_zwieback_cookie(from._internal_zwieback_cookie());
    }
  }
  _internal_metadata_.MergeFrom<std::string>(from._internal_metadata_);
}

void ExperimentsLog::CopyFrom(const ExperimentsLog& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:nearby.experiments.ExperimentsLog)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ExperimentsLog::IsInitialized() const {
  return true;
}

void ExperimentsLog::InternalSwap(ExperimentsLog* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_has_bits_[0], other->_has_bits_[0]);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      &experiment_token_, lhs_arena,
      &other->experiment_token_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
      &zwieback_cookie_, lhs_arena,
      &other->zwieback_cookie_, rhs_arena
  );
}

std::string ExperimentsLog::GetTypeName() const {
  return "nearby.experiments.ExperimentsLog";
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace experiments
}  // namespace nearby
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::nearby::experiments::ExperimentsLog* Arena::CreateMaybeMessage< ::nearby::experiments::ExperimentsLog >(Arena* arena) {
  return Arena::CreateMessageInternal< ::nearby::experiments::ExperimentsLog >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
