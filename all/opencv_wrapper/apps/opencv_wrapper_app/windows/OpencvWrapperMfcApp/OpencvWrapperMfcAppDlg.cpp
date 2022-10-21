// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// OpencvWrapperMfcAppDlg.cpp : implementation file
//

#include "OpencvWrapperMfcAppDlg.h"

#include <codecvt>
#include <locale>
#include <string>

#include "OpencvWrapperMfcApp.h"
#include "afxdialogex.h"
#include "opencv_wrapper/opencv_wrapper.h"
#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx {
 public:
  CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
  enum {IDD = IDD_ABOUTBOX};
#endif

 protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support

  // Implementation
 protected:
  DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// COpencvWrapperMfcAppDlg dialog

COpencvWrapperMfcAppDlg::COpencvWrapperMfcAppDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_OPENCV_WRAPPER_MFCAPP_DIALOG, pParent) {
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COpencvWrapperMfcAppDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_VERSION_BUTTON, version_);
}

BEGIN_MESSAGE_MAP(COpencvWrapperMfcAppDlg, CDialogEx)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_BN_CLICKED(IDC_VERSION_BUTTON, OnVersionButtonClicked)
END_MESSAGE_MAP()

// COpencvWrapperMfcAppDlg message handlers

BOOL COpencvWrapperMfcAppDlg::OnInitDialog() {
  CDialogEx::OnInitDialog();

  // Add "About..." menu item to system menu.

  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);

  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL) {
    BOOL bNameValid;
    CString strAboutMenu;
    bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
    ASSERT(bNameValid);
    if (!strAboutMenu.IsEmpty()) {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }

  // Set the icon for this dialog.  The framework does this automatically
  //  when the application's main window is not a dialog
  SetIcon(m_hIcon, TRUE);   // Set big icon
  SetIcon(m_hIcon, FALSE);  // Set small icon

  // TODO: Add extra initialization here

  return TRUE;  // return TRUE  unless you set the focus to a control
}

void COpencvWrapperMfcAppDlg::OnSysCommand(UINT nID, LPARAM lParam) {
  if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
  } else {
    CDialogEx::OnSysCommand(nID, lParam);
  }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void COpencvWrapperMfcAppDlg::OnPaint() {
  if (IsIconic()) {
    CPaintDC dc(this);  // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()),
                0);

    // Center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  } else {
    CDialogEx::OnPaint();
  }
}

// The system calls this function to obtain the cursor to display while the user
// drags the minimized window.
HCURSOR COpencvWrapperMfcAppDlg::OnQueryDragIcon() {
  return static_cast<HCURSOR>(m_hIcon);
}

void COpencvWrapperMfcAppDlg::OnVersionButtonClicked() {
  std::string version_str;
  version_str.append(OPENCV_WRAPPER::GetVersion());
#if _MSC_VER == 1900
  std::wstring_convert<std::codecvt_utf8<int16_t>, int16_t> convert;
#else
  std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
#endif
  auto dest = convert.from_bytes(version_str);
  const wchar_t* text = (const wchar_t*)dest.data();
  version_.SetWindowText(text);
}
