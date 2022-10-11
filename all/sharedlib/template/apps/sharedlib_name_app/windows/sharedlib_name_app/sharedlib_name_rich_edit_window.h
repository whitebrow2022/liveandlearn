#pragma once

#include "sharedlib_name_window.h"

class SharedlibNameRichEditWindow : public SharedlibNameWindow {
public:
  SharedlibNameRichEditWindow();
  ~SharedlibNameRichEditWindow();

private:
  LPCTSTR GetWindowClassName() const override {
    return TEXT("SharedlibNameRichEditWindow");
  }
  LPCTSTR GetSuperClassName() const override;
  UINT GetClassStyle() const override { return CS_VREDRAW | CS_HREDRAW; }

  LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param) override;
  void OnFinalMessage(HWND /*hwnd*/) override { delete this; }
};
