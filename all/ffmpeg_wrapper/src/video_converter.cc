#include "ffmpeg_wrapper/video_converter.h"

#include "ffmpeg_video_converter.h"

class VideoConverterWrapper : public VideoConverter {
 public:
  explicit VideoConverterWrapper(std::shared_ptr<BaseVideoConverter> converter)
      : converter_(converter) {}

  void Start() override { converter_->Start(); }
  void Stop() override { converter_->Stop(); }

 private:
  std::shared_ptr<BaseVideoConverter> converter_;
};

std::unique_ptr<VideoConverter> VideoConverter::MakeVideoConverter(
    const VideoConverter::Request& request, VideoConverter::Delegate* delegate,
    AsyncCallFuncType async_call_fun, const char* converter_name) {
  if (strcmp(converter_name, "ffmpeg") == 0) {
    return std::make_unique<VideoConverterWrapper>(
        std::make_unique<FfmpegVideoConverter>(request, delegate,
                                               async_call_fun));
  }
  // TODO: 支持其它视频格式转换器
  return nullptr;
}


std::vector<std::string> VideoConverter::AvailableVideoEncoders(
    const std::string& converter_name /* = "ffmpeg"*/) {
  std::vector<std::string> video_encoders;
  if (converter_name == "ffmpeg") {
    video_encoders = FfmpegVideoConverter::AvailableVideoEncoders();
  }
  return video_encoders;
}
std::vector<std::string> VideoConverter::AvailableAudioEncoders(
  const std::string& converter_name/* = "ffmpeg"*/) {
  std::vector<std::string> audio_encoders;
  if (converter_name == "ffmpeg") {
    audio_encoders = FfmpegVideoConverter::AvailableAudioEncoders();
  }
  return audio_encoders;
}
std::vector<std::string> VideoConverter::AvailableFileFormats(
  const std::string& converter_name/* = "ffmpeg"*/) {
  std::vector<std::string> file_formats;
  if (converter_name == "ffmpeg") {
    file_formats = FfmpegVideoConverter::AvailableFileFormats();
  }
  return file_formats;
}
