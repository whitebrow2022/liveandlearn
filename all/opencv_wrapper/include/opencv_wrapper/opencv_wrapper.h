// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "opencv_wrapper/opencv_wrapper_export.h"

BEGIN_NAMESPACE_OPENCV_WRAPPER

OPENCV_WRAPPER_API const char* GetVersion();
OPENCV_WRAPPER_API const char* GetOsName();
OPENCV_WRAPPER_API const char* GetName();

OPENCV_WRAPPER_API void ExtractVideoToFrames(const char* video_file_path,
                                             const char* output_dir);
OPENCV_WRAPPER_API bool ExtractVideoFirstValidFrameToImage(
    const char* video_file_path, const char* image_file_path);

// argb 32bit image
struct CvImage final {
  ~CvImage() = default;
  CvImage(int width, int height) {
    this->width = width;
    this->height = height;
    image_data = std::make_unique<unsigned char[]>(width * height * 4);
  }

  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  bool GetColor(int col, int row, unsigned char* r, unsigned char* g,
                unsigned char* b, unsigned char* a) const {
    if (!(r && g && b && a)) {
      return false;
    }
    if (col < 0 || col >= width || row < 0 || row >= height) {
      return false;
    }
    int color_pos = row * width * 4 + col * 4;
    *r = image_data.get()[color_pos];
    *g = image_data.get()[color_pos + 1];
    *b = image_data.get()[color_pos + 2];
    *a = image_data.get()[color_pos + 3];
    return true;
  }

  void SetColor(int col, int row, unsigned char r, unsigned char g,
                unsigned char b, unsigned char a) {
    int color_pos = row * width * 4 + col * 4;
    image_data.get()[color_pos] = r;
    image_data.get()[color_pos + 1] = g;
    image_data.get()[color_pos + 2] = b;
    image_data.get()[color_pos + 3] = a;
  }

 private:
  int width{0};
  int height{0};
  std::unique_ptr<unsigned char[]> image_data;

 private:
  CvImage(const CvImage&) = delete;
  CvImage& operator=(const CvImage&) = delete;
};
OPENCV_WRAPPER_API std::unique_ptr<CvImage>
ExtractVideoFirstValidFrameToImageBuffer(const char* video_file_path,
                                         unsigned int* duration);

//
OPENCV_WRAPPER_API bool ConvertVideoToMp4(const char* src_file_path,
                                          const char* dest_file_path);

END_NAMESPACE_OPENCV_WRAPPER
