#pragma once

enum class key_code : uint32_t {
  // 0x00 - 0xff is usage page
  extra_ = 0x1000,
  // keys will be handled in console_user_client
  fn,
  f1,
  f2,
  f3,
  f4,
  f5,
  f6,
  f7,
  f8,
  f9,
  f10,
  f11,
  f12,
  fn_f1,
  fn_f2,
  fn_f3,
  fn_f4,
  fn_f5,
  fn_f6,
  fn_f7,
  fn_f8,
  fn_f9,
  fn_f10,
  fn_f11,
  fn_f12,
};

enum class pointing_button : uint32_t {
};

enum class modifier_flag : uint32_t {
};
