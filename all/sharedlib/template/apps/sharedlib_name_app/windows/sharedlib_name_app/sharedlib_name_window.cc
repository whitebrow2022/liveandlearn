#include "sharedlib_name_window.h"

#include <memory>
#include <string>

#include "sharedlib_name/sharedlib_name.h"

std::wstring Utf8ToUtf16(const std::string& utf8_str) {
  std::wstring ret;
  int utf16_str_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(),
                                          utf8_str.length(), nullptr, 0);
  if (utf16_str_len != 0) {
    auto utf16_str = std::make_unique<wchar_t[]>(utf16_str_len + 1);
    std::fill(utf16_str.get(), utf16_str.get() + utf16_str_len + 1, 0);
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), utf8_str.length(),
                            utf16_str.get(), utf16_str_len) != 0) {
      ret = utf16_str.get();
    }
  }
  return ret;
}

SharedlibNameWindow::SharedlibNameWindow() {}
SharedlibNameWindow::~SharedlibNameWindow() {}

LRESULT SharedlibNameWindow::HandleMessage(UINT msg, WPARAM w_param,
                                           LPARAM l_param) {
  switch (msg) {
    case WM_CREATE: {
    } break;
    case WM_DESTROY: {
    } break;
    case WM_SETFOCUS: {
    } break;
    case WM_SIZE: {
      return 0;
    } break;
    case WM_ERASEBKGND: {
      HDC hdc = (HDC)w_param;
      RECT erase_area;
      GetClientRect(GetHwnd(), &erase_area);
      FillRect(hdc, &erase_area, (HBRUSH)GetStockObject(WHITE_BRUSH));
      return 1;
    } break;
    case WM_PAINT: {
      PAINTSTRUCT ps = {0};
      HDC hdc = ::BeginPaint(GetHwnd(), &ps);
      std::wstring text = TEXT("SharedlibNameWindow");
      std::wstring version = Utf8ToUtf16(SHAREDLIB_NAME::GetVersion());
      text.append(L" ");
      text.append(version);
      RECT client_area;
      GetClientRect(GetHwnd(), &client_area);
      ::DrawText(hdc, text.c_str(), text.length(), &client_area,
                 DT_CENTER | DT_VCENTER);
      ::EndPaint(GetHwnd(), &ps);
      return 0;
    } break;
  }
  return DuiWindow::HandleMessage(msg, w_param, l_param);
}
