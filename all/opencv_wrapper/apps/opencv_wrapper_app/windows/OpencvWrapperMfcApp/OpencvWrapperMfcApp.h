// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// OpencvWrapperMfcApp.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"  // main symbols

// COpencvWrapperMfcAppApp:
// See OpencvWrapperMfcApp.cpp for the implementation of this class
//

class COpencvWrapperMfcAppApp : public CWinApp {
 public:
  COpencvWrapperMfcAppApp();

  // Overrides
 public:
  virtual BOOL InitInstance();

  // Implementation

  DECLARE_MESSAGE_MAP()
};

extern COpencvWrapperMfcAppApp theApp;
