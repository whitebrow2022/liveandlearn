/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "hello_skia.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkSurface.h"
#include "include/effects/SkGradientShader.h"

using namespace sk_app;

Application* Application::Create(int argc, char** argv, void* platformData) {
  return new HelloSkia(argc, argv, platformData);
}

HelloSkia::HelloSkia(int argc, char** argv, void* platformData)
    : fBackendType(Window::kRaster_BackendType), fRotationAngle(0) {
  SkGraphics::Init();

  fWindow = Window::CreateNativeWindow(platformData);
  fWindow->setRequestedDisplayParams(DisplayParams());

  // register callbacks
  fWindow->pushLayer(this);

  fWindow->attach(fBackendType);
}

HelloSkia::~HelloSkia() {
  fWindow->detach();
  delete fWindow;
}

void HelloSkia::updateTitle() {
  if (!fWindow || fWindow->sampleCount() <= 1) {
    return;
  }

  SkString title("Hello World ");
  title.append(Window::kRaster_BackendType == fBackendType ? "Raster"
                                                           : "OpenGL");
  fWindow->setTitle(title.c_str());
}

void HelloSkia::onBackendCreated() {
  this->updateTitle();
  fWindow->show();
  fWindow->inval();
}

void HelloSkia::onPaint(SkSurface* surface) {
  SkCanvas* canvas = surface->getCanvas();

  // Clear background
  canvas->clear(SK_ColorWHITE);

  SkPaint paint;
  paint.setColor(SK_ColorRED);

  // Draw a rectangle with red paint
  SkRect rect = SkRect::MakeXYWH(10, 10, 128, 128);
  canvas->drawRect(rect, paint);

  // Set up a linear gradient and draw a circle
  {
    SkPoint linearPoints[] = {{0, 0}, {300, 300}};
    SkColor linearColors[] = {SK_ColorGREEN, SK_ColorBLACK};
    paint.setShader(SkGradientShader::MakeLinear(
        linearPoints, linearColors, nullptr, 2, SkTileMode::kMirror));
    paint.setAntiAlias(true);

    canvas->drawCircle(200, 200, 64, paint);

    // Detach shader
    paint.setShader(nullptr);
  }

  // Draw a message with a nice black paint
  SkFont font;
  font.setSubpixel(true);
  font.setSize(20);
  paint.setColor(SK_ColorBLACK);

  canvas->save();
  static const char message[] = "Hello World ";

  // Translate and rotate
  canvas->translate(300, 300);
  fRotationAngle += 0.2f;
  if (fRotationAngle > 360) {
    fRotationAngle -= 360;
  }
  canvas->rotate(fRotationAngle);

  // Draw the text
  canvas->drawSimpleText(message, strlen(message), SkTextEncoding::kUTF8, 0, 0,
                         font, paint);

  canvas->restore();
}

void HelloSkia::onIdle() {
  // Just re-paint continuously
  fWindow->inval();
}

bool HelloSkia::onChar(SkUnichar c, skui::ModifierKey modifiers) {
  if (' ' == c) {
    fBackendType = Window::kRaster_BackendType == fBackendType
                       ? Window::kNativeGL_BackendType
                       : Window::kRaster_BackendType;
    fWindow->detach();
    fWindow->attach(fBackendType);
  }
  return true;
}
