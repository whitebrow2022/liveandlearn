#pragma once

#include <Windows.h>

class DuiApplication final {
 public:
  static bool TranslateMessage(const MSG& msg);
  static HINSTANCE GetResourceInstance();
  static void SetResourceInstance(HINSTANCE inst);
  static void MessageLoop();

 private:
  static HINSTANCE instance_;
};
