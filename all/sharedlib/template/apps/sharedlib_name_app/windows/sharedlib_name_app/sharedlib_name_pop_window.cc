#include "sharedlib_name_pop_window.h"

#include "resource.h"

SharedlibNamePopWindow::SharedlibNamePopWindow() {}
SharedlibNamePopWindow::~SharedlibNamePopWindow() {}

LRESULT SharedlibNamePopWindow::HandleMessage(UINT msg, WPARAM w_param,
                                              LPARAM l_param) {
  switch (msg) {
    case WM_CREATE: {
      SetIcon(IDR_MAINFRAME);
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
      TCHAR text[] = TEXT("SharedlibNamePopWindow");
      RECT client_area;
      GetClientRect(GetHwnd(), &client_area);
      ::DrawText(hdc, text, sizeof(text) / sizeof(TCHAR) - 1, &client_area,
                 DT_CENTER | DT_VCENTER);
      ::EndPaint(GetHwnd(), &ps);
      return 0;
    } break;
  }
  return DuiWindow::HandleMessage(msg, w_param, l_param);
}
