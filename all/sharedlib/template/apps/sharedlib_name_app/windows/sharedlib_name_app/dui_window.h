#pragma once

#include <Windows.h>

class DuiWindow {
 public:
  DuiWindow();
  ~DuiWindow();

  HWND GetHwnd() const;
  operator HWND() const;

  bool RegisterWindowClass();
  bool RegisterSuperclass();

  HWND Create(HWND parent, LPCTSTR name, DWORD style, DWORD ex_style,
              const RECT area, HMENU menu = nullptr);
  HWND Create(HWND parent, LPCTSTR name, DWORD style, DWORD ex_style,
              int left = CW_USEDEFAULT, int top = CW_USEDEFAULT,
              int width = CW_USEDEFAULT, int height = CW_USEDEFAULT,
              HMENU menu = nullptr);

  void Show(bool take_focus = true);
  void Hide();
  bool ShowModal();
  void Close();
  void Center();
  void SetIcon(UINT res);
  void SetLocation(int left, int top);
  void SetSize(int width, int height);

 protected:
  virtual LPCTSTR GetWindowClassName() const = 0;
  virtual LPCTSTR GetSuperClassName() const;
  virtual UINT GetClassStyle() const;

  void ResizeClient(int width = -1, int height = -1);

  virtual LRESULT HandleMessage(UINT msg, WPARAM w_param, LPARAM l_param);
  virtual void OnFinalMessage(HWND hwnd);

 private:
  static LRESULT CALLBACK DuiWindowProc(HWND hwnd, UINT msg, WPARAM w_param,
                                        LPARAM l_param);
  static LRESULT CALLBACK DuiControlProc(HWND hwnd, UINT msg, WPARAM w_param,
                                         LPARAM l_param);

 private:
  HWND hwnd_;
  WNDPROC old_wndproc_;
};
