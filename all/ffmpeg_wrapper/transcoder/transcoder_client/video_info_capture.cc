// Created by liangxu on 2022/11/16.
//
// Copyright (c) 2022 The Transcoder Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "video_info_capture.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
#include <libavutil/display.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>
#include <memory>
#include <string>
#include <vector>

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

std::vector<std::string> VideoInfoCapture::AvailableVideoEncoders() {
  std::vector<std::string> video_encoders;
  video_encoders = AvailableEncoders(AVMEDIA_TYPE_VIDEO);
  return video_encoders;
}
std::vector<std::string> VideoInfoCapture::AvailableAudioEncoders() {
  std::vector<std::string> audio_encoders;
  std::vector<std::string> video_encoders;
  video_encoders = AvailableEncoders(AVMEDIA_TYPE_AUDIO);
  return video_encoders;
  return audio_encoders;
}
std::vector<std::string> VideoInfoCapture::AvailableFileFormats() {
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

namespace {

class VideoFirstValidFrameDecoder {
 public:
  VideoFirstValidFrameDecoder() = default;

 public:
  std::unique_ptr<VideoInfoCapture::Image> ExtractFirstValidFrame(
      const char *src_filename, unsigned int *duration_sec) {
    /* open input file, and allocate format context */
    int ret = avformat_open_input(&fmt_ctx, src_filename, nullptr, nullptr);
    if (ret < 0) {
      char err_buf[1024] = {0};
      av_strerror(ret, err_buf, 1024);
      fprintf(stderr, "Could not open source file %s\n", src_filename);
      return std::move(image);
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
      fprintf(stderr, "Could not find stream information\n");
      return std::move(image);
    }

    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx,
                           AVMEDIA_TYPE_VIDEO) >= 0) {
      video_stream = fmt_ctx->streams[video_stream_idx];

      /* allocate image where the decoded image will be put */
      width = video_dec_ctx->width;
      height = video_dec_ctx->height;
      pix_fmt = video_dec_ctx->pix_fmt;
      if (duration_sec) {
        *duration_sec = static_cast<double>(fmt_ctx->duration) / AV_TIME_BASE;
      }
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    if (!video_stream) {
      fprintf(stderr, "Could not find video stream in the input, aborting\n");
      ret = 1;
      goto end;
    }

    frame = av_frame_alloc();
    if (!frame) {
      fprintf(stderr, "Could not allocate frame\n");
      ret = AVERROR(ENOMEM);
      goto end;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
      fprintf(stderr, "Could not allocate packet\n");
      ret = AVERROR(ENOMEM);
      goto end;
    }

    /* read frames from the file */
    while (av_read_frame(fmt_ctx, pkt) >= 0 && !image) {
      // check if the packet belongs to a stream we are interested in, otherwise
      // skip it
      if (pkt->stream_index == video_stream_idx) {
        ret = decode_packet(video_dec_ctx, pkt);
      }
      av_packet_unref(pkt);
      if (ret < 0) {
        break;
      }
    }

    if (!image) {
      /* flush the decoders */
      if (video_dec_ctx) {
        decode_packet(video_dec_ctx, nullptr);
      }
    }

    printf("Demuxing succeeded.\n");

  end:
    avcodec_free_context(&video_dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_packet_free(&pkt);
    av_frame_free(&frame);
    return std::move(image);
  }

 private:
  int output_video_frame(AVFrame *frame) {
    if (frame->width != width || frame->height != height ||
        frame->format != pix_fmt) {
      /* To handle this change, one could call av_image_alloc again and
       * decode the following frames into another rawvideo file. */
      fprintf(stderr,
              "Error: Width, height and pixel format have to be "
              "constant in a rawvideo file, but the width, height or "
              "pixel format of the input video changed:\n"
              "old: width = %d, height = %d, format = %s\n"
              "new: width = %d, height = %d, format = %s\n",
              width, height, av_get_pix_fmt_name(pix_fmt), frame->width,
              frame->height,
              av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format)));
      return -1;
    }

    printf("video_frame n:%d coded_n:%d\n", video_frame_count++,
           frame->coded_picture_number);

    do {
      struct SwsContext *sws_ctx =
          sws_getContext(width, height, pix_fmt, width, height,
                         AV_PIX_FMT_ARGB,  // argb
                         SWS_BICUBIC, nullptr, nullptr, nullptr);
      AVFrame *argb_frame = av_frame_alloc();

      argb_frame->format = AV_PIX_FMT_ARGB;
      argb_frame->width = width;
      argb_frame->height = height;
      int sts = av_frame_get_buffer(argb_frame, 0);
      sts = sws_scale(sws_ctx,           // struct SwsContext* c,
                      frame->data,       // const uint8_t* const srcSlice[],
                      frame->linesize,   // const int srcStride[],
                      0,                 // int srcSliceY,
                      frame->height,     // int srcSliceH,
                      argb_frame->data,  // uint8_t* const dst[],
                      argb_frame->linesize);  // const int dstStride[]);

      if (sts != frame->height) {
        break;
      }

      image = std::make_unique<VideoInfoCapture::Image>(width, height);
      for (int y = 0; y < argb_frame->height; ++y) {
        for (int x = 0; x < argb_frame->width; ++x) {
          uint8_t a =
              *(argb_frame->data[0] + y * argb_frame->linesize[0] + x * 4);
          uint8_t r =
              *(argb_frame->data[0] + y * argb_frame->linesize[0] + x * 4 + 1);
          uint8_t g =
              *(argb_frame->data[0] + y * argb_frame->linesize[0] + x * 4 + 2);
          uint8_t b =
              *(argb_frame->data[0] + y * argb_frame->linesize[0] + x * 4 + 3);
          image->SetColor(x, y, r, g, b, a);
        }
      }

      sws_freeContext(sws_ctx);
      av_frame_free(&argb_frame);
    } while (0);

    return 0;
  }

  int decode_packet(AVCodecContext *dec, const AVPacket *pkt) {
    int ret = 0;

    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
      // fprintf(stderr, "Error submitting a packet for decoding (%s)\n",
      //        av_err2str(ret));
      return ret;
    }

    // get all the available frames from the decoder
    while (ret >= 0) {
      ret = avcodec_receive_frame(dec, frame);
      if (ret < 0) {
        // those two return values are special and mean there is no output
        // frame available, but there were no errors during decoding
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
          return 0;
        }

        // fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
        return ret;
      }

      // write the frame data to output file
      if (dec->codec->type == AVMEDIA_TYPE_VIDEO) {
        ret = output_video_frame(frame);
      }

      av_frame_unref(frame);
      if (ret < 0) return ret;
    }

    return 0;
  }

  int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx,
                         AVFormatContext *fmt_ctx, enum AVMediaType type) {
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec{nullptr};

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
    if (ret < 0) {
      return ret;
    } else {
      stream_index = ret;
      st = fmt_ctx->streams[stream_index];

      /* find decoder for the stream */
      dec = avcodec_find_decoder(st->codecpar->codec_id);
      if (!dec) {
        fprintf(stderr, "Failed to find %s codec\n",
                av_get_media_type_string(type));
        return AVERROR(EINVAL);
      }

      /* Allocate a codec context for the decoder */
      *dec_ctx = avcodec_alloc_context3(dec);
      if (!*dec_ctx) {
        fprintf(stderr, "Failed to allocate the %s codec context\n",
                av_get_media_type_string(type));
        return AVERROR(ENOMEM);
      }

      /* Copy codec parameters from input stream to output codec context */
      if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
        fprintf(stderr,
                "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(type));
        return ret;
      }

      /* Init the decoders */
      if ((ret = avcodec_open2(*dec_ctx, dec, nullptr)) < 0) {
        fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(type));
        return ret;
      }
      *stream_idx = stream_index;
    }

    return 0;
  }

 private:
  AVFormatContext *fmt_ctx{nullptr};
  AVCodecContext *video_dec_ctx{nullptr};
  int width{-1};
  int height{-1};
  AVPixelFormat pix_fmt;
  AVStream *video_stream{nullptr};

  int video_stream_idx{-1};
  AVFrame *frame{nullptr};
  AVPacket *pkt{nullptr};
  int video_frame_count{0};

  std::unique_ptr<VideoInfoCapture::Image> image;

 private:
  VideoFirstValidFrameDecoder(const VideoFirstValidFrameDecoder &) = delete;
  VideoFirstValidFrameDecoder &operator=(const VideoFirstValidFrameDecoder &) =
      delete;
};
}  // namespace

std::unique_ptr<VideoInfoCapture::Image>
VideoInfoCapture::ExtractVideoFirstValidFrameToImageBuffer(
    const char *video_file_path, unsigned int *duration) {
  VideoFirstValidFrameDecoder decoder;
  auto image = decoder.ExtractFirstValidFrame(video_file_path, duration);
  return image;
}

VideoInfoCapture::FileInfo VideoInfoCapture::ExtractFileInfo(
    const char *src_filename) {
  VideoInfoCapture::FileInfo file_info;

  AVFormatContext *fmt_ctx{nullptr};
  do {
    /* open input file, and allocate format context */
    int ret = avformat_open_input(&fmt_ctx, src_filename, nullptr, nullptr);
    if (ret < 0) {
      break;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
      break;
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
      AVCodecContext *avctx = avcodec_alloc_context3(nullptr);
      if (!avctx) {
        continue;
      }
      const AVStream *st = fmt_ctx->streams[i];
      ret = avcodec_parameters_to_context(avctx, st->codecpar);
      if (ret < 0) {
        avcodec_free_context(&avctx);
        continue;
      }

      int64_t bit_rate;
      switch (avctx->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
          bit_rate = avctx->bit_rate;
          file_info.video_stream_bitrates.insert(
              std::make_pair(st->id, bit_rate));
          break;
        case AVMEDIA_TYPE_DATA:
        case AVMEDIA_TYPE_SUBTITLE:
        case AVMEDIA_TYPE_ATTACHMENT:
          bit_rate = avctx->bit_rate;
          break;
        case AVMEDIA_TYPE_AUDIO: {
          int bits_per_sample = av_get_bits_per_sample(avctx->codec_id);
          if (bits_per_sample) {
            bit_rate =
                avctx->sample_rate * (int64_t)avctx->ch_layout.nb_channels;
            if (bit_rate > INT64_MAX / bits_per_sample) {
              bit_rate = 0;
            } else {
              bit_rate *= bits_per_sample;
            }
          } else {
            bit_rate = avctx->bit_rate;
          }
          file_info.audio_stream_bitrates.insert(
              std::make_pair(st->id, bit_rate));
        } break;
        default:
          bit_rate = 0;
          break;
      }
      avcodec_free_context(&avctx);
    }
  } while (false);
  if (fmt_ctx) {
    avformat_close_input(&fmt_ctx);
  }

  return file_info;
}

// get video info
namespace {
QImage ConvertCvImageToQImage(const VideoInfoCapture::Image &cvimage,
                              double afactor = 1.0) {
  int cvwidth = cvimage.GetWidth();
  int cvheight = cvimage.GetHeight();
  QImage dest(cvwidth, cvheight, QImage::Format_ARGB32);
  for (int y = 0; y < cvheight; ++y) {
    for (int x = 0; x < cvwidth; ++x) {
      unsigned char r, g, b, a;
      cvimage.GetColor(x, y, &r, &g, &b, &a);
      a = a * afactor;
      QColor qcolor(r, g, b, a);
      dest.setPixelColor(x, y, qcolor);
    }
  }
  return dest;
}
class VideoInfoDecoder {
 public:
  VideoInfoDecoder() = default;

 public:
  bool ExtractVideoInfo(const char *src_filename,
                        VideoInfoCapture::VideoInfo *video_info) {
    video_info_ = {};
    src_file_path_ = src_filename;
    /* open input file, and allocate format context */
    int ret = avformat_open_input(&fmt_ctx, src_filename, nullptr, nullptr);
    if (ret < 0) {
      char err_buf[1024] = {0};
      av_strerror(ret, err_buf, 1024);
      fprintf(stderr, "Could not open source file %s\n", src_filename);
      return false;
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
      fprintf(stderr, "Could not find stream information\n");
      return false;
    }

    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx,
                           AVMEDIA_TYPE_VIDEO) >= 0) {
      video_stream = fmt_ctx->streams[video_stream_idx];

      /* allocate image where the decoded image will be put */
      width = video_dec_ctx->width;
      height = video_dec_ctx->height;
      pix_fmt = video_dec_ctx->pix_fmt;

      video_info_.video_width = video_dec_ctx->width;
      video_info_.video_height = video_dec_ctx->height;
      // fmt_ctx->duration us
      video_info_.video_duration =
          static_cast<double>(fmt_ctx->duration) / 1000;
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);

    if (!video_stream) {
      fprintf(stderr, "Could not find video stream in the input, aborting\n");
      ret = 1;
      goto end;
    }

    frame = av_frame_alloc();
    if (!frame) {
      fprintf(stderr, "Could not allocate frame\n");
      ret = AVERROR(ENOMEM);
      goto end;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
      fprintf(stderr, "Could not allocate packet\n");
      ret = AVERROR(ENOMEM);
      goto end;
    }

    /* read frames from the file */
    while (av_read_frame(fmt_ctx, pkt) >= 0 &&
           video_info_.thumb_image_path.empty()) {
      // check if the packet belongs to a stream we are interested in, otherwise
      // skip it
      if (pkt->stream_index == video_stream_idx) {
        ret = decode_packet(video_dec_ctx, pkt);
      }
      av_packet_unref(pkt);
      if (ret < 0) {
        break;
      }
    }

    if (video_info_.thumb_image_path.empty()) {
      /* flush the decoders */
      if (video_dec_ctx) {
        decode_packet(video_dec_ctx, nullptr);
      }
    }

    printf("Demuxing succeeded.\n");

  end:
    avcodec_free_context(&video_dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_packet_free(&pkt);
    av_frame_free(&frame);

    if (ret != 0) {
      return false;
    }

    *video_info = video_info_;
    return true;
  }

 private:
  int output_video_frame(AVFrame *frame) {
    if (frame->width != width || frame->height != height ||
        frame->format != pix_fmt) {
      /* To handle this change, one could call av_image_alloc again and
       * decode the following frames into another rawvideo file. */
      fprintf(stderr,
              "Error: Width, height and pixel format have to be "
              "constant in a rawvideo file, but the width, height or "
              "pixel format of the input video changed:\n"
              "old: width = %d, height = %d, format = %s\n"
              "new: width = %d, height = %d, format = %s\n",
              width, height, av_get_pix_fmt_name(pix_fmt), frame->width,
              frame->height,
              av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format)));
      return -1;
    }

    printf("video_frame n:%d coded_n:%d\n", video_frame_count++,
           frame->coded_picture_number);

    do {
      constexpr const int kMaxThumbImageWidth = 1920;
      constexpr const int kMaxThumbImageHeight = 1080;
      int src_w = width;
      int src_h = height;
      int dest_w = width;
      int dest_h = height;
      // scale to dst
      auto w_scale = static_cast<double>(dest_w) / kMaxThumbImageWidth;
      auto h_scale = static_cast<double>(dest_h) / kMaxThumbImageHeight;
      if (dest_w > kMaxThumbImageWidth && dest_h > kMaxThumbImageHeight) {
        if (w_scale > h_scale) {
          // scale by width
          dest_h = src_h / w_scale;
          dest_w = kMaxThumbImageWidth;
        } else {
          // scale by height;
          dest_w = src_w / h_scale;
          dest_h = kMaxThumbImageHeight;
        }
      } else if (video_info_.video_width > kMaxThumbImageWidth) {
        dest_h = src_h / w_scale;
        dest_w = kMaxThumbImageWidth;
      } else if (video_info_.video_height > kMaxThumbImageHeight) {
        dest_w = src_w / h_scale;
        dest_h = kMaxThumbImageHeight;
      }
      struct SwsContext *sws_ctx =
          sws_getContext(src_w, src_h, pix_fmt, dest_w, dest_h,
                         AV_PIX_FMT_ARGB,  // argb
                         SWS_BICUBIC, nullptr, nullptr, nullptr);
      AVFrame *argb_frame = av_frame_alloc();

      argb_frame->format = AV_PIX_FMT_ARGB;
      argb_frame->width = dest_w;
      argb_frame->height = dest_h;
      int sts = av_frame_get_buffer(argb_frame, 0);
      sts = sws_scale(sws_ctx,           // struct SwsContext* c,
                      frame->data,       // const uint8_t* const srcSlice[],
                      frame->linesize,   // const int srcStride[],
                      0,                 // int srcSliceY,
                      frame->height,     // int srcSliceH,
                      argb_frame->data,  // uint8_t* const dst[],
                      argb_frame->linesize);  // const int dstStride[]);

      if (sts != argb_frame->height) {
        // scale failed
        break;
      }

      // calc image cache dir
      QString image_cache_dir =
          QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
      QDir dir(image_cache_dir);
      QString sub_dir_name = "mp4s";
      if (!dir.exists(sub_dir_name)) {
        dir.mkdir(sub_dir_name);
      }
      dir.cd(sub_dir_name);
      image_cache_dir = dir.absolutePath();

      QFileInfo src_video_file_info(QString(src_file_path_.c_str()));
      // calc image thumb path
      QString thumb_image_path =
          QString("%1/%2_thumb_%3.png")
              .arg(image_cache_dir)
              .arg(src_video_file_info.baseName())
              .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));

      QImage srcimage(argb_frame->width, argb_frame->height,
                      QImage::Format_ARGB32);
      for (int y = 0; y < argb_frame->height; ++y) {
        for (int x = 0; x < argb_frame->width; ++x) {
          uint8_t a =
              *(argb_frame->data[0] + y * argb_frame->linesize[0] + x * 4);
          uint8_t r =
              *(argb_frame->data[0] + y * argb_frame->linesize[0] + x * 4 + 1);
          uint8_t g =
              *(argb_frame->data[0] + y * argb_frame->linesize[0] + x * 4 + 2);
          uint8_t b =
              *(argb_frame->data[0] + y * argb_frame->linesize[0] + x * 4 + 3);
          QColor qcolor(r, g, b, a);
          srcimage.setPixelColor(x, y, qcolor);
        }
      }
      QImage thumb_image = srcimage;

#if 0
      if (video_info_.video_width > kMaxThumbImageWidth &&
          video_info_.video_height > kMaxThumbImageHeight) {
        auto w_scale =
            static_cast<double>(video_info_.video_width) / kMaxThumbImageWidth;
        auto h_scale = static_cast<double>(video_info_.video_height) /
                       kMaxThumbImageHeight;
        if (w_scale > h_scale) {
          // scale by width
          thumb_image = srcimage.scaledToWidth(kMaxThumbImageWidth);
        } else {
          // scale by height;
          thumb_image = srcimage.scaledToHeight(kMaxThumbImageHeight);
        }
      } else if (video_info_.video_width > kMaxThumbImageWidth) {
        thumb_image = srcimage.scaledToWidth(kMaxThumbImageWidth);
      } else if (video_info_.video_height > kMaxThumbImageHeight) {
        thumb_image = srcimage.scaledToHeight(kMaxThumbImageHeight);
      } else {
        thumb_image = srcimage;
      }
#endif
      bool ret = false;
      ret = thumb_image.save(thumb_image_path, "PNG");
      if (ret) {
        // TODO(liangxu): image cache path
        // 设置首帧图
        video_info_.thumb_image_width = thumb_image.width();
        video_info_.thumb_image_height = thumb_image.height();
        QFileInfo thumb_image_file_info(thumb_image_path);
        video_info_.thumb_image_size = thumb_image_file_info.size();
        video_info_.thumb_image_path = thumb_image_path.toStdString();
        video_info_.video_size = src_video_file_info.size();
        video_info_.video_path = src_file_path_;
      }
      sws_freeContext(sws_ctx);
      av_frame_free(&argb_frame);
    } while (false);

    return 0;
  }

  int decode_packet(AVCodecContext *dec, const AVPacket *pkt) {
    int ret = 0;

    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
      // fprintf(stderr, "Error submitting a packet for decoding (%s)\n",
      //        av_err2str(ret));
      return ret;
    }

    // get all the available frames from the decoder
    while (ret >= 0) {
      ret = avcodec_receive_frame(dec, frame);
      if (ret < 0) {
        // those two return values are special and mean there is no output
        // frame available, but there were no errors during decoding
        if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
          return 0;
        }

        // fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
        return ret;
      }

      // write the frame data to output file
      if (dec->codec->type == AVMEDIA_TYPE_VIDEO) {
        ret = output_video_frame(frame);
      }

      av_frame_unref(frame);
      if (ret < 0) return ret;
    }

    return 0;
  }

  int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx,
                         AVFormatContext *fmt_ctx, enum AVMediaType type) {
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec{nullptr};

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
    if (ret < 0) {
      return ret;
    } else {
      stream_index = ret;
      st = fmt_ctx->streams[stream_index];

      /* find decoder for the stream */
      dec = avcodec_find_decoder(st->codecpar->codec_id);
      if (!dec) {
        fprintf(stderr, "Failed to find %s codec\n",
                av_get_media_type_string(type));
        return AVERROR(EINVAL);
      }

      /* Allocate a codec context for the decoder */
      *dec_ctx = avcodec_alloc_context3(dec);
      if (!*dec_ctx) {
        fprintf(stderr, "Failed to allocate the %s codec context\n",
                av_get_media_type_string(type));
        return AVERROR(ENOMEM);
      }

      /* Copy codec parameters from input stream to output codec context */
      if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
        fprintf(stderr,
                "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(type));
        return ret;
      }

      /* Init the decoders */
      if ((ret = avcodec_open2(*dec_ctx, dec, nullptr)) < 0) {
        fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(type));
        return ret;
      }
      *stream_idx = stream_index;
    }

    return 0;
  }

 private:
  AVFormatContext *fmt_ctx{nullptr};
  AVCodecContext *video_dec_ctx{nullptr};
  int width{-1};
  int height{-1};
  AVPixelFormat pix_fmt;
  AVStream *video_stream{nullptr};

  int video_stream_idx{-1};
  AVFrame *frame{nullptr};
  AVPacket *pkt{nullptr};
  int video_frame_count{0};

  std::string src_file_path_;
  VideoInfoCapture::VideoInfo video_info_;

 private:
  VideoInfoDecoder(const VideoInfoDecoder &) = delete;
  VideoInfoDecoder &operator=(const VideoInfoDecoder &) = delete;
};
}  // namespace

bool VideoInfoCapture::ExtractVideInfo(
    const char *video_file_path, VideoInfoCapture::VideoInfo *video_info) {
  if (!video_info) {
    return false;
  }
  VideoInfoCapture::VideoInfo ret;

  // extract info
  VideoInfoDecoder decoder;
  auto image = decoder.ExtractVideoInfo(video_file_path, &ret);

  *video_info = ret;
  return true;
}
