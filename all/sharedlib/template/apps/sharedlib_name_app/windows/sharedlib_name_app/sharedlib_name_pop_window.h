#pragma once

#include "dui_window.h"

class SharedlibNamePopWindow : public DuiWindow {
 public:
  SharedlibNamePopWindow();
  ~SharedlibNamePopWindow();

 private:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("SharedlibNamePopWindow");
  }
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
};
