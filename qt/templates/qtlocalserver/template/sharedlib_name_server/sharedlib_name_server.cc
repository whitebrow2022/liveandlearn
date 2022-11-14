// Created by %username% on %date%.
//
// Copyright (c) %year% The %SharedlibName% Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QApplication>
#include <QDebug>
#include <QSettings>
#include <QTranslator>

#include "sharedlib_name/sharedlib_name_export.h"
#include "sharedlib_name_server_frame.h"

extern "C" {

SHAREDLIB_NAME_API  void* Run(int argc, char* argv[]) {
  SharedlibNameServerFrame* w = new SharedlibNameServerFrame();
  w->show();
  return w;
}

}
