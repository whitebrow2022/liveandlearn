#include "ffmpeg_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/bprint.h"
#include "libavutil/opt.h"

#ifdef __cplusplus
}
#endif

#include <sstream>

#if defined(_WIN32)
#include <Windows.h>
#endif

void CodecSpecifier::GenerateSpec() {
  std::stringstream ss;
  bool need_colon = false;
  if (stream_type_ != kUnSetStream) {
    ss << static_cast<char>(stream_type_);
    need_colon = true;
  }
  if (stream_id_ != -1) {
    if (need_colon) {
      ss << ":";
    }
    ss << stream_id_;
    need_colon = true;
  }
  spec_ = ss.str();
}

int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec) {
  int ret = avformat_match_stream_specifier(s, st, spec);
  if (ret < 0) av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
  return ret;
}

AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st,
                                const AVCodec *codec) {
  AVDictionary *ret = nullptr;
  const AVDictionaryEntry *t = nullptr;
  int flags =
      s->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
  char prefix = 0;
  const AVClass *cc = avcodec_get_class();

  if (!codec) {
    codec = s->oformat ? avcodec_find_encoder(codec_id)
                       : avcodec_find_decoder(codec_id);
  }
  switch (st->codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
      prefix = 'v';
      flags |= AV_OPT_FLAG_VIDEO_PARAM;
      break;
    case AVMEDIA_TYPE_AUDIO:
      prefix = 'a';
      flags |= AV_OPT_FLAG_AUDIO_PARAM;
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      prefix = 's';
      flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
      break;
  }

  while (t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)) {
    const AVClass *priv_class;
    char *p = strchr(t->key, ':');

    /* check stream specification in opt name */
    if (p) {
      switch (check_stream_specifier(s, st, p + 1)) {
        case 1:
          *p = 0;
          break;
        case 0:
          continue;
        default:
          return nullptr;
      }
    }
    if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
        !codec ||
        ((priv_class = codec->priv_class) &&
         av_opt_find(&priv_class, t->key, NULL, flags,
                     AV_OPT_SEARCH_FAKE_OBJ))) {
      av_dict_set(&ret, t->key, t->value, 0);
    } else if (t->key[0] == prefix && av_opt_find(&cc, t->key + 1, NULL, flags,
                                                  AV_OPT_SEARCH_FAKE_OBJ)) {
      av_dict_set(&ret, t->key + 1, t->value, 0);
    }
    if (p) {
      *p = ':';
    }
  }
  return ret;
}

AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
                                           AVDictionary *codec_opts) {
  int i;
  AVDictionary **opts;

  if (!s->nb_streams) return NULL;
  opts = static_cast<AVDictionary **>(av_calloc(s->nb_streams, sizeof(*opts)));
  if (!opts) {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc memory for stream options.\n");
    return nullptr;
  }
  for (i = 0; i < s->nb_streams; i++)
    opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id,
                                s, s->streams[i], NULL);
  return opts;
}

void *grow_array(void *array, int elem_size, int *size, int new_size) {
  if (new_size >= INT_MAX / elem_size) {
    av_log(NULL, AV_LOG_ERROR, "Array too big.\n");
    return nullptr;
  }
  if (*size < new_size) {
    uint8_t *tmp =
        static_cast<uint8_t *>(av_realloc_array(array, new_size, elem_size));
    if (!tmp) {
      av_log(NULL, AV_LOG_ERROR, "Could not alloc buffer.\n");
      return nullptr;
    }
    memset(tmp + *size * elem_size, 0, (new_size - *size) * elem_size);
    *size = new_size;
    return tmp;
  }
  return array;
}
void *allocate_array_elem(void *ptr, size_t elem_size, int *nb_elems) {
  void *new_elem;

  if (!(new_elem = av_mallocz(elem_size)) ||
      av_dynarray_add_nofree(ptr, nb_elems, new_elem) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Could not alloc buffer.\n");
    return nullptr;
  }
  return new_elem;
}

/* return a copy of the input with the stream specifiers removed from the keys
 */
AVDictionary *strip_specifiers(AVDictionary *dict) {
  const AVDictionaryEntry *e = NULL;
  AVDictionary *ret = NULL;

  while ((e = av_dict_get(dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
    char *p = strchr(e->key, ':');

    if (p) *p = 0;
    av_dict_set(&ret, e->key, e->value, 0);
    if (p) *p = ':';
  }
  return ret;
}

char *read_file(const char *filename) {
  AVIOContext *pb = NULL;
  int ret = avio_open(&pb, filename, AVIO_FLAG_READ);
  AVBPrint bprint;
  char *str;

  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Error opening file %s.\n", filename);
    return NULL;
  }

  av_bprint_init(&bprint, 0, AV_BPRINT_SIZE_UNLIMITED);
  ret = avio_read_to_bprint(pb, &bprint, SIZE_MAX);
  avio_closep(&pb);
  if (ret < 0) {
    av_bprint_finalize(&bprint, NULL);
    return NULL;
  }
  ret = av_bprint_finalize(&bprint, &str);
  if (ret < 0) return NULL;
  return str;
}

double parse_number_or_die(const char *context, const char *numstr, int type,
                           double min, double max) {
  char *tail;
  const char *error;
  double d = av_strtod(numstr, &tail);
  if (*tail)
    error = "Expected number for %s but found: %s\n";
  else if (d < min || d > max)
    error = "The value for %s was %s which is not within %f - %f\n";
  else if (type == OPT_INT64 && (int64_t)d != d)
    error = "Expected int64 for %s but found %s\n";
  else if (type == OPT_INT && (int)d != d)
    error = "Expected int for %s but found %s\n";
  else
    return d;
  av_log(NULL, AV_LOG_FATAL, error, context, numstr, min, max);

  // TODO:
  return 0;
}

void OutputLog(const char *str) {
#if defined(_WIN32)
  OutputDebugStringA(str);
#else
  printf(str);
#endif
}
