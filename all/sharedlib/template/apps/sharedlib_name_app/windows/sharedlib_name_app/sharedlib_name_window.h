#pragma once

#include "dui_window.h"

class SharedlibNameWindow : public DuiWindow {
 public:
  SharedlibNameWindow();
  ~SharedlibNameWindow();

 protected:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("SharedlibNameWindow");
  }
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
};
