// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// SharedlibNameMfcAppDlg.h : header file
//

#pragma once

// CSharedlibNameMfcAppDlg dialog
class CSharedlibNameMfcAppDlg : public CDialogEx {
  // Construction
 public:
  CSharedlibNameMfcAppDlg(CWnd* pParent = NULL);  // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_SHAREDLIB_NAME_MFCAPP_DIALOG };
#endif

 protected:
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support

  // Implementation
 protected:
  HICON m_hIcon;
  CButton version_;

  // Generated message map functions
  virtual BOOL OnInitDialog();
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  afx_msg void OnVersionButtonClicked();
  DECLARE_MESSAGE_MAP()
};
