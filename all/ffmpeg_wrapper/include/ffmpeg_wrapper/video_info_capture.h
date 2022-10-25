#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "ffmpeg_wrapper/ffmpeg_wrapper_export.h"

class FFMPEG_WRAPPER_API VideoInfoCapture {
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
};
