#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavcodec/bsf.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/avutil.h"
#include "libavutil/dict.h"
#include "libavutil/eval.h"
#include "libavutil/fifo.h"
#include "libavutil/hwcontext.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
#include "libavutil/threadmessage.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"

#ifdef __cplusplus
}
#endif

#include <memory>
#include <string>

// 一些重要的数据类型，流编解码选项
enum StreamType : char {
  kUnSetStream,
  kVideoStream = 'v',
  kAudioStream = 'a',
  kSubtitleStream = 's',
  kDataStream = 'd',
  kAttachmentStream = 't',
};

struct CodecSpecifier {
  CodecSpecifier(StreamType s_t, int s_id, const std::string& cname)
      : stream_type_(s_t), stream_id_(s_id), codec_name_(cname) {
    GenerateSpec();
  }

  const char *GetSpec() const { return spec_.c_str(); }
  const char *GetCodecName() const { return codec_name_.c_str(); }

 private:
  void GenerateSpec();

 private:
  StreamType stream_type_;
  int stream_id_{-1};
  std::string codec_name_;
  std::string spec_;
};

#define DEFAULT_PASS_LOGFILENAME_PREFIX "ffmpeg2pass"

#define HAS_ARG 0x0001
#define OPT_BOOL 0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO 0x0010
#define OPT_AUDIO 0x0020
#define OPT_INT 0x0080
#define OPT_FLOAT 0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_INT64 0x0400
#define OPT_EXIT 0x0800
#define OPT_DATA 0x1000
#define OPT_PERFILE                                         \
  0x2000 /* the option is per-file (currently ffmpeg-only). \
            implied by OPT_OFFSET or OPT_SPEC */
#define OPT_OFFSET \
  0x4000 /* option is specified as an offset in a passed optctx */
#define OPT_SPEC                                                 \
  0x8000 /* option is to be stored in an array of SpecifierOpt.  \
            Implies OPT_OFFSET. Next element after the offset is \
            an int containing element count in the array. */
#define OPT_TIME 0x10000
#define OPT_DOUBLE 0x20000
#define OPT_INPUT 0x40000
#define OPT_OUTPUT 0x80000

int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);

AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st,
                                const AVCodec *codec);

AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
                                           AVDictionary *codec_opts);

void *grow_array(void *array, int elem_size, int *size, int new_size);
void *allocate_array_elem(void *array, size_t elem_size, int *nb_elems);

AVDictionary *strip_specifiers(AVDictionary *dict);

char *read_file(const char *filename);

double parse_number_or_die(const char *context, const char *numstr, int type,
                           double min, double max);

void OutputLog(const char *str);

template <typename... Args>
void AvLog(void *avcl, int log_level, const std::string &format, Args... args) {
  if (AV_LOG_DEBUG == log_level) {
    return;
  }
  int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
               1;  // Extra space for '\0'
  if (size_s <= 0) {
    return;
  }
  auto size = static_cast<size_t>(size_s);
  std::unique_ptr<char[]> buf = std::make_unique<char[]>(size);
  std::snprintf(buf.get(), size, format.c_str(), args...);
  // output to console
  OutputLog(buf.get());
}

namespace {
const char *AvErr2Str(int errnum) {
  static char av_err_buf[AV_ERROR_MAX_STRING_SIZE] = {0};
  memset(av_err_buf, 0, AV_ERROR_MAX_STRING_SIZE);
  av_make_error_string(av_err_buf, AV_ERROR_MAX_STRING_SIZE, errnum);
  return av_err_buf;
}
const char *AvTs2TimeStr(int64_t ts, AVRational *tb) {
  static char av_str_buf[AV_TS_MAX_STRING_SIZE] = {0};
  memset(av_str_buf, 0, AV_TS_MAX_STRING_SIZE);
  av_ts_make_time_string(av_str_buf, ts, tb);
  return av_str_buf;
}
const char *AvTs2Str(int64_t ts) {
  static char av_str_buf[AV_TS_MAX_STRING_SIZE] = {0};
  memset(av_str_buf, 0, AV_TS_MAX_STRING_SIZE);
  av_ts_make_string(av_str_buf, ts);
  return av_str_buf;
}
void PrintError(const char *filename, int err) {
  char errbuf[128] = {0};
  const char *errbuf_ptr = errbuf;

  if (av_strerror(err, errbuf, sizeof(errbuf)) < 0) {
    errbuf_ptr = strerror(AVUNERROR(err));
  }
  AvLog(nullptr, AV_LOG_ERROR, "%s: %s\n", filename, errbuf_ptr);
}
// AVRational av_time_base_q{1, AV_TIME_BASE};
}  // namespace
