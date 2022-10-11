// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// SharedlibNameMfcApp.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"  // main symbols

// CSharedlibNameMfcAppApp:
// See SharedlibNameMfcApp.cpp for the implementation of this class
//

class CSharedlibNameMfcAppApp : public CWinApp {
 public:
  CSharedlibNameMfcAppApp();

  // Overrides
 public:
  virtual BOOL InitInstance();

  // Implementation

  DECLARE_MESSAGE_MAP()
};

extern CSharedlibNameMfcAppApp theApp;
