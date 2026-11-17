// Minimal declarations of the SCS Telemetry SDK (ETS2/ATS): only what this plugin uses.
// Binary layout matches the official SDK (scssdk.h, scssdk_value.h,
// scssdk_telemetry.h, scssdk_telemetry_event.h, scssdk_telemetry_channel.h).
// x64 only.
#pragma once
#include <cstdint>

typedef uint8_t     scs_u8_t;
typedef uint32_t    scs_u32_t;
typedef int32_t     scs_s32_t;
typedef uint64_t    scs_u64_t;
typedef int64_t     scs_s64_t;
typedef float       scs_float_t;
typedef double      scs_double_t;
typedef const char* scs_string_t;
typedef void*       scs_context_t;

const scs_u32_t SCS_U32_NIL = static_cast<scs_u32_t>(-1);
#define SCS_MAKE_VERSION(major, minor) (((major) << 16) | (minor))

// results
typedef scs_s32_t scs_result_t;
const scs_result_t SCS_RESULT_ok                 = 0;
const scs_result_t SCS_RESULT_unsupported        = -1;
const scs_result_t SCS_RESULT_invalid_parameter  = -2;
const scs_result_t SCS_RESULT_already_registered = -3;
const scs_result_t SCS_RESULT_not_found          = -4;
const scs_result_t SCS_RESULT_unsupported_type   = -5;
const scs_result_t SCS_RESULT_not_now            = -6;
const scs_result_t SCS_RESULT_generic_error      = -7;

// game log
typedef scs_s32_t scs_log_type_t;
const scs_log_type_t SCS_LOG_TYPE_message = 0;
const scs_log_type_t SCS_LOG_TYPE_warning = 1;
const scs_log_type_t SCS_LOG_TYPE_error   = 2;
typedef void (*scs_log_t)(const scs_log_type_t type, const scs_string_t message);

// values
typedef scs_u32_t scs_value_type_t;
const scs_value_type_t SCS_VALUE_TYPE_INVALID = 0;
const scs_value_type_t SCS_VALUE_TYPE_bool    = 1;
const scs_value_type_t SCS_VALUE_TYPE_s32     = 2;
const scs_value_type_t SCS_VALUE_TYPE_u32     = 3;
const scs_value_type_t SCS_VALUE_TYPE_u64     = 4;
const scs_value_type_t SCS_VALUE_TYPE_float   = 5;
const scs_value_type_t SCS_VALUE_TYPE_double  = 6;

struct scs_value_t {
    scs_value_type_t type;
    scs_u32_t        _padding;
    union {
        scs_u8_t     value_bool;
        scs_s32_t    value_s32;
        scs_u32_t    value_u32;
        scs_u64_t    value_u64;
        scs_s64_t    value_s64;
        scs_float_t  value_float;
        scs_double_t value_double;
        unsigned char _raw[40];   // largest SDK member: dplacement (40 bytes)
    };
};
static_assert(sizeof(scs_value_t) == 48, "unexpected layout of scs_value_t");

// initialization parameters
struct scs_sdk_init_params_v100_t {
    scs_string_t game_name;
    scs_string_t game_id;
    scs_u32_t    game_version;
    scs_u32_t    _padding;        // only present on x64
    scs_log_t    log;
};
static_assert(sizeof(scs_sdk_init_params_v100_t) == 32, "unexpected layout of scs_sdk_init_params_v100_t");

// events
typedef scs_u32_t scs_event_t;
const scs_event_t SCS_TELEMETRY_EVENT_invalid     = 0;
const scs_event_t SCS_TELEMETRY_EVENT_frame_start = 1;
const scs_event_t SCS_TELEMETRY_EVENT_frame_end   = 2;
const scs_event_t SCS_TELEMETRY_EVENT_paused      = 3;
const scs_event_t SCS_TELEMETRY_EVENT_started     = 4;

typedef void (*scs_telemetry_event_callback_t)(const scs_event_t event, const void* const event_info, const scs_context_t context);
typedef scs_result_t (*scs_telemetry_register_for_event_t)(const scs_event_t event, const scs_telemetry_event_callback_t callback, const scs_context_t context);
typedef scs_result_t (*scs_telemetry_unregister_from_event_t)(const scs_event_t event);

// channels
const scs_u32_t SCS_TELEMETRY_CHANNEL_FLAG_none       = 0x00000000;
const scs_u32_t SCS_TELEMETRY_CHANNEL_FLAG_each_frame = 0x00000001;
const scs_u32_t SCS_TELEMETRY_CHANNEL_FLAG_no_value   = 0x00000002;

typedef void (*scs_telemetry_channel_callback_t)(const scs_string_t name, const scs_u32_t index, const scs_value_t* const value, const scs_context_t context);
typedef scs_result_t (*scs_telemetry_register_for_channel_t)(const scs_string_t name, const scs_u32_t index, const scs_value_type_t type, const scs_u32_t flags, const scs_telemetry_channel_callback_t callback, const scs_context_t context);
typedef scs_result_t (*scs_telemetry_unregister_from_channel_t)(const scs_string_t name, const scs_u32_t index, const scs_value_type_t type);

struct scs_telemetry_init_params_v100_t {
    scs_sdk_init_params_v100_t              common;
    scs_telemetry_register_for_event_t      register_for_event;
    scs_telemetry_unregister_from_event_t   unregister_from_event;
    scs_telemetry_register_for_channel_t    register_for_channel;
    scs_telemetry_unregister_from_channel_t unregister_from_channel;
};
typedef scs_telemetry_init_params_v100_t scs_telemetry_init_params_v101_t;
static_assert(sizeof(scs_telemetry_init_params_v100_t) == 64, "unexpected layout of scs_telemetry_init_params_v100_t");

const scs_u32_t SCS_TELEMETRY_VERSION_1_00 = SCS_MAKE_VERSION(1, 0);
const scs_u32_t SCS_TELEMETRY_VERSION_1_01 = SCS_MAKE_VERSION(1, 1);

// channel names used by this plugin
#define SCS_TELEMETRY_TRUCK_CHANNEL_input_brake     "truck.input.brake"
#define SCS_TELEMETRY_TRUCK_CHANNEL_effective_brake "truck.effective.brake"
#define SCS_TELEMETRY_TRUCK_CHANNEL_speed           "truck.speed"
