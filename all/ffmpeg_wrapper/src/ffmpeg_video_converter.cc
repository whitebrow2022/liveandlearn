#include "ffmpeg_video_converter.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_IO_H
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/version.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavformat/avformat.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/avutil.h"
#include "libavutil/bprint.h"
#include "libavutil/channel_layout.h"
#include "libavutil/dict.h"
#include "libavutil/display.h"
#include "libavutil/fifo.h"
#include "libavutil/hwcontext.h"
#include "libavutil/imgutils.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/parseutils.h"
#include "libavutil/pixdesc.h"
#include "libavutil/samplefmt.h"
#include "libavutil/threadmessage.h"
#include "libavutil/time.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"

#ifdef __cplusplus
}
#endif

#include "ffmpeg_util.h"

#if defined(WIN32)
#include <Windows.h>
#define HAVE_GETPROCESSTIMES 1
#endif()

#include <sstream>

#define HAVE_GETRUSAGE 0

namespace {

const char *const forced_keyframes_const_names[] = {
    "n", "n_forced", "prev_forced_n", "prev_forced_t", "t", nullptr};

}

// hw
namespace {
int nb_hw_devices{0};
HWDevice **hw_devices{nullptr};
HWDevice *filter_hw_device{nullptr};

HWDevice *hw_device_get_by_type(AVHWDeviceType type) {
  HWDevice *found = nullptr;
  for (int i = 0; i < nb_hw_devices; i++) {
    if (hw_devices[i]->type == type) {
      if (found) {
        return nullptr;
      }
      found = hw_devices[i];
    }
  }
  return found;
}

HWDevice *hw_device_get_by_name(const char *name) {
  for (int i = 0; i < nb_hw_devices; i++) {
    if (!strcmp(hw_devices[i]->name, name)) {
      return hw_devices[i];
    }
  }
  return nullptr;
}

HWDevice *hw_device_add(void) {
  int err =
      av_reallocp_array(&hw_devices, nb_hw_devices + 1, sizeof(*hw_devices));
  if (err) {
    nb_hw_devices = 0;
    return nullptr;
  }
  hw_devices[nb_hw_devices] =
      static_cast<HWDevice *>(av_mallocz(sizeof(HWDevice)));
  if (!hw_devices[nb_hw_devices]) {
    return nullptr;
  }
  return hw_devices[nb_hw_devices++];
}

char *hw_device_default_name(AVHWDeviceType type) {
  // Make an automatic name of the form "type%d".  We arbitrarily
  // limit at 1000 anonymous devices of the same type - there is
  // probably something else very wrong if you get to this limit.
  const char *type_name = av_hwdevice_get_type_name(type);
  size_t index_pos = strlen(type_name);
  char *name = static_cast<char *>(av_malloc(index_pos + 4));
  if (!name) {
    return nullptr;
  }
  constexpr const int index_limit = 1000;
  int index = 0;
  for (index = 0; index < index_limit; index++) {
    snprintf(name, index_pos + 4, "%s%d", type_name, index);
    if (!hw_device_get_by_name(name)) break;
  }
  if (index >= index_limit) {
    av_freep(&name);
    return nullptr;
  }
  return name;
}

int hw_device_init_from_string(const char *arg, HWDevice **dev_out) {
  // "type=name"
  // "type=name,key=value,key2=value2"
  // "type=name:device,key=value,key2=value2"
  // "type:device,key=value,key2=value2"
  // -> av_hwdevice_ctx_create()
  // "type=name@name"
  // "type@name"
  // -> av_hwdevice_ctx_create_derived()

  AVDictionary *options = nullptr;
  const char *name = nullptr, *device = nullptr;
  enum AVHWDeviceType type;
  HWDevice *dev, *src;
  AVBufferRef *device_ref = nullptr;
  int err;
  const char *errmsg, *q;

  size_t k = strcspn(arg, ":=@");
  const char *p = arg + k;

  const char *type_name = av_strndup(arg, k);
  if (!type_name) {
    err = AVERROR(ENOMEM);
    goto fail;
  }
  type = av_hwdevice_find_type_by_name(type_name);
  if (type == AV_HWDEVICE_TYPE_NONE) {
    errmsg = "unknown device type";
    goto invalid;
  }

  if (*p == '=') {
    k = strcspn(p + 1, ":@,");

    name = av_strndup(p + 1, k);
    if (!name) {
      err = AVERROR(ENOMEM);
      goto fail;
    }
    if (hw_device_get_by_name(name)) {
      errmsg = "named device already exists";
      goto invalid;
    }

    p += 1 + k;
  } else {
    name = hw_device_default_name(type);
    if (!name) {
      err = AVERROR(ENOMEM);
      goto fail;
    }
  }

  if (!*p) {
    // New device with no parameters.
    err = av_hwdevice_ctx_create(&device_ref, type, nullptr, nullptr, 0);
    if (err < 0) {
      goto fail;
    }
  } else if (*p == ':') {
    // New device with some parameters.
    ++p;
    q = strchr(p, ',');
    if (q) {
      if (q - p > 0) {
        device = av_strndup(p, q - p);
        if (!device) {
          err = AVERROR(ENOMEM);
          goto fail;
        }
      }
      err = av_dict_parse_string(&options, q + 1, "=", ",", 0);
      if (err < 0) {
        errmsg = "failed to parse options";
        goto invalid;
      }
    }

    err = av_hwdevice_ctx_create(&device_ref, type,
                                 q      ? device
                                 : p[0] ? p
                                        : nullptr,
                                 options, 0);
    if (err < 0) {
      goto fail;
    }
  } else if (*p == '@') {
    // Derive from existing device.

    src = hw_device_get_by_name(p + 1);
    if (!src) {
      errmsg = "invalid source device name";
      goto invalid;
    }

    err = av_hwdevice_ctx_create_derived(&device_ref, type, src->device_ref, 0);
    if (err < 0) {
      goto fail;
    }
  } else if (*p == ',') {
    err = av_dict_parse_string(&options, p + 1, "=", ",", 0);

    if (err < 0) {
      errmsg = "failed to parse options";
      goto invalid;
    }

    err = av_hwdevice_ctx_create(&device_ref, type, nullptr, options, 0);
    if (err < 0) {
      goto fail;
    }
  } else {
    errmsg = "parse error";
    goto invalid;
  }

  dev = hw_device_add();
  if (!dev) {
    err = AVERROR(ENOMEM);
    goto fail;
  }

  dev->name = name;
  dev->type = type;
  dev->device_ref = device_ref;

  if (dev_out) {
    *dev_out = dev;
  }
  name = nullptr;
  err = 0;
done:
  av_freep(&type_name);
  av_freep(&name);
  av_freep(&device);
  av_dict_free(&options);
  return err;
invalid:
  AvLog(nullptr, AV_LOG_ERROR, "Invalid device specification \"%s\": %s\n", arg,
        errmsg);
  err = AVERROR(EINVAL);
  goto done;
fail:
  AvLog(nullptr, AV_LOG_ERROR, "Device creation failed: %d.\n", err);
  av_buffer_unref(&device_ref);
  goto done;
}

int hw_device_init_from_type(enum AVHWDeviceType type, const char *device,
                             HWDevice **dev_out) {
  AVBufferRef *device_ref = nullptr;
  HWDevice *dev;
  int err;

  char *name = hw_device_default_name(type);
  if (!name) {
    err = AVERROR(ENOMEM);
    goto fail;
  }

  err = av_hwdevice_ctx_create(&device_ref, type, device, nullptr, 0);
  if (err < 0) {
    AvLog(nullptr, AV_LOG_ERROR, "Device creation failed: %d.\n", err);
    goto fail;
  }

  dev = hw_device_add();
  if (!dev) {
    err = AVERROR(ENOMEM);
    goto fail;
  }

  dev->name = name;
  dev->type = type;
  dev->device_ref = device_ref;

  if (dev_out) {
    *dev_out = dev;
  }
  return 0;

fail:
  av_freep(&name);
  av_buffer_unref(&device_ref);
  return err;
}

void hw_device_free_all(void) {
  for (int i = 0; i < nb_hw_devices; i++) {
    av_freep(&hw_devices[i]->name);
    av_buffer_unref(&hw_devices[i]->device_ref);
    av_freep(&hw_devices[i]);
  }
  av_freep(&hw_devices);
  nb_hw_devices = 0;
}

HWDevice *hw_device_match_by_codec(const AVCodec *codec) {
  for (int i = 0;; i++) {
    const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
    if (!config) {
      return nullptr;
    }
    if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)) {
      continue;
    }
    HWDevice *dev = hw_device_get_by_type(config->device_type);
    if (dev) {
      return dev;
    }
  }
}

int hw_device_setup_for_decode(InputStream *ist) {
  const AVCodecHWConfig *config;
  enum AVHWDeviceType type;
  HWDevice *dev = nullptr;
  int err;
  bool auto_device = false;

  if (ist->hwaccel_device) {
    dev = hw_device_get_by_name(ist->hwaccel_device);
    if (!dev) {
      if (ist->hwaccel_id == HWACCEL_AUTO) {
        auto_device = true;
      } else if (ist->hwaccel_id == HWACCEL_GENERIC) {
        type = ist->hwaccel_device_type;
        err = hw_device_init_from_type(type, ist->hwaccel_device, &dev);
      } else {
        // This will be dealt with by API-specific initialisation
        // (using hwaccel_device), so nothing further needed here.
        return 0;
      }
    } else {
      if (ist->hwaccel_id == HWACCEL_AUTO) {
        ist->hwaccel_device_type = dev->type;
      } else if (ist->hwaccel_device_type != dev->type) {
        AvLog(ist->dec_ctx, AV_LOG_ERROR,
              "Invalid hwaccel device "
              "specified for decoder: device %s of type %s is not "
              "usable with hwaccel %s.\n",
              dev->name, av_hwdevice_get_type_name(dev->type),
              av_hwdevice_get_type_name(ist->hwaccel_device_type));
        return AVERROR(EINVAL);
      }
    }
  } else {
    if (ist->hwaccel_id == HWACCEL_AUTO) {
      auto_device = true;
    } else if (ist->hwaccel_id == HWACCEL_GENERIC) {
      type = ist->hwaccel_device_type;
      dev = hw_device_get_by_type(type);

      // When "-qsv_device device" is used, an internal QSV device named
      // as "__qsv_device" is created. Another QSV device is created too
      // if "-init_hw_device qsv=name:device" is used. There are 2 QSV devices
      // if both "-qsv_device device" and "-init_hw_device qsv=name:device"
      // are used, hw_device_get_by_type(AV_HWDEVICE_TYPE_QSV) returns nullptr.
      // To keep back-compatibility with the removed ad-hoc libmfx setup code,
      // call hw_device_get_by_name("__qsv_device") to select the internal QSV
      // device.
      if (!dev && type == AV_HWDEVICE_TYPE_QSV) {
        dev = hw_device_get_by_name("__qsv_device");
      }

      if (!dev) {
        err = hw_device_init_from_type(type, nullptr, &dev);
      }
    } else {
      dev = hw_device_match_by_codec(ist->dec);
      if (!dev) {
        // No device for this codec, but not using generic hwaccel
        // and therefore may well not need one - ignore.
        return 0;
      }
    }
  }

  if (auto_device) {
    if (!avcodec_get_hw_config(ist->dec, 0)) {
      // Decoder does not support any hardware devices.
      return 0;
    }
    for (int i = 0; !dev; i++) {
      config = avcodec_get_hw_config(ist->dec, i);
      if (!config) {
        break;
      }
      type = config->device_type;
      dev = hw_device_get_by_type(type);
      if (dev) {
        AvLog(ist->dec_ctx, AV_LOG_INFO,
              "Using auto "
              "hwaccel type %s with existing device %s.\n",
              av_hwdevice_get_type_name(type), dev->name);
      }
    }
    for (int i = 0; !dev; i++) {
      config = avcodec_get_hw_config(ist->dec, i);
      if (!config) {
        break;
      }
      type = config->device_type;
      // Try to make a new device of this type.
      err = hw_device_init_from_type(type, ist->hwaccel_device, &dev);
      if (err < 0) {
        // Can't make a device of this type.
        continue;
      }
      if (ist->hwaccel_device) {
        AvLog(ist->dec_ctx, AV_LOG_INFO,
              "Using auto "
              "hwaccel type %s with new device created "
              "from %s.\n",
              av_hwdevice_get_type_name(type), ist->hwaccel_device);
      } else {
        AvLog(ist->dec_ctx, AV_LOG_INFO,
              "Using auto "
              "hwaccel type %s with new default device.\n",
              av_hwdevice_get_type_name(type));
      }
    }
    if (dev) {
      ist->hwaccel_device_type = type;
    } else {
      AvLog(ist->dec_ctx, AV_LOG_INFO,
            "Auto hwaccel "
            "disabled: no device found.\n");
      ist->hwaccel_id = HWACCEL_NONE;
      return 0;
    }
  }

  if (!dev) {
    AvLog(ist->dec_ctx, AV_LOG_ERROR,
          "No device available "
          "for decoder: device type %s needed for codec %s.\n",
          av_hwdevice_get_type_name(type), ist->dec->name);
    return err;
  }

  ist->dec_ctx->hw_device_ctx = av_buffer_ref(dev->device_ref);
  if (!ist->dec_ctx->hw_device_ctx) {
    return AVERROR(ENOMEM);
  }
  return 0;
}

int hw_device_setup_for_encode(OutputStream *ost) {
  const AVCodecHWConfig *config;
  HWDevice *dev = nullptr;
  AVBufferRef *frames_ref = nullptr;

  if (ost->filter) {
    frames_ref = av_buffersink_get_hw_frames_ctx(ost->filter->filter);
    if (frames_ref && ((AVHWFramesContext *)frames_ref->data)->format ==
                          ost->enc_ctx->pix_fmt) {
      // Matching format, will try to use hw_frames_ctx.
    } else {
      frames_ref = nullptr;
    }
  }

  for (int i = 0;; i++) {
    config = avcodec_get_hw_config(ost->enc, i);
    if (!config) {
      break;
    }
    if (frames_ref &&
        config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_FRAMES_CTX &&
        (config->pix_fmt == AV_PIX_FMT_NONE ||
         config->pix_fmt == ost->enc_ctx->pix_fmt)) {
      AvLog(ost->enc_ctx, AV_LOG_VERBOSE,
            "Using input "
            "frames context (format %s) with %s encoder.\n",
            av_get_pix_fmt_name(ost->enc_ctx->pix_fmt), ost->enc->name);
      ost->enc_ctx->hw_frames_ctx = av_buffer_ref(frames_ref);
      if (!ost->enc_ctx->hw_frames_ctx) {
        return AVERROR(ENOMEM);
      }
      return 0;
    }

    if (!dev && config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) {
      dev = hw_device_get_by_type(config->device_type);
    }
  }

  if (dev) {
    AvLog(ost->enc_ctx, AV_LOG_VERBOSE,
          "Using device %s "
          "(type %s) with %s encoder.\n",
          dev->name, av_hwdevice_get_type_name(dev->type), ost->enc->name);
    ost->enc_ctx->hw_device_ctx = av_buffer_ref(dev->device_ref);
    if (!ost->enc_ctx->hw_device_ctx) {
      return AVERROR(ENOMEM);
    }
  } else {
    // No device required, or no device available.
  }
  return 0;
}

int hwaccel_retrieve_data(AVCodecContext *avctx, AVFrame *input) {
  InputStream *ist = static_cast<InputStream *>(avctx->opaque);
  AVFrame *output = nullptr;
  AVPixelFormat output_format = ist->hwaccel_output_format;

  if (input->format == output_format) {
    // Nothing to do.
    return 0;
  }

  output = av_frame_alloc();
  if (!output) {
    return AVERROR(ENOMEM);
  }

  output->format = output_format;

  int err = av_hwframe_transfer_data(output, input, 0);
  if (err < 0) {
    AvLog(avctx, AV_LOG_ERROR,
          "Failed to transfer data to "
          "output frame: %d.\n",
          err);
    goto fail;
  }

  err = av_frame_copy_props(output, input);
  if (err < 0) {
    av_frame_unref(output);
    goto fail;
  }

  av_frame_unref(input);
  av_frame_move_ref(input, output);
  av_frame_free(&output);

  return 0;

fail:
  av_frame_free(&output);
  return err;
}

int hwaccel_decode_init(AVCodecContext *avctx) {
  InputStream *ist = static_cast<InputStream *>(avctx->opaque);

  ist->hwaccel_retrieve_data = &hwaccel_retrieve_data;

  return 0;
}

int hw_device_setup_for_filter(FilterGraph *fg) {
  HWDevice *dev;

  // Pick the last hardware device if the user doesn't pick the device for
  // filters explicitly with the filter_hw_device option.
  if (filter_hw_device) {
    dev = filter_hw_device;
  } else if (nb_hw_devices > 0) {
    dev = hw_devices[nb_hw_devices - 1];

    if (nb_hw_devices > 1) {
      AvLog(nullptr, AV_LOG_WARNING,
            "There are %d hardware devices. device "
            "%s of type %s is picked for filters by default. Set hardware "
            "device explicitly with the filter_hw_device option if device "
            "%s is not usable for filters.\n",
            nb_hw_devices, dev->name, av_hwdevice_get_type_name(dev->type),
            dev->name);
    }
  } else {
    dev = nullptr;
  }

  if (dev) {
    for (int i = 0; i < fg->graph->nb_filters; i++) {
      fg->graph->filters[i]->hw_device_ctx = av_buffer_ref(dev->device_ref);
      if (!fg->graph->filters[i]->hw_device_ctx) {
        return AVERROR(ENOMEM);
      }
    }
  }

  return 0;
}
}  // namespace

namespace {
AVPixelFormat get_format(AVCodecContext *s,
                         const enum AVPixelFormat *pix_fmts) {
  InputStream *ist = static_cast<InputStream *>(s->opaque);
  const AVPixelFormat *p;

  for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(*p);
    if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) {
      break;
    }

    const AVCodecHWConfig *config = nullptr;
    if (ist->hwaccel_id == HWACCEL_GENERIC || ist->hwaccel_id == HWACCEL_AUTO) {
      for (int i = 0;; i++) {
        config = avcodec_get_hw_config(s->codec, i);
        if (!config) {
          break;
        }
        if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)) {
          continue;
        }
        if (config->pix_fmt == *p) {
          break;
        }
      }
    }
    if (config && config->device_type == ist->hwaccel_device_type) {
      int ret = hwaccel_decode_init(s);
      if (ret < 0) {
        if (ist->hwaccel_id == HWACCEL_GENERIC) {
          AvLog(nullptr, AV_LOG_FATAL,
                "%s hwaccel requested for input stream #%d:%d, "
                "but cannot be initialized.\n",
                av_hwdevice_get_type_name(config->device_type), ist->file_index,
                ist->st->index);
          return AV_PIX_FMT_NONE;
        }
        continue;
      }

      ist->hwaccel_pix_fmt = *p;
      break;
    }
  }

  return *p;
}

}  // namespace

namespace {
std::vector<std::string> AvailableEncoders(AVMediaType type) {
  std::vector<std::string> encoders;
  const AVCodecDescriptor *desc = nullptr;
  while ((desc = avcodec_descriptor_next(desc))) {
    if (strstr(desc->name, "_deprecated")) {
      continue;
    }
    if (desc->type != type) {
      continue;
    }
    if (!avcodec_find_encoder(desc->id)) {
      continue;
    }
    encoders.push_back(desc->name);
  }
  return encoders;
}
bool is_device(const AVClass *avclass) {
  if (!avclass) return false;
  return AV_IS_INPUT_DEVICE(avclass->category) ||
         AV_IS_OUTPUT_DEVICE(avclass->category);
}
}  // namespace

std::vector<std::string> FfmpegVideoConverter::AvailableVideoEncoders() {
  std::vector<std::string> video_encoders;
  video_encoders = AvailableEncoders(AVMEDIA_TYPE_VIDEO);
  return video_encoders;
}
std::vector<std::string> FfmpegVideoConverter::AvailableAudioEncoders() {
  std::vector<std::string> audio_encoders;
  std::vector<std::string> video_encoders;
  video_encoders = AvailableEncoders(AVMEDIA_TYPE_AUDIO);
  return video_encoders;
  return audio_encoders;
}
std::vector<std::string> FfmpegVideoConverter::AvailableFileFormats() {
  std::vector<std::string> file_formats;
  void *ofmt_opaque = nullptr;
  const AVOutputFormat *ofmt = nullptr;
  bool device_only = false;
  while ((ofmt = av_muxer_iterate(&ofmt_opaque))) {
    bool is_dev = is_device(ofmt->priv_class);
    if (!is_dev && device_only) continue;
    if (ofmt->name) {
      file_formats.push_back(ofmt->name);
    }
  }
  return file_formats;
}

FfmpegVideoConverter::FfmpegVideoConverter(
    const VideoConverter::Request &request, VideoConverter::Delegate *delegate,
    VideoConverter::AsyncCallFuncType call_fun)
    : BaseVideoConverter(request, delegate, call_fun) {
  oo.format = request.output_file_format;
  oo.vbitrate = request.output_video_bitrate;
  oo.abitrate = request.output_audio_bitrate;
  oo.codec_names.clear();
  oo.codec_names.push_back({kVideoStream, -1, request.video_encoder});
  oo.codec_names.push_back({kAudioStream, -1, request.audio_encoder});
  if (request.output_video_start_time > 0) {
    oo.start_time = request.output_video_start_time * 1000;
  }
  if (request.output_video_record_time > 0) {
    oo.recording_time = request.output_video_record_time * 1000;
  }
  int w = request.output_video_width;
  int h = request.output_video_height;
  if (w > 0 || h > 0) {
    oo.video_filters.clear();
    std::stringstream ss;
    ss << "scale=" << (w > 0 ? w : -1) << ":" << (h > 0 ? h : -1);
    oo.video_filters = ss.str();
  }
  auto threads = request.threads;
  if (threads > 0) {
    io.threads = threads;
    oo.threads = threads;
  }
}

FfmpegVideoConverter::~FfmpegVideoConverter() {
  if (worker_) {
    worker_->join();
  }
}

void FfmpegVideoConverter::Start() {
  // 重置所有参数为caller给的值
  if (async_call_fun_) {
    worker_ = std::make_unique<std::thread>(FfmpegVideoConverter::Run, this);
  } else {
    FfmpegVideoConverter::Run(this);
  }
}
void FfmpegVideoConverter::Stop() {
  received_sigterm = true;
  if (worker_) {
    worker_->join();
  }
}

bool FfmpegVideoConverter::Convert(const std::string &input,
                                   const std::string &output) {
  bool ret = false;
  do {
    input_file_ = input;
    output_file_ = output;
    OpenInputFile();
    ApplySyncOffsets();
    InitComplexFilters();
    OpenOutputFile();
    check_filter_outputs();

    for (int i = 0; i < nb_output_files; i++) {
      if (strcmp(output_files[i]->format->name, "rtp")) {
        want_sdp = false;
      }
    }

    BenchmarkTimeStamps ti;
    current_time = ti = get_benchmark_time_stamps();
    if (transcode() < 0) {
      ret = false;
      break;
    }
    if (do_benchmark) {
      int64_t utime, stime, rtime;
      current_time = get_benchmark_time_stamps();
      utime = current_time.user_usec - ti.user_usec;
      stime = current_time.sys_usec - ti.sys_usec;
      rtime = current_time.real_usec - ti.real_usec;
      AvLog(NULL, AV_LOG_INFO,
            "bench: utime=%0.3fs stime=%0.3fs rtime=%0.3fs\n",
            utime / 1000000.0, stime / 1000000.0, rtime / 1000000.0);
    }
    AvLog(NULL, AV_LOG_DEBUG,
          "%llu frames successfully decoded, %llu decoding errors\n",
          decode_error_stat[0], decode_error_stat[1]);
    if ((decode_error_stat[0] + decode_error_stat[1]) * max_error_rate <
        decode_error_stat[1]) {
      ret = false;
      break;
    }
  } while (false);

  ret = true;
  cleanup(ret);
  return ret;
}

bool FfmpegVideoConverter::OpenInputFile() {
  // 初始化配置选项
  io.Init();
  int ret = open_input_file(input_file_.c_str());
  io.UnInit();
  // 重置配置选项
  return ret == 0;
}
bool FfmpegVideoConverter::ApplySyncOffsets() { return apply_sync_offsets(); }
bool FfmpegVideoConverter::InitComplexFilters() {
  int ret = init_complex_filters();
  return ret == 0;
}
bool FfmpegVideoConverter::OpenOutputFile() {
  // 初始化配置选项
  oo.Init();
  int ret = open_output_file(output_file_.c_str());
  oo.UnInit();
  // 重置配置选项
  return ret == 0;
}

int FfmpegVideoConverter::open_input_file(const char *filename) {
  if (io.stop_time != INT64_MAX && io.recording_time != INT64_MAX) {
    io.stop_time = INT64_MAX;
    AvLog(NULL, AV_LOG_WARNING,
          "-t and -to cannot be used together; using -t.\n");
  }

  if (io.stop_time != INT64_MAX && io.recording_time == INT64_MAX) {
    int64_t start_time = io.start_time == AV_NOPTS_VALUE ? 0 : io.start_time;
    if (io.stop_time <= start_time) {
      AvLog(NULL, AV_LOG_ERROR, "-to value smaller than -ss; aborting.\n");
      return -1;
    } else {
      io.recording_time = io.stop_time - start_time;
    }
  }

  const AVInputFormat *file_iformat = nullptr;
  if (!io.format.empty()) {
    if (!(file_iformat = av_find_input_format(io.format.c_str()))) {
      AvLog(nullptr, AV_LOG_FATAL, "Unknown input format: '%s'\n",
            io.format.c_str());
      return -1;
    }
  }

  /* get default parameters from command line */
  AVFormatContext *ic = avformat_alloc_context();
  if (!ic) {
    PrintError(filename, AVERROR(ENOMEM));
    return ENOMEM;
  }

  if (io.nb_audio_sample_rate > 0) {
    // TODO(xiufeng.liu)
  }
  if (io.nb_audio_channels > 0) {
    // TODO(xiufeng.liu)
  }
  if (io.nb_audio_ch_layouts) {
    // TODO(xiufeng.liu)
  }
  if (io.nb_frame_rates) {
    /* set the format-level framerate option;
     * this is important for video grabbers, e.g. x11 */
    // TODO(xiufeng.liu)
  }
  if (io.nb_frame_sizes) {
    // TODO(xiufeng.liu)
  }
  if (io.nb_frame_pix_fmts) {
    // TODO(xiufeng.liu)
  }

  // TODO(xiufeng.liu): check codecs
  char *video_codec_name = nullptr;
  char *audio_codec_name = nullptr;
  char *subtitle_codec_name = nullptr;
  char *data_codec_name = nullptr;
  ic->video_codec_id =
      video_codec_name ? ic->video_codec->id : AV_CODEC_ID_NONE;
  ic->audio_codec_id =
      audio_codec_name ? ic->audio_codec->id : AV_CODEC_ID_NONE;
  ic->subtitle_codec_id =
      subtitle_codec_name ? ic->subtitle_codec->id : AV_CODEC_ID_NONE;
  ic->data_codec_id = data_codec_name ? ic->data_codec->id : AV_CODEC_ID_NONE;

  ic->flags |= AVFMT_FLAG_NONBLOCK;
  if (io.bitexact) {
    ic->flags |= AVFMT_FLAG_BITEXACT;
  }

  ic->interrupt_callback.callback = decode_interrupt_cb;
  ic->interrupt_callback.opaque = this;

  bool scan_all_pmts_set = false;
  if (!av_dict_get(io.format_opts, "scan_all_pmts", nullptr,
                   AV_DICT_MATCH_CASE)) {
    av_dict_set(&io.format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    scan_all_pmts_set = true;
  }

  /* open the input file with generic avformat function */
  int err = avformat_open_input(&ic, filename, file_iformat, &io.format_opts);
  if (err < 0) {
    PrintError(filename, err);
    if (err == AVERROR_PROTOCOL_NOT_FOUND) {
      AvLog(nullptr, AV_LOG_ERROR, "Did you mean file:%s?\n", filename);
    }
    return err;
  }
  if (scan_all_pmts_set) {
    av_dict_set(&io.format_opts, "scan_all_pmts", nullptr, AV_DICT_MATCH_CASE);
  }

  /* apply forced codec ids */
  for (int i = 0; i < ic->nb_streams; i++) {
    choose_decoder(ic, ic->streams[i]);
  }

  if (find_stream_info) {
    AVDictionary **opts = setup_find_stream_info_opts(ic, io.codec_opts);
    int orig_nb_streams = ic->nb_streams;

    /* If not enough info to get the stream parameters, we decode the
           first frames to get it. (used in mpeg case for example) */
    int ret = avformat_find_stream_info(ic, opts);

    for (int i = 0; i < orig_nb_streams; i++) {
      av_dict_free(&opts[i]);
    }
    av_freep(&opts);

    if (ret < 0) {
      AvLog(nullptr, AV_LOG_FATAL, "%s: could not find codec parameters\n",
            filename);
      if (ic->nb_streams == 0) {
        avformat_close_input(&ic);
        return ret;
      }
    }
  }

  if (io.start_time != AV_NOPTS_VALUE && io.start_time_eof != AV_NOPTS_VALUE) {
    AvLog(nullptr, AV_LOG_WARNING,
          "Cannot use -ss and -sseof both, using -ss for %s\n", filename);
    io.start_time_eof = AV_NOPTS_VALUE;
  }

  if (io.start_time_eof != AV_NOPTS_VALUE) {
    if (io.start_time_eof >= 0) {
      AvLog(nullptr, AV_LOG_ERROR, "-sseof value must be negative; aborting\n");
      return -1;
    }
    if (ic->duration > 0) {
      io.start_time = io.start_time_eof + ic->duration;
      if (io.start_time < 0) {
        AvLog(nullptr, AV_LOG_WARNING,
              "-sseof value seeks to before start of file %s; ignored\n",
              filename);
        io.start_time = AV_NOPTS_VALUE;
      }
    } else {
      AvLog(nullptr, AV_LOG_WARNING,
            "Cannot use -sseof, duration of %s not known\n", filename);
    }
  }
  int64_t timestamp = (io.start_time == AV_NOPTS_VALUE) ? 0 : io.start_time;
  /* add the stream start time */
  if (!io.seek_timestamp && ic->start_time != AV_NOPTS_VALUE) {
    timestamp += ic->start_time;
  }

  /* if seeking requested, we execute it */
  if (io.start_time != AV_NOPTS_VALUE) {
    int64_t seek_timestamp = timestamp;

    if (!(ic->iformat->flags & AVFMT_SEEK_TO_PTS)) {
      bool dts_heuristic = false;
      for (int i = 0; i < ic->nb_streams; i++) {
        const AVCodecParameters *par = ic->streams[i]->codecpar;
        if (par->video_delay) {
          dts_heuristic = true;
          break;
        }
      }
      if (dts_heuristic) {
        seek_timestamp -= 3 * AV_TIME_BASE / 23;
      }
    }
    int ret = avformat_seek_file(ic, -1, INT64_MIN, seek_timestamp,
                                 seek_timestamp, 0);
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
            filename, (double)timestamp / AV_TIME_BASE);
    }
  }

  /* update the current parameters so that they match the one of the input
   * stream */
  add_input_streams(ic);

  /* dump the file content */
  av_dump_format(ic, nb_input_files, filename, 0);

  InputFile *f = static_cast<InputFile *>(allocate_array_elem(
      &input_files, sizeof(*input_files[0]), &nb_input_files));

  f->ctx = ic;
  f->ist_index = nb_input_streams - ic->nb_streams;
  f->start_time = io.start_time;
  f->recording_time = io.recording_time;
  f->input_sync_ref = io.input_sync_ref;
  f->input_ts_offset = io.input_ts_offset;
  f->ts_offset = io.input_ts_offset -
                 (copy_ts ? (start_at_zero && ic->start_time != AV_NOPTS_VALUE
                                 ? ic->start_time
                                 : 0)
                          : timestamp);
  f->nb_streams = ic->nb_streams;
  f->rate_emu = io.rate_emu;
  f->accurate_seek = io.accurate_seek;
  f->loop = io.loop;
  f->duration = 0;
  f->time_base = {1, 1};

  f->readrate = io.readrate ? io.readrate : 0.0;
  if (f->readrate < 0.0f) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Option -readrate for Input #%d is %0.3f; it must be non-negative.\n",
          nb_input_files, f->readrate);
    return -1;
  }
  if (f->readrate && f->rate_emu) {
    AvLog(nullptr, AV_LOG_WARNING,
          "Both -readrate and -re set for Input #%d. Using -readrate %0.3f.\n",
          nb_input_files, f->readrate);
    f->rate_emu = false;
  }

  f->pkt = av_packet_alloc();
  if (!f->pkt) {
    return ENOMEM;
  }

  /* check if all codec options have been used */
  const AVDictionaryEntry *e = nullptr;
  AVDictionary *unused_opts = strip_specifiers(io.codec_opts);
  for (int i = f->ist_index; i < nb_input_streams; i++) {
    e = nullptr;
    while ((e = av_dict_get(input_streams[i]->decoder_opts, "", e,
                            AV_DICT_IGNORE_SUFFIX))) {
      av_dict_set(&unused_opts, e->key, nullptr, 0);
    }
  }

  e = nullptr;
  while ((e = av_dict_get(unused_opts, "", e, AV_DICT_IGNORE_SUFFIX))) {
    const AVClass *class_info = avcodec_get_class();
    const AVOption *option =
        av_opt_find(&class_info, e->key, nullptr, 0,
                    AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
    const AVClass *fclass = avformat_get_class();
    const AVOption *foption =
        av_opt_find(&fclass, e->key, nullptr, 0,
                    AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
    if (!option || foption) continue;

    if (!(option->flags & AV_OPT_FLAG_DECODING_PARAM)) {
      AvLog(nullptr, AV_LOG_ERROR,
            "Codec AVOption %s (%s) specified for "
            "input file #%d (%s) is not a decoding option.\n",
            e->key, option->help ? option->help : "", nb_input_files - 1,
            filename);
      return -1;
    }

    AvLog(nullptr, AV_LOG_WARNING,
          "Codec AVOption %s (%s) specified for "
          "input file #%d (%s) has not been used for any stream. The most "
          "likely reason is either wrong type (e.g. a video option with "
          "no video streams) or that it is a private option of some decoder "
          "which was not actually used for any stream.\n",
          e->key, option->help ? option->help : "", nb_input_files - 1,
          filename);
  }
  av_dict_free(&unused_opts);

  for (int i = 0; i < io.nb_dump_attachment; i++) {
    for (int j = 0; j < ic->nb_streams; j++) {
      // TODO(xiufeng.liu) attachments
      AVStream *st = ic->streams[j];
    }
  }

  input_stream_potentially_available = true;

  return 0;
}

/* Add all the streams from the given input file to the global
 * list of input streams. */
bool FfmpegVideoConverter::add_input_streams(AVFormatContext *ic) {
  for (int i = 0; i < ic->nb_streams; i++) {
    AVStream *st = ic->streams[i];
    AVCodecParameters *par = st->codecpar;
    InputStream *ist;
    char *next = nullptr;
    const AVClass *cc = avcodec_get_class();
    const AVOption *discard_opt =
        av_opt_find(&cc, "skip_frame", nullptr, 0, AV_OPT_SEARCH_FAKE_OBJ);

    ist = static_cast<InputStream *>(allocate_array_elem(
        &input_streams, sizeof(*input_streams[0]), &nb_input_streams));
    ist->st = st;
    ist->file_index = nb_input_files;
    ist->discard = true;
    st->discard = AVDISCARD_ALL;
    ist->nb_samples = 0;
    ist->first_dts = AV_NOPTS_VALUE;
    ist->min_pts = INT64_MAX;
    ist->max_pts = INT64_MIN;

    ist->ts_scale = 1.0;
    // MATCH_PER_STREAM_OPT(ts_scale, dbl, ist->ts_scale, ic, st);

    ist->autorotate = true;
    ist->autorotate = oo.autorotate;

    char *codec_tag = nullptr;
    // MATCH_PER_STREAM_OPT(codec_tags, str, codec_tag, ic, st);
    if (codec_tag) {
      uint32_t tag = strtol(codec_tag, &next, 0);
      if (*next) {
        tag = AV_RL32(codec_tag);
      }
      st->codecpar->codec_tag = tag;
    }

    ist->dec = choose_decoder(ic, st);
    ist->decoder_opts = filter_codec_opts(
        io.codec_opts, ist->st->codecpar->codec_id, ic, st, ist->dec);

    ist->reinit_filters = -1;
    // MATCH_PER_STREAM_OPT(reinit_filters, i, ist->reinit_filters, ic, st);

    char *discard_str = nullptr;
    // MATCH_PER_STREAM_OPT(discard, str, discard_str, ic, st);
    ist->user_set_discard = AVDISCARD_NONE;

    if ((io.video_disable &&
         ist->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) ||
        (io.audio_disable &&
         ist->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) ||
        (io.subtitle_disable &&
         ist->st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) ||
        (io.data_disable &&
         ist->st->codecpar->codec_type == AVMEDIA_TYPE_DATA)) {
      ist->user_set_discard = AVDISCARD_ALL;
    }
    if (discard_str &&
        av_opt_eval_int(&cc, discard_opt, discard_str,
                        reinterpret_cast<int *>(&ist->user_set_discard)) < 0) {
      AvLog(nullptr, AV_LOG_ERROR, "Error parsing discard %s.\n", discard_str);
      return false;
    }

    ist->filter_in_rescale_delta_last = AV_NOPTS_VALUE;
    ist->prev_pkt_pts = AV_NOPTS_VALUE;

    ist->dec_ctx = avcodec_alloc_context3(ist->dec);
    if (!ist->dec_ctx) {
      AvLog(nullptr, AV_LOG_ERROR, "Error allocating the decoder context.\n");
      return false;
    }

    int ret = avcodec_parameters_to_context(ist->dec_ctx, par);
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_ERROR, "Error initializing the decoder context.\n");
      return false;
    }

    ist->decoded_frame = av_frame_alloc();
    if (!ist->decoded_frame) {
      return false;
    }

    ist->pkt = av_packet_alloc();
    if (!ist->pkt) {
      return false;
    }

    if (io.bitexact) {
      ist->dec_ctx->flags |= AV_CODEC_FLAG_BITEXACT;
    }

    switch (par->codec_type) {
      case AVMEDIA_TYPE_VIDEO: {
        if (!ist->dec) {
          ist->dec = avcodec_find_decoder(par->codec_id);
        }
        // avformat_find_stream_info() doesn't set this for us anymore.
        ist->dec_ctx->framerate = st->avg_frame_rate;

        char *framerate = nullptr;
        // MATCH_PER_STREAM_OPT(frame_rates, str, framerate, ic, st);
        if (framerate && av_parse_video_rate(&ist->framerate, framerate) < 0) {
          AvLog(nullptr, AV_LOG_ERROR, "Error parsing framerate %s.\n",
                framerate);
          return false;
        }

        ist->top_field_first = -1;
        ist->top_field_first = o.top_field_first;

        const char *hwaccel = nullptr;
        // MATCH_PER_STREAM_OPT(hwaccels, str, hwaccel, ic, st);
        char *hwaccel_output_format = nullptr;
        // MATCH_PER_STREAM_OPT(hwaccel_output_formats, str,
        // hwaccel_output_format, ic, st);

        if (!hwaccel_output_format && hwaccel && !strcmp(hwaccel, "cuvid")) {
          AvLog(nullptr, AV_LOG_WARNING,
                "WARNING: defaulting hwaccel_output_format to cuda for "
                "compatibility "
                "with old commandlines. This behaviour is DEPRECATED and will "
                "be removed "
                "in the future. Please explicitly set "
                "\"-hwaccel_output_format cuda\".\n");
          ist->hwaccel_output_format = AV_PIX_FMT_CUDA;
        } else if (!hwaccel_output_format && hwaccel &&
                   !strcmp(hwaccel, "qsv")) {
          AvLog(nullptr, AV_LOG_WARNING,
                "WARNING: defaulting hwaccel_output_format to qsv for "
                "compatibility "
                "with old commandlines. This behaviour is DEPRECATED and will "
                "be removed "
                "in the future. Please explicitly set "
                "\"-hwaccel_output_format qsv\".\n");
          ist->hwaccel_output_format = AV_PIX_FMT_QSV;
        } else if (hwaccel_output_format) {
          ist->hwaccel_output_format = av_get_pix_fmt(hwaccel_output_format);
          if (ist->hwaccel_output_format == AV_PIX_FMT_NONE) {
            AvLog(nullptr, AV_LOG_FATAL,
                  "Unrecognised hwaccel output "
                  "format: %s",
                  hwaccel_output_format);
          }
        } else {
          ist->hwaccel_output_format = AV_PIX_FMT_NONE;
        }

        if (hwaccel) {
          // The NVDEC hwaccels use a CUDA device, so remap the name here.
          if (!strcmp(hwaccel, "nvdec") || !strcmp(hwaccel, "cuvid")) {
            hwaccel = "cuda";
          }
          if (!strcmp(hwaccel, "none")) {
            ist->hwaccel_id = HWACCEL_NONE;
          } else if (!strcmp(hwaccel, "auto")) {
            ist->hwaccel_id = HWACCEL_AUTO;
          } else {
            AVHWDeviceType type = av_hwdevice_find_type_by_name(hwaccel);
            if (type != AV_HWDEVICE_TYPE_NONE) {
              ist->hwaccel_id = HWACCEL_GENERIC;
              ist->hwaccel_device_type = type;
            }

            if (!ist->hwaccel_id) {
              AvLog(nullptr, AV_LOG_FATAL, "Unrecognized hwaccel: %s.\n",
                    hwaccel);
              AvLog(nullptr, AV_LOG_FATAL, "Supported hwaccels: ");
              type = AV_HWDEVICE_TYPE_NONE;
              while ((type = av_hwdevice_iterate_types(type)) !=
                     AV_HWDEVICE_TYPE_NONE) {
                AvLog(nullptr, AV_LOG_FATAL, "%s ",
                      av_hwdevice_get_type_name(type));
              }
              AvLog(nullptr, AV_LOG_FATAL, "\n");
              return false;
            }
          }
        }

        char *hwaccel_device = nullptr;
        // MATCH_PER_STREAM_OPT(hwaccel_devices, str, hwaccel_device, ic, st);
        if (hwaccel_device) {
          ist->hwaccel_device = av_strdup(hwaccel_device);
          if (!ist->hwaccel_device) {
            return false;
          }
        }

        ist->hwaccel_pix_fmt = AV_PIX_FMT_NONE;
      } break;
      case AVMEDIA_TYPE_AUDIO: {
        ist->guess_layout_max = INT_MAX;
        // MATCH_PER_STREAM_OPT(guess_layout_max, i, ist->guess_layout_max, ic,
        // st);
        guess_input_channel_layout(ist);
      } break;
      case AVMEDIA_TYPE_DATA:
      case AVMEDIA_TYPE_SUBTITLE: {
        char *canvas_size = nullptr;
        if (!ist->dec) {
          ist->dec = avcodec_find_decoder(par->codec_id);
        }
        // MATCH_PER_STREAM_OPT(fix_sub_duration, i, ist->fix_sub_duration, ic,
        // st);
        // MATCH_PER_STREAM_OPT(canvas_sizes, str, canvas_size, ic, st);
        if (canvas_size &&
            av_parse_video_size(&ist->dec_ctx->width, &ist->dec_ctx->height,
                                canvas_size) < 0) {
          AvLog(nullptr, AV_LOG_FATAL, "Invalid canvas size: %s.\n",
                canvas_size);
          return false;
        }
        break;
      }
      case AVMEDIA_TYPE_ATTACHMENT:
      case AVMEDIA_TYPE_UNKNOWN:
        break;
      default:
        abort();
    }

    ret = avcodec_parameters_from_context(par, ist->dec_ctx);
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_ERROR, "Error initializing the decoder context.\n");
      return false;
    }
  }
  return true;
}
/*
 * Check whether a packet from ist should be written into ost at this time
 */
bool FfmpegVideoConverter::check_output_constraints(InputStream *ist,
                                                    OutputStream *ost) {
  OutputFile *of = output_files[ost->file_index];
  int ist_index = input_files[ist->file_index]->ist_index + ist->st->index;

  if (ost->source_index != ist_index) {
    return false;
  }
  if (ost->finished & MUXER_FINISHED) {
    return false;
  }
  if (of->start_time != AV_NOPTS_VALUE && ist->pts < of->start_time) {
    return false;
  }
  return true;
}

bool FfmpegVideoConverter::do_streamcopy(InputStream *ist, OutputStream *ost,
                                         const AVPacket *pkt) {
  OutputFile *of = output_files[ost->file_index];
  InputFile *f = input_files[ist->file_index];
  int64_t start_time = (of->start_time == AV_NOPTS_VALUE) ? 0 : of->start_time;
  int64_t ost_tb_start_time =
      av_rescale_q(start_time, {1, AV_TIME_BASE}, ost->mux_timebase);
  AVPacket *opkt = ost->pkt;

  av_packet_unref(opkt);
  // EOF: flush output bitstream filters.
  if (!pkt) {
    output_packet(of, opkt, ost, 1);
    return true;
  }

  if (!ost->streamcopy_started && !(pkt->flags & AV_PKT_FLAG_KEY) &&
      !ost->copy_initial_nonkeyframes) {
    return true;
  }

  if (!ost->streamcopy_started && !ost->copy_prior_start) {
    int64_t comp_start = start_time;
    if (copy_ts && f->start_time != AV_NOPTS_VALUE) {
      comp_start = FFMAX(start_time, f->start_time + f->ts_offset);
    }
    if (pkt->pts == AV_NOPTS_VALUE
            ? ist->pts < comp_start
            : pkt->pts < av_rescale_q(comp_start, {1, AV_TIME_BASE},
                                      ist->st->time_base)) {
      return true;
    }
  }

  if (of->recording_time != INT64_MAX &&
      ist->pts >= of->recording_time + start_time) {
    close_output_stream(ost);
    return true;
  }

  if (f->recording_time != INT64_MAX) {
    start_time = 0;
    if (copy_ts) {
      start_time += f->start_time != AV_NOPTS_VALUE ? f->start_time : 0;
      start_time += start_at_zero ? 0 : f->ctx->start_time;
    }
    if (ist->pts >= f->recording_time + start_time) {
      close_output_stream(ost);
      return true;
    }
  }

  if (av_packet_ref(opkt, pkt) < 0) {
    return false;
  }

  if (pkt->pts != AV_NOPTS_VALUE) {
    opkt->pts = av_rescale_q(pkt->pts, ist->st->time_base, ost->mux_timebase) -
                ost_tb_start_time;
  }
  if (pkt->dts == AV_NOPTS_VALUE) {
    opkt->dts = av_rescale_q(ist->dts, {1, AV_TIME_BASE}, ost->mux_timebase);
  } else if (ost->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
    int duration = av_get_audio_frame_duration(ist->dec_ctx, pkt->size);
    if (!duration) {
      duration = ist->dec_ctx->frame_size;
    }
    opkt->dts = av_rescale_delta(
        ist->st->time_base, pkt->dts, {1, ist->dec_ctx->sample_rate}, duration,
        &ist->filter_in_rescale_delta_last, ost->mux_timebase);
    /* dts will be set immediately afterwards to what pts is now */
    opkt->pts = opkt->dts - ost_tb_start_time;
  } else {
    opkt->dts = av_rescale_q(pkt->dts, ist->st->time_base, ost->mux_timebase);
  }
  opkt->dts -= ost_tb_start_time;

  opkt->duration =
      av_rescale_q(pkt->duration, ist->st->time_base, ost->mux_timebase);

  ost->sync_opts += opkt->duration;

  output_packet(of, opkt, ost, 0);

  ost->streamcopy_started = true;
  return true;
}
bool FfmpegVideoConverter::guess_input_channel_layout(InputStream *ist) {
  AVCodecContext *dec = ist->dec_ctx;

  if (dec->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
    char layout_name[256] = {0};

    if (dec->ch_layout.nb_channels > ist->guess_layout_max) {
      return false;
    }
    av_channel_layout_default(&dec->ch_layout, dec->ch_layout.nb_channels);
    if (dec->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
      return false;
    }
    av_channel_layout_describe(&dec->ch_layout, layout_name,
                               sizeof(layout_name));
    AvLog(nullptr, AV_LOG_WARNING,
          "Guessed Channel Layout for Input Stream "
          "#%d.%d : %s\n",
          ist->file_index, ist->st->index, layout_name);
  }
  return true;
}

bool FfmpegVideoConverter::apply_sync_offsets(void) {
  for (int i = 0; i < nb_input_files; i++) {
    InputFile *self = input_files[i];
    if (self->input_sync_ref == -1 || self->input_sync_ref == i) {
      continue;
    }
    if (self->input_sync_ref >= nb_input_files || self->input_sync_ref < -1) {
      AvLog(nullptr, AV_LOG_FATAL,
            "-isync for input %d references non-existent input %d.\n", i,
            self->input_sync_ref);
      return false;
    }

    if (copy_ts && !start_at_zero) {
      AvLog(nullptr, AV_LOG_FATAL,
            "Use of -isync requires that start_at_zero be set if copyts is "
            "set.\n");
      return false;
    }

    InputFile *ref = input_files[self->input_sync_ref];
    if (ref->input_sync_ref != -1 &&
        ref->input_sync_ref != self->input_sync_ref) {
      AvLog(
          nullptr, AV_LOG_ERROR,
          "-isync for input %d references a resynced input %d. Sync not set.\n",
          i, self->input_sync_ref);
      continue;
    }

    bool start_times_set = true;
    int64_t self_start_time;
    int64_t ref_start_time;
    if (self->ctx->start_time_realtime != AV_NOPTS_VALUE &&
        ref->ctx->start_time_realtime != AV_NOPTS_VALUE) {
      self_start_time = self->ctx->start_time_realtime;
      ref_start_time = ref->ctx->start_time_realtime;
    } else if (self->ctx->start_time != AV_NOPTS_VALUE &&
               ref->ctx->start_time != AV_NOPTS_VALUE) {
      self_start_time = self->ctx->start_time;
      ref_start_time = ref->ctx->start_time;
    } else {
      start_times_set = false;
    }

    if (start_times_set) {
      int64_t self_seek_start =
          self->start_time == AV_NOPTS_VALUE ? 0 : self->start_time;
      int64_t ref_seek_start =
          ref->start_time == AV_NOPTS_VALUE ? 0 : ref->start_time;

      int64_t adjustment = (self_start_time - ref_start_time) +
                           !copy_ts * (self_seek_start - ref_seek_start) +
                           ref->input_ts_offset;

      self->ts_offset += adjustment;

      AvLog(nullptr, AV_LOG_INFO,
            "Adjusted ts offset for Input #%d by %" PRId64
            " us to sync with Input #%d.\n",
            i, adjustment, self->input_sync_ref);
    } else {
      AvLog(nullptr, AV_LOG_INFO,
            "Unable to identify start times for Inputs #%d and %d both. No "
            "sync adjustment made.\n",
            i, self->input_sync_ref);
    }
  }

  return true;
}

bool FfmpegVideoConverter::init_complex_filters(void) {
  for (int i = 0; i < nb_filtergraphs; i++) {
    int ret = init_complex_filtergraph(filtergraphs[i]);
    if (ret < 0) {
      return false;
    }
  }
  return true;
}

int FfmpegVideoConverter::open_output_file(const char *filename) {
  if (oo.stop_time != INT64_MAX && oo.recording_time != INT64_MAX) {
    oo.stop_time = INT64_MAX;
    AvLog(nullptr, AV_LOG_WARNING,
          "-t and -to cannot be used together; using -t.\n");
  }

  if (oo.stop_time != INT64_MAX && oo.recording_time == INT64_MAX) {
    int64_t start_time = oo.start_time == AV_NOPTS_VALUE ? 0 : oo.start_time;
    if (oo.stop_time <= start_time) {
      AvLog(nullptr, AV_LOG_ERROR, "-to value smaller than -ss; aborting.\n");
      return false;
    } else {
      oo.recording_time = oo.stop_time - start_time;
    }
  }

  OutputFile *of = static_cast<OutputFile *>(allocate_array_elem(
      &output_files, sizeof(*output_files[0]), &nb_output_files));

  of->index = nb_output_files - 1;
  of->ost_index = nb_output_streams;
  of->recording_time = oo.recording_time;
  of->start_time = oo.start_time;
  of->limit_filesize = oo.limit_filesize;
  of->shortest = oo.shortest;
  av_dict_copy(&of->opts, oo.format_opts, 0);

  AVFormatContext *oc = nullptr;
  const char *output_format = oo.format.empty() ? nullptr : oo.format.c_str();
  int err =
      avformat_alloc_output_context2(&oc, nullptr, output_format, filename);
  if (!oc) {
    PrintError(filename, err);
    return err;
  }

  of->ctx = oc;
  of->format = oc->oformat;
  if (oo.recording_time != INT64_MAX) {
    oc->duration = oo.recording_time;
  }

  oc->interrupt_callback.callback = decode_interrupt_cb;
  oc->interrupt_callback.opaque = this;

  if (oo.bitexact) {
    oc->flags |= AVFMT_FLAG_BITEXACT;
  }

  /* create streams for all unlabeled output pads */
  for (int i = 0; i < nb_filtergraphs; i++) {
    FilterGraph *fg = filtergraphs[i];
    for (int j = 0; j < fg->nb_outputs; j++) {
      OutputFilter *ofilter = fg->outputs[j];

      if (!ofilter->out_tmp || ofilter->out_tmp->name) {
        continue;
      }
      switch (ofilter->type) {
        case AVMEDIA_TYPE_VIDEO:
          oo.video_disable = true;
          break;
        case AVMEDIA_TYPE_AUDIO:
          oo.audio_disable = true;
          break;
        case AVMEDIA_TYPE_SUBTITLE:
          oo.subtitle_disable = true;
          break;
      }
      init_output_filter(ofilter, oc);
    }
  }

  InputStream *ist = nullptr;
  OutputStream *ost = nullptr;
  if (!oo.nb_stream_maps) {
    /* pick the "best" stream of each type */

    /* video: highest resolution */
    if (!oo.video_disable &&
        av_guess_codec(oc->oformat, nullptr, filename, nullptr,
                       AVMEDIA_TYPE_VIDEO) != AV_CODEC_ID_NONE) {
      int best_score = 0, idx = -1;
      int qcr = avformat_query_codec(oc->oformat, oc->oformat->video_codec, 0);
      for (int j = 0; j < nb_input_files; j++) {
        InputFile *ifile = input_files[j];
        int file_best_score = 0, file_best_idx = -1;
        for (int i = 0; i < ifile->nb_streams; i++) {
          int score;
          ist = input_streams[ifile->ist_index + i];
          score = ist->st->codecpar->width * ist->st->codecpar->height +
                  100000000 * !!(ist->st->event_flags &
                                 AVSTREAM_EVENT_FLAG_NEW_PACKETS) +
                  5000000 * !!(ist->st->disposition & AV_DISPOSITION_DEFAULT);
          if (ist->user_set_discard == AVDISCARD_ALL) {
            continue;
          }
          if ((qcr != MKTAG('A', 'P', 'I', 'C')) &&
              (ist->st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            score = 1;
          }
          if (ist->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
              score > file_best_score) {
            if ((qcr == MKTAG('A', 'P', 'I', 'C')) &&
                !(ist->st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
              continue;
            }
            file_best_score = score;
            file_best_idx = ifile->ist_index + i;
          }
        }
        if (file_best_idx >= 0) {
          if ((qcr == MKTAG('A', 'P', 'I', 'C')) ||
              !(ist->st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
            file_best_score -=
                5000000 * !!(input_streams[file_best_idx]->st->disposition &
                             AV_DISPOSITION_DEFAULT);
          }
          if (file_best_score > best_score) {
            best_score = file_best_score;
            idx = file_best_idx;
          }
        }
      }
      if (idx >= 0) {
        new_video_stream(oc, idx);
      }
    }

    /* audio: most channels */
    if (!oo.audio_disable &&
        av_guess_codec(oc->oformat, nullptr, filename, nullptr,
                       AVMEDIA_TYPE_AUDIO) != AV_CODEC_ID_NONE) {
      int best_score = 0, idx = -1;
      for (int j = 0; j < nb_input_files; j++) {
        InputFile *ifile = input_files[j];
        int file_best_score = 0, file_best_idx = -1;
        for (int i = 0; i < ifile->nb_streams; i++) {
          int score;
          ist = input_streams[ifile->ist_index + i];
          score = ist->st->codecpar->ch_layout.nb_channels +
                  100000000 * !!(ist->st->event_flags &
                                 AVSTREAM_EVENT_FLAG_NEW_PACKETS) +
                  5000000 * !!(ist->st->disposition & AV_DISPOSITION_DEFAULT);
          if (ist->user_set_discard == AVDISCARD_ALL) {
            continue;
          }
          if (ist->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
              score > file_best_score) {
            file_best_score = score;
            file_best_idx = ifile->ist_index + i;
          }
        }
        if (file_best_idx >= 0) {
          file_best_score -=
              5000000 * !!(input_streams[file_best_idx]->st->disposition &
                           AV_DISPOSITION_DEFAULT);
          if (file_best_score > best_score) {
            best_score = file_best_score;
            idx = file_best_idx;
          }
        }
      }
      if (idx >= 0) {
        new_audio_stream(oc, idx);
      }
    }

    /* subtitles: pick first */
    char *subtitle_codec_name = nullptr;
    // MATCH_PER_TYPE_OPT(codec_names, str, subtitle_codec_name, oc, "s");
    if (!oo.subtitle_disable &&
        (avcodec_find_encoder(oc->oformat->subtitle_codec) ||
         subtitle_codec_name)) {
      for (int i = 0; i < nb_input_streams; i++)
        if (input_streams[i]->st->codecpar->codec_type ==
            AVMEDIA_TYPE_SUBTITLE) {
          AVCodecDescriptor const *input_descriptor =
              avcodec_descriptor_get(input_streams[i]->st->codecpar->codec_id);
          AVCodecDescriptor const *output_descriptor = nullptr;
          AVCodec const *output_codec =
              avcodec_find_encoder(oc->oformat->subtitle_codec);
          int input_props = 0, output_props = 0;
          if (input_streams[i]->user_set_discard == AVDISCARD_ALL) {
            continue;
          }
          if (output_codec) {
            output_descriptor = avcodec_descriptor_get(output_codec->id);
          }
          if (input_descriptor) {
            input_props = input_descriptor->props &
                          (AV_CODEC_PROP_TEXT_SUB | AV_CODEC_PROP_BITMAP_SUB);
          }
          if (output_descriptor) {
            output_props = output_descriptor->props &
                           (AV_CODEC_PROP_TEXT_SUB | AV_CODEC_PROP_BITMAP_SUB);
          }
          if (subtitle_codec_name || input_props & output_props ||
              // Map dvb teletext which has neither property to any output
              // subtitle encoder
              input_descriptor && output_descriptor &&
                  (!input_descriptor->props || !output_descriptor->props)) {
            new_subtitle_stream(oc, i);
            break;
          }
        }
    }
    /* Data only if codec id match */
    if (!oo.data_disable) {
      AVCodecID codec_id = av_guess_codec(oc->oformat, nullptr, filename,
                                          nullptr, AVMEDIA_TYPE_DATA);
      for (int i = 0; codec_id != AV_CODEC_ID_NONE && i < nb_input_streams;
           i++) {
        if (input_streams[i]->user_set_discard == AVDISCARD_ALL) {
          continue;
        }
        if (input_streams[i]->st->codecpar->codec_type == AVMEDIA_TYPE_DATA &&
            input_streams[i]->st->codecpar->codec_id == codec_id) {
          new_data_stream(oc, i);
        }
      }
    }
  } else {
    for (int i = 0; i < oo.nb_stream_maps; i++) {
      StreamMap *map = &oo.stream_maps[i];

      if (map->disabled) {
        continue;
      }
      if (map->linklabel) {
        OutputFilter *ofilter = nullptr;
        for (int j = 0; j < nb_filtergraphs; j++) {
          FilterGraph *fg = filtergraphs[j];
          for (int k = 0; k < fg->nb_outputs; k++) {
            AVFilterInOut *out = fg->outputs[k]->out_tmp;
            if (out && !strcmp(out->name, map->linklabel)) {
              ofilter = fg->outputs[k];
              goto loop_end;
            }
          }
        }
      loop_end:
        if (!ofilter) {
          AvLog(nullptr, AV_LOG_FATAL,
                "Output with label '%s' does not exist "
                "in any defined filter graph, or was already used elsewhere.\n",
                map->linklabel);
          return -1;
        }
        init_output_filter(ofilter, oc);
      } else {
        int src_idx =
            input_files[map->file_index]->ist_index + map->stream_index;

        ist = input_streams[input_files[map->file_index]->ist_index +
                            map->stream_index];
        if (ist->user_set_discard == AVDISCARD_ALL) {
          AvLog(nullptr, AV_LOG_FATAL,
                "Stream #%d:%d is disabled and cannot be mapped.\n",
                map->file_index, map->stream_index);
          return -1;
        }
        if (oo.subtitle_disable &&
            ist->st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) {
          continue;
        }
        if (oo.audio_disable &&
            ist->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
          continue;
        }
        if (oo.video_disable &&
            ist->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
          continue;
        }
        if (oo.data_disable &&
            ist->st->codecpar->codec_type == AVMEDIA_TYPE_DATA) {
          continue;
        }
        ost = nullptr;
        switch (ist->st->codecpar->codec_type) {
          case AVMEDIA_TYPE_VIDEO:
            ost = new_video_stream(oc, src_idx);
            break;
          case AVMEDIA_TYPE_AUDIO:
            ost = new_audio_stream(oc, src_idx);
            break;
          case AVMEDIA_TYPE_SUBTITLE:
            ost = new_subtitle_stream(oc, src_idx);
            break;
          case AVMEDIA_TYPE_DATA:
            ost = new_data_stream(oc, src_idx);
            break;
          case AVMEDIA_TYPE_ATTACHMENT:
            ost = new_attachment_stream(oc, src_idx);
            break;
          case AVMEDIA_TYPE_UNKNOWN:
            if (copy_unknown_streams) {
              ost = new_unknown_stream(oc, src_idx);
              break;
            }
          default:
            AvLog(nullptr,
                  ignore_unknown_streams ? AV_LOG_WARNING : AV_LOG_FATAL,
                  "Cannot map stream #%d:%d - unsupported type.\n",
                  map->file_index, map->stream_index);
            if (!ignore_unknown_streams) {
              AvLog(nullptr, AV_LOG_FATAL,
                    "If you want unsupported types ignored instead "
                    "of failing, please use the -ignore_unknown option\n"
                    "If you want them copied, please use -copy_unknown\n");
              return -1;
            }
        }
        if (ost) {
          ost->sync_ist =
              input_streams[input_files[map->sync_file_index]->ist_index +
                            map->sync_stream_index];
        }
      }
    }
  }

  /* handle attached files */
  for (int i = 0; i < oo.nb_attachments; i++) {
    AVIOContext *pb;
    const AVIOInterruptCB int_cb{decode_interrupt_cb, this};
    if ((err = avio_open2(&pb, oo.attachments[i], AVIO_FLAG_READ, &int_cb,
                          nullptr)) < 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Could not open attachment file %s.\n",
            oo.attachments[i]);
      return -1;
    }
    int64_t len;
    if ((len = avio_size(pb)) <= 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Could not get size of the attachment %s.\n",
            oo.attachments[i]);
      return -1;
    }

    uint8_t *attachment;
    if (len > INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE ||
        !(attachment = static_cast<uint8_t *>(
              av_malloc(len + AV_INPUT_BUFFER_PADDING_SIZE)))) {
      AvLog(nullptr, AV_LOG_FATAL, "Attachment %s too large.\n",
            oo.attachments[i]);
      return -1;
    }
    avio_read(pb, attachment, len);
    memset(attachment + len, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    ost = new_attachment_stream(oc, -1);
    ost->stream_copy = 0;
    ost->attachment_filename = oo.attachments[i];
    ost->st->codecpar->extradata = attachment;
    ost->st->codecpar->extradata_size = len;

    const char *p = strrchr(oo.attachments[i], '/');
    av_dict_set(&ost->st->metadata, "filename",
                (p && *p) ? p + 1 : oo.attachments[i], AV_DICT_DONT_OVERWRITE);
    avio_closep(&pb);
  }

  if (!oc->nb_streams && !(oc->oformat->flags & AVFMT_NOSTREAMS)) {
    av_dump_format(oc, nb_output_files - 1, oc->url, 1);
    AvLog(nullptr, AV_LOG_ERROR,
          "Output file #%d does not contain any stream\n", nb_output_files - 1);
    return -1;
  }

  const AVDictionaryEntry *e = nullptr;
  /* check if all codec options have been used */
  AVDictionary *unused_opts = strip_specifiers(oo.codec_opts);
  for (int i = of->ost_index; i < nb_output_streams; i++) {
    e = nullptr;
    while ((e = av_dict_get(output_streams[i]->encoder_opts, "", e,
                            AV_DICT_IGNORE_SUFFIX))) {
      av_dict_set(&unused_opts, e->key, nullptr, 0);
    }
  }

  e = nullptr;
  while ((e = av_dict_get(unused_opts, "", e, AV_DICT_IGNORE_SUFFIX))) {
    const AVClass *classs = avcodec_get_class();
    const AVOption *option =
        av_opt_find(&classs, e->key, nullptr, 0,
                    AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
    const AVClass *fclass = avformat_get_class();
    const AVOption *foption =
        av_opt_find(&fclass, e->key, nullptr, 0,
                    AV_OPT_SEARCH_CHILDREN | AV_OPT_SEARCH_FAKE_OBJ);
    if (!option || foption) {
      continue;
    }
    if (!(option->flags & AV_OPT_FLAG_ENCODING_PARAM)) {
      AvLog(nullptr, AV_LOG_ERROR,
            "Codec AVOption %s (%s) specified for "
            "output file #%d (%s) is not an encoding option.\n",
            e->key, option->help ? option->help : "", nb_output_files - 1,
            filename);
      return -1;
    }

    // gop_timecode is injected by generic code but not always used
    if (!strcmp(e->key, "gop_timecode")) {
      continue;
    }
    AvLog(nullptr, AV_LOG_WARNING,
          "Codec AVOption %s (%s) specified for "
          "output file #%d (%s) has not been used for any stream. The most "
          "likely reason is either wrong type (e.g. a video option with "
          "no video streams) or that it is a private option of some encoder "
          "which was not actually used for any stream.\n",
          e->key, option->help ? option->help : "", nb_output_files - 1,
          filename);
  }
  av_dict_free(&unused_opts);

  /* set the decoding_needed flags and create simple filtergraphs */
  for (int i = of->ost_index; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];

    if (ost->encoding_needed && ost->source_index >= 0) {
      InputStream *ist = input_streams[ost->source_index];
      ist->decoding_needed |= DECODING_FOR_OST;
      ist->processing_needed = true;

      if (ost->st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ||
          ost->st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        err = init_simple_filtergraph(ist, ost);
        if (err < 0) {
          AvLog(nullptr, AV_LOG_ERROR,
                "Error initializing a simple filtergraph between streams "
                "%d:%d->%d:%d\n",
                ist->file_index, ost->source_index, nb_output_files - 1,
                ost->st->index);
          return -1;
        }
      }
    } else if (ost->stream_copy && ost->source_index >= 0) {
      InputStream *ist = input_streams[ost->source_index];
      ist->processing_needed = true;
    }

    /* set the filter output constraints */
    if (ost->filter) {
      OutputFilter *f = ost->filter;
      switch (ost->enc_ctx->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
          f->frame_rate = ost->frame_rate;
          f->width = ost->enc_ctx->width;
          f->height = ost->enc_ctx->height;
          if (ost->enc_ctx->pix_fmt != AV_PIX_FMT_NONE) {
            f->format = ost->enc_ctx->pix_fmt;
          } else {
            f->formats = reinterpret_cast<const int *>(ost->enc->pix_fmts);
          }
          break;
        case AVMEDIA_TYPE_AUDIO:
          if (ost->enc_ctx->sample_fmt != AV_SAMPLE_FMT_NONE) {
            f->format = ost->enc_ctx->sample_fmt;
          } else {
            f->formats = reinterpret_cast<const int *>(ost->enc->sample_fmts);
          }
          if (ost->enc_ctx->sample_rate) {
            f->sample_rate = ost->enc_ctx->sample_rate;
          } else {
            f->sample_rates = ost->enc->supported_samplerates;
          }
          if (ost->enc_ctx->ch_layout.nb_channels) {
            set_channel_layout(f, ost);
          } else if (ost->enc->ch_layouts) {
            f->ch_layouts = ost->enc->ch_layouts;
          }
          break;
      }
    }
  }

  /* check filename in case of an image number is expected */
  if (oc->oformat->flags & AVFMT_NEEDNUMBER) {
    if (!av_filename_number_test(oc->url)) {
      return -1;
    }
  }

  if (!(oc->oformat->flags & AVFMT_NOSTREAMS) &&
      !input_stream_potentially_available) {
    AvLog(nullptr, AV_LOG_ERROR,
          "No input streams but output needs an input stream\n");
    return -1;
  }

  if (!(oc->oformat->flags & AVFMT_NOFILE)) {
    /* test if it already exists to avoid losing precious files */
    // assert_file_overwrite(filename);

    /* open the file */
    if ((err = avio_open2(&oc->pb, filename, AVIO_FLAG_WRITE,
                          &oc->interrupt_callback, &of->opts)) < 0) {
      return -1;
    }
  } else if (strcmp(oc->oformat->name, "image2") == 0 &&
             !av_filename_number_test(filename)) {
    // assert_file_overwrite(filename);
  }

  if (oo.mux_preload) {
    av_dict_set_int(&of->opts, "preload", oo.mux_preload * AV_TIME_BASE, 0);
  }
  oc->max_delay = (int)(oo.mux_max_delay * AV_TIME_BASE);

  /* copy metadata */
  for (int i = 0; i < o.nb_metadata_map; i++) {
#if 0
    // TODO:
    char *p;
    int in_file_index = strtol(o.metadata_map[i].u.str, &p, 0);

    if (in_file_index >= nb_input_files) {
      AvLog(nullptr, AV_LOG_FATAL,
             "Invalid input file index %d while processing metadata maps\n",
             in_file_index);
      return -1;
    }
    copy_metadata(o->metadata_map[i].specifier, *p ? p + 1 : p, oc,
                  in_file_index >= 0 ? input_files[in_file_index]->ctx : nullptr,
                  o);
#endif
  }

  /* copy chapters */
  if (oo.chapters_input_file >= nb_input_files) {
    if (oo.chapters_input_file == INT_MAX) {
      /* copy chapters from the first input file that has them*/
      oo.chapters_input_file = -1;
      for (int i = 0; i < nb_input_files; i++)
        if (input_files[i]->ctx->nb_chapters) {
          oo.chapters_input_file = i;
          break;
        }
    } else {
      AvLog(nullptr, AV_LOG_FATAL,
            "Invalid input file index %d in chapter mapping.\n",
            oo.chapters_input_file);
      return -1;
    }
  }
  if (oo.chapters_input_file >= 0) {
    // TODO:
    // copy_chapters(input_files[o.chapters_input_file], of, oc,
    // !oo.metadata_chapters_manual);
  }

  /* copy global metadata by default */
  if (!oo.metadata_global_manual && nb_input_files) {
    av_dict_copy(&oc->metadata, input_files[0]->ctx->metadata,
                 AV_DICT_DONT_OVERWRITE);
    if (oo.recording_time != INT64_MAX) {
      av_dict_set(&oc->metadata, "duration", nullptr, 0);
    }
    av_dict_set(&oc->metadata, "creation_time", nullptr, 0);
    av_dict_set(&oc->metadata, "company_name", nullptr, 0);
    av_dict_set(&oc->metadata, "product_name", nullptr, 0);
    av_dict_set(&oc->metadata, "product_version", nullptr, 0);
  }
  if (!oo.metadata_streams_manual)
    for (int i = of->ost_index; i < nb_output_streams; i++) {
      if (output_streams[i]->source_index < 0) {
        /* this is true e.g. for attached files */
        continue;
      }
      InputStream *ist = input_streams[output_streams[i]->source_index];
      av_dict_copy(&output_streams[i]->st->metadata, ist->st->metadata,
                   AV_DICT_DONT_OVERWRITE);
      if (!output_streams[i]->stream_copy) {
        av_dict_set(&output_streams[i]->st->metadata, "encoder", nullptr, 0);
      }
    }

  /* process manually set programs */
  for (int i = 0; i < o.nb_program; i++) {
#if 0
    //TODO:
    const char *p = o.program[i].u.str;
    int progid = i + 1;
    AVProgram *program;

    while (*p) {
      const char *p2 = av_get_token(&p, ":");
      const char *to_dealloc = p2;
      char *key;
      if (!p2) break;

      if (*p) p++;

      key = av_get_token(&p2, "=");
      if (!key || !*p2) {
        av_freep(&to_dealloc);
        av_freep(&key);
        break;
      }
      p2++;

      if (!strcmp(key, "program_num")) progid = strtol(p2, nullptr, 0);
      av_freep(&to_dealloc);
      av_freep(&key);
    }

    program = av_new_program(oc, progid);

    p = o->program[i].u.str;
    while (*p) {
      const char *p2 = av_get_token(&p, ":");
      const char *to_dealloc = p2;
      char *key;
      if (!p2) break;
      if (*p) p++;

      key = av_get_token(&p2, "=");
      if (!key) {
        AvLog(nullptr, AV_LOG_FATAL, "No '=' character in program string %s.\n",
               p2);
        return -1;
      }
      if (!*p2) return -1;
      p2++;

      if (!strcmp(key, "title")) {
        av_dict_set(&program->metadata, "title", p2, 0);
      } else if (!strcmp(key, "program_num")) {
      } else if (!strcmp(key, "st")) {
        int st_num = strtol(p2, nullptr, 0);
        av_program_add_stream_index(oc, progid, st_num);
      } else {
        AvLog(nullptr, AV_LOG_FATAL, "Unknown program key %s.\n", key);
        return -1;
      }
      av_freep(&to_dealloc);
      av_freep(&key);
    }
#endif
  }

  /* process manually set metadata */
  for (int i = 0; i < o.nb_metadata; i++) {
#if 0
    AVDictionary **m;
    char type, *val;
    const char *stream_spec;
    int index = 0, j, ret = 0;

    val = strchr(o->metadata[i].u.str, '=');
    if (!val) {
      AvLog(nullptr, AV_LOG_FATAL, "No '=' character in metadata string %s.\n",
             o->metadata[i].u.str);
      return -1;
    }
    *val++ = 0;

    parse_meta_type(o->metadata[i].specifier, &type, &index, &stream_spec);
    if (type == 's') {
      for (j = 0; j < oc->nb_streams; j++) {
        ost = output_streams[nb_output_streams - oc->nb_streams + j];
        if ((ret = check_stream_specifier(oc, oc->streams[j], stream_spec)) >
            0) {
          if (!strcmp(o->metadata[i].u.str, "rotate")) {
            char *tail;
            double theta = av_strtod(val, &tail);
            if (!*tail) {
              ost->rotate_overridden = true;
              ost->rotate_override_value = theta;
            }
          } else {
            av_dict_set(&oc->streams[j]->metadata, o->metadata[i].u.str,
                        *val ? val : nullptr, 0);
          }
        } else if (ret < 0)
          return -1;
      }
    } else {
      switch (type) {
        case 'g':
          m = &oc->metadata;
          break;
        case 'c':
          if (index < 0 || index >= oc->nb_chapters) {
            AvLog(nullptr, AV_LOG_FATAL,
                   "Invalid chapter index %d in metadata specifier.\n", index);
            return -1;
          }
          m = &oc->chapters[index]->metadata;
          break;
        case 'p':
          if (index < 0 || index >= oc->nb_programs) {
            AvLog(nullptr, AV_LOG_FATAL,
                   "Invalid program index %d in metadata specifier.\n", index);
            return -1;
          }
          m = &oc->programs[index]->metadata;
          break;
        default:
          AvLog(nullptr, AV_LOG_FATAL, "Invalid metadata specifier %s.\n",
                 o->metadata[i].specifier);
          return -1;
      }
      av_dict_set(m, o->metadata[i].u.str, *val ? val : nullptr, 0);
    }
#endif
  }

  err = set_dispositions(of, oc);
  if (err < 0) {
    AvLog(nullptr, AV_LOG_FATAL, "Error setting output stream dispositions\n");
    return -1;
  }

  return 0;
}

bool FfmpegVideoConverter::init_output_filter(OutputFilter *ofilter,
                                              AVFormatContext *oc) {
  OutputStream *ost = nullptr;

  switch (ofilter->type) {
    case AVMEDIA_TYPE_VIDEO:
      ost = new_video_stream(oc, -1);
      break;
    case AVMEDIA_TYPE_AUDIO:
      ost = new_audio_stream(oc, -1);
      break;
    default:
      AvLog(nullptr, AV_LOG_FATAL,
            "Only video and audio filters are supported "
            "currently.\n");
      return false;
  }

  ost->filter = ofilter;

  ofilter->ost = ost;
  ofilter->format = -1;

  if (ost->stream_copy) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Streamcopy requested for output stream %d:%d, "
          "which is fed from a complex filtergraph. Filtering and streamcopy "
          "cannot be used together.\n",
          ost->file_index, ost->index);
    return false;
  }

  if (ost->avfilter && (ost->filters || ost->filters_script)) {
    const char *opt = ost->filters ? "-vf/-af/-filter" : "-filter_script";
    AvLog(
        nullptr, AV_LOG_ERROR,
        "%s '%s' was specified through the %s option "
        "for output stream %d:%d, which is fed from a complex filtergraph.\n"
        "%s and -filter_complex cannot be used together for the same stream.\n",
        ost->filters ? "Filtergraph" : "Filtergraph script",
        ost->filters ? ost->filters : ost->filters_script, opt, ost->file_index,
        ost->index, opt);
    return false;
  }

  avfilter_inout_free(&ofilter->out_tmp);
  return true;
}

OutputStream *FfmpegVideoConverter::new_video_stream(AVFormatContext *oc,
                                                     int source_index) {
  OutputStream *ost = new_output_stream(oc, AVMEDIA_TYPE_VIDEO, source_index);
  AVStream *st = ost->st;
  AVCodecContext *video_enc = ost->enc_ctx;

  // TODO(xiufeng.liu): 支持每个视频流
  const char *frame_rate =
      oo.frame_rate.empty() ? nullptr : oo.frame_rate.c_str();
  if (frame_rate && av_parse_video_rate(&ost->frame_rate, frame_rate) < 0) {
    AvLog(nullptr, AV_LOG_FATAL, "Invalid framerate value: %s\n", frame_rate);
    return nullptr;
  }

  // TODO(xiufeng.liu): 支持每个视频流
  const char *max_frame_rate =
      oo.max_frame_rate.empty() ? nullptr : oo.max_frame_rate.c_str();
  if (max_frame_rate &&
      av_parse_video_rate(&ost->max_frame_rate, max_frame_rate) < 0) {
    AvLog(nullptr, AV_LOG_FATAL, "Invalid maximum framerate value: %s\n",
          max_frame_rate);
    return nullptr;
  }

  if (frame_rate && max_frame_rate) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Only one of -fpsmax and -r can be set for a stream.\n");
    return nullptr;
  }

  if ((frame_rate || max_frame_rate) &&
      video_sync_method == VSYNC_PASSTHROUGH) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Using -vsync passthrough and -r/-fpsmax can produce invalid output "
          "files\n");
  }

  // TODO(xiufeng.liu): 支持每个视频流
  const char *frame_aspect_ratio =
      oo.frame_aspect_ratio.empty() ? nullptr : oo.frame_aspect_ratio.c_str();
  if (frame_aspect_ratio) {
    AVRational q;
    if (av_parse_ratio(&q, frame_aspect_ratio, 255, 0, nullptr) < 0 ||
        q.num <= 0 || q.den <= 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Invalid aspect ratio: %s\n",
            frame_aspect_ratio);
      return nullptr;
    }
    ost->frame_aspect_ratio = q;
  }

  // MATCH_PER_STREAM_OPT(filter_scripts, str, ost->filters_script, oc, st);
  // TODO(xiufeng.liu): 支持每个视频流
  ost->filters = const_cast<char *>(
      oo.video_filters.empty() ? nullptr : oo.video_filters.c_str());

  if (!ost->stream_copy) {
    char *frame_size = nullptr;
    // MATCH_PER_STREAM_OPT(frame_sizes, str, frame_size, oc, st);
    if (frame_size && av_parse_video_size(&video_enc->width, &video_enc->height,
                                          frame_size) < 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Invalid frame size: %s.\n", frame_size);
      return nullptr;
    }

    char *frame_pix_fmt = nullptr;
    // MATCH_PER_STREAM_OPT(frame_pix_fmts, str, frame_pix_fmt, oc, st);
    if (frame_pix_fmt && *frame_pix_fmt == '+') {
      ost->keep_pix_fmt = true;
      if (!*++frame_pix_fmt) {
        frame_pix_fmt = nullptr;
      }
    }
    if (frame_pix_fmt && (video_enc->pix_fmt = av_get_pix_fmt(frame_pix_fmt)) ==
                             AV_PIX_FMT_NONE) {
      AvLog(nullptr, AV_LOG_FATAL, "Unknown pixel format requested: %s.\n",
            frame_pix_fmt);
      return nullptr;
    }
    st->sample_aspect_ratio = video_enc->sample_aspect_ratio;

    char *intra_matrix = nullptr;
    // MATCH_PER_STREAM_OPT(intra_matrices, str, intra_matrix, oc, st);
    if (intra_matrix) {
      if (!(video_enc->intra_matrix = static_cast<uint16_t *>(
                av_mallocz(sizeof(*video_enc->intra_matrix) * 64)))) {
        AvLog(nullptr, AV_LOG_FATAL,
              "Could not allocate memory for intra matrix.\n");
        return nullptr;
      }
      parse_matrix_coeffs(video_enc->intra_matrix, intra_matrix);
    }
    char *chroma_intra_matrix = nullptr;
    // MATCH_PER_STREAM_OPT(chroma_intra_matrices, str, chroma_intra_matrix, oc,
    // st);
    if (chroma_intra_matrix) {
      uint16_t *p = static_cast<uint16_t *>(
          av_mallocz(sizeof(*video_enc->chroma_intra_matrix) * 64));
      if (!p) {
        AvLog(nullptr, AV_LOG_FATAL,
              "Could not allocate memory for intra matrix.\n");
        return nullptr;
      }
      video_enc->chroma_intra_matrix = p;
      parse_matrix_coeffs(p, chroma_intra_matrix);
    }
    char *inter_matrix = nullptr;
    // MATCH_PER_STREAM_OPT(inter_matrices, str, inter_matrix, oc, st);
    if (inter_matrix) {
      if (!(video_enc->inter_matrix = static_cast<uint16_t *>(
                av_mallocz(sizeof(*video_enc->inter_matrix) * 64)))) {
        AvLog(nullptr, AV_LOG_FATAL,
              "Could not allocate memory for inter matrix.\n");
        return nullptr;
      }
      parse_matrix_coeffs(video_enc->inter_matrix, inter_matrix);
    }

    const char *p = nullptr;
    // MATCH_PER_STREAM_OPT(rc_overrides, str, p, oc, st);
    int i;
    for (i = 0; p; i++) {
      int start, end, q;
      int e = sscanf(p, "%d,%d,%d", &start, &end, &q);
      if (e != 3) {
        AvLog(nullptr, AV_LOG_FATAL, "error parsing rc_override\n");
        return nullptr;
      }
      video_enc->rc_override = static_cast<RcOverride *>(
          av_realloc_array(video_enc->rc_override, i + 1, sizeof(RcOverride)));
      if (!video_enc->rc_override) {
        AvLog(nullptr, AV_LOG_FATAL,
              "Could not (re)allocate memory for rc_override.\n");
        return nullptr;
      }
      video_enc->rc_override[i].start_frame = start;
      video_enc->rc_override[i].end_frame = end;
      if (q > 0) {
        video_enc->rc_override[i].qscale = q;
        video_enc->rc_override[i].quality_factor = 1.0;
      } else {
        video_enc->rc_override[i].qscale = 0;
        video_enc->rc_override[i].quality_factor = -q / 100.0;
      }
      p = strchr(p, '/');
      if (p) p++;
    }
    video_enc->rc_override_count = i;

    if (do_psnr) {
      video_enc->flags |= AV_CODEC_FLAG_PSNR;
    }

    /* two pass mode */
    int do_pass = 0;
    // MATCH_PER_STREAM_OPT(pass, i, do_pass, oc, st);
    if (do_pass) {
      if (do_pass & 1) {
        video_enc->flags |= AV_CODEC_FLAG_PASS1;
        av_dict_set(&ost->encoder_opts, "flags", "+pass1", AV_DICT_APPEND);
      }
      if (do_pass & 2) {
        video_enc->flags |= AV_CODEC_FLAG_PASS2;
        av_dict_set(&ost->encoder_opts, "flags", "+pass2", AV_DICT_APPEND);
      }
    }

    // MATCH_PER_STREAM_OPT(passlogfiles, str, ost->logfile_prefix, oc, st);
    if (ost->logfile_prefix &&
        !(ost->logfile_prefix = av_strdup(ost->logfile_prefix))) {
      return nullptr;
    }

    if (do_pass) {
      char logfilename[1024] = {0};
      FILE *f = nullptr;

      snprintf(logfilename, sizeof(logfilename), "%s-%d.log",
               ost->logfile_prefix ? ost->logfile_prefix
                                   : DEFAULT_PASS_LOGFILENAME_PREFIX,
               nb_output_streams - 1);
      if (!strcmp(ost->enc->name, "libx264")) {
        av_dict_set(&ost->encoder_opts, "stats", logfilename,
                    AV_DICT_DONT_OVERWRITE);
      } else {
        if (video_enc->flags & AV_CODEC_FLAG_PASS2) {
          char *logbuffer = read_file(logfilename);

          if (!logbuffer) {
            AvLog(nullptr, AV_LOG_FATAL,
                  "Error reading log file '%s' for pass-2 encoding\n",
                  logfilename);
            return nullptr;
          }
          video_enc->stats_in = logbuffer;
        }
        if (video_enc->flags & AV_CODEC_FLAG_PASS1) {
          f = fopen(logfilename, "wb");
          if (!f) {
            AvLog(nullptr, AV_LOG_FATAL,
                  "Cannot write log file '%s' for pass-1 encoding: %s\n",
                  logfilename, strerror(errno));
            return nullptr;
          }
          ost->logfile = f;
        }
      }
    }

    // MATCH_PER_STREAM_OPT(forced_key_frames, str, ost->forced_keyframes, oc,
    // st);
    if (ost->forced_keyframes) {
      ost->forced_keyframes = av_strdup(ost->forced_keyframes);
    }

    // MATCH_PER_STREAM_OPT(force_fps, i, ost->force_fps, oc, st);

    ost->top_field_first = -1;
    ost->top_field_first = o.top_field_first;

    ost->vsync_method = video_sync_method;
    // TODO(xiufeng.liu): 支持每个视频流
    ost->fps_mode = oo.fps_mode.empty() ? nullptr : oo.fps_mode.c_str();
    if (ost->fps_mode) {
      parse_and_set_vsync(ost->fps_mode,
                          reinterpret_cast<int *>(&ost->vsync_method),
                          ost->file_index, ost->index, 0);
    }

    if (ost->vsync_method == VSYNC_AUTO) {
      if (!strcmp(oc->oformat->name, "avi")) {
        ost->vsync_method = VSYNC_VFR;
      } else {
        ost->vsync_method =
            (oc->oformat->flags & AVFMT_VARIABLE_FPS)
                ? ((oc->oformat->flags & AVFMT_NOTIMESTAMPS) ? VSYNC_PASSTHROUGH
                                                             : VSYNC_VFR)
                : VSYNC_CFR;
      }

      if (ost->source_index >= 0 && ost->vsync_method == VSYNC_CFR) {
        const InputStream *ist = input_streams[ost->source_index];
        const InputFile *ifile = input_files[ist->file_index];

        if (ifile->nb_streams == 1 && ifile->input_ts_offset == 0) {
          ost->vsync_method = VSYNC_VSCFR;
        }
      }

      if (ost->vsync_method == VSYNC_CFR && copy_ts) {
        ost->vsync_method = VSYNC_VSCFR;
      }
    }
    ost->is_cfr =
        (ost->vsync_method == VSYNC_CFR || ost->vsync_method == VSYNC_VSCFR);

    ost->avfilter = get_ost_filters(oc, ost);
    if (!ost->avfilter) {
      return nullptr;
    }

    ost->last_frame = av_frame_alloc();
    if (!ost->last_frame) {
      return nullptr;
    }
  }

  if (ost->stream_copy) {
    check_streamcopy_filters(oc, ost, AVMEDIA_TYPE_VIDEO);
  }

  return ost;
}

OutputStream *FfmpegVideoConverter::new_audio_stream(AVFormatContext *oc,
                                                     int source_index) {
  OutputStream *ost = new_output_stream(oc, AVMEDIA_TYPE_AUDIO, source_index);
  AVStream *st = ost->st;

  AVCodecContext *audio_enc = ost->enc_ctx;
  audio_enc->codec_type = AVMEDIA_TYPE_AUDIO;

  // MATCH_PER_STREAM_OPT(filter_scripts, str, ost->filters_script, oc, st);
  // MATCH_PER_STREAM_OPT(filters, str, ost->filters, oc, st);

  if (!ost->stream_copy) {
    int channels = 0;
    // MATCH_PER_STREAM_OPT(audio_channels, i, channels, oc, st);
    if (channels) {
      audio_enc->ch_layout.order = AV_CHANNEL_ORDER_UNSPEC;
      audio_enc->ch_layout.nb_channels = channels;
    }

    char *layout = nullptr;
    // MATCH_PER_STREAM_OPT(audio_ch_layouts, str, layout, oc, st);
    if (layout) {
      if (av_channel_layout_from_string(&audio_enc->ch_layout, layout) < 0) {
#if FF_API_OLD_CHANNEL_LAYOUT
        uint64_t mask;
        AV_NOWARN_DEPRECATED({ mask = av_get_channel_layout(layout); })
        if (!mask) {
#endif
          AvLog(nullptr, AV_LOG_FATAL, "Unknown channel layout: %s\n", layout);
          return nullptr;
#if FF_API_OLD_CHANNEL_LAYOUT
        }
        AvLog(nullptr, AV_LOG_WARNING,
              "Channel layout '%s' uses a deprecated syntax.\n", layout);
        av_channel_layout_from_mask(&audio_enc->ch_layout, mask);
#endif
      }
    }

    char *sample_fmt = nullptr;
    // MATCH_PER_STREAM_OPT(sample_fmts, str, sample_fmt, oc, st);
    if (sample_fmt && (audio_enc->sample_fmt = av_get_sample_fmt(sample_fmt)) ==
                          AV_SAMPLE_FMT_NONE) {
      AvLog(nullptr, AV_LOG_FATAL, "Invalid sample format '%s'\n", sample_fmt);
      return nullptr;
    }

    // MATCH_PER_STREAM_OPT(audio_sample_rate, i, audio_enc->sample_rate, oc,
    // st);

    // MATCH_PER_STREAM_OPT(apad, str, ost->apad, oc, st);
    ost->apad = av_strdup(ost->apad);

    ost->avfilter = get_ost_filters(oc, ost);
    if (!ost->avfilter) {
      return nullptr;
    }

    /* check for channel mapping for this audio stream */
    for (int n = 0; n < oo.nb_audio_channel_maps; n++) {
      AudioChannelMap *map = &oo.audio_channel_maps[n];
      if ((map->ofile_idx == -1 || ost->file_index == map->ofile_idx) &&
          (map->ostream_idx == -1 || ost->st->index == map->ostream_idx)) {
        InputStream *ist;

        if (map->channel_idx == -1) {
          ist = nullptr;
        } else if (ost->source_index < 0) {
          AvLog(nullptr, AV_LOG_FATAL,
                "Cannot determine input stream for channel mapping %d.%d\n",
                ost->file_index, ost->st->index);
          continue;
        } else {
          ist = input_streams[ost->source_index];
        }

        if (!ist || (ist->file_index == map->file_idx &&
                     ist->st->index == map->stream_idx)) {
          if (av_reallocp_array(&ost->audio_channels_map,
                                ost->audio_channels_mapped + 1,
                                sizeof(*ost->audio_channels_map)) < 0) {
            return nullptr;
          }

          ost->audio_channels_map[ost->audio_channels_mapped++] =
              map->channel_idx;
        }
      }
    }
  }

  if (ost->stream_copy) {
    check_streamcopy_filters(oc, ost, AVMEDIA_TYPE_AUDIO);
  }

  return ost;
}

OutputStream *FfmpegVideoConverter::new_output_stream(AVFormatContext *oc,
                                                      AVMediaType type,
                                                      int source_index) {
  AVStream *st = avformat_new_stream(oc, nullptr);
  int ret = 0;

  if (!st) {
    AvLog(nullptr, AV_LOG_FATAL, "Could not alloc stream.\n");
    return nullptr;
  }

  if (oc->nb_streams - 1 < o.nb_streamid_map) {
    st->id = o.streamid_map[oc->nb_streams - 1];
  }

  OutputStream *ost = static_cast<OutputStream *>(allocate_array_elem(
      &output_streams, sizeof(*output_streams[0]), &nb_output_streams));

  ost->file_index = nb_output_files - 1;
  int idx = oc->nb_streams - 1;
  ost->index = idx;
  ost->st = st;
  ost->forced_kf_ref_pts = AV_NOPTS_VALUE;
  st->codecpar->codec_type = type;

  if (!choose_encoder(oc, ost)) {
    AvLog(nullptr, AV_LOG_FATAL,
          "Error selecting an encoder for stream "
          "%d:%d\n",
          ost->file_index, ost->index);
    return nullptr;
  }

  ost->enc_ctx = avcodec_alloc_context3(ost->enc);
  if (!ost->enc_ctx) {
    AvLog(nullptr, AV_LOG_ERROR, "Error allocating the encoding context.\n");
    return nullptr;
  }
  ost->enc_ctx->codec_type = type;

  ost->ref_par = avcodec_parameters_alloc();
  if (!ost->ref_par) {
    AvLog(nullptr, AV_LOG_ERROR, "Error allocating the encoding parameters.\n");
    return nullptr;
  }

  ost->filtered_frame = av_frame_alloc();
  if (!ost->filtered_frame) {
    return nullptr;
  }

  ost->pkt = av_packet_alloc();
  if (!ost->pkt) {
    return nullptr;
  }

  if (ost->enc) {
    ost->encoder_opts =
        filter_codec_opts(oo.codec_opts, ost->enc->id, oc, st, ost->enc);

    char *preset = nullptr;
    // MATCH_PER_STREAM_OPT(presets, str, preset, oc, st);

    ost->autoscale = true;
    ost->autoscale = oo.autoscale;

    AVIOContext *s = nullptr;
    if (preset && (!(ret = get_preset_file_2(preset, ost->enc->name, &s)))) {
      AVBPrint bprint;
      av_bprint_init(&bprint, 0, AV_BPRINT_SIZE_UNLIMITED);
      do {
        av_bprint_clear(&bprint);
        char *buf = get_line(s, &bprint);
        if (!buf[0] || buf[0] == '#') {
          continue;
        }
        char *arg = nullptr;
        if (!(arg = strchr(buf, '='))) {
          AvLog(nullptr, AV_LOG_FATAL,
                "Invalid line found in the preset file.\n");
          return nullptr;
        }
        *arg++ = 0;
        av_dict_set(&ost->encoder_opts, buf, arg, AV_DICT_DONT_OVERWRITE);
      } while (!s->eof_reached);
      av_bprint_finalize(&bprint, nullptr);
      avio_closep(&s);
    }
    if (ret) {
      AvLog(nullptr, AV_LOG_FATAL,
            "Preset %s specified for stream %d:%d, but could not be opened.\n",
            preset, ost->file_index, ost->index);
      return nullptr;
    }
  } else {
    ost->encoder_opts =
        filter_codec_opts(oo.codec_opts, AV_CODEC_ID_NONE, oc, st, nullptr);
  }

  if (oo.bitexact) {
    ost->enc_ctx->flags |= AV_CODEC_FLAG_BITEXACT;
  }

  const char *time_base = nullptr;
  // MATCH_PER_STREAM_OPT(time_bases, str, time_base, oc, st);
  if (time_base) {
    AVRational q;
    if (av_parse_ratio(&q, time_base, INT_MAX, 0, nullptr) < 0 || q.num <= 0 ||
        q.den <= 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Invalid time base: %s\n", time_base);
      return nullptr;
    }
    st->time_base = q;
  }

  // MATCH_PER_STREAM_OPT(enc_time_bases, str, time_base, oc, st);
  if (time_base) {
    AVRational q;
    if (av_parse_ratio(&q, time_base, INT_MAX, 0, nullptr) < 0 || q.den <= 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Invalid time base: %s\n", time_base);
      return nullptr;
    }
    ost->enc_timebase = q;
  }

  ost->max_frames = oo.max_frames;

  ost->copy_prior_start = -1;
  // MATCH_PER_STREAM_OPT(copy_prior_start, i, ost->copy_prior_start, oc, st);

  const char *bsfs = nullptr;
  // MATCH_PER_STREAM_OPT(bitstream_filters, str, bsfs, oc, st);
  if (bsfs && *bsfs) {
    ret = av_bsf_list_parse_str(bsfs, &ost->bsf_ctx);
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_ERROR,
            "Error parsing bitstream filter sequence '%s': %s\n", bsfs,
            std::to_string(ret).c_str());
      return nullptr;
    }
  }

  char *next;
  char *codec_tag = nullptr;
  // MATCH_PER_STREAM_OPT(codec_tags, str, codec_tag, oc, st);
  if (codec_tag) {
    uint32_t tag = strtol(codec_tag, &next, 0);
    if (*next) tag = AV_RL32(codec_tag);
    ost->st->codecpar->codec_tag = ost->enc_ctx->codec_tag = tag;
  }

  double qscale = -1;
  // MATCH_PER_STREAM_OPT(qscale, dbl, qscale, oc, st);
  if (qscale >= 0) {
    ost->enc_ctx->flags |= AV_CODEC_FLAG_QSCALE;
    ost->enc_ctx->global_quality = FF_QP2LAMBDA * qscale;
  }

  // MATCH_PER_STREAM_OPT(disposition, str, ost->disposition, oc, st);
  ost->disposition = av_strdup(ost->disposition);

  ost->max_muxing_queue_size = 128;
  // MATCH_PER_STREAM_OPT(max_muxing_queue_size, i, ost->max_muxing_queue_size,
  // oc, st);

  ost->muxing_queue_data_size = 0;

  ost->muxing_queue_data_threshold = 50 * 1024 * 1024;
  // MATCH_PER_STREAM_OPT(muxing_queue_data_threshold, i,
  // ost->muxing_queue_data_threshold, oc, st);

  // MATCH_PER_STREAM_OPT(bits_per_raw_sample, i, ost->bits_per_raw_sample, oc,
  // st);

  if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
    ost->enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  av_dict_copy(&ost->sws_dict, oo.sws_dict, 0);

  av_dict_copy(&ost->swr_opts, oo.swr_opts, 0);
  if (ost->enc && av_get_exact_bits_per_sample(ost->enc->id) == 24) {
    av_dict_set(&ost->swr_opts, "output_sample_bits", "24", 0);
  }

  ost->source_index = source_index;
  if (source_index >= 0) {
    ost->sync_ist = input_streams[source_index];
    input_streams[source_index]->discard = false;
    input_streams[source_index]->st->discard =
        static_cast<AVDiscard>(input_streams[source_index]->user_set_discard);
  }
  ost->last_mux_dts = AV_NOPTS_VALUE;

  ost->muxing_queue = av_fifo_alloc2(8, sizeof(AVPacket *), 0);
  if (!ost->muxing_queue) {
    return nullptr;
  }

  // MATCH_PER_STREAM_OPT(copy_initial_nonkeyframes, i,
  // ost->copy_initial_nonkeyframes, oc, st);

  return ost;
}

OutputStream *FfmpegVideoConverter::new_data_stream(AVFormatContext *oc,
                                                    int source_index) {
  OutputStream *ost = new_output_stream(oc, AVMEDIA_TYPE_DATA, source_index);
  if (!ost->stream_copy) {
    AvLog(nullptr, AV_LOG_FATAL,
          "Data stream encoding not supported yet (only streamcopy)\n");
    return nullptr;
  }

  return ost;
}

OutputStream *FfmpegVideoConverter::new_unknown_stream(AVFormatContext *oc,
                                                       int source_index) {
  OutputStream *ost = new_output_stream(oc, AVMEDIA_TYPE_UNKNOWN, source_index);
  if (!ost->stream_copy) {
    AvLog(nullptr, AV_LOG_FATAL,
          "Unknown stream encoding not supported yet (only streamcopy)\n");
    return nullptr;
  }

  return ost;
}

OutputStream *FfmpegVideoConverter::new_attachment_stream(AVFormatContext *oc,
                                                          int source_index) {
  OutputStream *ost =
      new_output_stream(oc, AVMEDIA_TYPE_ATTACHMENT, source_index);
  ost->stream_copy = true;
  ost->finished = ENCODER_FINISHED;
  return ost;
}

OutputStream *FfmpegVideoConverter::new_subtitle_stream(AVFormatContext *oc,
                                                        int source_index) {
  OutputStream *ost =
      new_output_stream(oc, AVMEDIA_TYPE_SUBTITLE, source_index);
  AVStream *st = ost->st;
  AVCodecContext *subtitle_enc = ost->enc_ctx;

  subtitle_enc->codec_type = AVMEDIA_TYPE_SUBTITLE;

  if (!ost->stream_copy) {
    char *frame_size = nullptr;
    // MATCH_PER_STREAM_OPT(frame_sizes, str, frame_size, oc, st);

    if (frame_size &&
        av_parse_video_size(&subtitle_enc->width, &subtitle_enc->height,
                            frame_size) < 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Invalid frame size: %s.\n", frame_size);
      return nullptr;
    }
  }

  return ost;
}

const AVCodec *FfmpegVideoConverter::choose_decoder(AVFormatContext *s,
                                                    AVStream *st) {
  const char *codec_name = nullptr;
  for (const auto &codecname : io.codec_names) {
    int ret = check_stream_specifier(s, st, codecname.GetSpec());
    if (ret > 0) {
      codec_name = codecname.GetCodecName();
    } else if (ret < 0) {
      return nullptr;
    }
  }

  if (codec_name) {
    const AVCodec *codec =
        find_codec_or_die(codec_name, st->codecpar->codec_type, false);
    st->codecpar->codec_id = codec->id;
    if (o.recast_media && st->codecpar->codec_type != codec->type) {
      st->codecpar->codec_type = codec->type;
    }
    return codec;
  } else {
    return avcodec_find_decoder(st->codecpar->codec_id);
  }
}

const AVCodec *FfmpegVideoConverter::find_codec_or_die(const char *name,
                                                       enum AVMediaType type,
                                                       bool encoder) {
  const AVCodecDescriptor *desc;
  const char *codec_string = encoder ? "encoder" : "decoder";
  const AVCodec *codec;

  codec = encoder ? avcodec_find_encoder_by_name(name)
                  : avcodec_find_decoder_by_name(name);

  if (!codec && (desc = avcodec_descriptor_get_by_name(name))) {
    codec = encoder ? avcodec_find_encoder(desc->id)
                    : avcodec_find_decoder(desc->id);
    if (codec) {
      AvLog(nullptr, AV_LOG_VERBOSE, "Matched %s '%s' for codec '%s'.\n",
            codec_string, codec->name, desc->name);
    }
  }

  if (!codec) {
    AvLog(nullptr, AV_LOG_FATAL, "Unknown %s '%s'\n", codec_string, name);
    return nullptr;
  }
  if (codec->type != type && !o.recast_media) {
    AvLog(nullptr, AV_LOG_FATAL, "Invalid %s type '%s'\n", codec_string, name);
    return nullptr;
  }
  return codec;
}

bool FfmpegVideoConverter::choose_encoder(AVFormatContext *s,
                                          OutputStream *ost) {
  AVMediaType type = ost->st->codecpar->codec_type;
  if (type == AVMEDIA_TYPE_VIDEO || type == AVMEDIA_TYPE_AUDIO ||
      type == AVMEDIA_TYPE_SUBTITLE) {
    const char *codec_name = nullptr;
    for (const auto &codecname : oo.codec_names) {
      int ret = check_stream_specifier(s, ost->st, codecname.GetSpec());
      if (ret > 0) {
        codec_name = codecname.GetCodecName();
      } else if (ret < 0) {
        return false;
      }
    }
    if (!codec_name) {
      ost->st->codecpar->codec_id = av_guess_codec(
          s->oformat, nullptr, s->url, nullptr, ost->st->codecpar->codec_type);
      ost->enc = avcodec_find_encoder(ost->st->codecpar->codec_id);
      if (!ost->enc) {
        AvLog(
            NULL, AV_LOG_FATAL,
            "Automatic encoder selection failed for "
            "output stream #%d:%d. Default encoder for format %s (codec %s) is "
            "probably disabled. Please choose an encoder manually.\n",
            ost->file_index, ost->index, s->oformat->name,
            avcodec_get_name(ost->st->codecpar->codec_id));
        return false;
      }
    } else if (!strcmp(codec_name, "copy")) {
      ost->stream_copy = true;
    } else {
      ost->enc =
          find_codec_or_die(codec_name, ost->st->codecpar->codec_type, true);
      ost->st->codecpar->codec_id = ost->enc->id;
    }
    ost->encoding_needed = !ost->stream_copy;
  } else {
    /* no encoding supported for other media types */
    ost->stream_copy = true;
    ost->encoding_needed = false;
  }
  return true;
}

int FfmpegVideoConverter::get_preset_file_2(const char *preset_name,
                                            const char *codec_name,
                                            AVIOContext **s) {
  int i, ret = -1;
  char filename[1000];
  // TODO
  char *env_avconv_datadir = nullptr;  // getenv_utf8("AVCONV_DATADIR");
  char *env_home = nullptr;            // getenv_utf8("HOME");
  const char *base[3] = {
      env_avconv_datadir,
      env_home,
      nullptr,
  };

  for (i = 0; i < FF_ARRAY_ELEMS(base) && ret < 0; i++) {
    if (!base[i]) {
      continue;
    }
    const AVIOInterruptCB int_cb{decode_interrupt_cb, this};
    if (codec_name) {
      snprintf(filename, sizeof(filename), "%s%s/%s-%s.avpreset", base[i],
               i != 1 ? "" : "/.avconv", codec_name, preset_name);
      ret = avio_open2(s, filename, AVIO_FLAG_READ, &int_cb, nullptr);
    }
    if (ret < 0) {
      snprintf(filename, sizeof(filename), "%s%s/%s.avpreset", base[i],
               i != 1 ? "" : "/.avconv", preset_name);
      ret = avio_open2(s, filename, AVIO_FLAG_READ, &int_cb, nullptr);
    }
  }
  // TODO
  // freeenv_utf8(env_home);
  // freeenv_utf8(env_avconv_datadir);
  return ret;
}

char *FfmpegVideoConverter::get_line(AVIOContext *s, AVBPrint *bprint) {
  char c;

  while ((c = avio_r8(s)) && c != '\n') {
    av_bprint_chars(bprint, c, 1);
  }

  if (!av_bprint_is_complete(bprint)) {
    AvLog(nullptr, AV_LOG_FATAL,
          "Could not alloc buffer for reading preset.\n");
    return nullptr;
  }
  return bprint->str;
}

bool FfmpegVideoConverter::parse_matrix_coeffs(uint16_t *dest,
                                               const char *str) {
  const char *p = str;
  for (int i = 0;; i++) {
    dest[i] = atoi(p);
    if (i == 63) break;
    p = strchr(p, ',');
    if (!p) {
      AvLog(nullptr, AV_LOG_FATAL,
            "Syntax error in matrix \"%s\" at coeff %d\n", str, i);
      return false;
    }
    p++;
  }
  return true;
}

bool FfmpegVideoConverter::parse_and_set_vsync(const char *arg, int *vsync_var,
                                               int file_idx, int st_idx,
                                               int is_global) {
  if (!av_strcasecmp(arg, "cfr")) {
    *vsync_var = VSYNC_CFR;
  } else if (!av_strcasecmp(arg, "vfr")) {
    *vsync_var = VSYNC_VFR;
  } else if (!av_strcasecmp(arg, "passthrough")) {
    *vsync_var = VSYNC_PASSTHROUGH;
  } else if (!av_strcasecmp(arg, "drop")) {
    *vsync_var = VSYNC_DROP;
  } else if (!is_global && !av_strcasecmp(arg, "auto")) {
    *vsync_var = VSYNC_AUTO;
  } else if (!is_global) {
    AvLog(nullptr, AV_LOG_FATAL,
          "Invalid value %s specified for fps_mode of #%d:%d.\n", arg, file_idx,
          st_idx);
    return false;
  }

  if (is_global && *vsync_var == VSYNC_AUTO) {
    video_sync_method = static_cast<VideoSyncMethod>(
        parse_number_or_die("vsync", arg, OPT_INT, VSYNC_AUTO, VSYNC_VFR));
    AvLog(nullptr, AV_LOG_WARNING,
          "Passing a number to -vsync is deprecated,"
          " use a string argument as described in the manual.\n");
  }
  return true;
}

char *FfmpegVideoConverter::get_ost_filters(AVFormatContext *oc,
                                            OutputStream *ost) {
  AVStream *st = ost->st;

  if (ost->filters_script && ost->filters) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Both -filter and -filter_script set for "
          "output stream #%d:%d.\n",
          nb_output_files, st->index);
    return nullptr;
  }

  if (ost->filters_script) {
    return read_file(ost->filters_script);
  } else if (ost->filters) {
    return av_strdup(ost->filters);
  }

  return av_strdup(st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ? "null"
                                                                  : "anull");
}

bool FfmpegVideoConverter::check_streamcopy_filters(AVFormatContext *oc,
                                                    const OutputStream *ost,
                                                    AVMediaType type) {
  if (ost->filters_script || ost->filters) {
    AvLog(nullptr, AV_LOG_ERROR,
          "%s '%s' was defined for %s output stream %d:%d but codec copy was "
          "selected.\n"
          "Filtering and streamcopy cannot be used together.\n",
          ost->filters ? "Filtergraph" : "Filtergraph script",
          ost->filters ? ost->filters : ost->filters_script,
          av_get_media_type_string(type), ost->file_index, ost->index);
    return false;
  }
  return true;
}

int FfmpegVideoConverter::set_dispositions(OutputFile *of,
                                           AVFormatContext *ctx) {
  int nb_streams[AVMEDIA_TYPE_NB] = {0};
  int have_default[AVMEDIA_TYPE_NB] = {0};
  int have_manual = 0;

  // first, copy the input dispositions
  for (int i = 0; i < ctx->nb_streams; i++) {
    OutputStream *ost = output_streams[of->ost_index + i];

    nb_streams[ost->st->codecpar->codec_type]++;

    have_manual |= !!ost->disposition;

    if (ost->source_index >= 0) {
      ost->st->disposition = input_streams[ost->source_index]->st->disposition;

      if (ost->st->disposition & AV_DISPOSITION_DEFAULT) {
        have_default[ost->st->codecpar->codec_type] = 1;
      }
    }
  }

  if (have_manual) {
    // process manually set dispositions - they override the above copy
    for (int i = 0; i < ctx->nb_streams; i++) {
      OutputStream *ost = output_streams[of->ost_index + i];
      int ret;

      if (!ost->disposition) {
        continue;
      }

#if LIBAVFORMAT_VERSION_MAJOR >= 60
      ret = av_opt_set(ost->st, "disposition", ost->disposition, 0);
#else
      {
        const AVClass *classs = av_stream_get_class();
        const AVOption *o = av_opt_find(&classs, "disposition", nullptr, 0,
                                        AV_OPT_SEARCH_FAKE_OBJ);

        av_assert0(o);
        ret = av_opt_eval_flags(&classs, o, ost->disposition,
                                &ost->st->disposition);
      }
#endif

      if (ret < 0) {
        return ret;
      }
    }
  } else {
    // For each media type with more than one stream, find a suitable stream to
    // mark as default, unless one is already marked default.
    // "Suitable" means the first of that type, skipping attached pictures.
    for (int i = 0; i < ctx->nb_streams; i++) {
      OutputStream *ost = output_streams[of->ost_index + i];
      enum AVMediaType type = ost->st->codecpar->codec_type;

      if (nb_streams[type] < 2 || have_default[type] ||
          ost->st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
        continue;
      }

      ost->st->disposition |= AV_DISPOSITION_DEFAULT;
      have_default[type] = 1;
    }
  }

  return 0;
}

bool FfmpegVideoConverter::set_channel_layout(OutputFilter *f,
                                              OutputStream *ost) {
  int i, err;

  if (ost->enc_ctx->ch_layout.order != AV_CHANNEL_ORDER_UNSPEC) {
    /* Pass the layout through for all orders but UNSPEC */
    err = av_channel_layout_copy(&f->ch_layout, &ost->enc_ctx->ch_layout);
    if (err < 0) {
      return false;
    }
  }

  /* Requested layout is of order UNSPEC */
  if (!ost->enc->ch_layouts) {
    /* Use the default native layout for the requested amount of channels when
       the encoder doesn't have a list of supported layouts */
    av_channel_layout_default(&f->ch_layout,
                              ost->enc_ctx->ch_layout.nb_channels);
    return true;
  }
  /* Encoder has a list of supported layouts. Pick the first layout in it with
     the same amount of channels as the requested layout */
  for (i = 0; ost->enc->ch_layouts[i].nb_channels; i++) {
    if (ost->enc->ch_layouts[i].nb_channels ==
        ost->enc_ctx->ch_layout.nb_channels) {
      break;
    }
  }
  if (ost->enc->ch_layouts[i].nb_channels) {
    /* Use it if one is found */
    err = av_channel_layout_copy(&f->ch_layout, &ost->enc->ch_layouts[i]);
    if (err < 0) {
      return false;
    }
  }
  /* If no layout for the amount of channels requested was found, use the
     default native layout for it. */
  av_channel_layout_default(&f->ch_layout, ost->enc_ctx->ch_layout.nb_channels);
  return true;
}

namespace {
int64_t parse_time_or_die(const char *context, const char *timestr,
                          int is_duration) {
  int64_t us;
  if (av_parse_time(&us, timestr, is_duration) < 0) {
    AvLog(nullptr, AV_LOG_FATAL, "Invalid %s specification for %s: %s\n",
          is_duration ? "duration" : "date", context, timestr);
    return -1;
  }
  return us;
}
int compare_int64(const void *a, const void *b) {
  return FFDIFFSIGN(*(const int64_t *)a, *(const int64_t *)b);
}
int init_output_bsfs(OutputStream *ost) {
  AVBSFContext *ctx = ost->bsf_ctx;

  if (!ctx) {
    return 0;
  }

  int ret = avcodec_parameters_copy(ctx->par_in, ost->st->codecpar);
  if (ret < 0) {
    return ret;
  }

  ctx->time_base_in = ost->st->time_base;

  ret = av_bsf_init(ctx);
  if (ret < 0) {
    AvLog(nullptr, AV_LOG_ERROR, "Error initializing bitstream filter: %s\n",
          ctx->filter->name);
    return ret;
  }

  ret = avcodec_parameters_copy(ost->st->codecpar, ctx->par_out);
  if (ret < 0) {
    return ret;
  }
  ost->st->time_base = ctx->time_base_out;

  return 0;
}
double get_rotation(int32_t *displaymatrix) {
  double theta = 0;
  if (displaymatrix) {
    theta = -round(av_display_rotation_get((int32_t *)displaymatrix));
  }

  theta -= 360 * floor(theta / 360 + 0.9 / 360);

  if (fabs(theta - 90 * round(theta / 90)) > 2) {
    AvLog(
        nullptr, AV_LOG_WARNING,
        "Odd rotation angle.\n"
        "If you want to help, upload a sample "
        "of this file to https://streams.videolan.org/upload/ "
        "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");
  }
  return theta;
}

int sub2video_get_blank_frame(InputStream *ist) {
  AVFrame *frame = ist->sub2video.frame;
  av_frame_unref(frame);
  ist->sub2video.frame->width =
      ist->dec_ctx->width ? ist->dec_ctx->width : ist->sub2video.w;
  ist->sub2video.frame->height =
      ist->dec_ctx->height ? ist->dec_ctx->height : ist->sub2video.h;
  ist->sub2video.frame->format = AV_PIX_FMT_RGB32;
  int ret;
  if ((ret = av_frame_get_buffer(frame, 0)) < 0) {
    return ret;
  }
  memset(frame->data[0], 0, frame->height * frame->linesize[0]);
  return 0;
}

void sub2video_copy_rect(uint8_t *dst, int dst_linesize, int w, int h,
                         AVSubtitleRect *r) {
  if (r->type != SUBTITLE_BITMAP) {
    AvLog(nullptr, AV_LOG_WARNING, "sub2video: non-bitmap subtitle\n");
    return;
  }
  if (r->x < 0 || r->x + r->w > w || r->y < 0 || r->y + r->h > h) {
    AvLog(nullptr, AV_LOG_WARNING,
          "sub2video: rectangle (%d %d %d %d) overflowing %d %d\n", r->x, r->y,
          r->w, r->h, w, h);
    return;
  }

  dst += r->y * dst_linesize + r->x * 4;
  uint8_t *src = r->data[0];
  uint32_t *pal = (uint32_t *)r->data[1];
  for (int y = 0; y < r->h; y++) {
    uint32_t *dst2 = (uint32_t *)dst;
    uint8_t *src2 = src;
    for (int x = 0; x < r->w; x++) {
      *(dst2++) = pal[*(src2++)];
    }
    dst += dst_linesize;
    src += r->linesize[0];
  }
}

void sub2video_push_ref(InputStream *ist, int64_t pts) {
  AVFrame *frame = ist->sub2video.frame;
  av_assert1(frame->data[0]);
  ist->sub2video.last_pts = frame->pts = pts;
  for (int i = 0; i < ist->nb_filters; i++) {
    int ret = av_buffersrc_add_frame_flags(
        ist->filters[i]->filter, frame,
        AV_BUFFERSRC_FLAG_KEEP_REF | AV_BUFFERSRC_FLAG_PUSH);
    if (ret != AVERROR_EOF && ret < 0) {
      AvLog(nullptr, AV_LOG_WARNING,
            "Error while add the frame to buffer source(%s).\n",
            AvErr2Str(ret));
    }
  }
}

void sub2video_update(InputStream *ist, int64_t heartbeat_pts,
                      AVSubtitle *sub) {
  AVFrame *frame = ist->sub2video.frame;

  if (!frame) {
    return;
  }

  int num_rects;
  int64_t pts, end_pts;
  if (sub) {
    pts = av_rescale_q(sub->pts + sub->start_display_time * 1000LL,
                       {1, AV_TIME_BASE}, ist->st->time_base);
    end_pts = av_rescale_q(sub->pts + sub->end_display_time * 1000LL,
                           {1, AV_TIME_BASE}, ist->st->time_base);
    num_rects = sub->num_rects;
  } else {
    /* If we are initializing the system, utilize current heartbeat
       PTS as the start time, and show until the following subpicture
       is received. Otherwise, utilize the previous subpicture's end time
       as the fall-back value. */
    pts = ist->sub2video.initialize ? heartbeat_pts : ist->sub2video.end_pts;
    end_pts = INT64_MAX;
    num_rects = 0;
  }
  if (sub2video_get_blank_frame(ist) < 0) {
    AvLog(ist->dec_ctx, AV_LOG_ERROR, "Impossible to get a blank canvas.\n");
    return;
  }

  uint8_t *dst = frame->data[0];
  int dst_linesize = frame->linesize[0];
  for (int i = 0; i < num_rects; i++) {
    sub2video_copy_rect(dst, dst_linesize, frame->width, frame->height,
                        sub->rects[i]);
  }
  sub2video_push_ref(ist, pts);
  ist->sub2video.end_pts = end_pts;
  ist->sub2video.initialize = false;
}

}  // namespace

bool FfmpegVideoConverter::parse_forced_key_frames(char *kf, OutputStream *ost,
                                                   AVCodecContext *avctx) {
  char *p;
  int n = 1;
  int index = 0;
  for (p = kf; *p; p++) {
    if (*p == ',') {
      n++;
    }
  }
  int size = n;
  int64_t *pts = static_cast<int64_t *>(av_malloc_array(size, sizeof(*pts)));
  if (!pts) {
    AvLog(nullptr, AV_LOG_FATAL,
          "Could not allocate forced key frames array.\n");
    return false;
  }

  p = kf;
  for (int i = 0; i < n; i++) {
    char *next = strchr(p, ',');

    if (next) {
      *next++ = 0;
    }

    int64_t t;
    if (!memcmp(p, "chapters", 8)) {
      AVFormatContext *avf = output_files[ost->file_index]->ctx;
      if (avf->nb_chapters > INT_MAX - size ||
          !(pts = static_cast<int64_t *>(av_realloc_f(
                pts, size += avf->nb_chapters - 1, sizeof(*pts))))) {
        AvLog(nullptr, AV_LOG_FATAL,
              "Could not allocate forced key frames array.\n");
        return false;
      }
      t = p[8] ? parse_time_or_die("force_key_frames", p + 8, 1) : 0;
      t = av_rescale_q(t, {1, AV_TIME_BASE}, avctx->time_base);

      for (int j = 0; j < avf->nb_chapters; j++) {
        AVChapter *c = avf->chapters[j];
        av_assert1(index < size);
        pts[index++] =
            av_rescale_q(c->start, c->time_base, avctx->time_base) + t;
      }

    } else {
      t = parse_time_or_die("force_key_frames", p, 1);
      av_assert1(index < size);
      pts[index++] = av_rescale_q(t, {1, AV_TIME_BASE}, avctx->time_base);
    }

    p = next;
  }

  av_assert0(index == size);
  qsort(pts, size, sizeof(*pts), compare_int64);
  ost->forced_kf_count = size;
  ost->forced_kf_pts = pts;
  return true;
}

void FfmpegVideoConverter::update_benchmark(const char *fmt, ...) {
  if (do_benchmark_all) {
    BenchmarkTimeStamps t = get_benchmark_time_stamps();
    va_list va;
    char buf[1024];

    if (fmt) {
      va_start(va, fmt);
      vsnprintf(buf, sizeof(buf), fmt, va);
      va_end(va);
      AvLog(nullptr, AV_LOG_INFO,
            "bench: %8"
            "llu"
            " user %8"
            "llu"
            " sys %8"
            "llu"
            " real %s \n",
            t.user_usec - current_time.user_usec,
            t.sys_usec - current_time.sys_usec,
            t.real_usec - current_time.real_usec, buf);
    }
    current_time = t;
  }
}

void FfmpegVideoConverter::close_output_stream(OutputStream *ost) {
  OutputFile *of = output_files[ost->file_index];
  AVRational time_base =
      ost->stream_copy ? ost->mux_timebase : ost->enc_ctx->time_base;

  ost->finished = static_cast<OSTFinished>(ost->finished | ENCODER_FINISHED);
  if (of->shortest) {
    int64_t end = av_rescale_q(ost->sync_opts - ost->first_pts, time_base,
                               {1, AV_TIME_BASE});
    of->recording_time = FFMIN(of->recording_time, end);
  }
}

/*
 * Send a single packet to the output, applying any bitstream filters
 * associated with the output stream.  This may result in any number
 * of packets actually being written, depending on what bitstream
 * filters are applied.  The supplied packet is consumed and will be
 * blank (as if newly-allocated) when this function returns.
 *
 * If eof is set, instead indicate EOF to all bitstream filters and
 * therefore flush any delayed packets to the output.  A blank packet
 * must be supplied in this case.
 */
bool FfmpegVideoConverter::output_packet(OutputFile *of, AVPacket *pkt,
                                         OutputStream *ost, bool eof) {
  int ret = 0;

  /* apply the output bitstream filters */
  if (ost->bsf_ctx) {
    ret = av_bsf_send_packet(ost->bsf_ctx, eof ? nullptr : pkt);
    if (ret < 0) {
      goto finish;
    }
    while ((ret = av_bsf_receive_packet(ost->bsf_ctx, pkt)) >= 0) {
      of_write_packet(of, pkt, ost, 0);
    }
    if (ret == AVERROR(EAGAIN)) {
      ret = 0;
    }
  } else if (!eof) {
    of_write_packet(of, pkt, ost, 0);
  }

finish:
  if (ret < 0 && ret != AVERROR_EOF) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Error applying bitstream filters to an output "
          "packet for stream #%d:%d.\n",
          ost->file_index, ost->index);
    if (exit_on_error) {
      return false;
    }
  }
  return true;
}

int FfmpegVideoConverter::need_output(void) {
  for (int i = 0; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];
    OutputFile *of = output_files[ost->file_index];
    AVFormatContext *os = output_files[ost->file_index]->ctx;

    if (ost->finished || (os->pb && avio_tell(os->pb) >= of->limit_filesize)) {
      continue;
    }
    if (ost->frame_number >= ost->max_frames) {
      for (int j = 0; j < of->ctx->nb_streams; j++) {
        close_output_stream(output_streams[of->ost_index + j]);
      }
      continue;
    }

    return 1;
  }

  return 0;
}

/**
 * Select the output stream to process.
 *
 * @return  selected output stream, or nullptr if none available
 */
OutputStream *FfmpegVideoConverter::choose_output(void) {
  int64_t opts_min = INT64_MAX;
  OutputStream *ost_min = nullptr;

  for (int i = 0; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];
    int64_t opts = ost->last_mux_dts == AV_NOPTS_VALUE
                       ? INT64_MIN
                       : av_rescale_q(ost->last_mux_dts, ost->st->time_base,
                                      {1, AV_TIME_BASE});
    if (ost->last_mux_dts == AV_NOPTS_VALUE) {
      AvLog(nullptr, AV_LOG_DEBUG,
            "cur_dts is invalid st:%d (%d) [init:%d i_done:%d finish:%d] "
            "(this is harmless if it occurs once at the start per stream)\n",
            ost->st->index, ost->st->id, ost->initialized, ost->inputs_done,
            ost->finished);
    }

    if (!ost->initialized && !ost->inputs_done) {
      return ost->unavailable ? nullptr : ost;
    }

    if (!ost->finished && opts < opts_min) {
      opts_min = opts;
      ost_min = ost->unavailable ? nullptr : ost;
    }
  }
  return ost_min;
}

/*
 * The following code is the main loop of the file converter
 */
int FfmpegVideoConverter::transcode(void) {
  AVFormatContext *os;
  OutputStream *ost;
  InputStream *ist;
  int64_t timer_start;
  int64_t total_packets_written = 0;

  int ret = 0;
  if (!transcode_init()) {
    ret = -1;
    goto fail;
  }

  timer_start = av_gettime_relative();

  while (!received_sigterm) {
    int64_t cur_time = av_gettime_relative();

    /* check if there's any stream where output is still needed */
    if (!need_output()) {
      AvLog(nullptr, AV_LOG_VERBOSE,
            "No more output streams to write to, finishing.\n");
      break;
    }

    ret = transcode_step();
    if (ret < 0 && ret != AVERROR_EOF) {
      break;
    }

    /* dump report by using the output first video and audio streams */
    print_report(false, timer_start, cur_time);
  }

  /* at the end of stream, we must flush the decoder buffers */
  for (int i = 0; i < nb_input_streams; i++) {
    ist = input_streams[i];
    if (!input_files[ist->file_index]->eof_reached) {
      process_input_packet(ist, nullptr, 0);
    }
  }
  flush_encoders();

  // term_exit();

  /* write the trailer if needed */
  for (int i = 0; i < nb_output_files; i++) {
    ret = of_write_trailer(output_files[i]);
    if (ret < 0 && exit_on_error) {
      return -1;
    }
  }

  /* dump report by using the first video and audio streams */
  print_report(true, timer_start, av_gettime_relative());

  /* close the output files */
  for (int i = 0; i < nb_output_files; i++) {
    os = output_files[i]->ctx;
    if (os && os->oformat && !(os->oformat->flags & AVFMT_NOFILE)) {
      if ((ret = avio_closep(&os->pb)) < 0) {
        if (exit_on_error) {
          return -1;
        }
      }
    }
  }

  /* close each encoder */
  for (int i = 0; i < nb_output_streams; i++) {
    ost = output_streams[i];
    if (ost->encoding_needed) {
      av_freep(&ost->enc_ctx->stats_in);
    }
    total_packets_written += ost->packets_written;
    if (!ost->packets_written &&
        (abort_on_flags & ABORT_ON_FLAG_EMPTY_OUTPUT_STREAM)) {
      AvLog(nullptr, AV_LOG_FATAL, "Empty output on stream %d.\n", i);
      return -1;
    }
  }

  if (!total_packets_written && (abort_on_flags & ABORT_ON_FLAG_EMPTY_OUTPUT)) {
    AvLog(nullptr, AV_LOG_FATAL, "Empty output\n");
    return -1;
  }

  /* close each decoder */
  for (int i = 0; i < nb_input_streams; i++) {
    ist = input_streams[i];
    if (ist->decoding_needed) {
      avcodec_close(ist->dec_ctx);
      if (ist->hwaccel_uninit) {
        ist->hwaccel_uninit(ist->dec_ctx);
      }
    }
  }

  hw_device_free_all();

  /* finished ! */
  ret = 0;

fail:
  if (output_streams) {
    for (int i = 0; i < nb_output_streams; i++) {
      ost = output_streams[i];
      if (ost) {
        if (ost->logfile) {
          if (fclose(ost->logfile)) {
          }
          ost->logfile = nullptr;
        }
        av_freep(&ost->forced_kf_pts);
        av_freep(&ost->apad);
        av_freep(&ost->disposition);
        av_dict_free(&ost->encoder_opts);
        av_dict_free(&ost->sws_dict);
        av_dict_free(&ost->swr_opts);
      }
    }
  }
  return ret;
}

bool FfmpegVideoConverter::transcode_init(void) {
  AVFormatContext *oc;
  OutputStream *ost;
  InputStream *ist;
  char error[1024] = {0};

  int ret = 0;
  for (int i = 0; i < nb_filtergraphs; i++) {
    FilterGraph *fg = filtergraphs[i];
    for (int j = 0; j < fg->nb_outputs; j++) {
      OutputFilter *ofilter = fg->outputs[j];
      if (!ofilter->ost || ofilter->ost->source_index >= 0) {
        continue;
      }
      if (fg->nb_inputs != 1) {
        continue;
      }
      int k;
      for (k = nb_input_streams - 1; k >= 0; k--) {
        if (fg->inputs[0]->ist == input_streams[k]) {
          break;
        }
      }
      ofilter->ost->source_index = k;
    }
  }

  /* init framerate emulation */
  for (int i = 0; i < nb_input_files; i++) {
    InputFile *ifile = input_files[i];
    if (ifile->readrate || ifile->rate_emu) {
      for (int j = 0; j < ifile->nb_streams; j++) {
        input_streams[j + ifile->ist_index]->start = av_gettime_relative();
      }
    }
  }

  /* init input streams */
  for (int i = 0; i < nb_input_streams; i++)
    if ((ret = init_input_stream(i, error, sizeof(error))) < 0) {
      for (int j = 0; j < nb_output_streams; j++) {
        ost = output_streams[j];
        avcodec_close(ost->enc_ctx);
      }
      goto dump_format;
    }

  /*
   * initialize stream copy and subtitle/data streams.
   * Encoded AVFrame based streams will get initialized as follows:
   * - when the first AVFrame is received in do_video_out
   * - just before the first AVFrame is received in either transcode_step
   *   or reap_filters due to us requiring the filter chain buffer sink
   *   to be configured with the correct audio frame size, which is only
   *   known after the encoder is initialized.
   */
  for (int i = 0; i < nb_output_streams; i++) {
    if (!output_streams[i]->stream_copy &&
        (output_streams[i]->enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
         output_streams[i]->enc_ctx->codec_type == AVMEDIA_TYPE_AUDIO)) {
      continue;
    }

    ret = init_output_stream_wrapper(output_streams[i], nullptr, 0);
    if (ret < 0) {
      goto dump_format;
    }
  }

  /* discard unused programs */
  for (int i = 0; i < nb_input_files; i++) {
    InputFile *ifile = input_files[i];
    for (int j = 0; j < ifile->ctx->nb_programs; j++) {
      AVProgram *p = ifile->ctx->programs[j];
      AVDiscard discard = AVDISCARD_ALL;

      for (int k = 0; k < p->nb_stream_indexes; k++)
        if (!input_streams[ifile->ist_index + p->stream_index[k]]->discard) {
          discard = AVDISCARD_DEFAULT;
          break;
        }
      p->discard = discard;
    }
  }

  /* write headers for files with no streams */
  for (int i = 0; i < nb_output_files; i++) {
    oc = output_files[i]->ctx;
    if (output_files[i]->format->flags & AVFMT_NOSTREAMS &&
        oc->nb_streams == 0) {
      if (!of_check_init(output_files[i])) {
        goto dump_format;
      }
    }
  }

dump_format:
  /* dump the stream mapping */
  AvLog(nullptr, AV_LOG_INFO, "Stream mapping:\n");
  for (int i = 0; i < nb_input_streams; i++) {
    ist = input_streams[i];

    for (int j = 0; j < ist->nb_filters; j++) {
      if (!filtergraph_is_simple(ist->filters[j]->graph)) {
        AvLog(nullptr, AV_LOG_INFO, "  Stream #%d:%d (%s) -> %s",
              ist->file_index, ist->st->index, ist->dec ? ist->dec->name : "?",
              ist->filters[j]->name);
        if (nb_filtergraphs > 1)
          AvLog(nullptr, AV_LOG_INFO, " (graph %d)",
                ist->filters[j]->graph->index);
        AvLog(nullptr, AV_LOG_INFO, "\n");
      }
    }
  }

  for (int i = 0; i < nb_output_streams; i++) {
    ost = output_streams[i];

    if (ost->attachment_filename) {
      /* an attached file */
      AvLog(nullptr, AV_LOG_INFO, "  File %s -> Stream #%d:%d\n",
            ost->attachment_filename, ost->file_index, ost->index);
      continue;
    }

    if (ost->filter && !filtergraph_is_simple(ost->filter->graph)) {
      /* output from a complex graph */
      AvLog(nullptr, AV_LOG_INFO, "  %s", ost->filter->name);
      if (nb_filtergraphs > 1)
        AvLog(nullptr, AV_LOG_INFO, " (graph %d)", ost->filter->graph->index);

      AvLog(nullptr, AV_LOG_INFO, " -> Stream #%d:%d (%s)\n", ost->file_index,
            ost->index, ost->enc ? ost->enc->name : "?");
      continue;
    }

    AvLog(nullptr, AV_LOG_INFO, "  Stream #%d:%d -> #%d:%d",
          input_streams[ost->source_index]->file_index,
          input_streams[ost->source_index]->st->index, ost->file_index,
          ost->index);
    if (ost->sync_ist != input_streams[ost->source_index])
      AvLog(nullptr, AV_LOG_INFO, " [sync #%d:%d]", ost->sync_ist->file_index,
            ost->sync_ist->st->index);
    if (ost->stream_copy) {
      AvLog(nullptr, AV_LOG_INFO, " (copy)");
    } else {
      const AVCodec *in_codec = input_streams[ost->source_index]->dec;
      const AVCodec *out_codec = ost->enc;
      const char *decoder_name = "?";
      const char *in_codec_name = "?";
      const char *encoder_name = "?";
      const char *out_codec_name = "?";
      const AVCodecDescriptor *desc;

      if (in_codec) {
        decoder_name = in_codec->name;
        desc = avcodec_descriptor_get(in_codec->id);
        if (desc) in_codec_name = desc->name;
        if (!strcmp(decoder_name, in_codec_name)) decoder_name = "native";
      }

      if (out_codec) {
        encoder_name = out_codec->name;
        desc = avcodec_descriptor_get(out_codec->id);
        if (desc) out_codec_name = desc->name;
        if (!strcmp(encoder_name, out_codec_name)) encoder_name = "native";
      }

      AvLog(nullptr, AV_LOG_INFO, " (%s (%s) -> %s (%s))", in_codec_name,
            decoder_name, out_codec_name, encoder_name);
    }
    AvLog(nullptr, AV_LOG_INFO, "\n");
  }

  if (ret != 0) {
    AvLog(nullptr, AV_LOG_ERROR, "%s\n", error);
    return false;
  }

  return true;
}

int FfmpegVideoConverter::init_input_stream(int ist_index, char *error,
                                            int error_len) {
  InputStream *ist = input_streams[ist_index];

  if (ist->decoding_needed) {
    const AVCodec *codec = ist->dec;
    if (!codec) {
      snprintf(error, error_len,
               "Decoder (codec %s) not found for input stream #%d:%d",
               avcodec_get_name(ist->dec_ctx->codec_id), ist->file_index,
               ist->st->index);
      return AVERROR(EINVAL);
    }

    ist->dec_ctx->opaque = ist;
    ist->dec_ctx->get_format = get_format;
#if LIBAVCODEC_VERSION_MAJOR < 60
    AV_NOWARN_DEPRECATED({ ist->dec_ctx->thread_safe_callbacks = 1; })
#endif

    if (ist->dec_ctx->codec_id == AV_CODEC_ID_DVB_SUBTITLE &&
        (ist->decoding_needed & DECODING_FOR_OST)) {
      av_dict_set(&ist->decoder_opts, "compute_edt", "1",
                  AV_DICT_DONT_OVERWRITE);
      if (ist->decoding_needed & DECODING_FOR_FILTER)
        AvLog(
            nullptr, AV_LOG_WARNING,
            "Warning using DVB subtitles for filtering and output at the same "
            "time is not fully supported, also see -compute_edt [0|1]\n");
    }

    /* Useful for subtitles retiming by lavf (FIXME), skipping samples in
     * audio, and video decoders such as cuvid or mediacodec */
    ist->dec_ctx->pkt_timebase = ist->st->time_base;

    if (!av_dict_get(ist->decoder_opts, "threads", nullptr, 0)) {
      av_dict_set(&ist->decoder_opts, "threads", "auto", 0);
    }
    /* Attached pics are sparse, therefore we would not want to delay their
     * decoding till EOF. */
    if (ist->st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
      av_dict_set(&ist->decoder_opts, "threads", "1", 0);
    }

    int ret = hw_device_setup_for_decode(ist);
    if (ret < 0) {
      return ret;
    }

    if ((ret = avcodec_open2(ist->dec_ctx, codec, &ist->decoder_opts)) < 0) {
      if (ret == AVERROR_EXPERIMENTAL) {
        return ret;
      }
      snprintf(error, error_len,
               "Error while opening decoder for input stream "
               "#%d:%d : %s",
               ist->file_index, ist->st->index, AvErr2Str(ret));
      return ret;
    }
    // assert_avoptions(ist->decoder_opts);
  }

  ist->next_pts = AV_NOPTS_VALUE;
  ist->next_dts = AV_NOPTS_VALUE;

  return 0;
}

int FfmpegVideoConverter::init_output_stream_wrapper(OutputStream *ost,
                                                     AVFrame *frame,
                                                     unsigned int fatal) {
  char error[1024] = {0};

  if (ost->initialized) {
    return 0;
  }

  int ret = init_output_stream(ost, frame, error, sizeof(error));
  if (ret < 0) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Error initializing output stream %d:%d -- %s\n", ost->file_index,
          ost->index, error);

    if (fatal) {
      return ret;
    }
  }

  return ret;
}

InputStream *FfmpegVideoConverter::get_input_stream(OutputStream *ost) {
  if (ost->source_index >= 0) {
    return input_streams[ost->source_index];
  }
  return nullptr;
}

int FfmpegVideoConverter::init_output_stream_streamcopy(OutputStream *ost) {
  OutputFile *of = output_files[ost->file_index];
  InputStream *ist = get_input_stream(ost);
  AVCodecParameters *par_dst = ost->st->codecpar;
  AVCodecParameters *par_src = ost->ref_par;
  AVRational sar;
  uint32_t codec_tag = par_dst->codec_tag;

  av_assert0(ist && !ost->filter);

  int ret = avcodec_parameters_to_context(ost->enc_ctx, ist->st->codecpar);
  if (ret >= 0) ret = av_opt_set_dict(ost->enc_ctx, &ost->encoder_opts);
  if (ret < 0) {
    AvLog(nullptr, AV_LOG_FATAL, "Error setting up codec context options.\n");
    return ret;
  }

  ret = avcodec_parameters_from_context(par_src, ost->enc_ctx);
  if (ret < 0) {
    AvLog(nullptr, AV_LOG_FATAL, "Error getting reference codec parameters.\n");
    return ret;
  }

  if (!codec_tag) {
    unsigned int codec_tag_tmp;
    if (!of->format->codec_tag ||
        av_codec_get_id(of->format->codec_tag, par_src->codec_tag) ==
            par_src->codec_id ||
        !av_codec_get_tag2(of->format->codec_tag, par_src->codec_id,
                           &codec_tag_tmp))
      codec_tag = par_src->codec_tag;
  }

  ret = avcodec_parameters_copy(par_dst, par_src);
  if (ret < 0) return ret;

  par_dst->codec_tag = codec_tag;

  if (!ost->frame_rate.num) ost->frame_rate = ist->framerate;

  if (ost->frame_rate.num)
    ost->st->avg_frame_rate = ost->frame_rate;
  else
    ost->st->avg_frame_rate = ist->st->avg_frame_rate;

  ret = avformat_transfer_internal_stream_timing_info(of->format, ost->st,
                                                      ist->st, copy_tb);
  if (ret < 0) return ret;

  // copy timebase while removing common factors
  if (ost->st->time_base.num <= 0 || ost->st->time_base.den <= 0) {
    if (ost->frame_rate.num)
      ost->st->time_base = av_inv_q(ost->frame_rate);
    else
      ost->st->time_base =
          av_add_q(av_stream_get_codec_timebase(ost->st), {0, 1});
  }

  // copy estimated duration as a hint to the muxer
  if (ost->st->duration <= 0 && ist->st->duration > 0)
    ost->st->duration =
        av_rescale_q(ist->st->duration, ist->st->time_base, ost->st->time_base);

  if (ist->st->nb_side_data) {
    for (int i = 0; i < ist->st->nb_side_data; i++) {
      const AVPacketSideData *sd_src = &ist->st->side_data[i];
      uint8_t *dst_data;

      dst_data = av_stream_new_side_data(ost->st, sd_src->type, sd_src->size);
      if (!dst_data) {
        return AVERROR(ENOMEM);
      }
      memcpy(dst_data, sd_src->data, sd_src->size);
    }
  }

  if (ost->rotate_overridden) {
    uint8_t *sd = av_stream_new_side_data(ost->st, AV_PKT_DATA_DISPLAYMATRIX,
                                          sizeof(int32_t) * 9);
    if (sd) {
      av_display_rotation_set((int32_t *)sd, -ost->rotate_override_value);
    }
  }

  switch (par_dst->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
      if (audio_volume != 256) {
        AvLog(nullptr, AV_LOG_FATAL,
              "-acodec copy and -vol are incompatible (frames are not "
              "decoded)\n");
        return -1;
      }
      if ((par_dst->block_align == 1 || par_dst->block_align == 1152 ||
           par_dst->block_align == 576) &&
          par_dst->codec_id == AV_CODEC_ID_MP3)
        par_dst->block_align = 0;
      if (par_dst->codec_id == AV_CODEC_ID_AC3) par_dst->block_align = 0;
      break;
    case AVMEDIA_TYPE_VIDEO:
      if (ost->frame_aspect_ratio
              .num) {  // overridden by the -aspect cli option
        sar = av_mul_q(ost->frame_aspect_ratio,
                       {par_dst->height, par_dst->width});
        AvLog(nullptr, AV_LOG_WARNING,
              "Overriding aspect ratio "
              "with stream copy may produce invalid files\n");
      } else if (ist->st->sample_aspect_ratio.num)
        sar = ist->st->sample_aspect_ratio;
      else
        sar = par_src->sample_aspect_ratio;
      ost->st->sample_aspect_ratio = par_dst->sample_aspect_ratio = sar;
      ost->st->avg_frame_rate = ist->st->avg_frame_rate;
      ost->st->r_frame_rate = ist->st->r_frame_rate;
      break;
  }

  ost->mux_timebase = ist->st->time_base;

  return 0;
}

bool FfmpegVideoConverter::set_encoder_id(OutputFile *of, OutputStream *ost) {
  int format_flags = 0;
  int codec_flags = ost->enc_ctx->flags;

  if (av_dict_get(ost->st->metadata, "encoder", nullptr, 0)) {
    return true;
  }
  const AVDictionaryEntry *e = av_dict_get(of->opts, "fflags", nullptr, 0);
  if (e) {
    const AVOption *o = av_opt_find(of->ctx, "fflags", nullptr, 0, 0);
    if (!o) return true;
    av_opt_eval_flags(of->ctx, o, e->value, &format_flags);
  }
  e = av_dict_get(ost->encoder_opts, "flags", nullptr, 0);
  if (e) {
    const AVOption *o = av_opt_find(ost->enc_ctx, "flags", nullptr, 0, 0);
    if (!o) return true;
    av_opt_eval_flags(ost->enc_ctx, o, e->value, &codec_flags);
  }

  int encoder_string_len =
      sizeof(LIBAVCODEC_IDENT) + strlen(ost->enc->name) + 2;
  char *encoder_string = static_cast<char *>(av_mallocz(encoder_string_len));
  if (!encoder_string) {
    return false;
  }

  if (!(format_flags & AVFMT_FLAG_BITEXACT) &&
      !(codec_flags & AV_CODEC_FLAG_BITEXACT))
    av_strlcpy(encoder_string, LIBAVCODEC_IDENT " ", encoder_string_len);
  else
    av_strlcpy(encoder_string, "Lavc ", encoder_string_len);
  av_strlcat(encoder_string, ost->enc->name, encoder_string_len);
  av_dict_set(&ost->st->metadata, "encoder", encoder_string,
              AV_DICT_DONT_STRDUP_VAL | AV_DICT_DONT_OVERWRITE);
  return true;
}

void FfmpegVideoConverter::init_encoder_time_base(
    OutputStream *ost, AVRational default_time_base) {
  InputStream *ist = get_input_stream(ost);
  AVCodecContext *enc_ctx = ost->enc_ctx;
  AVFormatContext *oc;

  if (ost->enc_timebase.num > 0) {
    enc_ctx->time_base = ost->enc_timebase;
    return;
  }

  if (ost->enc_timebase.num < 0) {
    if (ist) {
      enc_ctx->time_base = ist->st->time_base;
      return;
    }

    oc = output_files[ost->file_index]->ctx;
    AvLog(oc, AV_LOG_WARNING,
          "Input stream data not available, using default time base\n");
  }

  enc_ctx->time_base = default_time_base;
}

int FfmpegVideoConverter::init_output_stream_encode(OutputStream *ost,
                                                    AVFrame *frame) {
  InputStream *ist = get_input_stream(ost);
  AVCodecContext *enc_ctx = ost->enc_ctx;
  AVCodecContext *dec_ctx = nullptr;
  OutputFile *of = output_files[ost->file_index];
  AVFormatContext *oc = of->ctx;
  int ret;

  set_encoder_id(output_files[ost->file_index], ost);

  if (ist) {
    dec_ctx = ist->dec_ctx;
  }

  if (enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
    if (!ost->frame_rate.num)
      ost->frame_rate = av_buffersink_get_frame_rate(ost->filter->filter);
    if (ist && !ost->frame_rate.num && !ost->max_frame_rate.num) {
      ost->frame_rate = {25, 1};
      AvLog(nullptr, AV_LOG_WARNING,
            "No information "
            "about the input framerate is available. Falling "
            "back to a default value of 25fps for output stream #%d:%d. Use "
            "the -r option "
            "if you want a different framerate.\n",
            ost->file_index, ost->index);
    }

    if (ost->max_frame_rate.num &&
        (av_q2d(ost->frame_rate) > av_q2d(ost->max_frame_rate) ||
         !ost->frame_rate.den))
      ost->frame_rate = ost->max_frame_rate;

    if (ost->enc->supported_framerates && !ost->force_fps) {
      int idx = av_find_nearest_q_idx(ost->frame_rate,
                                      ost->enc->supported_framerates);
      ost->frame_rate = ost->enc->supported_framerates[idx];
    }
    // reduce frame rate for mpeg4 to be within the spec limits
    if (enc_ctx->codec_id == AV_CODEC_ID_MPEG4) {
      av_reduce(&ost->frame_rate.num, &ost->frame_rate.den, ost->frame_rate.num,
                ost->frame_rate.den, 65535);
    }
  }

  switch (enc_ctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
      enc_ctx->sample_fmt = static_cast<AVSampleFormat>(
          av_buffersink_get_format(ost->filter->filter));
      enc_ctx->sample_rate = av_buffersink_get_sample_rate(ost->filter->filter);
      ret =
          av_buffersink_get_ch_layout(ost->filter->filter, &enc_ctx->ch_layout);
      if (ret < 0) return ret;

      if (ost->bits_per_raw_sample)
        enc_ctx->bits_per_raw_sample = ost->bits_per_raw_sample;
      else if (dec_ctx && ost->filter->graph->is_meta)
        enc_ctx->bits_per_raw_sample =
            FFMIN(dec_ctx->bits_per_raw_sample,
                  av_get_bytes_per_sample(enc_ctx->sample_fmt) << 3);

      init_encoder_time_base(ost, av_make_q(1, enc_ctx->sample_rate));
      break;

    case AVMEDIA_TYPE_VIDEO:
      init_encoder_time_base(ost, av_inv_q(ost->frame_rate));

      if (!(enc_ctx->time_base.num && enc_ctx->time_base.den))
        enc_ctx->time_base = av_buffersink_get_time_base(ost->filter->filter);
      if (av_q2d(enc_ctx->time_base) < 0.001 &&
          ost->vsync_method != VSYNC_PASSTHROUGH &&
          (ost->vsync_method == VSYNC_CFR || ost->vsync_method == VSYNC_VSCFR ||
           (ost->vsync_method == VSYNC_AUTO &&
            !(of->format->flags & AVFMT_VARIABLE_FPS)))) {
        AvLog(
            oc, AV_LOG_WARNING,
            "Frame rate very high for a muxer not efficiently supporting it.\n"
            "Please consider specifying a lower framerate, a different muxer "
            "or "
            "setting vsync/fps_mode to vfr\n");
      }

      enc_ctx->width = av_buffersink_get_w(ost->filter->filter);
      enc_ctx->height = av_buffersink_get_h(ost->filter->filter);
      enc_ctx->sample_aspect_ratio = ost->st->sample_aspect_ratio =
          ost->frame_aspect_ratio.num
              ?  // overridden by the -aspect cli option
              av_mul_q(ost->frame_aspect_ratio,
                       {enc_ctx->height, enc_ctx->width})
              : av_buffersink_get_sample_aspect_ratio(ost->filter->filter);

      enc_ctx->pix_fmt = static_cast<AVPixelFormat>(
          av_buffersink_get_format(ost->filter->filter));

      if (ost->bits_per_raw_sample)
        enc_ctx->bits_per_raw_sample = ost->bits_per_raw_sample;
      else if (dec_ctx && ost->filter->graph->is_meta)
        enc_ctx->bits_per_raw_sample =
            FFMIN(dec_ctx->bits_per_raw_sample,
                  av_pix_fmt_desc_get(enc_ctx->pix_fmt)->comp[0].depth);

      if (frame) {
        enc_ctx->color_range = frame->color_range;
        enc_ctx->color_primaries = frame->color_primaries;
        enc_ctx->color_trc = frame->color_trc;
        enc_ctx->colorspace = frame->colorspace;
        enc_ctx->chroma_sample_location = frame->chroma_location;
      }

      enc_ctx->framerate = ost->frame_rate;

      ost->st->avg_frame_rate = ost->frame_rate;

      // Field order: autodetection
      if (frame) {
        if (enc_ctx->flags &
                (AV_CODEC_FLAG_INTERLACED_DCT | AV_CODEC_FLAG_INTERLACED_ME) &&
            ost->top_field_first >= 0)
          frame->top_field_first = !!ost->top_field_first;

        if (frame->interlaced_frame) {
          if (enc_ctx->codec->id == AV_CODEC_ID_MJPEG)
            enc_ctx->field_order =
                frame->top_field_first ? AV_FIELD_TT : AV_FIELD_BB;
          else
            enc_ctx->field_order =
                frame->top_field_first ? AV_FIELD_TB : AV_FIELD_BT;
        } else
          enc_ctx->field_order = AV_FIELD_PROGRESSIVE;
      }

      // Field order: override
      if (ost->top_field_first == 0) {
        enc_ctx->field_order = AV_FIELD_BB;
      } else if (ost->top_field_first == 1) {
        enc_ctx->field_order = AV_FIELD_TT;
      }

      if (ost->forced_keyframes) {
        if (!strncmp(ost->forced_keyframes, "expr:", 5)) {
          ret = av_expr_parse(&ost->forced_keyframes_pexpr,
                              ost->forced_keyframes + 5,
                              forced_keyframes_const_names, nullptr, nullptr,
                              nullptr, nullptr, 0, nullptr);
          if (ret < 0) {
            AvLog(nullptr, AV_LOG_ERROR,
                  "Invalid force_key_frames expression '%s'\n",
                  ost->forced_keyframes + 5);
            return ret;
          }
          ost->forced_keyframes_expr_const_values[FKF_N] = 0;
          ost->forced_keyframes_expr_const_values[FKF_N_FORCED] = 0;
          ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_N] = NAN;
          ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_T] = NAN;

          // Don't parse the 'forced_keyframes' in case of
          // 'keep-source-keyframes', parse it only for static kf timings
        } else if (strncmp(ost->forced_keyframes, "source", 6)) {
          parse_forced_key_frames(ost->forced_keyframes, ost, ost->enc_ctx);
        }
      }
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      enc_ctx->time_base = {1, AV_TIME_BASE};
      if (!enc_ctx->width) {
        enc_ctx->width = input_streams[ost->source_index]->st->codecpar->width;
        enc_ctx->height =
            input_streams[ost->source_index]->st->codecpar->height;
      }
      break;
    case AVMEDIA_TYPE_DATA:
      break;
    default:
      abort();
      break;
  }

  ost->mux_timebase = enc_ctx->time_base;

  return 0;
}

void FfmpegVideoConverter::report_new_stream(int input_index, AVPacket *pkt) {
  InputFile *file = input_files[input_index];
  AVStream *st = file->ctx->streams[pkt->stream_index];

  if (pkt->stream_index < file->nb_streams_warn) {
    return;
  }
  AvLog(file->ctx, AV_LOG_WARNING,
        "New %s stream %d:%d at pos:%lld and DTS:%ss\n",
        av_get_media_type_string(st->codecpar->codec_type), input_index,
        pkt->stream_index, pkt->pos, AvTs2TimeStr(pkt->dts, &st->time_base));
  file->nb_streams_warn = pkt->stream_index + 1;
}

bool FfmpegVideoConverter::check_recording_time(OutputStream *ost) {
  OutputFile *of = output_files[ost->file_index];

  if (of->recording_time != INT64_MAX &&
      av_compare_ts(ost->sync_opts - ost->first_pts, ost->enc_ctx->time_base,
                    of->recording_time, {1, AV_TIME_BASE}) >= 0) {
    close_output_stream(ost);
    return false;
  }
  return true;
}

double FfmpegVideoConverter::adjust_frame_pts_to_encoder_tb(OutputFile *of,
                                                            OutputStream *ost,
                                                            AVFrame *frame) {
  double float_pts = AV_NOPTS_VALUE;  // this is identical to frame.pts but with
                                      // higher precision
  AVCodecContext *enc = ost->enc_ctx;
  if (!frame || frame->pts == AV_NOPTS_VALUE || !enc || !ost->filter ||
      !ost->filter->graph->graph)
    goto early_exit;

  {
    AVFilterContext *filter = ost->filter->filter;

    int64_t start_time =
        (of->start_time == AV_NOPTS_VALUE) ? 0 : of->start_time;
    AVRational filter_tb = av_buffersink_get_time_base(filter);
    AVRational tb = enc->time_base;
    int extra_bits = av_clip(29 - av_log2(tb.den), 0, 16);

    tb.den <<= extra_bits;
    float_pts = av_rescale_q(frame->pts, filter_tb, tb) -
                av_rescale_q(start_time, {1, AV_TIME_BASE}, tb);
    float_pts /= 1 << extra_bits;
    // avoid exact midoints to reduce the chance of rounding differences, this
    // can be removed in case the fps code is changed to work with integers
    float_pts += FFSIGN(float_pts) * 1.0 / (1 << 17);

    frame->pts = av_rescale_q(frame->pts, filter_tb, enc->time_base) -
                 av_rescale_q(start_time, {1, AV_TIME_BASE}, enc->time_base);
  }

early_exit:

  if (debug_ts) {
    AvLog(nullptr, AV_LOG_INFO,
          "filter -> pts:%s pts_time:%s exact:%f time_base:%d/%d\n",
          frame ? AvTs2Str(frame->pts) : "NULL",
          frame ? AvTs2TimeStr(frame->pts, &enc->time_base) : "NULL", float_pts,
          enc ? enc->time_base.num : -1, enc ? enc->time_base.den : -1);
  }

  return float_pts;
}

int FfmpegVideoConverter::init_output_stream(OutputStream *ost, AVFrame *frame,
                                             char *error, int error_len) {
  int ret = 0;

  if (ost->encoding_needed) {
    const AVCodec *codec = ost->enc;
    AVCodecContext *dec = nullptr;
    InputStream *ist;

    ret = init_output_stream_encode(ost, frame);
    if (ret < 0) return ret;

    if ((ist = get_input_stream(ost))) dec = ist->dec_ctx;
    if (dec && dec->subtitle_header) {
      /* ASS code assumes this buffer is null terminated so add extra byte. */
      ost->enc_ctx->subtitle_header =
          static_cast<uint8_t *>(av_mallocz(dec->subtitle_header_size + 1));
      if (!ost->enc_ctx->subtitle_header) return AVERROR(ENOMEM);
      memcpy(ost->enc_ctx->subtitle_header, dec->subtitle_header,
             dec->subtitle_header_size);
      ost->enc_ctx->subtitle_header_size = dec->subtitle_header_size;
    }
    if (!av_dict_get(ost->encoder_opts, "threads", nullptr, 0))
      av_dict_set(&ost->encoder_opts, "threads", "auto", 0);

    ret = hw_device_setup_for_encode(ost);
    if (ret < 0) {
      snprintf(error, error_len,
               "Device setup failed for "
               "encoder on output stream #%d:%d : %s",
               ost->file_index, ost->index, AvErr2Str(ret));
      return ret;
    }

    if (ist && ist->dec->type == AVMEDIA_TYPE_SUBTITLE &&
        ost->enc->type == AVMEDIA_TYPE_SUBTITLE) {
      int input_props = 0, output_props = 0;
      AVCodecDescriptor const *input_descriptor =
          avcodec_descriptor_get(dec->codec_id);
      AVCodecDescriptor const *output_descriptor =
          avcodec_descriptor_get(ost->enc_ctx->codec_id);
      if (input_descriptor)
        input_props = input_descriptor->props &
                      (AV_CODEC_PROP_TEXT_SUB | AV_CODEC_PROP_BITMAP_SUB);
      if (output_descriptor)
        output_props = output_descriptor->props &
                       (AV_CODEC_PROP_TEXT_SUB | AV_CODEC_PROP_BITMAP_SUB);
      if (input_props && output_props && input_props != output_props) {
        snprintf(error, error_len,
                 "Subtitle encoding currently only possible from text to text "
                 "or bitmap to bitmap");
        return AVERROR_INVALIDDATA;
      }
    }

    if ((ret = avcodec_open2(ost->enc_ctx, codec, &ost->encoder_opts)) < 0) {
      if (ret == AVERROR_EXPERIMENTAL) {
        return ret;
      }
      snprintf(
          error, error_len,
          "Error while opening encoder for output stream #%d:%d - "
          "maybe incorrect parameters such as bit_rate, rate, width or height",
          ost->file_index, ost->index);
      return ret;
    }
    if (ost->enc->type == AVMEDIA_TYPE_AUDIO &&
        !(ost->enc->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
      av_buffersink_set_frame_size(ost->filter->filter,
                                   ost->enc_ctx->frame_size);
    // assert_avoptions(ost->encoder_opts);
    if (ost->enc_ctx->bit_rate && ost->enc_ctx->bit_rate < 1000 &&
        ost->enc_ctx->codec_id !=
            AV_CODEC_ID_CODEC2 /* don't complain about 700 bit/s modes */)
      AvLog(nullptr, AV_LOG_WARNING,
            "The bitrate parameter is set too low."
            " It takes bits/s as argument, not kbits/s\n");

    ret = avcodec_parameters_from_context(ost->st->codecpar, ost->enc_ctx);
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_FATAL,
            "Error initializing the output stream codec context.\n");
      return ret;
    }

    if (ost->enc_ctx->nb_coded_side_data) {
      int i;

      for (i = 0; i < ost->enc_ctx->nb_coded_side_data; i++) {
        const AVPacketSideData *sd_src = &ost->enc_ctx->coded_side_data[i];
        uint8_t *dst_data;

        dst_data = av_stream_new_side_data(ost->st, sd_src->type, sd_src->size);
        if (!dst_data) return AVERROR(ENOMEM);
        memcpy(dst_data, sd_src->data, sd_src->size);
      }
    }

    /*
     * Add global input side data. For now this is naive, and copies it
     * from the input stream's global side data. All side data should
     * really be funneled over AVFrame and libavfilter, then added back to
     * packet side data, and then potentially using the first packet for
     * global side data.
     */
    if (ist) {
      int i;
      for (i = 0; i < ist->st->nb_side_data; i++) {
        AVPacketSideData *sd = &ist->st->side_data[i];
        if (sd->type != AV_PKT_DATA_CPB_PROPERTIES) {
          uint8_t *dst = av_stream_new_side_data(ost->st, sd->type, sd->size);
          if (!dst) return AVERROR(ENOMEM);
          memcpy(dst, sd->data, sd->size);
          if (ist->autorotate && sd->type == AV_PKT_DATA_DISPLAYMATRIX)
            av_display_rotation_set((int32_t *)dst, 0);
        }
      }
    }

    // copy timebase while removing common factors
    if (ost->st->time_base.num <= 0 || ost->st->time_base.den <= 0)
      ost->st->time_base = av_add_q(ost->enc_ctx->time_base, {0, 1});

    // copy estimated duration as a hint to the muxer
    if (ost->st->duration <= 0 && ist && ist->st->duration > 0)
      ost->st->duration = av_rescale_q(ist->st->duration, ist->st->time_base,
                                       ost->st->time_base);
  } else if (ost->stream_copy) {
    ret = init_output_stream_streamcopy(ost);
    if (ret < 0) return ret;
  }

  /* initialize bitstream filters for the output stream
   * needs to be done here, because the codec id for streamcopy is not
   * known until now */
  ret = init_output_bsfs(ost);
  if (ret < 0) {
    return ret;
  }

  ost->initialized = true;

  if (!of_check_init(output_files[ost->file_index])) {
    return -1;
  }

  return ret;
}

// set duration to max(tmp, duration) in a proper time base and return
// duration's time_base
AVRational FfmpegVideoConverter::duration_max(int64_t tmp, int64_t *duration,
                                              AVRational tmp_time_base,
                                              AVRational time_base) {
  int ret;

  if (!*duration) {
    *duration = tmp;
    return tmp_time_base;
  }

  ret = av_compare_ts(*duration, time_base, tmp, tmp_time_base);
  if (ret < 0) {
    *duration = tmp;
    return tmp_time_base;
  }

  return time_base;
}

int FfmpegVideoConverter::seek_to_start(InputFile *ifile, AVFormatContext *is) {
  InputStream *ist;
  AVCodecContext *avctx;
  int i, ret, has_audio = 0;
  int64_t duration = 0;

  ret =
      avformat_seek_file(is, -1, INT64_MIN, is->start_time, is->start_time, 0);
  if (ret < 0) return ret;

  for (i = 0; i < ifile->nb_streams; i++) {
    ist = input_streams[ifile->ist_index + i];
    avctx = ist->dec_ctx;

    /* duration is the length of the last frame in a stream
     * when audio stream is present we don't care about
     * last video frame length because it's not defined exactly */
    if (avctx->codec_type == AVMEDIA_TYPE_AUDIO && ist->nb_samples)
      has_audio = 1;
  }

  for (i = 0; i < ifile->nb_streams; i++) {
    ist = input_streams[ifile->ist_index + i];
    avctx = ist->dec_ctx;

    if (has_audio) {
      if (avctx->codec_type == AVMEDIA_TYPE_AUDIO && ist->nb_samples) {
        AVRational sample_rate = {1, avctx->sample_rate};

        duration =
            av_rescale_q(ist->nb_samples, sample_rate, ist->st->time_base);
      } else {
        continue;
      }
    } else {
      if (ist->framerate.num) {
        duration =
            av_rescale_q(1, av_inv_q(ist->framerate), ist->st->time_base);
      } else if (ist->st->avg_frame_rate.num) {
        duration = av_rescale_q(1, av_inv_q(ist->st->avg_frame_rate),
                                ist->st->time_base);
      } else {
        duration = 1;
      }
    }
    if (!ifile->duration) ifile->time_base = ist->st->time_base;
    /* the total duration of the stream, max_pts - min_pts is
     * the duration of the stream without the last frame */
    if (ist->max_pts > ist->min_pts &&
        ist->max_pts - (uint64_t)ist->min_pts < INT64_MAX - duration)
      duration += ist->max_pts - ist->min_pts;
    ifile->time_base = duration_max(duration, &ifile->duration,
                                    ist->st->time_base, ifile->time_base);
  }

  if (ifile->loop > 0) ifile->loop--;

  return ret;
}

/*
 * Return
 * - 0 -- one packet was read and processed
 * - AVERROR(EAGAIN) -- no packets were available for selected file,
 *   this function should be called again
 * - AVERROR_EOF -- this function should not be called again
 */
int FfmpegVideoConverter::process_input(int file_index) {
  InputFile *ifile = input_files[file_index];
  AVFormatContext *is;
  InputStream *ist;
  AVPacket *pkt;
  int64_t duration;
  int64_t pkt_dts;
  int disable_discontinuity_correction = copy_ts;

  is = ifile->ctx;
  int ret = get_input_packet(ifile, &pkt);

  if (ret == AVERROR(EAGAIN)) {
    ifile->eagain = true;
    return ret;
  }
  if (ret < 0 && ifile->loop) {
    AVCodecContext *avctx;
    for (int i = 0; i < ifile->nb_streams; i++) {
      ist = input_streams[ifile->ist_index + i];
      avctx = ist->dec_ctx;
      if (ist->processing_needed) {
        ret = process_input_packet(ist, nullptr, 1);
        if (ret > 0) {
          return 0;
        }
        if (ist->decoding_needed) {
          avcodec_flush_buffers(avctx);
        }
      }
    }

    ret = seek_to_start(ifile, is);
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_WARNING, "Seek to start failed.\n");
    } else {
      ret = get_input_packet(ifile, &pkt);
    }
    if (ret == AVERROR(EAGAIN)) {
      ifile->eagain = true;
      return ret;
    }
  }
  if (ret < 0) {
    if (ret != AVERROR_EOF) {
      PrintError(is->url, ret);
      if (exit_on_error) {
        return ret;
      }
    }

    for (int i = 0; i < ifile->nb_streams; i++) {
      ist = input_streams[ifile->ist_index + i];
      if (ist->processing_needed) {
        ret = process_input_packet(ist, nullptr, 0);
        if (ret > 0) {
          return 0;
        }
      }

      /* mark all outputs that don't go through lavfi as finished */
      for (int j = 0; j < nb_output_streams; j++) {
        OutputStream *ost = output_streams[j];

        if (ost->source_index == ifile->ist_index + i &&
            (ost->stream_copy || ost->enc->type == AVMEDIA_TYPE_SUBTITLE)) {
          finish_output_stream(ost);
        }
      }
    }

    ifile->eof_reached = true;
    return AVERROR(EAGAIN);
  }

  reset_eagain();

  if (do_pkt_dump) {
    av_pkt_dump_log2(nullptr, AV_LOG_INFO, pkt, do_hex_dump,
                     is->streams[pkt->stream_index]);
  }
  /* the following test is needed in case new streams appear
     dynamically in stream : we ignore them */
  if (pkt->stream_index >= ifile->nb_streams) {
    report_new_stream(file_index, pkt);
    goto discard_packet;
  }

  ist = input_streams[ifile->ist_index + pkt->stream_index];

  ist->data_size += pkt->size;
  ist->nb_packets++;

  if (ist->discard) {
    goto discard_packet;
  }

  if (pkt->flags & AV_PKT_FLAG_CORRUPT) {
    AvLog(nullptr, exit_on_error ? AV_LOG_FATAL : AV_LOG_WARNING,
          "%s: corrupt input packet in stream %d\n", is->url,
          pkt->stream_index);
    if (exit_on_error) {
      return -1;
    }
  }

  if (debug_ts) {
    AVRational av_time_base_q{1, AV_TIME_BASE};
    AvLog(
        nullptr, AV_LOG_INFO,
        "demuxer -> ist_index:%d type:%s "
        "next_dts:%s next_dts_time:%s next_pts:%s next_pts_time:%s pkt_pts:%s "
        "pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s duration:%s "
        "duration_time:%s off:%s off_time:%s\n",
        ifile->ist_index + pkt->stream_index,
        av_get_media_type_string(ist->dec_ctx->codec_type),
        AvTs2Str(ist->next_dts), AvTs2TimeStr(ist->next_dts, &av_time_base_q),
        AvTs2Str(ist->next_pts), AvTs2TimeStr(ist->next_pts, &av_time_base_q),
        AvTs2Str(pkt->pts), AvTs2TimeStr(pkt->pts, &ist->st->time_base),
        AvTs2Str(pkt->dts), AvTs2TimeStr(pkt->dts, &ist->st->time_base),
        AvTs2Str(pkt->duration),
        AvTs2TimeStr(pkt->duration, &ist->st->time_base),
        AvTs2Str(input_files[ist->file_index]->ts_offset),
        AvTs2TimeStr(input_files[ist->file_index]->ts_offset, &av_time_base_q));
  }

  if (!ist->wrap_correction_done && is->start_time != AV_NOPTS_VALUE &&
      ist->st->pts_wrap_bits < 64) {
    // Correcting starttime based on the enabled streams
    // FIXME this ideally should be done before the first use of starttime but
    // we do not know which are the enabled streams at that point.
    //       so we instead do it here as part of discontinuity handling
    if (ist->next_dts == AV_NOPTS_VALUE &&
        ifile->ts_offset == -is->start_time &&
        (is->iformat->flags & AVFMT_TS_DISCONT)) {
      int64_t new_start_time = INT64_MAX;
      for (int i = 0; i < is->nb_streams; i++) {
        AVStream *st = is->streams[i];
        if (st->discard == AVDISCARD_ALL || st->start_time == AV_NOPTS_VALUE)
          continue;
        new_start_time = FFMIN(
            new_start_time,
            av_rescale_q(st->start_time, st->time_base, {1, AV_TIME_BASE}));
      }
      if (new_start_time > is->start_time) {
        AvLog(is, AV_LOG_VERBOSE,
              "Correcting start time by %"
              "lld"
              "\n",
              new_start_time - is->start_time);
        ifile->ts_offset = -new_start_time;
      }
    }

    int64_t stime =
        av_rescale_q(is->start_time, {1, AV_TIME_BASE}, ist->st->time_base);
    int64_t stime2 = stime + (1ULL << ist->st->pts_wrap_bits);
    ist->wrap_correction_done = true;

    if (stime2 > stime && pkt->dts != AV_NOPTS_VALUE &&
        pkt->dts > stime + (1LL << (ist->st->pts_wrap_bits - 1))) {
      pkt->dts -= 1ULL << ist->st->pts_wrap_bits;
      ist->wrap_correction_done = false;
    }
    if (stime2 > stime && pkt->pts != AV_NOPTS_VALUE &&
        pkt->pts > stime + (1LL << (ist->st->pts_wrap_bits - 1))) {
      pkt->pts -= 1ULL << ist->st->pts_wrap_bits;
      ist->wrap_correction_done = false;
    }
  }

  /* add the stream-global side data to the first packet */
  if (ist->nb_packets == 1) {
    for (int i = 0; i < ist->st->nb_side_data; i++) {
      AVPacketSideData *src_sd = &ist->st->side_data[i];

      if (src_sd->type == AV_PKT_DATA_DISPLAYMATRIX) {
        continue;
      }

      if (av_packet_get_side_data(pkt, src_sd->type, nullptr)) {
        continue;
      }

      uint8_t *dst_data =
          av_packet_new_side_data(pkt, src_sd->type, src_sd->size);
      if (!dst_data) {
        return -1;
      }

      memcpy(dst_data, src_sd->data, src_sd->size);
    }
  }

  if (pkt->dts != AV_NOPTS_VALUE) {
    pkt->dts +=
        av_rescale_q(ifile->ts_offset, {1, AV_TIME_BASE}, ist->st->time_base);
  }
  if (pkt->pts != AV_NOPTS_VALUE) {
    pkt->pts +=
        av_rescale_q(ifile->ts_offset, {1, AV_TIME_BASE}, ist->st->time_base);
  }

  if (pkt->pts != AV_NOPTS_VALUE) {
    pkt->pts *= ist->ts_scale;
  }
  if (pkt->dts != AV_NOPTS_VALUE) {
    pkt->dts *= ist->ts_scale;
  }

  pkt_dts = av_rescale_q_rnd(
      pkt->dts, ist->st->time_base, {1, AV_TIME_BASE},
      static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
  if ((ist->dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
       ist->dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) &&
      pkt_dts != AV_NOPTS_VALUE && ist->next_dts == AV_NOPTS_VALUE &&
      !copy_ts && (is->iformat->flags & AVFMT_TS_DISCONT) &&
      ifile->last_ts != AV_NOPTS_VALUE) {
    int64_t delta = pkt_dts - ifile->last_ts;
    if (delta < -1LL * dts_delta_threshold * AV_TIME_BASE ||
        delta > 1LL * dts_delta_threshold * AV_TIME_BASE) {
      ifile->ts_offset -= delta;
      AvLog(nullptr, AV_LOG_DEBUG,
            "Inter stream timestamp discontinuity %lld"
            ", new offset= %lld\n",
            delta, ifile->ts_offset);
      pkt->dts -= av_rescale_q(delta, {1, AV_TIME_BASE}, ist->st->time_base);
      if (pkt->pts != AV_NOPTS_VALUE) {
        pkt->pts -= av_rescale_q(delta, {1, AV_TIME_BASE}, ist->st->time_base);
      }
    }
  }

  duration =
      av_rescale_q(ifile->duration, ifile->time_base, ist->st->time_base);
  if (pkt->pts != AV_NOPTS_VALUE) {
    pkt->pts += duration;
    ist->max_pts = FFMAX(pkt->pts, ist->max_pts);
    ist->min_pts = FFMIN(pkt->pts, ist->min_pts);
  }

  if (pkt->dts != AV_NOPTS_VALUE) {
    pkt->dts += duration;
  }

  pkt_dts = av_rescale_q_rnd(
      pkt->dts, ist->st->time_base, {1, AV_TIME_BASE},
      static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

  if (copy_ts && pkt_dts != AV_NOPTS_VALUE && ist->next_dts != AV_NOPTS_VALUE &&
      (is->iformat->flags & AVFMT_TS_DISCONT) && ist->st->pts_wrap_bits < 60) {
    int64_t wrap_dts = av_rescale_q_rnd(
        pkt->dts + (1LL << ist->st->pts_wrap_bits), ist->st->time_base,
        {1, AV_TIME_BASE},
        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    if (FFABS(wrap_dts - ist->next_dts) < FFABS(pkt_dts - ist->next_dts) / 10) {
      disable_discontinuity_correction = 0;
    }
  }

  if ((ist->dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
       ist->dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) &&
      pkt_dts != AV_NOPTS_VALUE && ist->next_dts != AV_NOPTS_VALUE &&
      !disable_discontinuity_correction) {
    int64_t delta = pkt_dts - ist->next_dts;
    if (is->iformat->flags & AVFMT_TS_DISCONT) {
      if (delta < -1LL * dts_delta_threshold * AV_TIME_BASE ||
          delta > 1LL * dts_delta_threshold * AV_TIME_BASE ||
          pkt_dts + AV_TIME_BASE / 10 < FFMAX(ist->pts, ist->dts)) {
        ifile->ts_offset -= delta;
        AvLog(nullptr, AV_LOG_DEBUG,
              "timestamp discontinuity for stream #%d:%d "
              "(id=%d, type=%s): %lld, new offset= %lld\n",
              ist->file_index, ist->st->index, ist->st->id,
              av_get_media_type_string(ist->dec_ctx->codec_type), delta,
              ifile->ts_offset);
        pkt->dts -= av_rescale_q(delta, {1, AV_TIME_BASE}, ist->st->time_base);
        if (pkt->pts != AV_NOPTS_VALUE) {
          pkt->pts -=
              av_rescale_q(delta, {1, AV_TIME_BASE}, ist->st->time_base);
        }
      }
    } else {
      if (delta < -1LL * dts_error_threshold * AV_TIME_BASE ||
          delta > 1LL * dts_error_threshold * AV_TIME_BASE) {
        AvLog(nullptr, AV_LOG_WARNING,
              "DTS %lld, next:%lld st:%d invalid dropping\n", pkt->dts,
              ist->next_dts, pkt->stream_index);
        pkt->dts = AV_NOPTS_VALUE;
      }
      if (pkt->pts != AV_NOPTS_VALUE) {
        int64_t pkt_pts =
            av_rescale_q(pkt->pts, ist->st->time_base, {1, AV_TIME_BASE});
        delta = pkt_pts - ist->next_dts;
        if (delta < -1LL * dts_error_threshold * AV_TIME_BASE ||
            delta > 1LL * dts_error_threshold * AV_TIME_BASE) {
          AvLog(nullptr, AV_LOG_WARNING,
                "PTS %lld, next:%lld invalid dropping st:%d\n", pkt->pts,
                ist->next_dts, pkt->stream_index);
          pkt->pts = AV_NOPTS_VALUE;
        }
      }
    }
  }

  if (pkt->dts != AV_NOPTS_VALUE) {
    ifile->last_ts =
        av_rescale_q(pkt->dts, ist->st->time_base, {1, AV_TIME_BASE});
  }

  if (debug_ts) {
    AVRational av_time_base_q{1, AV_TIME_BASE};
    AvLog(
        NULL, AV_LOG_INFO,
        "demuxer+ffmpeg -> ist_index:%d type:%s pkt_pts:%s pkt_pts_time:%s "
        "pkt_dts:%s pkt_dts_time:%s duration:%s duration_time:%s off:%s "
        "off_time:%s\n",
        ifile->ist_index + pkt->stream_index,
        av_get_media_type_string(ist->dec_ctx->codec_type), AvTs2Str(pkt->pts),
        AvTs2TimeStr(pkt->pts, &ist->st->time_base), AvTs2Str(pkt->dts),
        AvTs2TimeStr(pkt->dts, &ist->st->time_base), AvTs2Str(pkt->duration),
        AvTs2TimeStr(pkt->duration, &ist->st->time_base),
        AvTs2Str(input_files[ist->file_index]->ts_offset),
        AvTs2TimeStr(input_files[ist->file_index]->ts_offset, &av_time_base_q));
  }

  sub2video_heartbeat(ist, pkt->pts);

  process_input_packet(ist, pkt, 0);

discard_packet:
  av_packet_unref(pkt);

  return 0;
}

int FfmpegVideoConverter::ifilter_parameters_from_codecpar(
    InputFilter *ifilter, AVCodecParameters *par) {
  // We never got any input. Set a fake format, which will
  // come from libavformat.
  ifilter->format = par->format;
  ifilter->sample_rate = par->sample_rate;
  ifilter->width = par->width;
  ifilter->height = par->height;
  ifilter->sample_aspect_ratio = par->sample_aspect_ratio;
  int ret = av_channel_layout_copy(&ifilter->ch_layout, &par->ch_layout);
  if (ret < 0) {
    return ret;
  }

  return 0;
}
int FfmpegVideoConverter::ifilter_send_frame(InputFilter *ifilter,
                                             AVFrame *frame,
                                             bool keep_reference) {
  int buffersrc_flags = AV_BUFFERSRC_FLAG_PUSH;
  if (keep_reference) {
    buffersrc_flags |= AV_BUFFERSRC_FLAG_KEEP_REF;
  }

  /* determine if the parameters for this input changed */
  bool need_reinit = ifilter->format != frame->format;

  switch (ifilter->ist->st->codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
      need_reinit |= (ifilter->sample_rate != frame->sample_rate) ||
                     (av_channel_layout_compare(&ifilter->ch_layout,
                                                &frame->ch_layout) != 0);
      break;
    case AVMEDIA_TYPE_VIDEO:
      need_reinit |=
          ifilter->width != frame->width || ifilter->height != frame->height;
      break;
  }

  FilterGraph *fg = ifilter->graph;
  if (!ifilter->ist->reinit_filters && fg->graph) {
    need_reinit = false;
  }

  if (!!ifilter->hw_frames_ctx != !!frame->hw_frames_ctx ||
      (ifilter->hw_frames_ctx &&
       ifilter->hw_frames_ctx->data != frame->hw_frames_ctx->data)) {
    need_reinit = true;
  }

  if (AVFrameSideData *sd =
          av_frame_get_side_data(frame, AV_FRAME_DATA_DISPLAYMATRIX)) {
    if (!ifilter->displaymatrix ||
        memcmp(sd->data, ifilter->displaymatrix, sizeof(int32_t) * 9)) {
      need_reinit = true;
    }
  } else if (ifilter->displaymatrix) {
    need_reinit = true;
  }

  if (need_reinit) {
    int ret = ifilter_parameters_from_frame(ifilter, frame);
    if (ret < 0) {
      return ret;
    }
  }

  /* (re)init the graph if possible, otherwise buffer the frame and return */
  if (need_reinit || !fg->graph) {
    if (!ifilter_has_all_input_formats(fg)) {
      AVFrame *tmp = av_frame_clone(frame);
      if (!tmp) {
        return AVERROR(ENOMEM);
      }

      int ret = av_fifo_write(ifilter->frame_queue, &tmp, 1);
      if (ret < 0) {
        av_frame_free(&tmp);
      }

      return ret;
    }

    int ret = reap_filters(1);
    if (ret < 0 && ret != AVERROR_EOF) {
      AvLog(nullptr, AV_LOG_ERROR, "Error while filtering: %s\n",
            AvErr2Str(ret));
      return ret;
    }

    ret = configure_filtergraph(fg);
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_ERROR, "Error reinitializing filters!\n");
      return ret;
    }
  }

  int ret =
      av_buffersrc_add_frame_flags(ifilter->filter, frame, buffersrc_flags);
  if (ret < 0) {
    if (ret != AVERROR_EOF) {
      AvLog(nullptr, AV_LOG_ERROR, "Error while filtering: %s\n",
            AvErr2Str(ret));
    }
    return ret;
  }

  return 0;
}
int FfmpegVideoConverter::ifilter_send_eof(InputFilter *ifilter, int64_t pts) {
  ifilter->eof = true;

  int ret;
  if (ifilter->filter) {
    ret = av_buffersrc_close(ifilter->filter, pts, AV_BUFFERSRC_FLAG_PUSH);
    if (ret < 0) return ret;
  } else {
    // the filtergraph was never configured
    if (ifilter->format < 0) {
      ret =
          ifilter_parameters_from_codecpar(ifilter, ifilter->ist->st->codecpar);
      if (ret < 0) {
        return ret;
      }
    }
    if (ifilter->format < 0 && (ifilter->type == AVMEDIA_TYPE_AUDIO ||
                                ifilter->type == AVMEDIA_TYPE_VIDEO)) {
      AvLog(nullptr, AV_LOG_ERROR,
            "Cannot determine format of input stream %d:%d after EOF\n",
            ifilter->ist->file_index, ifilter->ist->st->index);
      return AVERROR_INVALIDDATA;
    }
  }

  return 0;
}

// This does not quite work like avcodec_decode_audio4/avcodec_decode_video2.
// There is the following difference: if you got a frame, you must call
// it again with pkt=nullptr. pkt==nullptr is treated differently from
// pkt->size==0 (pkt==nullptr means get more output, pkt->size==0 is a
// flush/drain packet)
int FfmpegVideoConverter::decode(AVCodecContext *avctx, AVFrame *frame,
                                 bool *got_frame, AVPacket *pkt) {
  *got_frame = false;
  if (pkt) {
    int ret = avcodec_send_packet(avctx, pkt);
    // In particular, we don't expect AVERROR(EAGAIN), because we read all
    // decoded frames with avcodec_receive_frame() until done.
    if (ret < 0 && ret != AVERROR_EOF) {
      return ret;
    }
  }

  int ret = avcodec_receive_frame(avctx, frame);
  if (ret < 0 && ret != AVERROR(EAGAIN)) {
    return ret;
  }
  if (ret >= 0) {
    *got_frame = true;
  }
  return 0;
}

int FfmpegVideoConverter::send_frame_to_filters(InputStream *ist,
                                                AVFrame *decoded_frame) {
  int ret = -1;
  av_assert1(ist->nb_filters > 0); /* ensure ret is initialized */
  for (int i = 0; i < ist->nb_filters; i++) {
    ret = ifilter_send_frame(ist->filters[i], decoded_frame,
                             i < ist->nb_filters - 1);
    if (ret == AVERROR_EOF) {
      ret = 0; /* ignore */
    }
    if (ret < 0) {
      break;
    }
  }
  return ret;
}

int FfmpegVideoConverter::decode_audio(InputStream *ist, AVPacket *pkt,
                                       bool *got_output, bool *decode_failed) {
  AVFrame *decoded_frame = ist->decoded_frame;
  AVCodecContext *avctx = ist->dec_ctx;

  update_benchmark(nullptr);
  int ret = decode(avctx, decoded_frame, got_output, pkt);
  update_benchmark("decode_audio %d.%d", ist->file_index, ist->st->index);
  if (ret < 0) {
    *decode_failed = true;
  }

  if (ret >= 0 && avctx->sample_rate <= 0) {
    AvLog(avctx, AV_LOG_ERROR, "Sample rate %d invalid\n", avctx->sample_rate);
    ret = AVERROR_INVALIDDATA;
  }

  if (ret != AVERROR_EOF) {
    check_decode_result(ist, got_output, ret);
  }

  if (!*got_output || ret < 0) {
    return ret;
  }

  ist->samples_decoded += decoded_frame->nb_samples;
  ist->frames_decoded++;

  /* increment next_dts to use for the case where the input stream does not
     have timestamps or there are multiple frames in the packet */
  ist->next_pts +=
      ((int64_t)AV_TIME_BASE * decoded_frame->nb_samples) / avctx->sample_rate;
  ist->next_dts +=
      ((int64_t)AV_TIME_BASE * decoded_frame->nb_samples) / avctx->sample_rate;

  AVRational decoded_frame_tb{1, AV_TIME_BASE};
  if (decoded_frame->pts != AV_NOPTS_VALUE) {
    decoded_frame_tb = ist->st->time_base;
  } else if (pkt && pkt->pts != AV_NOPTS_VALUE) {
    decoded_frame->pts = pkt->pts;
    decoded_frame_tb = ist->st->time_base;
  } else {
    decoded_frame->pts = ist->dts;
  }
  if (pkt && pkt->duration && ist->prev_pkt_pts != AV_NOPTS_VALUE &&
      pkt->pts != AV_NOPTS_VALUE &&
      pkt->pts - ist->prev_pkt_pts > pkt->duration) {
    ist->filter_in_rescale_delta_last = AV_NOPTS_VALUE;
  }
  if (pkt) {
    ist->prev_pkt_pts = pkt->pts;
  }
  if (decoded_frame->pts != AV_NOPTS_VALUE) {
    decoded_frame->pts = av_rescale_delta(
        decoded_frame_tb, decoded_frame->pts, {1, avctx->sample_rate},
        decoded_frame->nb_samples, &ist->filter_in_rescale_delta_last,
        {1, avctx->sample_rate});
  }
  ist->nb_samples = decoded_frame->nb_samples;
  int err = send_frame_to_filters(ist, decoded_frame);

  av_frame_unref(decoded_frame);
  return err < 0 ? err : ret;
}

int FfmpegVideoConverter::decode_video(InputStream *ist, AVPacket *pkt,
                                       bool *got_output, int64_t *duration_pts,
                                       bool eof, bool *decode_failed) {
  AVFrame *decoded_frame = ist->decoded_frame;
  int err = 0;
  int64_t best_effort_timestamp;
  int64_t dts = AV_NOPTS_VALUE;

  // With fate-indeo3-2, we're getting 0-sized packets before EOF for some
  // reason. This seems like a semi-critical bug. Don't trigger EOF, and
  // skip the packet.
  if (!eof && pkt && pkt->size == 0) {
    return 0;
  }

  if (ist->dts != AV_NOPTS_VALUE) {
    dts = av_rescale_q(ist->dts, {1, AV_TIME_BASE}, ist->st->time_base);
  }
  if (pkt) {
    pkt->dts = dts;  // ffmpeg.c probably shouldn't do this
  }

  // The old code used to set dts on the drain packet, which does not work
  // with the new API anymore.
  if (eof) {
    void *neww = av_realloc_array(ist->dts_buffer, ist->nb_dts_buffer + 1,
                                  sizeof(ist->dts_buffer[0]));
    if (!neww) {
      return AVERROR(ENOMEM);
    }
    ist->dts_buffer = static_cast<int64_t *>(neww);
    ist->dts_buffer[ist->nb_dts_buffer++] = dts;
  }

  update_benchmark(nullptr);
  int ret = decode(ist->dec_ctx, decoded_frame, got_output, pkt);
  update_benchmark("decode_video %d.%d", ist->file_index, ist->st->index);
  if (ret < 0) {
    *decode_failed = true;
  }

  // The following line may be required in some cases where there is no parser
  // or the parser does not has_b_frames correctly
  if (ist->st->codecpar->video_delay < ist->dec_ctx->has_b_frames) {
    if (ist->dec_ctx->codec_id == AV_CODEC_ID_H264) {
      ist->st->codecpar->video_delay = ist->dec_ctx->has_b_frames;
    } else
      AvLog(ist->dec_ctx, AV_LOG_WARNING,
            "video_delay is larger in decoder than demuxer %d > %d.\n"
            "If you want to help, upload a sample "
            "of this file to https://streams.videolan.org/upload/ "
            "and contact the ffmpeg-devel mailing list. "
            "(ffmpeg-devel@ffmpeg.org)\n",
            ist->dec_ctx->has_b_frames, ist->st->codecpar->video_delay);
  }

  if (ret != AVERROR_EOF) {
    check_decode_result(ist, got_output, ret);
  }

  if (*got_output && ret >= 0) {
    if (ist->dec_ctx->width != decoded_frame->width ||
        ist->dec_ctx->height != decoded_frame->height ||
        ist->dec_ctx->pix_fmt != decoded_frame->format) {
      AvLog(nullptr, AV_LOG_DEBUG,
            "Frame parameters mismatch context %d,%d,%d != %d,%d,%d\n",
            decoded_frame->width, decoded_frame->height, decoded_frame->format,
            ist->dec_ctx->width, ist->dec_ctx->height, ist->dec_ctx->pix_fmt);
    }
  }

  if (!*got_output || ret < 0) {
    return ret;
  }

  if (ist->top_field_first >= 0) {
    decoded_frame->top_field_first = ist->top_field_first;
  }

  ist->frames_decoded++;

  if (ist->hwaccel_retrieve_data &&
      decoded_frame->format == ist->hwaccel_pix_fmt) {
    err = ist->hwaccel_retrieve_data(ist->dec_ctx, decoded_frame);
    if (err < 0) {
      goto fail;
    }
  }
  ist->hwaccel_retrieved_pix_fmt =
      static_cast<AVPixelFormat>(decoded_frame->format);

  best_effort_timestamp = decoded_frame->best_effort_timestamp;
  *duration_pts = decoded_frame->pkt_duration;

  if (ist->framerate.num) {
    best_effort_timestamp = ist->cfr_next_pts++;
  }

  if (eof && best_effort_timestamp == AV_NOPTS_VALUE &&
      ist->nb_dts_buffer > 0) {
    best_effort_timestamp = ist->dts_buffer[0];

    for (int i = 0; i < ist->nb_dts_buffer - 1; i++) {
      ist->dts_buffer[i] = ist->dts_buffer[i + 1];
    }
    ist->nb_dts_buffer--;
  }

  if (best_effort_timestamp != AV_NOPTS_VALUE) {
    int64_t ts = av_rescale_q(decoded_frame->pts = best_effort_timestamp,
                              ist->st->time_base, {1, AV_TIME_BASE});

    if (ts != AV_NOPTS_VALUE) {
      ist->next_pts = ist->pts = ts;
    }
  }

  if (debug_ts) {
    AvLog(nullptr, AV_LOG_INFO,
          "decoder -> ist_index:%d type:video "
          "frame_pts:%s frame_pts_time:%s best_effort_ts:%"
          "lld"
          " best_effort_ts_time:%s keyframe:%d frame_type:%d time_base:%d/%d\n",
          ist->st->index, AvTs2Str(decoded_frame->pts),
          AvTs2TimeStr(decoded_frame->pts, &ist->st->time_base),
          best_effort_timestamp,
          AvTs2TimeStr(best_effort_timestamp, &ist->st->time_base),
          decoded_frame->key_frame, decoded_frame->pict_type,
          ist->st->time_base.num, ist->st->time_base.den);
  }

  if (ist->st->sample_aspect_ratio.num) {
    decoded_frame->sample_aspect_ratio = ist->st->sample_aspect_ratio;
  }

  err = send_frame_to_filters(ist, decoded_frame);

fail:
  av_frame_unref(decoded_frame);
  return err < 0 ? err : ret;
}

int FfmpegVideoConverter::transcode_subtitles(InputStream *ist, AVPacket *pkt,
                                              bool *got_output,
                                              bool *decode_failed) {
  AVSubtitle subtitle;
  int free_sub = 1;
  int got_sub_ptr;
  int ret =
      avcodec_decode_subtitle2(ist->dec_ctx, &subtitle, &got_sub_ptr, pkt);
  *got_output = !(got_sub_ptr == 0);

  check_decode_result(nullptr, got_output, ret);

  if (ret < 0 || !*got_output) {
    *decode_failed = true;
    if (!pkt->size) {
      sub2video_flush(ist);
    }
    return ret;
  }

  if (ist->fix_sub_duration) {
    int end = 1;
    if (ist->prev_sub.got_output) {
      end = av_rescale(subtitle.pts - ist->prev_sub.subtitle.pts, 1000,
                       AV_TIME_BASE);
      if (end < ist->prev_sub.subtitle.end_display_time) {
        AvLog(ist->dec_ctx, AV_LOG_DEBUG,
              "Subtitle duration reduced from %"
              "d"
              " to %d%s\n",
              ist->prev_sub.subtitle.end_display_time, end,
              end <= 0 ? ", dropping it" : "");
        ist->prev_sub.subtitle.end_display_time = end;
      }
    }
    FFSWAP(bool, *got_output, ist->prev_sub.got_output);
    FFSWAP(int, ret, ist->prev_sub.ret);
    FFSWAP(AVSubtitle, subtitle, ist->prev_sub.subtitle);
    if (end <= 0) {
      goto out;
    }
  }

  if (!*got_output) {
    return ret;
  }

  if (ist->sub2video.frame) {
    sub2video_update(ist, INT64_MIN, &subtitle);
  } else if (ist->nb_filters) {
    if (!ist->sub2video.sub_queue)
      ist->sub2video.sub_queue =
          av_fifo_alloc2(8, sizeof(AVSubtitle), AV_FIFO_FLAG_AUTO_GROW);
    if (!ist->sub2video.sub_queue) {
      return -1;
    }

    ret = av_fifo_write(ist->sub2video.sub_queue, &subtitle, 1);
    if (ret < 0) {
      return ret;
    }
    free_sub = 0;
  }

  if (!subtitle.num_rects) goto out;

  ist->frames_decoded++;

  for (int i = 0; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];

    if (!check_output_constraints(ist, ost) || !ost->encoding_needed ||
        ost->enc->type != AVMEDIA_TYPE_SUBTITLE)
      continue;

    do_subtitle_out(output_files[ost->file_index], ost, &subtitle);
  }

out:
  if (free_sub) avsubtitle_free(&subtitle);
  return ret;
}

int FfmpegVideoConverter::send_filter_eof(InputStream *ist) {
  int i, ret;
  /* TODO keep pts also in stream time base to avoid converting back */
  int64_t pts = av_rescale_q_rnd(
      ist->pts, {1, AV_TIME_BASE}, ist->st->time_base,
      static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

  for (i = 0; i < ist->nb_filters; i++) {
    ret = ifilter_send_eof(ist->filters[i], pts);
    if (ret < 0) return ret;
  }
  return 0;
}

/* pkt = nullptr means EOF (needed to flush decoder buffers) */
int FfmpegVideoConverter::process_input_packet(InputStream *ist,
                                               const AVPacket *pkt,
                                               int no_eof) {
  int ret = 0, i;
  int repeating = 0;
  bool eof_reached = false;

  AVPacket *avpkt = ist->pkt;

  if (!ist->saw_first_ts) {
    ist->first_dts = ist->dts = ist->st->avg_frame_rate.num
                                    ? -ist->dec_ctx->has_b_frames *
                                          AV_TIME_BASE /
                                          av_q2d(ist->st->avg_frame_rate)
                                    : 0;
    ist->pts = 0;
    if (pkt && pkt->pts != AV_NOPTS_VALUE && !ist->decoding_needed) {
      ist->first_dts = ist->dts +=
          av_rescale_q(pkt->pts, ist->st->time_base, {1, AV_TIME_BASE});
      ist->pts = ist->dts;  // unused but better to set it to a value thats not
                            // totally wrong
    }
    ist->saw_first_ts = 1;
  }

  if (ist->next_dts == AV_NOPTS_VALUE) ist->next_dts = ist->dts;
  if (ist->next_pts == AV_NOPTS_VALUE) ist->next_pts = ist->pts;

  if (pkt) {
    av_packet_unref(avpkt);
    ret = av_packet_ref(avpkt, pkt);
    if (ret < 0) return ret;
  }

  if (pkt && pkt->dts != AV_NOPTS_VALUE) {
    ist->next_dts = ist->dts =
        av_rescale_q(pkt->dts, ist->st->time_base, {1, AV_TIME_BASE});
    if (ist->dec_ctx->codec_type != AVMEDIA_TYPE_VIDEO || !ist->decoding_needed)
      ist->next_pts = ist->pts = ist->dts;
  }

  // while we have more to decode or while the decoder did output something on
  // EOF
  while (ist->decoding_needed) {
    int64_t duration_dts = 0;
    int64_t duration_pts = 0;
    bool got_output = false;
    bool decode_failed = false;

    ist->pts = ist->next_pts;
    ist->dts = ist->next_dts;

    switch (ist->dec_ctx->codec_type) {
      case AVMEDIA_TYPE_AUDIO:
        ret = decode_audio(ist, repeating ? nullptr : avpkt, &got_output,
                           &decode_failed);
        av_packet_unref(avpkt);
        break;
      case AVMEDIA_TYPE_VIDEO:
        ret = decode_video(ist, repeating ? nullptr : avpkt, &got_output,
                           &duration_pts, !pkt, &decode_failed);
        if (!repeating || !pkt || got_output) {
          if (pkt && pkt->duration) {
            duration_dts = av_rescale_q(pkt->duration, ist->st->time_base,
                                        {1, AV_TIME_BASE});
          } else if (ist->dec_ctx->framerate.num != 0 &&
                     ist->dec_ctx->framerate.den != 0) {
            int ticks = av_stream_get_parser(ist->st)
                            ? av_stream_get_parser(ist->st)->repeat_pict + 1
                            : ist->dec_ctx->ticks_per_frame;
            duration_dts =
                ((int64_t)AV_TIME_BASE * ist->dec_ctx->framerate.den * ticks) /
                ist->dec_ctx->framerate.num / ist->dec_ctx->ticks_per_frame;
          }

          if (ist->dts != AV_NOPTS_VALUE && duration_dts) {
            ist->next_dts += duration_dts;
          } else
            ist->next_dts = AV_NOPTS_VALUE;
        }

        if (got_output) {
          if (duration_pts > 0) {
            ist->next_pts += av_rescale_q(duration_pts, ist->st->time_base,
                                          {1, AV_TIME_BASE});
          } else {
            ist->next_pts += duration_dts;
          }
        }
        av_packet_unref(avpkt);
        break;
      case AVMEDIA_TYPE_SUBTITLE:
        if (repeating) break;
        ret = transcode_subtitles(ist, avpkt, &got_output, &decode_failed);
        if (!pkt && ret >= 0) ret = AVERROR_EOF;
        av_packet_unref(avpkt);
        break;
      default:
        return -1;
    }

    if (ret == AVERROR_EOF) {
      eof_reached = true;
      break;
    }

    if (ret < 0) {
      if (decode_failed) {
        AvLog(nullptr, AV_LOG_ERROR, "Error while decoding stream #%d:%d: %s\n",
              ist->file_index, ist->st->index, AvErr2Str(ret));
      } else {
        AvLog(nullptr, AV_LOG_FATAL,
              "Error while processing the decoded "
              "data for stream #%d:%d\n",
              ist->file_index, ist->st->index);
      }
      if (!decode_failed || exit_on_error) {
        return -1;
      }
      break;
    }

    if (got_output) ist->got_output = true;

    if (!got_output) break;

    // During draining, we might get multiple output frames in this loop.
    // ffmpeg.c does not drain the filter chain on configuration changes,
    // which means if we send multiple frames at once to the filters, and
    // one of those frames changes configuration, the buffered frames will
    // be lost. This can upset certain FATE tests.
    // Decode only 1 frame per call on EOF to appease these FATE tests.
    // The ideal solution would be to rewrite decoding to use the new
    // decoding API in a better way.
    if (!pkt) break;

    repeating = 1;
  }

  /* after flushing, send an EOF on all the filter inputs attached to the stream
   */
  /* except when looping we need to flush but not to send an EOF */
  if (!pkt && ist->decoding_needed && eof_reached && !no_eof) {
    int ret = send_filter_eof(ist);
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Error marking filters as finished\n");
      return -1;
    }
  }

  /* handle stream copy */
  if (!ist->decoding_needed && pkt) {
    ist->dts = ist->next_dts;
    switch (ist->dec_ctx->codec_type) {
      case AVMEDIA_TYPE_AUDIO:
        av_assert1(pkt->duration >= 0);
        if (ist->dec_ctx->sample_rate) {
          ist->next_dts += ((int64_t)AV_TIME_BASE * ist->dec_ctx->frame_size) /
                           ist->dec_ctx->sample_rate;
        } else {
          ist->next_dts += av_rescale_q(pkt->duration, ist->st->time_base,
                                        {1, AV_TIME_BASE});
        }
        break;
      case AVMEDIA_TYPE_VIDEO:
        if (ist->framerate.num) {
          // TODO: Remove work-around for c99-to-c89 issue 7
          AVRational time_base_q = {1, AV_TIME_BASE};
          int64_t next_dts = av_rescale_q(ist->next_dts, time_base_q,
                                          av_inv_q(ist->framerate));
          ist->next_dts =
              av_rescale_q(next_dts + 1, av_inv_q(ist->framerate), time_base_q);
        } else if (pkt->duration) {
          ist->next_dts += av_rescale_q(pkt->duration, ist->st->time_base,
                                        {1, AV_TIME_BASE});
        } else if (ist->dec_ctx->framerate.num != 0) {
          int ticks = av_stream_get_parser(ist->st)
                          ? av_stream_get_parser(ist->st)->repeat_pict + 1
                          : ist->dec_ctx->ticks_per_frame;
          ist->next_dts +=
              ((int64_t)AV_TIME_BASE * ist->dec_ctx->framerate.den * ticks) /
              ist->dec_ctx->framerate.num / ist->dec_ctx->ticks_per_frame;
        }
        break;
    }
    ist->pts = ist->dts;
    ist->next_pts = ist->next_dts;
  } else if (!ist->decoding_needed) {
    eof_reached = true;
  }

  for (i = 0; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];

    if (!check_output_constraints(ist, ost) || ost->encoding_needed) continue;

    do_streamcopy(ist, ost, pkt);
  }

  return !eof_reached;
}

bool FfmpegVideoConverter::do_audio_out(OutputFile *of, OutputStream *ost,
                                        AVFrame *frame) {
  adjust_frame_pts_to_encoder_tb(of, ost, frame);

  if (!check_recording_time(ost)) return true;

  if (frame->pts == AV_NOPTS_VALUE || audio_sync_method < 0) {
    frame->pts = ost->sync_opts;
  }
  ost->sync_opts = frame->pts + frame->nb_samples;
  ost->samples_encoded += frame->nb_samples;

  int ret = encode_frame(of, ost, frame);
  if (ret < 0) {
    return false;
  }
  return true;
}

bool FfmpegVideoConverter::do_subtitle_out(OutputFile *of, OutputStream *ost,
                                           AVSubtitle *sub) {
  int subtitle_out_max_size = 1024 * 1024;
  int subtitle_out_size;
  AVPacket *pkt = ost->pkt;
  int64_t pts;

  if (sub->pts == AV_NOPTS_VALUE) {
    AvLog(nullptr, AV_LOG_ERROR, "Subtitle packets must have a pts\n");
    if (exit_on_error) return false;
    return true;
  }

  AVCodecContext *enc = ost->enc_ctx;

  if (!subtitle_out) {
    subtitle_out = static_cast<uint8_t *>(av_malloc(subtitle_out_max_size));
    if (!subtitle_out) {
      AvLog(nullptr, AV_LOG_FATAL, "Failed to allocate subtitle_out\n");
      return false;
    }
  }

  /* Note: DVB subtitle need one packet to draw them and one other
     packet to clear them */
  /* XXX: signal it in the codec context ? */
  int nb;
  if (enc->codec_id == AV_CODEC_ID_DVB_SUBTITLE) {
    nb = 2;
  } else {
    nb = 1;
  }

  /* shift timestamp to honor -ss and make check_recording_time() work with -t
   */
  pts = sub->pts;
  if (output_files[ost->file_index]->start_time != AV_NOPTS_VALUE)
    pts -= output_files[ost->file_index]->start_time;
  for (int i = 0; i < nb; i++) {
    unsigned save_num_rects = sub->num_rects;

    ost->sync_opts = av_rescale_q(pts, {1, AV_TIME_BASE}, enc->time_base);
    if (!check_recording_time(ost)) return true;

    sub->pts = pts;
    // start_display_time is required to be 0
    sub->pts +=
        av_rescale_q(sub->start_display_time, {1, 1000}, {1, AV_TIME_BASE});
    sub->end_display_time -= sub->start_display_time;
    sub->start_display_time = 0;
    if (i == 1) {
      sub->num_rects = 0;
    }

    ost->frames_encoded++;

    subtitle_out_size =
        avcodec_encode_subtitle(enc, subtitle_out, subtitle_out_max_size, sub);
    if (i == 1) sub->num_rects = save_num_rects;
    if (subtitle_out_size < 0) {
      AvLog(nullptr, AV_LOG_FATAL, "Subtitle encoding failed\n");
      return false;
    }

    av_packet_unref(pkt);
    pkt->data = subtitle_out;
    pkt->size = subtitle_out_size;
    pkt->pts = av_rescale_q(sub->pts, {1, AV_TIME_BASE}, ost->mux_timebase);
    pkt->duration =
        av_rescale_q(sub->end_display_time, {1, 1000}, ost->mux_timebase);
    if (enc->codec_id == AV_CODEC_ID_DVB_SUBTITLE) {
      /* XXX: the pts correction is handled here. Maybe handling
         it in the codec would be better */
      if (i == 0) {
        pkt->pts +=
            av_rescale_q(sub->start_display_time, {1, 1000}, ost->mux_timebase);
      } else {
        pkt->pts +=
            av_rescale_q(sub->end_display_time, {1, 1000}, ost->mux_timebase);
      }
    }
    pkt->dts = pkt->pts;
    output_packet(of, pkt, ost, 0);
  }
  return true;
}

namespace {
av_const int mid_pred(int a, int b, int c) {
  if (a > b) {
    if (c > b) {
      if (c > a)
        b = a;
      else
        b = c;
    }
  } else {
    if (b > c) {
      if (c > a)
        b = c;
      else
        b = a;
    }
  }
  return b;
}
}  // namespace

/* May modify/reset next_picture */
bool FfmpegVideoConverter::do_video_out(OutputFile *of, OutputStream *ost,
                                        AVFrame *next_picture) {
  int ret;
  AVCodecContext *enc = ost->enc_ctx;
  AVRational frame_rate;
  int64_t nb_frames, nb0_frames, i;
  double delta, delta0;
  double duration = 0;
  double sync_ipts = AV_NOPTS_VALUE;
  InputStream *ist = nullptr;
  AVFilterContext *filter = ost->filter->filter;

  init_output_stream_wrapper(ost, next_picture, 1);
  sync_ipts = adjust_frame_pts_to_encoder_tb(of, ost, next_picture);

  if (ost->source_index >= 0) ist = input_streams[ost->source_index];

  frame_rate = av_buffersink_get_frame_rate(filter);
  if (frame_rate.num > 0 && frame_rate.den > 0)
    duration = 1 / (av_q2d(frame_rate) * av_q2d(enc->time_base));

  if (ist && ist->st->start_time != AV_NOPTS_VALUE &&
      ist->first_dts != AV_NOPTS_VALUE && ost->frame_rate.num)
    duration =
        FFMIN(duration, 1 / (av_q2d(ost->frame_rate) * av_q2d(enc->time_base)));

  if (!ost->filters_script && !ost->filters &&
      (nb_filtergraphs == 0 || !filtergraphs[0]->graph_desc) && next_picture &&
      ist &&
      lrintf(next_picture->pkt_duration * av_q2d(ist->st->time_base) /
             av_q2d(enc->time_base)) > 0) {
    duration = lrintf(next_picture->pkt_duration * av_q2d(ist->st->time_base) /
                      av_q2d(enc->time_base));
  }

  if (!next_picture) {
    // end, flushing
    nb0_frames = nb_frames =
        mid_pred(ost->last_nb0_frames[0], ost->last_nb0_frames[1],
                 ost->last_nb0_frames[2]);
  } else {
    delta0 = sync_ipts - ost->sync_opts;  // delta0 is the "drift" between the
                                          // input frame (next_picture) and
                                          // where it would fall in the output.
    delta = delta0 + duration;

    /* by default, we output a single frame */
    nb0_frames = 0;  // tracks the number of times the PREVIOUS frame should be
                     // duplicated, mostly for variable framerate (VFR)
    nb_frames = 1;

    if (delta0 < 0 && delta > 0 && ost->vsync_method != VSYNC_PASSTHROUGH &&
        ost->vsync_method != VSYNC_DROP) {
      if (delta0 < -0.6) {
        AvLog(nullptr, AV_LOG_VERBOSE, "Past duration %f too large\n", -delta0);
      } else
        AvLog(nullptr, AV_LOG_DEBUG,
              "Clipping frame in rate conversion by %f\n", -delta0);
      sync_ipts = ost->sync_opts;
      duration += delta0;
      delta0 = 0;
    }

    switch (ost->vsync_method) {
      case VSYNC_VSCFR:
        if (ost->frame_number == 0 && delta0 >= 0.5) {
          AvLog(nullptr, AV_LOG_DEBUG, "Not duplicating %d initial frames\n",
                (int)lrintf(delta0));
          delta = duration;
          delta0 = 0;
          ost->sync_opts = llrint(sync_ipts);
        }
      case VSYNC_CFR:
        // FIXME set to 0.5 after we fix some dts/pts bugs like in avidec.c
        if (frame_drop_threshold && delta < frame_drop_threshold &&
            ost->frame_number) {
          nb_frames = 0;
        } else if (delta < -1.1)
          nb_frames = 0;
        else if (delta > 1.1) {
          nb_frames = llrintf(delta);
          if (delta0 > 1.1) nb0_frames = llrintf(delta0 - 0.6);
        }
        break;
      case VSYNC_VFR:
        if (delta <= -0.6)
          nb_frames = 0;
        else if (delta > 0.6)
          ost->sync_opts = llrint(sync_ipts);
        break;
      case VSYNC_DROP:
      case VSYNC_PASSTHROUGH:
        ost->sync_opts = llrint(sync_ipts);
        break;
      default:
        av_assert0(0);
        break;
    }
  }

  /*
   * For video, number of frames in == number of packets out.
   * But there may be reordering, so we can't throw away frames on encoder
   * flush, we need to limit them here, before they go into encoder.
   */
  nb_frames = FFMIN(nb_frames, ost->max_frames - ost->frame_number);
  nb0_frames = FFMIN(nb0_frames, nb_frames);

  memmove(ost->last_nb0_frames + 1, ost->last_nb0_frames,
          sizeof(ost->last_nb0_frames[0]) *
              (FF_ARRAY_ELEMS(ost->last_nb0_frames) - 1));
  ost->last_nb0_frames[0] = nb0_frames;

  if (nb0_frames == 0 && ost->last_dropped) {
    nb_frames_drop++;
    AvLog(nullptr, AV_LOG_VERBOSE,
          "*** dropping frame %lld"
          " from stream %d at ts %lld\n",
          ost->frame_number, ost->st->index, ost->last_frame->pts);
  }
  if (nb_frames >
      (nb0_frames && ost->last_dropped) + (nb_frames > nb0_frames)) {
    if (nb_frames > dts_error_threshold * 30) {
      AvLog(nullptr, AV_LOG_ERROR,
            "%lld frame duplication too large, skipping\n", nb_frames - 1);
      nb_frames_drop++;
      return true;
    }
    nb_frames_dup += nb_frames - (nb0_frames && ost->last_dropped) -
                     (nb_frames > nb0_frames);
    AvLog(nullptr, AV_LOG_VERBOSE, "*** %lld dup!\n", nb_frames - 1);
    if (nb_frames_dup > dup_warning) {
      AvLog(nullptr, AV_LOG_WARNING,
            "More than %"
            "llu"
            " frames duplicated\n",
            dup_warning);
      dup_warning *= 10;
    }
  }
  ost->last_dropped = nb_frames == nb0_frames && next_picture;
  ost->dropped_keyframe =
      ost->last_dropped && next_picture && next_picture->key_frame;

  /* duplicates frame if needed */
  for (i = 0; i < nb_frames; i++) {
    AVFrame *in_picture;
    bool forced_keyframe = false;
    double pts_time;

    if (i < nb0_frames && ost->last_frame->buf[0]) {
      in_picture = ost->last_frame;
    } else
      in_picture = next_picture;

    if (!in_picture) return true;

    in_picture->pts = ost->sync_opts;

    if (!check_recording_time(ost)) return true;

    in_picture->quality = enc->global_quality;
    in_picture->pict_type = AV_PICTURE_TYPE_NONE;

    if (ost->forced_kf_ref_pts == AV_NOPTS_VALUE &&
        in_picture->pts != AV_NOPTS_VALUE)
      ost->forced_kf_ref_pts = in_picture->pts;

    pts_time = in_picture->pts != AV_NOPTS_VALUE
                   ? (in_picture->pts - ost->forced_kf_ref_pts) *
                         av_q2d(enc->time_base)
                   : NAN;
    if (ost->forced_kf_index < ost->forced_kf_count &&
        in_picture->pts >= ost->forced_kf_pts[ost->forced_kf_index]) {
      ost->forced_kf_index++;
      forced_keyframe = true;
    } else if (ost->forced_keyframes_pexpr) {
      double res;
      ost->forced_keyframes_expr_const_values[FKF_T] = pts_time;
      res = av_expr_eval(ost->forced_keyframes_pexpr,
                         ost->forced_keyframes_expr_const_values, nullptr);
      AvLog(nullptr, AV_LOG_DEBUG,
            "force_key_frame: n:%f n_forced:%f prev_forced_n:%f t:%f "
            "prev_forced_t:%f -> res:%f\n",
            ost->forced_keyframes_expr_const_values[FKF_N],
            ost->forced_keyframes_expr_const_values[FKF_N_FORCED],
            ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_N],
            ost->forced_keyframes_expr_const_values[FKF_T],
            ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_T], res);
      if (res) {
        forced_keyframe = true;
        ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_N] =
            ost->forced_keyframes_expr_const_values[FKF_N];
        ost->forced_keyframes_expr_const_values[FKF_PREV_FORCED_T] =
            ost->forced_keyframes_expr_const_values[FKF_T];
        ost->forced_keyframes_expr_const_values[FKF_N_FORCED] += 1;
      }

      ost->forced_keyframes_expr_const_values[FKF_N] += 1;
    } else if (ost->forced_keyframes &&
               !strncmp(ost->forced_keyframes, "source", 6) &&
               in_picture->key_frame == 1 && !i) {
      forced_keyframe = true;
    } else if (ost->forced_keyframes &&
               !strncmp(ost->forced_keyframes, "source_no_drop", 14) && !i) {
      forced_keyframe = (in_picture->key_frame == 1) || ost->dropped_keyframe;
      ost->dropped_keyframe = false;
    }

    if (forced_keyframe) {
      in_picture->pict_type = AV_PICTURE_TYPE_I;
      AvLog(nullptr, AV_LOG_DEBUG, "Forced keyframe at time %f\n", pts_time);
    }

    ret = encode_frame(of, ost, in_picture);
    if (ret < 0) return false;

    ost->sync_opts++;
    ost->frame_number++;
  }

  av_frame_unref(ost->last_frame);
  if (next_picture) av_frame_move_ref(ost->last_frame, next_picture);

  return true;
}

void FfmpegVideoConverter::finish_output_stream(OutputStream *ost) {
  OutputFile *of = output_files[ost->file_index];
  AVRational time_base =
      ost->stream_copy ? ost->mux_timebase : ost->enc_ctx->time_base;

  ost->finished = static_cast<OSTFinished>(ENCODER_FINISHED | MUXER_FINISHED);

  if (of->shortest) {
    int64_t end = av_rescale_q(ost->sync_opts - ost->first_pts, time_base,
                               {1, AV_TIME_BASE});
    of->recording_time = FFMIN(of->recording_time, end);
  }
}

/**
 * Get and encode new output from any of the filtergraphs, without causing
 * activity.
 *
 * @return  0 for success, <0 for severe errors
 */
int FfmpegVideoConverter::reap_filters(int flush) {
  AVFrame *filtered_frame = nullptr;

  /* Reap all buffers present in the buffer sinks */
  for (int i = 0; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];
    OutputFile *of = output_files[ost->file_index];
    AVFilterContext *filter;
    AVCodecContext *enc = ost->enc_ctx;
    int ret = 0;

    if (!ost->filter || !ost->filter->graph->graph) continue;
    filter = ost->filter->filter;

    /*
     * Unlike video, with audio the audio frame size matters.
     * Currently we are fully reliant on the lavfi filter chain to
     * do the buffering deed for us, and thus the frame size parameter
     * needs to be set accordingly. Where does one get the required
     * frame size? From the initialized AVCodecContext of an audio
     * encoder. Thus, if we have gotten to an audio stream, initialize
     * the encoder earlier than receiving the first AVFrame.
     */
    if (av_buffersink_get_type(filter) == AVMEDIA_TYPE_AUDIO)
      init_output_stream_wrapper(ost, nullptr, 1);

    filtered_frame = ost->filtered_frame;

    while (1) {
      ret = av_buffersink_get_frame_flags(filter, filtered_frame,
                                          AV_BUFFERSINK_FLAG_NO_REQUEST);
      if (ret < 0) {
        if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
          AvLog(nullptr, AV_LOG_WARNING,
                "Error in av_buffersink_get_frame_flags(): %s\n",
                AvErr2Str(ret));
        } else if (flush && ret == AVERROR_EOF) {
          if (av_buffersink_get_type(filter) == AVMEDIA_TYPE_VIDEO) {
            do_video_out(of, ost, nullptr);
          }
        }
        break;
      }
      if (ost->finished) {
        av_frame_unref(filtered_frame);
        continue;
      }

      switch (av_buffersink_get_type(filter)) {
        case AVMEDIA_TYPE_VIDEO:
          if (!ost->frame_aspect_ratio.num)
            enc->sample_aspect_ratio = filtered_frame->sample_aspect_ratio;

          do_video_out(of, ost, filtered_frame);
          break;
        case AVMEDIA_TYPE_AUDIO:
          if (!(enc->codec->capabilities & AV_CODEC_CAP_PARAM_CHANGE) &&
              enc->ch_layout.nb_channels !=
                  filtered_frame->ch_layout.nb_channels) {
            AvLog(nullptr, AV_LOG_ERROR,
                  "Audio filter graph output is not normalized and encoder "
                  "does not support parameter changes\n");
            break;
          }
          do_audio_out(of, ost, filtered_frame);
          break;
        default:
          // TODO support subtitle filters
          av_assert0(0);
          break;
      }

      av_frame_unref(filtered_frame);
    }
  }

  return 0;
}

/**
 * Perform a step of transcoding for the specified filter graph.
 *
 * @param[in]  graph     filter graph to consider
 * @param[out] best_ist  input stream where a frame would allow to continue
 * @return  0 for success, <0 for error
 */
int FfmpegVideoConverter::transcode_from_filter(FilterGraph *graph,
                                                InputStream **best_ist) {
  int i, ret;
  int nb_requests, nb_requests_max = 0;
  InputFilter *ifilter;
  InputStream *ist;

  *best_ist = nullptr;
  ret = avfilter_graph_request_oldest(graph->graph);
  if (ret >= 0) return reap_filters(0);

  if (ret == AVERROR_EOF) {
    ret = reap_filters(1);
    for (i = 0; i < graph->nb_outputs; i++)
      close_output_stream(graph->outputs[i]->ost);
    return ret;
  }
  if (ret != AVERROR(EAGAIN)) return ret;

  for (i = 0; i < graph->nb_inputs; i++) {
    ifilter = graph->inputs[i];
    ist = ifilter->ist;
    if (input_files[ist->file_index]->eagain ||
        input_files[ist->file_index]->eof_reached)
      continue;
    nb_requests = av_buffersrc_get_nb_failed_requests(ifilter->filter);
    if (nb_requests > nb_requests_max) {
      nb_requests_max = nb_requests;
      *best_ist = ist;
    }
  }

  if (!*best_ist) {
    for (i = 0; i < graph->nb_outputs; i++) {
      graph->outputs[i]->ost->unavailable = true;
    }
  }

  return 0;
}

/**
 * Run a single step of transcoding.
 *
 * @return  0 for success, <0 for error
 */
int FfmpegVideoConverter::transcode_step(void) {
  {
    VideoConverter::Response r;
    this->PostConvertProgress(r);
  }
  
  int ret;

  OutputStream *ost = choose_output();
  if (!ost) {
    if (got_eagain()) {
      reset_eagain();
      av_usleep(10000);
      return 0;
    }
    AvLog(nullptr, AV_LOG_VERBOSE, "No more inputs to read from, finishing.\n");
    return AVERROR_EOF;
  }

  if (ost->filter && !ost->filter->graph->graph) {
    if (ifilter_has_all_input_formats(ost->filter->graph)) {
      ret = configure_filtergraph(ost->filter->graph);
      if (ret < 0) {
        AvLog(nullptr, AV_LOG_ERROR, "Error reinitializing filters!\n");
        return ret;
      }
    }
  }

  InputStream *ist = nullptr;
  if (ost->filter && ost->filter->graph->graph) {
    /*
     * Similar case to the early audio initialization in reap_filters.
     * Audio is special in ffmpeg.c currently as we depend on lavfi's
     * audio frame buffering/creation to get the output audio frame size
     * in samples correct. The audio frame size for the filter chain is
     * configured during the output stream initialization.
     *
     * Apparently avfilter_graph_request_oldest (called in
     * transcode_from_filter just down the line) peeks. Peeking already
     * puts one frame "ready to be given out", which means that any
     * update in filter buffer sink configuration afterwards will not
     * help us. And yes, even if it would be utilized,
     * av_buffersink_get_samples is affected, as it internally utilizes
     * the same early exit for peeked frames.
     *
     * In other words, if avfilter_graph_request_oldest would not make
     * further filter chain configuration or usage of
     * av_buffersink_get_samples useless (by just causing the return
     * of the peeked AVFrame as-is), we could get rid of this additional
     * early encoder initialization.
     */
    if (av_buffersink_get_type(ost->filter->filter) == AVMEDIA_TYPE_AUDIO)
      init_output_stream_wrapper(ost, nullptr, 1);

    if ((ret = transcode_from_filter(ost->filter->graph, &ist)) < 0) return ret;
    if (!ist) return 0;
  } else if (ost->filter) {
    int i;
    for (i = 0; i < ost->filter->graph->nb_inputs; i++) {
      InputFilter *ifilter = ost->filter->graph->inputs[i];
      if (!ifilter->ist->got_output &&
          !input_files[ifilter->ist->file_index]->eof_reached) {
        ist = ifilter->ist;
        break;
      }
    }
    if (!ist) {
      ost->inputs_done = true;
      return 0;
    }
  } else {
    av_assert0(ost->source_index >= 0);
    ist = input_streams[ost->source_index];
  }

  ret = process_input(ist->file_index);
  if (ret == AVERROR(EAGAIN)) {
    if (input_files[ist->file_index]->eagain) ost->unavailable = true;
    return 0;
  }

  if (ret < 0) return ret == AVERROR_EOF ? 0 : ret;

  return reap_filters(0);
}
void FfmpegVideoConverter::sub2video_heartbeat(InputStream *ist, int64_t pts) {
  InputFile *infile = input_files[ist->file_index];
  /* When a frame is read from a file, examine all sub2video streams in
     the same file and send the sub2video frame again. Otherwise, decoded
     video frames could be accumulating in the filter graph while a filter
     (possibly overlay) is desperately waiting for a subtitle frame. */
  for (int i = 0; i < infile->nb_streams; i++) {
    InputStream *ist2 = input_streams[infile->ist_index + i];
    if (!ist2->sub2video.frame) {
      continue;
    }
    /* subtitles seem to be usually muxed ahead of other streams;
       if not, subtracting a larger time here is necessary */
    int64_t pts2 =
        av_rescale_q(pts, ist->st->time_base, ist2->st->time_base) - 1;
    /* do not send the heartbeat frame if the subtitle is already ahead */
    if (pts2 <= ist2->sub2video.last_pts) {
      continue;
    }
    if (pts2 >= ist2->sub2video.end_pts || ist2->sub2video.initialize) {
      /* if we have hit the end of the current displayed subpicture,
         or if we need to initialize the system, update the
         overlayed subpicture and its start/end times */
      sub2video_update(ist2, pts2 + 1, nullptr);
    }
    int nb_reqs = 0;
    for (int j = 0; j < ist2->nb_filters; j++) {
      nb_reqs += av_buffersrc_get_nb_failed_requests(ist2->filters[j]->filter);
    }
    if (nb_reqs) {
      sub2video_push_ref(ist2, pts2);
    }
  }
}
void FfmpegVideoConverter::sub2video_flush(InputStream *ist) {
  if (ist->sub2video.end_pts < INT64_MAX) {
    sub2video_update(ist, INT64_MAX, nullptr);
  }
  for (int i = 0; i < ist->nb_filters; i++) {
    int ret = av_buffersrc_add_frame(ist->filters[i]->filter, nullptr);
    if (ret != AVERROR_EOF && ret < 0) {
      AvLog(nullptr, AV_LOG_WARNING, "Flush the frame error.\n");
    }
  }
}

bool FfmpegVideoConverter::check_decode_result(InputStream *ist,
                                               bool *got_output, int ret) {
  if (*got_output || ret < 0) {
    if (ret < 0) {
      decode_error_stat[1]++;
    } else {
      decode_error_stat[0]++;
    }
  }

  if (ret < 0 && exit_on_error) {
    return false;
  }

  if (*got_output && ist) {
    if (ist->decoded_frame->decode_error_flags ||
        (ist->decoded_frame->flags & AV_FRAME_FLAG_CORRUPT)) {
      AvLog(nullptr, exit_on_error ? AV_LOG_FATAL : AV_LOG_WARNING,
            "%s: corrupt decoded frame in stream %d\n",
            input_files[ist->file_index]->ctx->url, ist->st->index);
      if (exit_on_error) {
        return false;
      }
    }
  }
  return true;
}

bool FfmpegVideoConverter::ifilter_has_all_input_formats(FilterGraph *fg) {
  for (int i = 0; i < fg->nb_inputs; i++) {
    if (fg->inputs[i]->format < 0 &&
        (fg->inputs[i]->type == AVMEDIA_TYPE_AUDIO ||
         fg->inputs[i]->type == AVMEDIA_TYPE_VIDEO)) {
      return false;
    }
  }
  return true;
}
int FfmpegVideoConverter::get_input_packet(InputFile *f, AVPacket **pkt) {
  if (f->readrate || f->rate_emu) {
    int64_t file_start =
        copy_ts * ((f->ctx->start_time != AV_NOPTS_VALUE
                        ? f->ctx->start_time * !start_at_zero
                        : 0) +
                   (f->start_time != AV_NOPTS_VALUE ? f->start_time : 0));
    float scale = f->rate_emu ? 1.0 : f->readrate;
    for (int i = 0; i < f->nb_streams; i++) {
      InputStream *ist = input_streams[f->ist_index + i];
      int64_t stream_ts_offset, pts, now;
      if (!ist->nb_packets || (ist->decoding_needed && !ist->got_output)) {
        continue;
      }
      stream_ts_offset = FFMAX(
          ist->first_dts != AV_NOPTS_VALUE ? ist->first_dts : 0, file_start);
      pts = av_rescale(ist->dts, 1000000, AV_TIME_BASE);
      now = (av_gettime_relative() - ist->start) * scale + stream_ts_offset;
      if (pts > now) {
        return AVERROR(EAGAIN);
      }
    }
  }

  *pkt = f->pkt;
  return av_read_frame(f->ctx, *pkt);
}
bool FfmpegVideoConverter::got_eagain() {
  for (int i = 0; i < nb_output_streams; i++) {
    if (output_streams[i]->unavailable) {
      return true;
    }
  }
  return false;
}

void FfmpegVideoConverter::reset_eagain() {
  for (int i = 0; i < nb_input_files; i++) {
    input_files[i]->eagain = false;
  }
  for (int i = 0; i < nb_output_streams; i++) {
    output_streams[i]->unavailable = false;
  }
}

bool FfmpegVideoConverter::flush_encoders(void) {
  int ret;

  for (int i = 0; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];
    AVCodecContext *enc = ost->enc_ctx;
    OutputFile *of = output_files[ost->file_index];

    if (!ost->encoding_needed) continue;

    // Try to enable encoding with no input frames.
    // Maybe we should just let encoding fail instead.
    if (!ost->initialized) {
      FilterGraph *fg = ost->filter->graph;

      AvLog(nullptr, AV_LOG_WARNING,
            "Finishing stream %d:%d without any data written to it.\n",
            ost->file_index, ost->st->index);

      if (ost->filter && !fg->graph) {
        int x;
        for (x = 0; x < fg->nb_inputs; x++) {
          InputFilter *ifilter = fg->inputs[x];
          if (ifilter->format < 0 &&
              ifilter_parameters_from_codecpar(
                  ifilter, ifilter->ist->st->codecpar) < 0) {
            AvLog(nullptr, AV_LOG_ERROR,
                  "Error copying paramerets from input stream\n");
            return false;
          }
        }

        if (!ifilter_has_all_input_formats(fg)) continue;

        ret = configure_filtergraph(fg);
        if (ret < 0) {
          AvLog(nullptr, AV_LOG_ERROR, "Error configuring filter graph\n");
          return false;
        }

        finish_output_stream(ost);
      }

      init_output_stream_wrapper(ost, nullptr, 1);
    }

    if (enc->codec_type != AVMEDIA_TYPE_VIDEO &&
        enc->codec_type != AVMEDIA_TYPE_AUDIO)
      continue;

    ret = encode_frame(of, ost, nullptr);
    if (ret != AVERROR_EOF) {
      return false;
    }
  }
  return true;
}

namespace {
double psnr(double d) { return -10.0 * log10(d); }
}  // namespace

bool FfmpegVideoConverter::update_video_stats(OutputStream *ost,
                                              const AVPacket *pkt,
                                              int write_vstats) {
  const uint8_t *sd =
      av_packet_get_side_data(pkt, AV_PKT_DATA_QUALITY_STATS, nullptr);
  AVCodecContext *enc = ost->enc_ctx;
  int64_t frame_number;
  double ti1, bitrate, avg_bitrate;

  ost->quality = sd ? AV_RL32(sd) : -1;
  ost->pict_type =
      sd ? static_cast<AVPictureType>(sd[4]) : AV_PICTURE_TYPE_NONE;

  for (int i = 0; i < FF_ARRAY_ELEMS(ost->error); i++) {
    if (sd && i < sd[5])
      ost->error[i] = AV_RL64(sd + 8 + 8 * i);
    else
      ost->error[i] = -1;
  }

  if (!write_vstats) return true;

  /* this is executed just the first time update_video_stats is called */
  if (!vstats_file) {
    vstats_file = fopen(vstats_filename, "w");
    if (!vstats_file) {
      perror("fopen");
      return false;
    }
  }

  frame_number = ost->packets_encoded;
  if (vstats_version <= 1) {
    fprintf(vstats_file, "frame= %5lld q= %2.1f ", frame_number,
            ost->quality / (float)FF_QP2LAMBDA);
  } else {
    fprintf(vstats_file, "out= %2d st= %2d frame= %5lld q= %2.1f ",
            ost->file_index, ost->index, frame_number,
            ost->quality / (float)FF_QP2LAMBDA);
  }

  if (ost->error[0] >= 0 && (enc->flags & AV_CODEC_FLAG_PSNR))
    fprintf(vstats_file, "PSNR= %6.2f ",
            psnr(ost->error[0] / (enc->width * enc->height * 255.0 * 255.0)));

  fprintf(vstats_file, "f_size= %6d ", pkt->size);
  /* compute pts value */
  ti1 = pkt->dts * av_q2d(ost->mux_timebase);
  if (ti1 < 0.01) ti1 = 0.01;

  bitrate = (pkt->size * 8) / av_q2d(enc->time_base) / 1000.0;
  avg_bitrate = (double)(ost->data_size * 8) / ti1 / 1000.0;
  fprintf(vstats_file,
          "s_size= %8.0fkB time= %0.3f br= %7.1fkbits/s avg_br= %7.1fkbits/s ",
          (double)ost->data_size / 1024, ti1, bitrate, avg_bitrate);
  fprintf(vstats_file, "type= %c\n",
          av_get_picture_type_char(static_cast<AVPictureType>(ost->pict_type)));
  return true;
}

int FfmpegVideoConverter::encode_frame(OutputFile *of, OutputStream *ost,
                                       AVFrame *frame) {
  AVCodecContext *enc = ost->enc_ctx;
  AVPacket *pkt = ost->pkt;
  const char *type_desc = av_get_media_type_string(enc->codec_type);
  const char *action = frame ? "encode" : "flush";
  int ret;

  if (frame) {
    ost->frames_encoded++;

    if (debug_ts) {
    }
  }

  update_benchmark(nullptr);

  ret = avcodec_send_frame(enc, frame);
  if (ret < 0 && !(ret == AVERROR_EOF && !frame)) {
    AvLog(nullptr, AV_LOG_ERROR, "Error submitting %s frame to the encoder\n",
          type_desc);
    return ret;
  }

  while (true) {
    ret = avcodec_receive_packet(enc, pkt);
    update_benchmark("%s_%s %d.%d", action, type_desc, ost->file_index,
                     ost->index);

    /* if two pass, output log on success and EOF */
    if ((ret >= 0 || ret == AVERROR_EOF) && ost->logfile && enc->stats_out) {
      fprintf(ost->logfile, "%s", enc->stats_out);
    }

    if (ret == AVERROR(EAGAIN)) {
      av_assert0(frame);  // should never happen during flushing
      return 0;
    } else if (ret == AVERROR_EOF) {
      output_packet(of, pkt, ost, 1);
      return ret;
    } else if (ret < 0) {
      AvLog(nullptr, AV_LOG_ERROR, "%s encoding failed\n", type_desc);
      return ret;
    }

    if (debug_ts) {
      AvLog(nullptr, AV_LOG_INFO,
            "encoder -> type:%s "
            "pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s "
            "duration:%s duration_time:%s\n",
            type_desc, AvTs2Str(pkt->pts),
            AvTs2TimeStr(pkt->pts, &enc->time_base), AvTs2Str(pkt->dts),
            AvTs2TimeStr(pkt->dts, &enc->time_base), AvTs2Str(pkt->duration),
            AvTs2TimeStr(pkt->duration, &enc->time_base));
    }

    av_packet_rescale_ts(pkt, enc->time_base, ost->mux_timebase);

    if (debug_ts) {
      av_log(nullptr, AV_LOG_INFO,
             "encoder -> type:%s "
             "pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s "
             "duration:%s duration_time:%s\n",
             type_desc, AvTs2Str(pkt->pts),
             AvTs2TimeStr(pkt->pts, &enc->time_base), AvTs2Str(pkt->dts),
             AvTs2TimeStr(pkt->dts, &enc->time_base), AvTs2Str(pkt->duration),
             AvTs2TimeStr(pkt->duration, &enc->time_base));
    }

    if (enc->codec_type == AVMEDIA_TYPE_VIDEO)
      update_video_stats(ost, pkt, !!vstats_filename);

    ost->packets_encoded++;

    output_packet(of, pkt, ost, 0);
  }

  av_assert0(0);
}

HWDevice *FfmpegVideoConverter::hw_device_get_by_name(const char *name) {
  return ::hw_device_get_by_name(name);
}

void FfmpegVideoConverter::close_all_output_streams(OutputStream *ost,
                                                    OSTFinished this_stream,
                                                    OSTFinished others) {
  for (int i = 0; i < nb_output_streams; i++) {
    OutputStream *ost2 = output_streams[i];
    ost2->finished = static_cast<OSTFinished>(
        (ost2->finished) | (ost == ost2 ? this_stream : others));
  }
}

bool FfmpegVideoConverter::of_write_packet(OutputFile *of, AVPacket *pkt,
                                           OutputStream *ost, int unqueue) {
  AVFormatContext *s = of->ctx;
  AVStream *st = ost->st;
  int ret;

  /*
   * Audio encoders may split the packets --  #frames in != #packets out.
   * But there is no reordering, so we can limit the number of output packets
   * by simply dropping them here.
   * Counting encoded video frames needs to be done separately because of
   * reordering, see do_video_out().
   * Do not count the packet when unqueued because it has been counted when
   * queued.
   */
  if (!(st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
        ost->encoding_needed) &&
      !unqueue) {
    if (ost->frame_number >= ost->max_frames) {
      av_packet_unref(pkt);
      return true;
    }
    ost->frame_number++;
  }

  if (!of->header_written) {
    AVPacket *tmp_pkt;
    /* the muxer is not initialized yet, buffer the packet */
    if (!av_fifo_can_write(ost->muxing_queue)) {
      size_t cur_size = av_fifo_can_read(ost->muxing_queue);
      unsigned int are_we_over_size =
          (ost->muxing_queue_data_size + pkt->size) >
          ost->muxing_queue_data_threshold;
      size_t limit = are_we_over_size ? ost->max_muxing_queue_size : SIZE_MAX;
      size_t new_size = FFMIN(2 * cur_size, limit);

      if (new_size <= cur_size) {
        AvLog(nullptr, AV_LOG_ERROR,
              "Too many packets buffered for output stream %d:%d.\n",
              ost->file_index, ost->st->index);
        return false;
      }
      ret = av_fifo_grow2(ost->muxing_queue, new_size - cur_size);
      if (ret < 0) {
        return false;
      }
    }
    ret = av_packet_make_refcounted(pkt);
    if (ret < 0) {
      return false;
    }
    tmp_pkt = av_packet_alloc();
    if (!tmp_pkt) {
      return false;
    }
    av_packet_move_ref(tmp_pkt, pkt);
    ost->muxing_queue_data_size += tmp_pkt->size;
    av_fifo_write(ost->muxing_queue, &tmp_pkt, 1);
    return true;
  }

  if ((st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
       ost->vsync_method == VSYNC_DROP) ||
      (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_sync_method < 0))
    pkt->pts = pkt->dts = AV_NOPTS_VALUE;

  if (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    if (ost->frame_rate.num && ost->is_cfr) {
      if (pkt->duration > 0)
        AvLog(nullptr, AV_LOG_WARNING,
              "Overriding packet duration by frame rate, this should not "
              "happen\n");
      pkt->duration =
          av_rescale_q(1, av_inv_q(ost->frame_rate), ost->mux_timebase);
    }
  }

  av_packet_rescale_ts(pkt, ost->mux_timebase, ost->st->time_base);

  if (!(s->oformat->flags & AVFMT_NOTIMESTAMPS)) {
    if (pkt->dts != AV_NOPTS_VALUE && pkt->pts != AV_NOPTS_VALUE &&
        pkt->dts > pkt->pts) {
      AvLog(s, AV_LOG_WARNING,
            "Invalid DTS: %lld PTS: %lld"
            " in output stream %d:%d, replacing by guess\n",
            pkt->dts, pkt->pts, ost->file_index, ost->st->index);
      pkt->pts = pkt->dts = pkt->pts + pkt->dts + ost->last_mux_dts + 1 -
                            FFMIN3(pkt->pts, pkt->dts, ost->last_mux_dts + 1) -
                            FFMAX3(pkt->pts, pkt->dts, ost->last_mux_dts + 1);
    }
    if ((st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ||
         st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ||
         st->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) &&
        pkt->dts != AV_NOPTS_VALUE && ost->last_mux_dts != AV_NOPTS_VALUE) {
      int64_t max =
          ost->last_mux_dts + !(s->oformat->flags & AVFMT_TS_NONSTRICT);
      if (pkt->dts < max) {
        int loglevel =
            max - pkt->dts > 2 || st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
                ? AV_LOG_WARNING
                : AV_LOG_DEBUG;
        if (exit_on_error) loglevel = AV_LOG_ERROR;
        AvLog(s, loglevel,
              "Non-monotonous DTS in output stream "
              "%d:%d; previous: %lld, current: %lld; ",
              ost->file_index, ost->st->index, ost->last_mux_dts, pkt->dts);
        if (exit_on_error) {
          AvLog(nullptr, AV_LOG_FATAL, "aborting.\n");
          return false;
        }
        AvLog(s, loglevel,
              "changing to %lld. This may result "
              "in incorrect timestamps in the output file.\n",
              max);
        if (pkt->pts >= pkt->dts) pkt->pts = FFMAX(pkt->pts, max);
        pkt->dts = max;
      }
    }
  }
  ost->last_mux_dts = pkt->dts;

  ost->data_size += pkt->size;
  ost->packets_written++;

  pkt->stream_index = ost->index;

  if (debug_ts) {
    AvLog(nullptr, AV_LOG_INFO,
          "muxer <- type:%s "
          "pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s duration:%s "
          "duration_time:%s size:%d\n",
          av_get_media_type_string(ost->enc_ctx->codec_type),
          AvTs2Str(pkt->pts), AvTs2TimeStr(pkt->pts, &ost->st->time_base),
          AvTs2Str(pkt->dts), AvTs2TimeStr(pkt->dts, &ost->st->time_base),
          AvTs2Str(pkt->duration),
          AvTs2TimeStr(pkt->duration, &ost->st->time_base), pkt->size);
  }

  ret = av_interleaved_write_frame(s, pkt);
  if (ret < 0) {
    PrintError("av_interleaved_write_frame()", ret);
    main_return_code = 1;
    close_all_output_streams(
        ost, static_cast<OSTFinished>(MUXER_FINISHED | ENCODER_FINISHED),
        ENCODER_FINISHED);
  }
  return true;
}

int FfmpegVideoConverter::print_sdp(void) { return 0; }

/* open the muxer when all the streams are initialized */
bool FfmpegVideoConverter::of_check_init(OutputFile *of) {
  for (int i = 0; i < of->ctx->nb_streams; i++) {
    OutputStream *ost = output_streams[of->ost_index + i];
    if (!ost->initialized) {
      return true;
    }
  }

  int ret = avformat_write_header(of->ctx, &of->opts);
  if (ret < 0) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Could not write header for output file #%d "
          "(incorrect codec parameters ?): %s\n",
          of->index, AvErr2Str(ret));
    return false;
  }
  // assert_avoptions(of->opts);
  of->header_written = true;

  av_dump_format(of->ctx, of->index, of->ctx->url, 1);
  nb_output_dumped++;

  if (sdp_filename || want_sdp) {
    ret = print_sdp();
    if (ret < 0) {
      AvLog(nullptr, AV_LOG_ERROR, "Error writing the SDP.\n");
      return false;
    }
  }

  /* flush the muxing queues */
  for (int i = 0; i < of->ctx->nb_streams; i++) {
    OutputStream *ost = output_streams[of->ost_index + i];
    AVPacket *pkt;

    /* try to improve muxing time_base (only possible if nothing has been
     * written yet) */
    if (!av_fifo_can_read(ost->muxing_queue)) {
      ost->mux_timebase = ost->st->time_base;
    }

    while (av_fifo_read(ost->muxing_queue, &pkt, 1) >= 0) {
      ost->muxing_queue_data_size -= pkt->size;
      of_write_packet(of, pkt, ost, 1);
      av_packet_free(&pkt);
    }
  }

  return true;
}

int FfmpegVideoConverter::of_write_trailer(OutputFile *of) {
  int ret;

  if (!of->header_written) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Nothing was written into output file %d (%s), because "
          "at least one of its streams received no packets.\n",
          of->index, of->ctx->url);
    return AVERROR(EINVAL);
  }

  ret = av_write_trailer(of->ctx);
  if (ret < 0) {
    return ret;
  }

  return 0;
}

void FfmpegVideoConverter::of_close(OutputFile **pof) {
  OutputFile *of = *pof;
  AVFormatContext *s;

  if (!of) return;

  s = of->ctx;
  if (s && s->oformat && !(s->oformat->flags & AVFMT_NOFILE))
    avio_closep(&s->pb);
  avformat_free_context(s);
  av_dict_free(&of->opts);

  av_freep(pof);
}

// FIXME: YUV420P etc. are actually supported with full color range,
// yet the latter information isn't available here.
const AVPixelFormat *FfmpegVideoConverter::get_compliance_normal_pix_fmts(
    const AVCodec *codec, const AVPixelFormat default_formats[]) {
  static const enum AVPixelFormat mjpeg_formats[] = {
      AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_YUVJ444P,
      AV_PIX_FMT_NONE};

  if (!strcmp(codec->name, "mjpeg")) {
    return mjpeg_formats;
  } else {
    return default_formats;
  }
}

AVPixelFormat FfmpegVideoConverter::choose_pixel_fmt(AVStream *st,
                                                     AVCodecContext *enc_ctx,
                                                     const AVCodec *codec,
                                                     AVPixelFormat target) {
  if (codec && codec->pix_fmts) {
    const AVPixelFormat *p = codec->pix_fmts;
    const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(target);
    // FIXME: This should check for AV_PIX_FMT_FLAG_ALPHA after PAL8 pixel
    // format without alpha is implemented
    int has_alpha = desc ? desc->nb_components % 2 == 0 : 0;
    AVPixelFormat best = AV_PIX_FMT_NONE;

    if (enc_ctx->strict_std_compliance > FF_COMPLIANCE_UNOFFICIAL) {
      p = get_compliance_normal_pix_fmts(codec, p);
    }
    for (; *p != AV_PIX_FMT_NONE; p++) {
      best = av_find_best_pix_fmt_of_2(best, *p, target, has_alpha, nullptr);
      if (*p == target) break;
    }
    if (*p == AV_PIX_FMT_NONE) {
      if (target != AV_PIX_FMT_NONE)
        AvLog(nullptr, AV_LOG_WARNING,
              "Incompatible pixel format '%s' for codec '%s', auto-selecting "
              "format '%s'\n",
              av_get_pix_fmt_name(target), codec->name,
              av_get_pix_fmt_name(best));
      return best;
    }
  }
  return target;
}

/* May return nullptr (no pixel format found), a static string or a string
 * backed by the bprint. Nothing has been written to the AVBPrint in case
 * nullptr is returned. The AVBPrint provided should be clean. */
const char *FfmpegVideoConverter::choose_pix_fmts(OutputFilter *ofilter,
                                                  AVBPrint *bprint) {
  OutputStream *ost = ofilter->ost;
  const AVDictionaryEntry *strict_dict =
      av_dict_get(ost->encoder_opts, "strict", nullptr, 0);
  if (strict_dict)
    // used by choose_pixel_fmt() and below
    av_opt_set(ost->enc_ctx, "strict", strict_dict->value, 0);

  if (ost->keep_pix_fmt) {
    avfilter_graph_set_auto_convert(ofilter->graph->graph,
                                    AVFILTER_AUTO_CONVERT_NONE);
    if (ost->enc_ctx->pix_fmt == AV_PIX_FMT_NONE) return nullptr;
    return av_get_pix_fmt_name(ost->enc_ctx->pix_fmt);
  }
  if (ost->enc_ctx->pix_fmt != AV_PIX_FMT_NONE) {
    return av_get_pix_fmt_name(choose_pixel_fmt(ost->st, ost->enc_ctx, ost->enc,
                                                ost->enc_ctx->pix_fmt));
  } else if (ost->enc && ost->enc->pix_fmts) {
    const enum AVPixelFormat *p;

    p = ost->enc->pix_fmts;
    if (ost->enc_ctx->strict_std_compliance > FF_COMPLIANCE_UNOFFICIAL) {
      p = get_compliance_normal_pix_fmts(ost->enc, p);
    }

    for (; *p != AV_PIX_FMT_NONE; p++) {
      const char *name = av_get_pix_fmt_name(*p);
      av_bprintf(bprint, "%s%c", name, p[1] == AV_PIX_FMT_NONE ? '\0' : '|');
    }
    if (!av_bprint_is_complete(bprint)) {
      return nullptr;
    }
    return bprint->str;
  } else
    return nullptr;
}

/* Define a function for appending a list of allowed formats
 * to an AVBPrint. If nonempty, the list will have a header. */
#define DEF_CHOOSE_FORMAT(name, type, var, supported_list, none,       \
                          printf_format, get_name)                     \
  static void choose_##name(OutputFilter *ofilter, AVBPrint *bprint) { \
    if (ofilter->var == none && !ofilter->supported_list) return;      \
    av_bprintf(bprint, #name "=");                                     \
    if (ofilter->var != none) {                                        \
      av_bprintf(bprint, printf_format, get_name(ofilter->var));       \
    } else {                                                           \
      const type *p;                                                   \
                                                                       \
      for (p = ofilter->supported_list; *p != none; p++) {             \
        av_bprintf(bprint, printf_format "|", get_name(*p));           \
      }                                                                \
      if (bprint->len > 0) bprint->str[--bprint->len] = '\0';          \
    }                                                                  \
    av_bprint_chars(bprint, ':', 1);                                   \
  }

static void choose_sample_fmts(OutputFilter *ofilter, AVBPrint *bprint) {
  if (ofilter->format == AV_SAMPLE_FMT_NONE && !ofilter->formats) return;
  av_bprintf(bprint, "sample_fmts =");
  if (ofilter->format != AV_SAMPLE_FMT_NONE) {
    av_bprintf(
        bprint, "%s",
        av_get_sample_fmt_name(static_cast<AVSampleFormat>(ofilter->format)));
  } else {
    const int *p;

    for (p = ofilter->formats; *p != AV_SAMPLE_FMT_NONE; p++) {
      av_bprintf(bprint, "%s|",
                 av_get_sample_fmt_name(static_cast<AVSampleFormat>(*p)));
    }
    if (bprint->len > 0) bprint->str[--bprint->len] = '\0';
  }
  av_bprint_chars(bprint, ':', 1);
}

DEF_CHOOSE_FORMAT(sample_rates, int, sample_rate, sample_rates, 0, "%d", )

void FfmpegVideoConverter::choose_channel_layouts(OutputFilter *ofilter,
                                                  AVBPrint *bprint) {
  if (av_channel_layout_check(&ofilter->ch_layout)) {
    av_bprintf(bprint, "channel_layouts=");
    av_channel_layout_describe_bprint(&ofilter->ch_layout, bprint);
  } else if (ofilter->ch_layouts) {
    const AVChannelLayout *p;

    av_bprintf(bprint, "channel_layouts=");
    for (p = ofilter->ch_layouts; p->nb_channels; p++) {
      av_channel_layout_describe_bprint(p, bprint);
      av_bprintf(bprint, "|");
    }
    if (bprint->len > 0) bprint->str[--bprint->len] = '\0';
  } else
    return;
  av_bprint_chars(bprint, ':', 1);
}

int FfmpegVideoConverter::init_simple_filtergraph(InputStream *ist,
                                                  OutputStream *ost) {
  FilterGraph *fg = static_cast<FilterGraph *>(av_mallocz(sizeof(*fg)));
  OutputFilter *ofilter;
  InputFilter *ifilter;

  if (!fg) {
    return -1;
  }
  fg->index = nb_filtergraphs;

  ofilter = static_cast<OutputFilter *>(allocate_array_elem(
      &fg->outputs, sizeof(*fg->outputs[0]), &fg->nb_outputs));
  ofilter->ost = ost;
  ofilter->graph = fg;
  ofilter->format = -1;

  ost->filter = ofilter;

  ifilter = static_cast<InputFilter *>(
      allocate_array_elem(&fg->inputs, sizeof(*fg->inputs[0]), &fg->nb_inputs));
  ifilter->ist = ist;
  ifilter->graph = fg;
  ifilter->format = -1;

  ifilter->frame_queue =
      av_fifo_alloc2(8, sizeof(AVFrame *), AV_FIFO_FLAG_AUTO_GROW);
  if (!ifilter->frame_queue) {
    return -1;
  }

  ist->filters = static_cast<InputFilter **>(
      grow_array(ist->filters, sizeof(*ist->filters), &ist->nb_filters,
                 ist->nb_filters + 1));
  ist->filters[ist->nb_filters - 1] = ifilter;

  filtergraphs = static_cast<FilterGraph **>(
      grow_array(filtergraphs, sizeof(*filtergraphs), &nb_filtergraphs,
                 nb_filtergraphs + 1));
  filtergraphs[nb_filtergraphs - 1] = fg;

  return 0;
}

char *FfmpegVideoConverter::describe_filter_link(FilterGraph *fg,
                                                 AVFilterInOut *inout, int in) {
  AVFilterContext *ctx = inout->filter_ctx;
  AVFilterPad *pads = in ? ctx->input_pads : ctx->output_pads;
  int nb_pads = in ? ctx->nb_inputs : ctx->nb_outputs;
  char *res;

  if (nb_pads > 1)
    res = av_strdup(ctx->filter->name);
  else
    res = av_asprintf("%s:%s", ctx->filter->name,
                      avfilter_pad_get_name(pads, inout->pad_idx));
  if (!res) {
    return nullptr;
  }
  return res;
}

bool FfmpegVideoConverter::init_input_filter(FilterGraph *fg,
                                             AVFilterInOut *in) {
  InputStream *ist = nullptr;
  AVMediaType type =
      avfilter_pad_get_type(in->filter_ctx->input_pads, in->pad_idx);
  InputFilter *ifilter;
  int i;

  // TODO: support other filter types
  if (type != AVMEDIA_TYPE_VIDEO && type != AVMEDIA_TYPE_AUDIO) {
    AvLog(nullptr, AV_LOG_FATAL,
          "Only video and audio filters supported "
          "currently.\n");
    return false;
  }

  if (in->name) {
    AVFormatContext *s;
    AVStream *st = nullptr;
    char *p;
    int file_idx = strtol(in->name, &p, 0);

    if (file_idx < 0 || file_idx >= nb_input_files) {
      AvLog(nullptr, AV_LOG_FATAL,
            "Invalid file index %d in filtergraph description %s.\n", file_idx,
            fg->graph_desc);
      return false;
    }
    s = input_files[file_idx]->ctx;

    for (i = 0; i < s->nb_streams; i++) {
      enum AVMediaType stream_type = s->streams[i]->codecpar->codec_type;
      if (stream_type != type &&
          !(stream_type == AVMEDIA_TYPE_SUBTITLE &&
            type == AVMEDIA_TYPE_VIDEO /* sub2video hack */))
        continue;
      if (check_stream_specifier(s, s->streams[i], *p == ':' ? p + 1 : p) ==
          1) {
        st = s->streams[i];
        break;
      }
    }
    if (!st) {
      AvLog(nullptr, AV_LOG_FATAL,
            "Stream specifier '%s' in filtergraph description %s "
            "matches no streams.\n",
            p, fg->graph_desc);
      return false;
    }
    ist = input_streams[input_files[file_idx]->ist_index + st->index];
    if (ist->user_set_discard == AVDISCARD_ALL) {
      AvLog(nullptr, AV_LOG_FATAL,
            "Stream specifier '%s' in filtergraph description %s "
            "matches a disabled input stream.\n",
            p, fg->graph_desc);
      return false;
    }
  } else {
    /* find the first unused stream of corresponding type */
    for (i = 0; i < nb_input_streams; i++) {
      ist = input_streams[i];
      if (ist->user_set_discard == AVDISCARD_ALL) continue;
      if (ist->dec_ctx->codec_type == type && ist->discard) break;
    }
    if (i == nb_input_streams) {
      AvLog(nullptr, AV_LOG_FATAL,
            "Cannot find a matching stream for "
            "unlabeled input pad %d on filter %s\n",
            in->pad_idx, in->filter_ctx->name);
      return false;
    }
  }

  av_assert0(ist);
  if (!ist) {
    return false;
  }

  ist->discard = false;
  ist->decoding_needed |= DECODING_FOR_FILTER;
  ist->processing_needed = true;
  ist->st->discard = AVDISCARD_NONE;

  ifilter = static_cast<InputFilter *>(
      allocate_array_elem(&fg->inputs, sizeof(*fg->inputs[0]), &fg->nb_inputs));
  ifilter->ist = ist;
  ifilter->graph = fg;
  ifilter->format = -1;
  ifilter->type = ist->st->codecpar->codec_type;
  ifilter->name = reinterpret_cast<uint8_t *>(describe_filter_link(fg, in, 1));

  ifilter->frame_queue =
      av_fifo_alloc2(8, sizeof(AVFrame *), AV_FIFO_FLAG_AUTO_GROW);
  if (!ifilter->frame_queue) {
    return false;
  }

  ist->filters = static_cast<InputFilter **>(
      grow_array(ist->filters, sizeof(*ist->filters), &ist->nb_filters,
                 ist->nb_filters + 1));
  ist->filters[ist->nb_filters - 1] = ifilter;
  return true;
}

int FfmpegVideoConverter::init_complex_filtergraph(FilterGraph *fg) {
  AVFilterInOut *inputs, *outputs, *cur;
  AVFilterGraph *graph;
  int ret = 0;

  /* this graph is only used for determining the kinds of inputs
   * and outputs we have, and is discarded on exit from this function */
  graph = avfilter_graph_alloc();
  if (!graph) {
    return AVERROR(ENOMEM);
  }
  graph->nb_threads = 1;

  ret = avfilter_graph_parse2(graph, fg->graph_desc, &inputs, &outputs);
  if (ret < 0) goto fail;

  for (cur = inputs; cur; cur = cur->next) init_input_filter(fg, cur);

  for (cur = outputs; cur;) {
    OutputFilter *const ofilter =
        static_cast<OutputFilter *>(allocate_array_elem(
            &fg->outputs, sizeof(*fg->outputs[0]), &fg->nb_outputs));

    ofilter->graph = fg;
    ofilter->out_tmp = cur;
    ofilter->type =
        avfilter_pad_get_type(cur->filter_ctx->output_pads, cur->pad_idx);
    ofilter->name =
        reinterpret_cast<uint8_t *>(describe_filter_link(fg, cur, 0));
    cur = cur->next;
    ofilter->out_tmp->next = nullptr;
  }

fail:
  avfilter_inout_free(&inputs);
  avfilter_graph_free(&graph);
  return ret;
}

int FfmpegVideoConverter::insert_trim(int64_t start_time, int64_t duration,
                                      AVFilterContext **last_filter,
                                      int *pad_idx, const char *filter_name) {
  AVFilterGraph *graph = (*last_filter)->graph;
  AVFilterContext *ctx;
  const AVFilter *trim;
  enum AVMediaType type =
      avfilter_pad_get_type((*last_filter)->output_pads, *pad_idx);
  const char *name = (type == AVMEDIA_TYPE_VIDEO) ? "trim" : "atrim";
  int ret = 0;

  if (duration == INT64_MAX && start_time == AV_NOPTS_VALUE) return 0;

  trim = avfilter_get_by_name(name);
  if (!trim) {
    AvLog(nullptr, AV_LOG_ERROR,
          "%s filter not present, cannot limit "
          "recording time.\n",
          name);
    return AVERROR_FILTER_NOT_FOUND;
  }

  ctx = avfilter_graph_alloc_filter(graph, trim, filter_name);
  if (!ctx) return AVERROR(ENOMEM);

  if (duration != INT64_MAX) {
    ret = av_opt_set_int(ctx, "durationi", duration, AV_OPT_SEARCH_CHILDREN);
  }
  if (ret >= 0 && start_time != AV_NOPTS_VALUE) {
    ret = av_opt_set_int(ctx, "starti", start_time, AV_OPT_SEARCH_CHILDREN);
  }
  if (ret < 0) {
    AvLog(ctx, AV_LOG_ERROR, "Error configuring the %s filter", name);
    return ret;
  }

  ret = avfilter_init_str(ctx, nullptr);
  if (ret < 0) return ret;

  ret = avfilter_link(*last_filter, *pad_idx, ctx, 0);
  if (ret < 0) return ret;

  *last_filter = ctx;
  *pad_idx = 0;
  return 0;
}

int FfmpegVideoConverter::insert_filter(AVFilterContext **last_filter,
                                        int *pad_idx, const char *filter_name,
                                        const char *args) {
  AVFilterGraph *graph = (*last_filter)->graph;
  AVFilterContext *ctx;
  int ret;

  ret = avfilter_graph_create_filter(&ctx, avfilter_get_by_name(filter_name),
                                     filter_name, args, nullptr, graph);
  if (ret < 0) return ret;

  ret = avfilter_link(*last_filter, *pad_idx, ctx, 0);
  if (ret < 0) return ret;

  *last_filter = ctx;
  *pad_idx = 0;
  return 0;
}

int FfmpegVideoConverter::configure_output_video_filter(FilterGraph *fg,
                                                        OutputFilter *ofilter,
                                                        AVFilterInOut *out) {
  OutputStream *ost = ofilter->ost;
  OutputFile *of = output_files[ost->file_index];
  AVFilterContext *last_filter = out->filter_ctx;
  AVBPrint bprint;
  int pad_idx = out->pad_idx;
  int ret;
  const char *pix_fmts;
  char name[255];

  snprintf(name, sizeof(name), "out_%d_%d", ost->file_index, ost->index);
  ret = avfilter_graph_create_filter(&ofilter->filter,
                                     avfilter_get_by_name("buffersink"), name,
                                     nullptr, nullptr, fg->graph);

  if (ret < 0) return ret;

  if ((ofilter->width || ofilter->height) && ofilter->ost->autoscale) {
    char args[255];
    AVFilterContext *filter;
    const AVDictionaryEntry *e = nullptr;

    snprintf(args, sizeof(args), "%d:%d", ofilter->width, ofilter->height);

    while ((e = av_dict_get(ost->sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
      av_strlcatf(args, sizeof(args), ":%s=%s", e->key, e->value);
    }

    snprintf(name, sizeof(name), "scaler_out_%d_%d", ost->file_index,
             ost->index);
    if ((ret = avfilter_graph_create_filter(&filter,
                                            avfilter_get_by_name("scale"), name,
                                            args, nullptr, fg->graph)) < 0)
      return ret;
    if ((ret = avfilter_link(last_filter, pad_idx, filter, 0)) < 0) return ret;

    last_filter = filter;
    pad_idx = 0;
  }

  av_bprint_init(&bprint, 0, AV_BPRINT_SIZE_UNLIMITED);
  if ((pix_fmts = choose_pix_fmts(ofilter, &bprint))) {
    AVFilterContext *filter;

    ret = avfilter_graph_create_filter(&filter, avfilter_get_by_name("format"),
                                       "format", pix_fmts, nullptr, fg->graph);
    av_bprint_finalize(&bprint, nullptr);
    if (ret < 0) return ret;
    if ((ret = avfilter_link(last_filter, pad_idx, filter, 0)) < 0) return ret;

    last_filter = filter;
    pad_idx = 0;
  }

  if (ost->frame_rate.num && 0) {
    AVFilterContext *fps;
    char args[255];

    snprintf(args, sizeof(args), "fps=%d/%d", ost->frame_rate.num,
             ost->frame_rate.den);
    snprintf(name, sizeof(name), "fps_out_%d_%d", ost->file_index, ost->index);
    ret = avfilter_graph_create_filter(&fps, avfilter_get_by_name("fps"), name,
                                       args, nullptr, fg->graph);
    if (ret < 0) return ret;

    ret = avfilter_link(last_filter, pad_idx, fps, 0);
    if (ret < 0) return ret;
    last_filter = fps;
    pad_idx = 0;
  }

  snprintf(name, sizeof(name), "trim_out_%d_%d", ost->file_index, ost->index);
  ret = insert_trim(of->start_time, of->recording_time, &last_filter, &pad_idx,
                    name);
  if (ret < 0) return ret;

  if ((ret = avfilter_link(last_filter, pad_idx, ofilter->filter, 0)) < 0)
    return ret;

  return 0;
}

int FfmpegVideoConverter::configure_output_audio_filter(FilterGraph *fg,
                                                        OutputFilter *ofilter,
                                                        AVFilterInOut *out) {
  OutputStream *ost = ofilter->ost;
  OutputFile *of = output_files[ost->file_index];
  AVCodecContext *codec = ost->enc_ctx;
  AVFilterContext *last_filter = out->filter_ctx;
  int pad_idx = out->pad_idx;
  AVBPrint args;
  char name[255];
  int ret;

  snprintf(name, sizeof(name), "out_%d_%d", ost->file_index, ost->index);
  ret = avfilter_graph_create_filter(&ofilter->filter,
                                     avfilter_get_by_name("abuffersink"), name,
                                     nullptr, nullptr, fg->graph);
  if (ret < 0) return ret;
  if ((ret = av_opt_set_int(ofilter->filter, "all_channel_counts", 1,
                            AV_OPT_SEARCH_CHILDREN)) < 0)
    return ret;

#define AUTO_INSERT_FILTER(opt_name, filter_name, arg)                        \
  do {                                                                        \
    AVFilterContext *filt_ctx;                                                \
                                                                              \
    AvLog(nullptr, AV_LOG_INFO,                                               \
          opt_name                                                            \
          " is forwarded to lavfi "                                           \
          "similarly to -af " filter_name "=%s.\n",                           \
          arg);                                                               \
                                                                              \
    ret = avfilter_graph_create_filter(&filt_ctx,                             \
                                       avfilter_get_by_name(filter_name),     \
                                       filter_name, arg, nullptr, fg->graph); \
    if (ret < 0) goto fail;                                                   \
                                                                              \
    ret = avfilter_link(last_filter, pad_idx, filt_ctx, 0);                   \
    if (ret < 0) goto fail;                                                   \
                                                                              \
    last_filter = filt_ctx;                                                   \
    pad_idx = 0;                                                              \
  } while (0)
  av_bprint_init(&args, 0, AV_BPRINT_SIZE_UNLIMITED);
  if (ost->audio_channels_mapped) {
    AVChannelLayout mapped_layout = {};
    int i;
    av_channel_layout_default(&mapped_layout, ost->audio_channels_mapped);
    av_channel_layout_describe_bprint(&mapped_layout, &args);
    for (i = 0; i < ost->audio_channels_mapped; i++)
      if (ost->audio_channels_map[i] != -1)
        av_bprintf(&args, "|c%d=c%d", i, ost->audio_channels_map[i]);

    AUTO_INSERT_FILTER("-map_channel", "pan", args.str);
    av_bprint_clear(&args);
  }

  if (codec->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
    av_channel_layout_default(&codec->ch_layout, codec->ch_layout.nb_channels);

  choose_sample_fmts(ofilter, &args);
  choose_sample_rates(ofilter, &args);
  choose_channel_layouts(ofilter, &args);
  if (!av_bprint_is_complete(&args)) {
    ret = AVERROR(ENOMEM);
    goto fail;
  }
  if (args.len) {
    AVFilterContext *format;

    snprintf(name, sizeof(name), "format_out_%d_%d", ost->file_index,
             ost->index);
    ret = avfilter_graph_create_filter(&format, avfilter_get_by_name("aformat"),
                                       name, args.str, nullptr, fg->graph);
    if (ret < 0) goto fail;

    ret = avfilter_link(last_filter, pad_idx, format, 0);
    if (ret < 0) goto fail;

    last_filter = format;
    pad_idx = 0;
  }

  if (ost->apad && of->shortest) {
    int i;

    for (i = 0; i < of->ctx->nb_streams; i++)
      if (of->ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        break;

    if (i < of->ctx->nb_streams) {
      AUTO_INSERT_FILTER("-apad", "apad", ost->apad);
    }
  }

  snprintf(name, sizeof(name), "trim for output stream %d:%d", ost->file_index,
           ost->index);
  ret = insert_trim(of->start_time, of->recording_time, &last_filter, &pad_idx,
                    name);
  if (ret < 0) goto fail;

  if ((ret = avfilter_link(last_filter, pad_idx, ofilter->filter, 0)) < 0)
    goto fail;
fail:
  av_bprint_finalize(&args, nullptr);

  return ret;
}

int FfmpegVideoConverter::configure_output_filter(FilterGraph *fg,
                                                  OutputFilter *ofilter,
                                                  AVFilterInOut *out) {
  if (!ofilter->ost) {
    AvLog(nullptr, AV_LOG_FATAL, "Filter %s has an unconnected output\n",
          ofilter->name);
    return false;
  }

  switch (avfilter_pad_get_type(out->filter_ctx->output_pads, out->pad_idx)) {
    case AVMEDIA_TYPE_VIDEO:
      return configure_output_video_filter(fg, ofilter, out);
    case AVMEDIA_TYPE_AUDIO:
      return configure_output_audio_filter(fg, ofilter, out);
    default:
      av_assert0(0);
      return 0;
  }
}

bool FfmpegVideoConverter::check_filter_outputs(void) {
  for (int i = 0; i < nb_filtergraphs; i++) {
    for (int n = 0; n < filtergraphs[i]->nb_outputs; n++) {
      OutputFilter *output = filtergraphs[i]->outputs[n];
      if (!output->ost) {
        AvLog(nullptr, AV_LOG_FATAL, "Filter %s has an unconnected output\n",
              output->name);
        return false;
      }
    }
  }
  return true;
}

int FfmpegVideoConverter::sub2video_prepare(InputStream *ist,
                                            InputFilter *ifilter) {
  AVFormatContext *avf = input_files[ist->file_index]->ctx;

  /* Compute the size of the canvas for the subtitles stream.
     If the subtitles codecpar has set a size, use it. Otherwise use the
     maximum dimensions of the video streams in the same file. */
  int w = ifilter->width;
  int h = ifilter->height;
  if (!(w && h)) {
    for (int i = 0; i < avf->nb_streams; i++) {
      if (avf->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        w = FFMAX(w, avf->streams[i]->codecpar->width);
        h = FFMAX(h, avf->streams[i]->codecpar->height);
      }
    }
    if (!(w && h)) {
      w = FFMAX(w, 720);
      h = FFMAX(h, 576);
    }
    AvLog(avf, AV_LOG_INFO, "sub2video: using %dx%d canvas\n", w, h);
  }
  ist->sub2video.w = ifilter->width = w;
  ist->sub2video.h = ifilter->height = h;

  ifilter->width = ist->dec_ctx->width ? ist->dec_ctx->width : ist->sub2video.w;
  ifilter->height =
      ist->dec_ctx->height ? ist->dec_ctx->height : ist->sub2video.h;

  /* rectangles are AV_PIX_FMT_PAL8, but we have no guarantee that the
     palettes for all rectangles are identical or compatible */
  ifilter->format = AV_PIX_FMT_RGB32;

  ist->sub2video.frame = av_frame_alloc();
  if (!ist->sub2video.frame) return AVERROR(ENOMEM);
  ist->sub2video.last_pts = INT64_MIN;
  ist->sub2video.end_pts = INT64_MIN;

  /* sub2video structure has been (re-)initialized.
     Mark it as such so that the system will be
     initialized with the first received heartbeat. */
  ist->sub2video.initialize = true;

  return 0;
}

int FfmpegVideoConverter::configure_input_video_filter(FilterGraph *fg,
                                                       InputFilter *ifilter,
                                                       AVFilterInOut *in) {
  AVFilterContext *last_filter;
  const AVFilter *buffer_filt = avfilter_get_by_name("buffer");
  const AVPixFmtDescriptor *desc;
  InputStream *ist = ifilter->ist;
  InputFile *f = input_files[ist->file_index];
  AVRational tb =
      ist->framerate.num ? av_inv_q(ist->framerate) : ist->st->time_base;
  AVRational fr = ist->framerate;
  AVRational sar;
  AVBPrint args;
  char name[255];
  int ret, pad_idx = 0;
  int64_t tsoffset = 0;
  AVBufferSrcParameters *par = av_buffersrc_parameters_alloc();

  if (!par) return AVERROR(ENOMEM);
  memset(par, 0, sizeof(*par));
  par->format = AV_PIX_FMT_NONE;

  if (ist->dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Cannot connect video filter to audio input\n");
    ret = AVERROR(EINVAL);
    goto fail;
  }

  if (!fr.num)
    fr = av_guess_frame_rate(input_files[ist->file_index]->ctx, ist->st,
                             nullptr);

  if (ist->dec_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
    ret = sub2video_prepare(ist, ifilter);
    if (ret < 0) goto fail;
  }

  sar = ifilter->sample_aspect_ratio;
  if (!sar.den) sar = {0, 1};
  av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
  av_bprintf(&args,
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:"
             "pixel_aspect=%d/%d",
             ifilter->width, ifilter->height, ifilter->format, tb.num, tb.den,
             sar.num, sar.den);
  if (fr.num && fr.den) av_bprintf(&args, ":frame_rate=%d/%d", fr.num, fr.den);
  snprintf(name, sizeof(name), "graph %d input from stream %d:%d", fg->index,
           ist->file_index, ist->st->index);

  if ((ret = avfilter_graph_create_filter(&ifilter->filter, buffer_filt, name,
                                          args.str, nullptr, fg->graph)) < 0)
    goto fail;
  par->hw_frames_ctx = ifilter->hw_frames_ctx;
  ret = av_buffersrc_parameters_set(ifilter->filter, par);
  if (ret < 0) goto fail;
  av_freep(&par);
  last_filter = ifilter->filter;

  desc = av_pix_fmt_desc_get(static_cast<AVPixelFormat>(ifilter->format));
  av_assert0(desc);

  // TODO: insert hwaccel enabled filters like transpose_vaapi into the graph
  if (ist->autorotate && !(desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) {
    int32_t *displaymatrix = ifilter->displaymatrix;
    double theta;

    if (!displaymatrix)
      displaymatrix = (int32_t *)av_stream_get_side_data(
          ist->st, AV_PKT_DATA_DISPLAYMATRIX, nullptr);
    theta = get_rotation(displaymatrix);

    if (fabs(theta - 90) < 1.0) {
      ret = insert_filter(&last_filter, &pad_idx, "transpose",
                          displaymatrix[3] > 0 ? "cclock_flip" : "clock");
    } else if (fabs(theta - 180) < 1.0) {
      if (displaymatrix[0] < 0) {
        ret = insert_filter(&last_filter, &pad_idx, "hflip", nullptr);
        if (ret < 0) return ret;
      }
      if (displaymatrix[4] < 0) {
        ret = insert_filter(&last_filter, &pad_idx, "vflip", nullptr);
      }
    } else if (fabs(theta - 270) < 1.0) {
      ret = insert_filter(&last_filter, &pad_idx, "transpose",
                          displaymatrix[3] < 0 ? "clock_flip" : "cclock");
    } else if (fabs(theta) > 1.0) {
      char rotate_buf[64];
      snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
      ret = insert_filter(&last_filter, &pad_idx, "rotate", rotate_buf);
    } else if (fabs(theta) < 1.0) {
      if (displaymatrix && displaymatrix[4] < 0) {
        ret = insert_filter(&last_filter, &pad_idx, "vflip", nullptr);
      }
    }
    if (ret < 0) return ret;
  }

  snprintf(name, sizeof(name), "trim_in_%d_%d", ist->file_index,
           ist->st->index);
  if (copy_ts) {
    tsoffset = f->start_time == AV_NOPTS_VALUE ? 0 : f->start_time;
    if (!start_at_zero && f->ctx->start_time != AV_NOPTS_VALUE)
      tsoffset += f->ctx->start_time;
  }
  ret = insert_trim(((f->start_time == AV_NOPTS_VALUE) || !f->accurate_seek)
                        ? AV_NOPTS_VALUE
                        : tsoffset,
                    f->recording_time, &last_filter, &pad_idx, name);
  if (ret < 0) return ret;

  if ((ret = avfilter_link(last_filter, 0, in->filter_ctx, in->pad_idx)) < 0)
    return ret;
  return 0;
fail:
  av_freep(&par);

  return ret;
}

int FfmpegVideoConverter::configure_input_audio_filter(FilterGraph *fg,
                                                       InputFilter *ifilter,
                                                       AVFilterInOut *in) {
  AVFilterContext *last_filter;
  const AVFilter *abuffer_filt = avfilter_get_by_name("abuffer");
  InputStream *ist = ifilter->ist;
  InputFile *f = input_files[ist->file_index];
  AVBPrint args;
  char name[255];
  int ret, pad_idx = 0;
  int64_t tsoffset = 0;

  if (ist->dec_ctx->codec_type != AVMEDIA_TYPE_AUDIO) {
    AvLog(nullptr, AV_LOG_ERROR,
          "Cannot connect audio filter to non audio input\n");
    return AVERROR(EINVAL);
  }

  av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
  av_bprintf(
      &args, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s", 1,
      ifilter->sample_rate, ifilter->sample_rate,
      av_get_sample_fmt_name(static_cast<AVSampleFormat>(ifilter->format)));
  if (av_channel_layout_check(&ifilter->ch_layout) &&
      ifilter->ch_layout.order != AV_CHANNEL_ORDER_UNSPEC) {
    av_bprintf(&args, ":channel_layout=");
    av_channel_layout_describe_bprint(&ifilter->ch_layout, &args);
  } else
    av_bprintf(&args, ":channels=%d", ifilter->ch_layout.nb_channels);
  snprintf(name, sizeof(name), "graph_%d_in_%d_%d", fg->index, ist->file_index,
           ist->st->index);

  if ((ret = avfilter_graph_create_filter(&ifilter->filter, abuffer_filt, name,
                                          args.str, nullptr, fg->graph)) < 0)
    return ret;
  last_filter = ifilter->filter;

#define AUTO_INSERT_FILTER_INPUT(opt_name, filter_name, arg)              \
  do {                                                                    \
    AVFilterContext *filt_ctx;                                            \
                                                                          \
    AvLog(nullptr, AV_LOG_INFO,                                           \
          opt_name                                                        \
          " is forwarded to lavfi "                                       \
          "similarly to -af " filter_name "=%s.\n",                       \
          arg);                                                           \
                                                                          \
    snprintf(name, sizeof(name), "graph_%d_%s_in_%d_%d", fg->index,       \
             filter_name, ist->file_index, ist->st->index);               \
    ret = avfilter_graph_create_filter(&filt_ctx,                         \
                                       avfilter_get_by_name(filter_name), \
                                       name, arg, nullptr, fg->graph);    \
    if (ret < 0) return ret;                                              \
                                                                          \
    ret = avfilter_link(last_filter, 0, filt_ctx, 0);                     \
    if (ret < 0) return ret;                                              \
                                                                          \
    last_filter = filt_ctx;                                               \
  } while (0)

  if (audio_sync_method > 0) {
    char args[256] = {0};

    av_strlcatf(args, sizeof(args), "async=%d", audio_sync_method);
    if (audio_drift_threshold != 0.1)
      av_strlcatf(args, sizeof(args), ":min_hard_comp=%f",
                  audio_drift_threshold);
    if (!fg->reconfiguration) av_strlcatf(args, sizeof(args), ":first_pts=0");
    AUTO_INSERT_FILTER_INPUT("-async", "aresample", args);
  }

  //     if (ost->audio_channels_mapped) {
  //         int i;
  //         AVBPrint pan_buf;
  //         av_bprint_init(&pan_buf, 256, 8192);
  //         av_bprintf(&pan_buf, "0x%"PRIx64,
  //                    av_get_default_channel_layout(ost->audio_channels_mapped));
  //         for (i = 0; i < ost->audio_channels_mapped; i++)
  //             if (ost->audio_channels_map[i] != -1)
  //                 av_bprintf(&pan_buf, ":c%d=c%d", i,
  //                 ost->audio_channels_map[i]);
  //         AUTO_INSERT_FILTER_INPUT("-map_channel", "pan", pan_buf.str);
  //         av_bprint_finalize(&pan_buf, nullptr);
  //     }

  if (audio_volume != 256) {
    char args[256];

    AvLog(nullptr, AV_LOG_WARNING,
          "-vol has been deprecated. Use the volume "
          "audio filter instead.\n");

    snprintf(args, sizeof(args), "%f", audio_volume / 256.);
    AUTO_INSERT_FILTER_INPUT("-vol", "volume", args);
  }

  snprintf(name, sizeof(name), "trim for input stream %d:%d", ist->file_index,
           ist->st->index);
  if (copy_ts) {
    tsoffset = f->start_time == AV_NOPTS_VALUE ? 0 : f->start_time;
    if (!start_at_zero && f->ctx->start_time != AV_NOPTS_VALUE)
      tsoffset += f->ctx->start_time;
  }
  ret = insert_trim(((f->start_time == AV_NOPTS_VALUE) || !f->accurate_seek)
                        ? AV_NOPTS_VALUE
                        : tsoffset,
                    f->recording_time, &last_filter, &pad_idx, name);
  if (ret < 0) {
    return ret;
  }

  if ((ret = avfilter_link(last_filter, 0, in->filter_ctx, in->pad_idx)) < 0) {
    return ret;
  }

  return 0;
}

int FfmpegVideoConverter::configure_input_filter(FilterGraph *fg,
                                                 InputFilter *ifilter,
                                                 AVFilterInOut *in) {
  if (!ifilter->ist->dec) {
    AvLog(nullptr, AV_LOG_ERROR,
          "No decoder for stream #%d:%d, filtering impossible\n",
          ifilter->ist->file_index, ifilter->ist->st->index);
    return AVERROR_DECODER_NOT_FOUND;
  }
  switch (avfilter_pad_get_type(in->filter_ctx->input_pads, in->pad_idx)) {
    case AVMEDIA_TYPE_VIDEO:
      return configure_input_video_filter(fg, ifilter, in);
    case AVMEDIA_TYPE_AUDIO:
      return configure_input_audio_filter(fg, ifilter, in);
    default:
      av_assert0(0);
      return 0;
  }
}

void FfmpegVideoConverter::cleanup_filtergraph(FilterGraph *fg) {
  for (int i = 0; i < fg->nb_outputs; i++)
    fg->outputs[i]->filter = (AVFilterContext *)nullptr;
  for (int i = 0; i < fg->nb_inputs; i++)
    fg->inputs[i]->filter = (AVFilterContext *)nullptr;
  avfilter_graph_free(&fg->graph);
}

bool FfmpegVideoConverter::filter_is_buffersrc(const AVFilterContext *f) {
  return f->nb_inputs == 0 && (!strcmp(f->filter->name, "buffer") ||
                               !strcmp(f->filter->name, "abuffer"));
}

bool FfmpegVideoConverter::graph_is_meta(AVFilterGraph *graph) {
  for (unsigned i = 0; i < graph->nb_filters; i++) {
    const AVFilterContext *f = graph->filters[i];

    /* in addition to filters flagged as meta, also
     * disregard sinks and buffersources (but not other sources,
     * since they introduce data we are not aware of)
     */
    if (!((f->filter->flags & AVFILTER_FLAG_METADATA_ONLY) ||
          f->nb_outputs == 0 || filter_is_buffersrc(f)))
      return false;
  }
  return true;
}

int FfmpegVideoConverter::configure_filtergraph(FilterGraph *fg) {
  AVFilterInOut *inputs, *outputs, *cur;
  int ret, i;
  bool simple = filtergraph_is_simple(fg);
  const char *graph_desc =
      simple ? fg->outputs[0]->ost->avfilter : fg->graph_desc;

  cleanup_filtergraph(fg);
  if (!(fg->graph = avfilter_graph_alloc())) return AVERROR(ENOMEM);

  if (simple) {
    OutputStream *ost = fg->outputs[0]->ost;
    char args[512];
    const AVDictionaryEntry *e = nullptr;

    if (filter_nbthreads) {
      ret = av_opt_set(fg->graph, "threads", filter_nbthreads, 0);
      if (ret < 0) goto fail;
    } else {
      e = av_dict_get(ost->encoder_opts, "threads", nullptr, 0);
      if (e) av_opt_set(fg->graph, "threads", e->value, 0);
    }

    args[0] = 0;
    e = nullptr;
    while ((e = av_dict_get(ost->sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
      av_strlcatf(args, sizeof(args), "%s=%s:", e->key, e->value);
    }
    if (strlen(args)) {
      args[strlen(args) - 1] = 0;
      fg->graph->scale_sws_opts = av_strdup(args);
    }

    args[0] = 0;
    e = nullptr;
    while ((e = av_dict_get(ost->swr_opts, "", e, AV_DICT_IGNORE_SUFFIX))) {
      av_strlcatf(args, sizeof(args), "%s=%s:", e->key, e->value);
    }
    if (strlen(args)) args[strlen(args) - 1] = 0;
    av_opt_set(fg->graph, "aresample_swr_opts", args, 0);
  } else {
    fg->graph->nb_threads = filter_complex_nbthreads;
  }

  if ((ret = avfilter_graph_parse2(fg->graph, graph_desc, &inputs, &outputs)) <
      0)
    goto fail;

  ret = hw_device_setup_for_filter(fg);
  if (ret < 0) goto fail;

  if (simple && (!inputs || inputs->next || !outputs || outputs->next)) {
    const char *num_inputs;
    const char *num_outputs;
    if (!outputs) {
      num_outputs = "0";
    } else if (outputs->next) {
      num_outputs = ">1";
    } else {
      num_outputs = "1";
    }
    if (!inputs) {
      num_inputs = "0";
    } else if (inputs->next) {
      num_inputs = ">1";
    } else {
      num_inputs = "1";
    }
    AvLog(nullptr, AV_LOG_ERROR,
          "Simple filtergraph '%s' was expected "
          "to have exactly 1 input and 1 output."
          " However, it had %s input(s) and %s output(s)."
          " Please adjust, or use a complex filtergraph (-filter_complex) "
          "instead.\n",
          graph_desc, num_inputs, num_outputs);
    ret = AVERROR(EINVAL);
    goto fail;
  }

  for (cur = inputs, i = 0; cur; cur = cur->next, i++)
    if ((ret = configure_input_filter(fg, fg->inputs[i], cur)) < 0) {
      avfilter_inout_free(&inputs);
      avfilter_inout_free(&outputs);
      goto fail;
    }
  avfilter_inout_free(&inputs);

  for (cur = outputs, i = 0; cur; cur = cur->next, i++)
    configure_output_filter(fg, fg->outputs[i], cur);
  avfilter_inout_free(&outputs);

  if (!auto_conversion_filters)
    avfilter_graph_set_auto_convert(fg->graph, AVFILTER_AUTO_CONVERT_NONE);
  if ((ret = avfilter_graph_config(fg->graph, nullptr)) < 0) goto fail;

  fg->is_meta = graph_is_meta(fg->graph);

  /* limit the lists of allowed formats to the ones selected, to
   * make sure they stay the same if the filtergraph is reconfigured later */
  for (i = 0; i < fg->nb_outputs; i++) {
    OutputFilter *ofilter = fg->outputs[i];
    AVFilterContext *sink = ofilter->filter;

    ofilter->format = av_buffersink_get_format(sink);

    ofilter->width = av_buffersink_get_w(sink);
    ofilter->height = av_buffersink_get_h(sink);

    ofilter->sample_rate = av_buffersink_get_sample_rate(sink);
    av_channel_layout_uninit(&ofilter->ch_layout);
    ret = av_buffersink_get_ch_layout(sink, &ofilter->ch_layout);
    if (ret < 0) goto fail;
  }

  fg->reconfiguration = 1;

  for (i = 0; i < fg->nb_outputs; i++) {
    OutputStream *ost = fg->outputs[i]->ost;
    if (!ost->enc) {
      /* identical to the same check in ffmpeg.c, needed because
         complex filter graphs are initialized earlier */
      AvLog(nullptr, AV_LOG_ERROR,
            "Encoder (codec %s) not found for output stream #%d:%d\n",
            avcodec_get_name(ost->st->codecpar->codec_id), ost->file_index,
            ost->index);
      ret = AVERROR(EINVAL);
      goto fail;
    }
    if (ost->enc->type == AVMEDIA_TYPE_AUDIO &&
        !(ost->enc->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE))
      av_buffersink_set_frame_size(ost->filter->filter,
                                   ost->enc_ctx->frame_size);
  }

  for (i = 0; i < fg->nb_inputs; i++) {
    AVFrame *tmp;
    while (av_fifo_read(fg->inputs[i]->frame_queue, &tmp, 1) >= 0) {
      ret = av_buffersrc_add_frame(fg->inputs[i]->filter, tmp);
      av_frame_free(&tmp);
      if (ret < 0) goto fail;
    }
  }

  /* send the EOFs for the finished inputs */
  for (i = 0; i < fg->nb_inputs; i++) {
    if (fg->inputs[i]->eof) {
      ret = av_buffersrc_add_frame(fg->inputs[i]->filter, nullptr);
      if (ret < 0) goto fail;
    }
  }

  /* process queued up subtitle packets */
  for (i = 0; i < fg->nb_inputs; i++) {
    InputStream *ist = fg->inputs[i]->ist;
    if (ist->sub2video.sub_queue && ist->sub2video.frame) {
      AVSubtitle tmp;
      while (av_fifo_read(ist->sub2video.sub_queue, &tmp, 1) >= 0) {
        sub2video_update(ist, INT64_MIN, &tmp);
        avsubtitle_free(&tmp);
      }
    }
  }

  return 0;

fail:
  cleanup_filtergraph(fg);
  return ret;
}

int FfmpegVideoConverter::ifilter_parameters_from_frame(InputFilter *ifilter,
                                                        const AVFrame *frame) {
  av_buffer_unref(&ifilter->hw_frames_ctx);

  ifilter->format = frame->format;

  ifilter->width = frame->width;
  ifilter->height = frame->height;
  ifilter->sample_aspect_ratio = frame->sample_aspect_ratio;

  ifilter->sample_rate = frame->sample_rate;
  int ret = av_channel_layout_copy(&ifilter->ch_layout, &frame->ch_layout);
  if (ret < 0) {
    return ret;
  }

  av_freep(&ifilter->displaymatrix);
  AVFrameSideData *sd =
      av_frame_get_side_data(frame, AV_FRAME_DATA_DISPLAYMATRIX);
  if (sd) {
    ifilter->displaymatrix =
        static_cast<int32_t *>(av_memdup(sd->data, sizeof(int32_t) * 9));
  }

  if (frame->hw_frames_ctx) {
    ifilter->hw_frames_ctx = av_buffer_ref(frame->hw_frames_ctx);
    if (!ifilter->hw_frames_ctx) {
      return AVERROR(ENOMEM);
    }
  }

  return 0;
}

bool FfmpegVideoConverter::filtergraph_is_simple(FilterGraph *fg) {
  return !fg->graph_desc;
}

void FfmpegVideoConverter::print_report(bool is_last_report,
                                        int64_t timer_start, int64_t cur_time) {
  AVBPrint buf, buf_script;
  OutputStream *ost;
  AVFormatContext *oc;
  int64_t total_size;
  AVCodecContext *enc;
  int vid, i;
  double bitrate;
  double speed;
  int64_t pts = INT64_MIN + 1;
  static int64_t last_time = -1;
  static int first_report = 1;
  static int qp_histogram[52];
  int hours, mins, secs, us;
  const char *hours_sign;
  int ret;
  float t;

  if (!print_stats && !is_last_report && !progress_avio) {
    return;
  }

  if (!is_last_report) {
    if (last_time == -1) {
      last_time = cur_time;
    }
    if (((cur_time - last_time) < stats_period && !first_report) ||
        (first_report && nb_output_dumped < nb_output_files))
      return;
    last_time = cur_time;
  }

  t = (cur_time - timer_start) / 1000000.0;

  oc = output_files[0]->ctx;

  total_size = avio_size(oc->pb);
  if (total_size <=
      0)  // FIXME improve avio_size() so it works with non seekable output too
    total_size = avio_tell(oc->pb);

  vid = 0;
  av_bprint_init(&buf, 0, AV_BPRINT_SIZE_AUTOMATIC);
  av_bprint_init(&buf_script, 0, AV_BPRINT_SIZE_AUTOMATIC);
  for (i = 0; i < nb_output_streams; i++) {
    float q = -1;
    ost = output_streams[i];
    enc = ost->enc_ctx;
    if (!ost->stream_copy) q = ost->quality / (float)FF_QP2LAMBDA;

    if (vid && enc->codec_type == AVMEDIA_TYPE_VIDEO) {
      av_bprintf(&buf, "q=%2.1f ", q);
      av_bprintf(&buf_script, "stream_%d_%d_q=%.1f\n", ost->file_index,
                 ost->index, q);
    }
    if (!vid && enc->codec_type == AVMEDIA_TYPE_VIDEO) {
      float fps;
      int64_t frame_number = ost->frame_number;

      fps = t > 1 ? frame_number / t : 0;
      av_bprintf(&buf, "frame=%5" PRId64 " fps=%3.*f q=%3.1f ", frame_number,
                 fps < 9.95, fps, q);
      av_bprintf(&buf_script, "frame=%" PRId64 "\n", frame_number);
      av_bprintf(&buf_script, "fps=%.2f\n", fps);
      av_bprintf(&buf_script, "stream_%d_%d_q=%.1f\n", ost->file_index,
                 ost->index, q);
      if (is_last_report) av_bprintf(&buf, "L");
      if (qp_hist) {
        int j;
        int qp = lrintf(q);
        if (qp >= 0 && qp < FF_ARRAY_ELEMS(qp_histogram)) qp_histogram[qp]++;
        for (j = 0; j < 32; j++)
          av_bprintf(&buf, "%X", av_log2(qp_histogram[j] + 1));
      }

      if ((enc->flags & AV_CODEC_FLAG_PSNR) &&
          (ost->pict_type != AV_PICTURE_TYPE_NONE || is_last_report)) {
        int j;
        double error, error_sum = 0;
        double scale, scale_sum = 0;
        double p;
        char type[3] = {'Y', 'U', 'V'};
        av_bprintf(&buf, "PSNR=");
        for (j = 0; j < 3; j++) {
          if (is_last_report) {
            error = enc->error[j];
            scale = enc->width * enc->height * 255.0 * 255.0 * frame_number;
          } else {
            error = ost->error[j];
            scale = enc->width * enc->height * 255.0 * 255.0;
          }
          if (j) scale /= 4;
          error_sum += error;
          scale_sum += scale;
          p = psnr(error / scale);
          av_bprintf(&buf, "%c:%2.2f ", type[j], p);
          av_bprintf(&buf_script, "stream_%d_%d_psnr_%c=%2.2f\n",
                     ost->file_index, ost->index, type[j] | 32, p);
        }
        p = psnr(error_sum / scale_sum);
        av_bprintf(&buf, "*:%2.2f ", psnr(error_sum / scale_sum));
        av_bprintf(&buf_script, "stream_%d_%d_psnr_all=%2.2f\n",
                   ost->file_index, ost->index, p);
      }
      vid = 1;
    }
    static int64_t copy_ts_first_pts = AV_NOPTS_VALUE;
    /* compute min output value */
    if (av_stream_get_end_pts(ost->st) != AV_NOPTS_VALUE) {
      pts = FFMAX(pts, av_rescale_q(av_stream_get_end_pts(ost->st),
                                    ost->st->time_base, {1, AV_TIME_BASE}));
      if (copy_ts) {
        if (copy_ts_first_pts == AV_NOPTS_VALUE && pts > 1)
          copy_ts_first_pts = pts;
        if (copy_ts_first_pts != AV_NOPTS_VALUE) pts -= copy_ts_first_pts;
      }
    }

    if (is_last_report) nb_frames_drop += ost->last_dropped;
  }

  secs = FFABS(pts) / AV_TIME_BASE;
  us = FFABS(pts) % AV_TIME_BASE;
  mins = secs / 60;
  secs %= 60;
  hours = mins / 60;
  mins %= 60;
  hours_sign = (pts < 0) ? "-" : "";

  bitrate = pts && total_size >= 0 ? total_size * 8 / (pts / 1000.0) : -1;
  speed = t != 0.0 ? (double)pts / AV_TIME_BASE / t : -1;

  if (total_size < 0)
    av_bprintf(&buf, "size=N/A time=");
  else
    av_bprintf(&buf, "size=%8.0fkB time=", total_size / 1024.0);
  if (pts == AV_NOPTS_VALUE) {
    av_bprintf(&buf, "N/A ");
  } else {
    av_bprintf(&buf, "%s%02d:%02d:%02d.%02d ", hours_sign, hours, mins, secs,
               (100 * us) / AV_TIME_BASE);
  }

  if (bitrate < 0) {
    av_bprintf(&buf, "bitrate=N/A");
    av_bprintf(&buf_script, "bitrate=N/A\n");
  } else {
    av_bprintf(&buf, "bitrate=%6.1fkbits/s", bitrate);
    av_bprintf(&buf_script, "bitrate=%6.1fkbits/s\n", bitrate);
  }

  if (total_size < 0)
    av_bprintf(&buf_script, "total_size=N/A\n");
  else
    av_bprintf(&buf_script, "total_size=%" PRId64 "\n", total_size);
  if (pts == AV_NOPTS_VALUE) {
    av_bprintf(&buf_script, "out_time_us=N/A\n");
    av_bprintf(&buf_script, "out_time_ms=N/A\n");
    av_bprintf(&buf_script, "out_time=N/A\n");
  } else {
    av_bprintf(&buf_script, "out_time_us=%" PRId64 "\n", pts);
    av_bprintf(&buf_script, "out_time_ms=%" PRId64 "\n", pts);
    av_bprintf(&buf_script, "out_time=%s%02d:%02d:%02d.%06d\n", hours_sign,
               hours, mins, secs, us);
  }

  if (nb_frames_dup || nb_frames_drop)
    av_bprintf(&buf, " dup=%" PRId64 " drop=%" PRId64, nb_frames_dup,
               nb_frames_drop);
  av_bprintf(&buf_script, "dup_frames=%" PRId64 "\n", nb_frames_dup);
  av_bprintf(&buf_script, "drop_frames=%" PRId64 "\n", nb_frames_drop);

  if (speed < 0) {
    av_bprintf(&buf, " speed=N/A");
    av_bprintf(&buf_script, "speed=N/A\n");
  } else {
    av_bprintf(&buf, " speed=%4.3gx", speed);
    av_bprintf(&buf_script, "speed=%4.3gx\n", speed);
  }

  if (print_stats || is_last_report) {
    const char end = is_last_report ? '\n' : '\r';
    if (print_stats == 1 && AV_LOG_INFO > av_log_get_level()) {
      fprintf(stderr, "%s    %c", buf.str, end);
    } else
      AvLog(NULL, AV_LOG_INFO, "%s    %c", buf.str, end);

    fflush(stderr);
  }
  av_bprint_finalize(&buf, NULL);

  if (progress_avio) {
    av_bprintf(&buf_script, "progress=%s\n",
               is_last_report ? "end" : "continue");
    avio_write(progress_avio,
               reinterpret_cast<const unsigned char *>(buf_script.str),
               FFMIN(buf_script.len, buf_script.size - 1));
    avio_flush(progress_avio);
    av_bprint_finalize(&buf_script, NULL);
    if (is_last_report) {
      if ((ret = avio_closep(&progress_avio)) < 0)
        AvLog(NULL, AV_LOG_ERROR,
              "Error closing progress log, loss of information possible: %s\n",
              AvErr2Str(ret));
    }
  }

  first_report = 0;

  if (is_last_report) {
    print_final_stats(total_size);
  }
}
void FfmpegVideoConverter::print_final_stats(int64_t total_size) {
  uint64_t video_size = 0, audio_size = 0, extra_size = 0, other_size = 0;
  uint64_t subtitle_size = 0;
  uint64_t data_size = 0;
  float percent = -1.0;
  int i, j;
  int pass1_used = 1;

  for (i = 0; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];
    switch (ost->enc_ctx->codec_type) {
      case AVMEDIA_TYPE_VIDEO:
        video_size += ost->data_size;
        break;
      case AVMEDIA_TYPE_AUDIO:
        audio_size += ost->data_size;
        break;
      case AVMEDIA_TYPE_SUBTITLE:
        subtitle_size += ost->data_size;
        break;
      default:
        other_size += ost->data_size;
        break;
    }
    extra_size += ost->enc_ctx->extradata_size;
    data_size += ost->data_size;
    if ((ost->enc_ctx->flags & (AV_CODEC_FLAG_PASS1 | AV_CODEC_FLAG_PASS2)) !=
        AV_CODEC_FLAG_PASS1)
      pass1_used = 0;
  }

  if (data_size && total_size > 0 && total_size >= data_size)
    percent = 100.0 * (total_size - data_size) / data_size;

  AvLog(NULL, AV_LOG_INFO,
        "video:%1.0fkB audio:%1.0fkB subtitle:%1.0fkB other streams:%1.0fkB "
        "global headers:%1.0fkB muxing overhead: ",
        video_size / 1024.0, audio_size / 1024.0, subtitle_size / 1024.0,
        other_size / 1024.0, extra_size / 1024.0);
  if (percent >= 0.0)
    AvLog(NULL, AV_LOG_INFO, "%f%%", percent);
  else
    AvLog(NULL, AV_LOG_INFO, "unknown");
  AvLog(NULL, AV_LOG_INFO, "\n");

  /* print verbose per-stream stats */
  for (i = 0; i < nb_input_files; i++) {
    InputFile *f = input_files[i];
    uint64_t total_packets = 0, total_size = 0;

    AvLog(NULL, AV_LOG_VERBOSE, "Input file #%d (%s):\n", i, f->ctx->url);

    for (j = 0; j < f->nb_streams; j++) {
      InputStream *ist = input_streams[f->ist_index + j];
      enum AVMediaType type = ist->dec_ctx->codec_type;

      total_size += ist->data_size;
      total_packets += ist->nb_packets;

      AvLog(NULL, AV_LOG_VERBOSE, "  Input stream #%d:%d (%s): ", i, j,
            av_get_media_type_string(type));
      AvLog(NULL, AV_LOG_VERBOSE,
            "%" PRIu64 " packets read (%" PRIu64 " bytes); ", ist->nb_packets,
            ist->data_size);

      if (ist->decoding_needed) {
        AvLog(NULL, AV_LOG_VERBOSE, "%" PRIu64 " frames decoded",
              ist->frames_decoded);
        if (type == AVMEDIA_TYPE_AUDIO)
          AvLog(NULL, AV_LOG_VERBOSE, " (%" PRIu64 " samples)",
                ist->samples_decoded);
        AvLog(NULL, AV_LOG_VERBOSE, "; ");
      }

      AvLog(NULL, AV_LOG_VERBOSE, "\n");
    }

    AvLog(NULL, AV_LOG_VERBOSE,
          "  Total: %" PRIu64 " packets (%" PRIu64 " bytes) demuxed\n",
          total_packets, total_size);
  }

  for (i = 0; i < nb_output_files; i++) {
    OutputFile *of = output_files[i];
    uint64_t total_packets = 0, total_size = 0;

    AvLog(NULL, AV_LOG_VERBOSE, "Output file #%d (%s):\n", i, of->ctx->url);

    for (j = 0; j < of->ctx->nb_streams; j++) {
      OutputStream *ost = output_streams[of->ost_index + j];
      enum AVMediaType type = ost->enc_ctx->codec_type;

      total_size += ost->data_size;
      total_packets += ost->packets_written;

      AvLog(NULL, AV_LOG_VERBOSE, "  Output stream #%d:%d (%s): ", i, j,
            av_get_media_type_string(type));
      if (ost->encoding_needed) {
        AvLog(NULL, AV_LOG_VERBOSE, "%" PRIu64 " frames encoded",
              ost->frames_encoded);
        if (type == AVMEDIA_TYPE_AUDIO)
          AvLog(NULL, AV_LOG_VERBOSE, " (%" PRIu64 " samples)",
                ost->samples_encoded);
        AvLog(NULL, AV_LOG_VERBOSE, "; ");
      }

      AvLog(NULL, AV_LOG_VERBOSE,
            "%" PRIu64 " packets muxed (%" PRIu64 " bytes); ",
            ost->packets_written, ost->data_size);

      AvLog(NULL, AV_LOG_VERBOSE, "\n");
    }

    AvLog(NULL, AV_LOG_VERBOSE,
          "  Total: %" PRIu64 " packets (%" PRIu64 " bytes) muxed\n",
          total_packets, total_size);
  }
  if (video_size + data_size + audio_size + subtitle_size + extra_size == 0) {
    AvLog(NULL, AV_LOG_WARNING, "Output file is empty, nothing was encoded ");
    if (pass1_used) {
      AvLog(NULL, AV_LOG_WARNING, "\n");
    } else {
      AvLog(NULL, AV_LOG_WARNING,
            "(check -ss / -t / -frames parameters if used)\n");
    }
  }
}

int FfmpegVideoConverter::decode_interrupt_cb(void *ctx) {
  FfmpegVideoConverter *converter = static_cast<FfmpegVideoConverter *>(ctx);
  return converter->received_sigterm;
}

FfmpegVideoConverter::InputOption::~InputOption() { UnInit(); }

void FfmpegVideoConverter::InputOption::Init() {
  if (threads > 0) {
    av_dict_set_int(&codec_opts, "threads", threads, 0);
  }
}
void FfmpegVideoConverter::InputOption::UnInit() {
  if (codec_opts) {
    av_dict_free(&codec_opts);
  }
}

FfmpegVideoConverter::OutputOption::~OutputOption() { UnInit(); }

void FfmpegVideoConverter::OutputOption::Init() {
  if (!vbitrate.empty()) {
    av_dict_set(&codec_opts, "b:v",
                (vbitrate.empty() ? nullptr : vbitrate.c_str()), 0);
  }
  if (threads > 0) {
    av_dict_set_int(&codec_opts, "threads", threads, 0);
  }
  if (!abitrate.empty()) {
    av_dict_set(&codec_opts, "b:a",
                (abitrate.empty() ? nullptr : abitrate.c_str()), 0);
  }
}
void FfmpegVideoConverter::OutputOption::UnInit() {
  if (codec_opts) {
    av_dict_free(&codec_opts);
  }
}

FfmpegVideoConverter::BenchmarkTimeStamps
FfmpegVideoConverter::get_benchmark_time_stamps(void) {
  BenchmarkTimeStamps time_stamps = {av_gettime_relative()};
#if HAVE_GETRUSAGE
  struct rusage rusage;

  getrusage(RUSAGE_SELF, &rusage);
  time_stamps.user_usec =
      (rusage.ru_utime.tv_sec * 1000000LL) + rusage.ru_utime.tv_usec;
  time_stamps.sys_usec =
      (rusage.ru_stime.tv_sec * 1000000LL) + rusage.ru_stime.tv_usec;
#elif HAVE_GETPROCESSTIMES
  HANDLE proc;
  FILETIME c, e, k, u;
  proc = GetCurrentProcess();
  GetProcessTimes(proc, &c, &e, &k, &u);
  time_stamps.user_usec =
      ((int64_t)u.dwHighDateTime << 32 | u.dwLowDateTime) / 10;
  time_stamps.sys_usec =
      ((int64_t)k.dwHighDateTime << 32 | k.dwLowDateTime) / 10;
#else
  time_stamps.user_usec = time_stamps.sys_usec = 0;
#endif
  return time_stamps;
}

namespace {
int64_t getmaxrss(void) {
#if HAVE_GETRUSAGE && HAVE_STRUCT_RUSAGE_RU_MAXRSS
  struct rusage rusage;
  getrusage(RUSAGE_SELF, &rusage);
  return (int64_t)rusage.ru_maxrss * 1024;
#elif HAVE_GETPROCESSMEMORYINFO
  HANDLE proc;
  PROCESS_MEMORY_COUNTERS memcounters;
  proc = GetCurrentProcess();
  memcounters.cb = sizeof(memcounters);
  GetProcessMemoryInfo(proc, &memcounters, sizeof(memcounters));
  return memcounters.PeakPagefileUsage;
#else
  return 0;
#endif
}
}  // namespace

void FfmpegVideoConverter::cleanup(bool success) {
  if (do_benchmark) {
    int maxrss = getmaxrss() / 1024;
    AvLog(NULL, AV_LOG_INFO, "bench: maxrss=%ikB\n", maxrss);
  }

  for (int i = 0; i < nb_filtergraphs; i++) {
    FilterGraph *fg = filtergraphs[i];
    avfilter_graph_free(&fg->graph);
    for (int j = 0; j < fg->nb_inputs; j++) {
      InputFilter *ifilter = fg->inputs[j];
      struct InputStream *ist = ifilter->ist;

      if (ifilter->frame_queue) {
        AVFrame *frame;
        while (av_fifo_read(ifilter->frame_queue, &frame, 1) >= 0)
          av_frame_free(&frame);
        av_fifo_freep2(&ifilter->frame_queue);
      }
      av_freep(&ifilter->displaymatrix);
      if (ist->sub2video.sub_queue) {
        AVSubtitle sub;
        while (av_fifo_read(ist->sub2video.sub_queue, &sub, 1) >= 0)
          avsubtitle_free(&sub);
        av_fifo_freep2(&ist->sub2video.sub_queue);
      }
      av_buffer_unref(&ifilter->hw_frames_ctx);
      av_freep(&ifilter->name);
      av_freep(&fg->inputs[j]);
    }
    av_freep(&fg->inputs);
    for (int j = 0; j < fg->nb_outputs; j++) {
      OutputFilter *ofilter = fg->outputs[j];

      avfilter_inout_free(&ofilter->out_tmp);
      av_freep(&ofilter->name);
      av_channel_layout_uninit(&ofilter->ch_layout);
      av_freep(&fg->outputs[j]);
    }
    av_freep(&fg->outputs);
    av_freep(&fg->graph_desc);

    av_freep(&filtergraphs[i]);
  }
  av_freep(&filtergraphs);

  av_freep(&subtitle_out);

  /* close files */
  for (int i = 0; i < nb_output_files; i++) of_close(&output_files[i]);

  for (int i = 0; i < nb_output_streams; i++) {
    OutputStream *ost = output_streams[i];

    if (!ost) continue;

    av_bsf_free(&ost->bsf_ctx);

    av_frame_free(&ost->filtered_frame);
    av_frame_free(&ost->last_frame);
    av_packet_free(&ost->pkt);
    av_dict_free(&ost->encoder_opts);

    av_freep(&ost->forced_keyframes);
    av_expr_free(ost->forced_keyframes_pexpr);
    av_freep(&ost->avfilter);
    av_freep(&ost->logfile_prefix);

    av_freep(&ost->audio_channels_map);
    ost->audio_channels_mapped = 0;

    av_dict_free(&ost->sws_dict);
    av_dict_free(&ost->swr_opts);

    avcodec_free_context(&ost->enc_ctx);
    avcodec_parameters_free(&ost->ref_par);

    if (ost->muxing_queue) {
      AVPacket *pkt;
      while (av_fifo_read(ost->muxing_queue, &pkt, 1) >= 0)
        av_packet_free(&pkt);
      av_fifo_freep2(&ost->muxing_queue);
    }

    av_freep(&output_streams[i]);
  }
  for (int i = 0; i < nb_input_files; i++) {
    avformat_close_input(&input_files[i]->ctx);
    av_packet_free(&input_files[i]->pkt);
    av_freep(&input_files[i]);
  }
  for (int i = 0; i < nb_input_streams; i++) {
    InputStream *ist = input_streams[i];

    av_frame_free(&ist->decoded_frame);
    av_packet_free(&ist->pkt);
    av_dict_free(&ist->decoder_opts);
    avsubtitle_free(&ist->prev_sub.subtitle);
    av_frame_free(&ist->sub2video.frame);
    av_freep(&ist->filters);
    av_freep(&ist->hwaccel_device);
    av_freep(&ist->dts_buffer);

    avcodec_free_context(&ist->dec_ctx);

    av_freep(&input_streams[i]);
  }

  if (vstats_file) {
    if (fclose(vstats_file))
      AvLog(NULL, AV_LOG_ERROR,
            "Error closing vstats file, loss of information possible: %s\n",
            AvErr2Str(AVERROR(errno)));
  }
  av_freep(&vstats_filename);
  av_freep(&filter_nbthreads);

  av_freep(&input_streams);
  av_freep(&input_files);
  av_freep(&output_streams);
  av_freep(&output_files);

  // uninit_opts();

  avformat_network_deinit();

  if (received_sigterm) {
    AvLog(nullptr, AV_LOG_INFO,
          "Exiting normally, received signal set by user\n");
  } else if (!success) {
    AvLog(nullptr, AV_LOG_INFO, "Conversion failed!\n");
  }
}

void FfmpegVideoConverter::Run(void *arg) {
  auto converter = static_cast<FfmpegVideoConverter *>(arg);
  converter->Convert(converter->input_file_, converter->output_file_);
  VideoConverter::Response r;
  r.output_file = converter->output_file_;
  converter->PostConvertEnd(r);
}
