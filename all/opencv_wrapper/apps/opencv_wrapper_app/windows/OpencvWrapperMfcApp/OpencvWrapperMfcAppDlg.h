// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// OpencvWrapperMfcAppDlg.h : header file
//

#pragma once

// COpencvWrapperMfcAppDlg dialog
class COpencvWrapperMfcAppDlg : public CDialogEx {
  // Construction
 public:
  explicit COpencvWrapperMfcAppDlg(
      CWnd* pParent = NULL);  // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_OPENCV_WRAPPER_MFCAPP_DIALOG };
#endif

 protected:
  void DoDataExchange(CDataExchange* pDX) override;  // DDX/DDV support

  // Implementation
 protected:
  HICON m_hIcon;
  CButton version_;

  // Generated message map functions
  BOOL OnInitDialog() override;
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  afx_msg void OnVersionButtonClicked();
  DECLARE_MESSAGE_MAP()
};
