#pragma once

#include <memory>

#include "ffmpeg_wrapper/video_converter.h"

class BaseVideoConverter
    : public std::enable_shared_from_this<BaseVideoConverter> {
 public:
  BaseVideoConverter(const VideoConverter::Request& request,
                     VideoConverter::Delegate* delegate,
                     VideoConverter::AsyncCallFuncType call_fun)
      : input_file_(request.input_file),
        output_file_(request.output_file),
        delegate_(delegate),
        async_call_fun_(call_fun) {}
  
  virtual ~BaseVideoConverter() {}

  virtual void Start() = 0;
  virtual void Stop() = 0;

 protected:
  void PostConvertBegin(VideoConverter::Response r);
  void PostConvertProgress(VideoConverter::Response r);
  void PostConvertEnd(VideoConverter::Response r);

 protected:
  std::string input_file_;
  std::string output_file_;

 protected:
  VideoConverter::Delegate* delegate_{nullptr};
  VideoConverter::AsyncCallFuncType async_call_fun_;
};
