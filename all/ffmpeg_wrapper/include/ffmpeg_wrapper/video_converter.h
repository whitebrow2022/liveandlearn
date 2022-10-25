#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ffmpeg_wrapper/ffmpeg_wrapper_export.h"

class FFMPEG_WRAPPER_API VideoConverter {
 public:
  // help function
  static std::vector<std::string> AvailableVideoEncoders(
      const std::string& converter_name = "ffmpeg");
  static std::vector<std::string> AvailableAudioEncoders(
      const std::string& converter_name = "ffmpeg");
  static std::vector<std::string> AvailableFileFormats(
      const std::string& converter_name = "ffmpeg");

 public:
  struct Request {
    std::string input_file;
    std::string output_file;
    std::string output_file_format;
    std::string video_encoder;
    std::string audio_encoder;
    int output_video_width{-1};
    int output_video_height{-1};
    uint32_t output_video_start_time{0};  // 单位毫秒，从什么时间点开始转码
    uint32_t output_video_record_time{0};  // 单位毫秒，转码多长时间
    std::string output_video_bitrate{};
    std::string output_audio_bitrate{};
    int threads{0};
  };

 public:
  struct Response {
    std::string output_file;
  };
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void OnConvertBegin(Response r) {}
    virtual void OnConvertProgress(Response r) {}
    virtual void OnConvertEnd(Response r) {}
  };
  using AsyncCallFuncType = std::function<void(std::function<void()>)>;

 public:
  virtual ~VideoConverter() {}
  virtual void Start() = 0;
  virtual void Stop() = 0;

 public:
  static std::unique_ptr<VideoConverter> MakeVideoConverter(
      const VideoConverter::Request& request,
      VideoConverter::Delegate* delegate, AsyncCallFuncType async_call_fun,
      const char* converter_name = "ffmpeg");
};
