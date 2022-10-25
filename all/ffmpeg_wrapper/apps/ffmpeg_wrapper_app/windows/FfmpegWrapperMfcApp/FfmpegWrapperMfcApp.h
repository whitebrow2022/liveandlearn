// Created by liangxu on 2022/10/25.
//
// Copyright (c) 2022 The FfmpegWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// FfmpegWrapperMfcApp.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"  // main symbols

// CFfmpegWrapperMfcAppApp:
// See FfmpegWrapperMfcApp.cpp for the implementation of this class
//

class CFfmpegWrapperMfcAppApp : public CWinApp {
 public:
  CFfmpegWrapperMfcAppApp();

  // Overrides
 public:
  virtual BOOL InitInstance();

  // Implementation

  DECLARE_MESSAGE_MAP()
};

extern CFfmpegWrapperMfcAppApp theApp;
