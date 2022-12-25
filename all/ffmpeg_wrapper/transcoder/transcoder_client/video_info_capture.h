// Created by liangxu on 2022/11/16.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class VideoInfoCapture {
 public:
  // help function
  static std::vector<std::string> AvailableVideoEncoders();
  static std::vector<std::string> AvailableAudioEncoders();
  static std::vector<std::string> AvailableFileFormats();

 public:
  // argb 32bit image
  struct Image final {
    ~Image() { printf("destroy image\n"); }
    Image(int width, int height) {
      printf("contruct image\n");
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
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
  };

  static std::unique_ptr<Image> ExtractVideoFirstValidFrameToImageBuffer(
      const char* video_file_path, unsigned int* duration);

  struct FileInfo {
    std::unordered_map<int, int64_t> audio_stream_bitrates;
    std::unordered_map<int, int64_t> video_stream_bitrates;
  };
  static FileInfo ExtractFileInfo(const char* src_filename);

  struct VideoInfo {
    std::string thumb_image_path;
    int thumb_image_width{0};
    int thumb_image_height{0};
    int thumb_image_size{0};
    std::string video_path;
    int video_duration{0}; // msec
    int video_width{0};
    int video_height{0};
    int video_size{0};
  };
  static bool ExtractVideInfo(const char* src_file_path, VideoInfo* video_info = nullptr);
};
