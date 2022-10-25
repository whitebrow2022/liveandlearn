#pragma once

#include "base_video_converter.h"

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
#include "libswresample/swresample.h"

#ifdef __cplusplus
}
#endif

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ffmpeg_util.h"

enum VideoSyncMethod {
  VSYNC_AUTO = -1,
  VSYNC_PASSTHROUGH,
  VSYNC_CFR,
  VSYNC_VFR,
  VSYNC_VSCFR,
  VSYNC_DROP,
};

enum HWAccelID {
  HWACCEL_NONE = 0,
  HWACCEL_AUTO,
  HWACCEL_GENERIC,
};

struct HWDevice {
  const char *name;
  AVHWDeviceType type;
  AVBufferRef *device_ref;
};

/* select an input stream for an output stream */
struct StreamMap {
  int disabled; /* 1 is this mapping is disabled by a negative map */
  int file_index;
  int stream_index;
  int sync_file_index;
  int sync_stream_index;
  char *linklabel; /* name of an output link, for mapping lavfi outputs */
};

struct AudioChannelMap {
  // input
  int file_idx;
  int stream_idx;
  int channel_idx;
  // output
  int ofile_idx;
  int ostream_idx;
};

struct InputStream;
struct FilterGraph;

struct InputFilter {
  AVFilterContext *filter;
  InputStream *ist;
  FilterGraph *graph;
  uint8_t *name;
  AVMediaType type;  // AVMEDIA_TYPE_SUBTITLE for sub2video

  AVFifo *frame_queue;

  // parameters configured for this input
  int format;

  int width;
  int height;
  AVRational sample_aspect_ratio;

  int sample_rate;
  AVChannelLayout ch_layout;

  AVBufferRef *hw_frames_ctx;
  int32_t *displaymatrix;

  bool eof;
};

struct OutputStream;
struct FilterGraph;
struct OutputFilter {
  AVFilterContext *filter;
  OutputStream *ost;
  FilterGraph *graph;
  uint8_t *name;

  /* temporary storage until stream maps are processed */
  AVFilterInOut *out_tmp;
  AVMediaType type;

  /* desired output stream properties */
  int width, height;
  AVRational frame_rate;
  int format;
  int sample_rate;
  AVChannelLayout ch_layout;

  // those are only set if no format is specified and the encoder gives us
  // multiple options They point directly to the relevant lists of the encoder.
  const int *formats;
  const AVChannelLayout *ch_layouts;
  const int *sample_rates;
};

struct FilterGraph {
  int index;
  const char *graph_desc;

  AVFilterGraph *graph;
  int reconfiguration;
  // true when the filtergraph contains only meta filters
  // that do not modify the frame data
  bool is_meta;

  InputFilter **inputs;
  int nb_inputs;
  OutputFilter **outputs;
  int nb_outputs;
};

struct InputStream {
  int file_index;
  AVStream *st;
  bool discard; /* true if stream data should be discarded */
  AVDiscard user_set_discard;
  int decoding_needed; /* non zero if the packets must be decoded in 'raw_fifo',
                          see DECODING_FOR_* */
#define DECODING_FOR_OST 1
#define DECODING_FOR_FILTER 2
  bool processing_needed; /* non zero if the packets must be processed */

  AVCodecContext *dec_ctx;
  const AVCodec *dec;
  AVFrame *decoded_frame;
  AVPacket *pkt;

  int64_t prev_pkt_pts;
  int64_t start; /* time when read started */
  /* predicted dts of the next packet read for this stream or (when there are
   * several frames in a packet) of the next frame in current packet (in
   * AV_TIME_BASE units) */
  int64_t next_dts;
  int64_t first_dts;  ///< dts of the first packet read for this stream (in
                      ///< AV_TIME_BASE units)
  int64_t dts;        ///< dts of the last packet read for this stream (in
                      ///< AV_TIME_BASE units)

  int64_t next_pts;  ///< synthetic pts for the next decode frame (in
                     ///< AV_TIME_BASE units)
  int64_t pts;  ///< current pts of the decoded frame  (in AV_TIME_BASE units)
  bool wrap_correction_done;

  int64_t filter_in_rescale_delta_last;

  int64_t min_pts; /* pts with the smallest value in a current stream */
  int64_t max_pts; /* pts with the higher value in a current stream */

  // when forcing constant input framerate through -r,
  // this contains the pts that will be given to the next decoded frame
  int64_t cfr_next_pts;

  int64_t nb_samples; /* number of samples in the last decoded audio frame
                         before looping */

  double ts_scale;
  int saw_first_ts;
  AVDictionary *decoder_opts;
  AVRational framerate; /* framerate forced with -r */
  int top_field_first;  // top=1/bottom=0/auto=-1 field first
  int guess_layout_max;

  bool autorotate;

  bool fix_sub_duration;
  struct { /* previous decoded subtitle and related variables */
    bool got_output;
    int ret;
    AVSubtitle subtitle;
  } prev_sub;

  struct sub2video {
    int64_t last_pts;
    int64_t end_pts;
    AVFifo *sub_queue;  ///< queue of AVSubtitle* before filter init
    AVFrame *frame;
    int w;
    int h;
    bool initialize;  ///< marks if sub2video_update should force an
                      ///< initialization
  } sub2video;

  /* decoded data from this stream goes into all those filters
   * currently video and audio only */
  InputFilter **filters;
  int nb_filters;

  int reinit_filters;

  /* hwaccel options */
  HWAccelID hwaccel_id;
  AVHWDeviceType hwaccel_device_type;
  char *hwaccel_device;
  AVPixelFormat hwaccel_output_format;

  /* hwaccel context */
  void (*hwaccel_uninit)(AVCodecContext *s);
  int (*hwaccel_retrieve_data)(AVCodecContext *s, AVFrame *frame);
  AVPixelFormat hwaccel_pix_fmt;
  AVPixelFormat hwaccel_retrieved_pix_fmt;

  /* stats */
  // combined size of all the packets read
  uint64_t data_size;
  /* number of packets successfully read for this stream */
  uint64_t nb_packets;
  // number of frames/samples retrieved from the decoder
  uint64_t frames_decoded;
  uint64_t samples_decoded;

  int64_t *dts_buffer;
  int nb_dts_buffer;

  bool got_output;
};

struct InputFile {
  AVFormatContext *ctx;
  bool eof_reached;     /* true if eof reached */
  bool eagain;          /* true if last read attempt returned EAGAIN */
  int ist_index;        /* index of first stream in input_streams */
  int loop;             /* set number of times input stream should be looped */
  int64_t duration;     /* actual duration of the longest stream in a file
                           at the moment when looping happens */
  AVRational time_base; /* time base of the duration */
  int64_t input_ts_offset;
  int input_sync_ref;

  int64_t ts_offset;
  int64_t last_ts;
  int64_t start_time; /* user-specified start time in AV_TIME_BASE or
                         AV_NOPTS_VALUE */
  int64_t recording_time;
  int nb_streams; /* number of stream that ffmpeg is aware of; may be different
                     from ctx.nb_streams if new streams appear during
                     av_read_frame() */
  int nb_streams_warn; /* number of streams that the user was warned of */
  bool rate_emu;
  float readrate;
  bool accurate_seek;

  AVPacket *pkt;
};

enum forced_keyframes_const {
  FKF_N,
  FKF_N_FORCED,
  FKF_PREV_FORCED_N,
  FKF_PREV_FORCED_T,
  FKF_T,
  FKF_NB
};

#define ABORT_ON_FLAG_EMPTY_OUTPUT (1 << 0)
#define ABORT_ON_FLAG_EMPTY_OUTPUT_STREAM (1 << 1)

enum OSTFinished {
  ENCODER_FINISHED = 1,
  MUXER_FINISHED = 2,
};

struct OutputStream {
  int file_index;             /* file index */
  int index;                  /* stream index in the output file */
  int source_index;           /* InputStream index */
  AVStream *st;               /* stream in the output file */
  bool encoding_needed{true}; /* true if encoding needed for this stream */
  int64_t frame_number;
  /* input pts and corresponding output pts
     for A/V sync */
  InputStream *sync_ist; /* input stream to sync against */
  int64_t sync_opts;
  /* output frame counter, could be changed to some true timestamp */  // FIXME
                                                                       // look
                                                                       // at
                                                                       // frame_number
  /* pts of the first frame encoded for this stream, used for limiting
   * recording time */
  int64_t first_pts;
  /* dts of the last packet sent to the muxer */
  int64_t last_mux_dts;
  // the timebase of the packets sent to the muxer
  AVRational mux_timebase;
  AVRational enc_timebase;

  AVBSFContext *bsf_ctx;

  AVCodecContext *enc_ctx;
  AVCodecParameters *ref_par; /* associated input codec parameters with encoders
                                 options applied */
  const AVCodec *enc;
  int64_t max_frames;
  AVFrame *filtered_frame;
  AVFrame *last_frame;
  AVPacket *pkt;
  int64_t last_dropped;
  int64_t last_nb0_frames[3];

  /* video only */
  AVRational frame_rate;
  AVRational max_frame_rate;
  VideoSyncMethod vsync_method;
  bool is_cfr;
  const char *fps_mode;
  bool force_fps;
  int top_field_first;  // top=1/bottom=0/auto=-1 field first
  bool rotate_overridden;
  bool autoscale;
  int bits_per_raw_sample;
  double rotate_override_value;

  AVRational frame_aspect_ratio;

  /* forced key frames */
  int64_t forced_kf_ref_pts;
  int64_t *forced_kf_pts;
  int forced_kf_count;
  int forced_kf_index;
  char *forced_keyframes;
  AVExpr *forced_keyframes_pexpr;
  double forced_keyframes_expr_const_values[FKF_NB];
  bool dropped_keyframe;

  /* audio only */
  int *audio_channels_map;   /* list of the channels id to pick from the source
                                stream */
  int audio_channels_mapped; /* number of channels in audio_channels_map */

  char *logfile_prefix{nullptr};
  FILE *logfile{nullptr};

  OutputFilter *filter;
  char *avfilter;
  char *filters;         ///< filtergraph associated to the -filter option
  char *filters_script;  ///< filtergraph script associated to the
                         ///< -filter_script option

  AVDictionary *encoder_opts;
  AVDictionary *sws_dict;
  AVDictionary *swr_opts;
  char *apad;
  OSTFinished finished; /* no more packets should be written for this stream */
  bool unavailable; /* true if the steram is unavailable (possibly temporarily)
                     */
  bool stream_copy{false};

  // init_output_stream() has been called for this stream
  // The encoder and the bitstream filters have been initialized and the stream
  // parameters are set in the AVStream.
  bool initialized{false};

  bool inputs_done{false};

  const char *attachment_filename;
  bool streamcopy_started{false};
  bool copy_initial_nonkeyframes{false};
  int copy_prior_start;
  char *disposition;

  bool keep_pix_fmt;

  /* stats */
  // combined size of all the packets written
  uint64_t data_size;
  // number of packets send to the muxer
  uint64_t packets_written;
  // number of frames/samples sent to the encoder
  uint64_t frames_encoded;
  uint64_t samples_encoded;
  // number of packets received from the encoder
  uint64_t packets_encoded;

  /* packet quality factor */
  int quality;

  int max_muxing_queue_size;

  /* the packets are buffered here until the muxer is ready to be initialized */
  AVFifo *muxing_queue;

  /*
   * The size of the AVPackets' buffers in queue.
   * Updated when a packet is either pushed or pulled from the queue.
   */
  size_t muxing_queue_data_size;

  /* Threshold after which max_muxing_queue_size will be in effect */
  size_t muxing_queue_data_threshold;

  /* packet picture type */
  AVPictureType pict_type{AV_PICTURE_TYPE_NONE};

  /* frame encode sum of squared error values */
  int64_t error[4];
};

struct OutputFile {
  int index;

  const AVOutputFormat *format;

  AVFormatContext *ctx;
  AVDictionary *opts;
  int ost_index;           /* index of the first stream in output_streams */
  int64_t recording_time;  ///< desired length of the resulting file in
                           ///< microseconds == AV_TIME_BASE units
  int64_t start_time;      ///< start time in microseconds == AV_TIME_BASE units
  uint64_t limit_filesize; /* filesize limit expressed in bytes */

  bool shortest;

  bool header_written;
};

class FfmpegVideoConverter : public BaseVideoConverter {
 public:
  static std::vector<std::string> AvailableVideoEncoders();
  static std::vector<std::string> AvailableAudioEncoders();
  static std::vector<std::string> AvailableFileFormats();

 public:
  FfmpegVideoConverter(const VideoConverter::Request &request,
                       VideoConverter::Delegate *delegate,
                       VideoConverter::AsyncCallFuncType call_fun);
  ~FfmpegVideoConverter() override;

  void Start() override;
  void Stop() override;

 private:
  bool Convert(const std::string &input, const std::string &output);

 private:
  bool OpenInputFile();
  bool ApplySyncOffsets();
  bool InitComplexFilters();
  bool OpenOutputFile();

 private:
  // input file
  int open_input_file(const char *filename);
  bool add_input_streams(AVFormatContext *ic);
  bool check_output_constraints(InputStream *ist, OutputStream *ost);
  bool do_streamcopy(InputStream *ist, OutputStream *ost, const AVPacket *pkt);
  bool guess_input_channel_layout(InputStream *ist);

  bool apply_sync_offsets(void);

  // filter
  bool init_complex_filters(void);

  // output file
  int open_output_file(const char *filename);
  bool init_output_filter(OutputFilter *ofilter, AVFormatContext *oc);
  OutputStream *new_video_stream(AVFormatContext *oc, int source_index);
  OutputStream *new_audio_stream(AVFormatContext *oc, int source_index);
  OutputStream *new_output_stream(AVFormatContext *oc, AVMediaType type,
                                  int source_index);
  OutputStream *new_data_stream(AVFormatContext *oc, int source_index);
  OutputStream *new_unknown_stream(AVFormatContext *oc, int source_index);
  OutputStream *new_attachment_stream(AVFormatContext *oc, int source_index);
  OutputStream *new_subtitle_stream(AVFormatContext *oc, int source_index);
  const AVCodec *choose_decoder(AVFormatContext *s, AVStream *st);
  const AVCodec *find_codec_or_die(const char *name, enum AVMediaType type,
                                   bool encoder);
  bool choose_encoder(AVFormatContext *s, OutputStream *ost);
  int get_preset_file_2(const char *preset_name, const char *codec_name,
                        AVIOContext **s);
  char *get_line(AVIOContext *s, AVBPrint *bprint);
  bool parse_matrix_coeffs(uint16_t *dest, const char *str);
  bool parse_and_set_vsync(const char *arg, int *vsync_var, int file_idx,
                           int st_idx, int is_global);

  char *get_ost_filters(AVFormatContext *oc, OutputStream *ost);
  bool check_streamcopy_filters(AVFormatContext *oc, const OutputStream *ost,
                                AVMediaType type);
  int set_dispositions(OutputFile *of, AVFormatContext *ctx);
  bool set_channel_layout(OutputFilter *f, OutputStream *ost);

  bool parse_forced_key_frames(char *kf, OutputStream *ost,
                               AVCodecContext *avctx);

  // transcode
  void update_benchmark(const char *fmt, ...);
  void close_output_stream(OutputStream *ost);
  bool output_packet(OutputFile *of, AVPacket *pkt, OutputStream *ost,
                     bool eof);
  int need_output(void);
  OutputStream *choose_output(void);
  int transcode(void);
  bool transcode_init(void);
  int init_input_stream(int ist_index, char *error, int error_len);
  int init_output_stream_wrapper(OutputStream *ost, AVFrame *frame,
                                 unsigned int fatal);
  InputStream *get_input_stream(OutputStream *ost);
  int init_output_stream_streamcopy(OutputStream *ost);
  bool set_encoder_id(OutputFile *of, OutputStream *ost);
  void init_encoder_time_base(OutputStream *ost, AVRational default_time_base);
  int init_output_stream_encode(OutputStream *ost, AVFrame *frame);
  void report_new_stream(int input_index, AVPacket *pkt);
  bool check_recording_time(OutputStream *ost);
  double adjust_frame_pts_to_encoder_tb(OutputFile *of, OutputStream *ost,
                                        AVFrame *frame);
  int init_output_stream(OutputStream *ost, AVFrame *frame, char *error,
                         int error_len);
  AVRational duration_max(int64_t tmp, int64_t *duration,
                          AVRational tmp_time_base, AVRational time_base);
  int seek_to_start(InputFile *ifile, AVFormatContext *is);
  int process_input(int file_index);
  int ifilter_send_frame(InputFilter *ifilter, AVFrame *frame,
                         bool keep_reference);
  int ifilter_send_eof(InputFilter *ifilter, int64_t pts);
  int ifilter_parameters_from_codecpar(InputFilter *ifilter,
                                       AVCodecParameters *par);
  int decode(AVCodecContext *avctx, AVFrame *frame, bool *got_frame,
             AVPacket *pkt);
  int send_frame_to_filters(InputStream *ist, AVFrame *decoded_frame);
  int decode_audio(InputStream *ist, AVPacket *pkt, bool *got_output,
                   bool *decode_failed);
  int decode_video(InputStream *ist, AVPacket *pkt, bool *got_output,
                   int64_t *duration_pts, bool eof, bool *decode_failed);
  int transcode_subtitles(InputStream *ist, AVPacket *pkt, bool *got_output,
                          bool *decode_failed);
  int send_filter_eof(InputStream *ist);
  int process_input_packet(InputStream *ist, const AVPacket *pkt, int no_eof);
  bool do_audio_out(OutputFile *of, OutputStream *ost, AVFrame *frame);
  bool do_subtitle_out(OutputFile *of, OutputStream *ost, AVSubtitle *sub);
  bool do_video_out(OutputFile *of, OutputStream *ost, AVFrame *next_picture);
  void finish_output_stream(OutputStream *ost);
  int reap_filters(int flush);
  int transcode_from_filter(FilterGraph *graph, InputStream **best_ist);
  int transcode_step(void);
  void sub2video_heartbeat(InputStream *ist, int64_t pts);
  void sub2video_flush(InputStream *ist);
  bool check_decode_result(InputStream *ist, bool *got_output, int ret);
  bool ifilter_has_all_input_formats(FilterGraph *fg);
  int get_input_packet(InputFile *f, AVPacket **pkt);
  bool got_eagain();
  void reset_eagain();
  bool flush_encoders(void);
  bool update_video_stats(OutputStream *ost, const AVPacket *pkt,
                          int write_vstats);
  int encode_frame(OutputFile *of, OutputStream *ost, AVFrame *frame);

  // hw
  HWDevice *hw_device_get_by_name(const char *name);

  // mux
  void close_all_output_streams(OutputStream *ost, OSTFinished this_stream,
                                OSTFinished others);
  bool of_write_packet(OutputFile *of, AVPacket *pkt, OutputStream *ost,
                       int unqueue);
  int print_sdp(void);
  bool of_check_init(OutputFile *of);
  int of_write_trailer(OutputFile *of);
  void of_close(OutputFile **pof);

  // filter
  const AVPixelFormat *get_compliance_normal_pix_fmts(
      const AVCodec *codec, const AVPixelFormat default_formats[]);
  AVPixelFormat choose_pixel_fmt(AVStream *st, AVCodecContext *enc_ctx,
                                 const AVCodec *codec, AVPixelFormat target);
  const char *choose_pix_fmts(OutputFilter *ofilter, AVBPrint *bprint);
  void choose_channel_layouts(OutputFilter *ofilter, AVBPrint *bprint);
  int init_simple_filtergraph(InputStream *ist, OutputStream *ost);
  char *describe_filter_link(FilterGraph *fg, AVFilterInOut *inout, int in);
  bool init_input_filter(FilterGraph *fg, AVFilterInOut *in);
  int init_complex_filtergraph(FilterGraph *fg);
  int insert_trim(int64_t start_time, int64_t duration,
                  AVFilterContext **last_filter, int *pad_idx,
                  const char *filter_name);
  int insert_filter(AVFilterContext **last_filter, int *pad_idx,
                    const char *filter_name, const char *args);
  int configure_output_video_filter(FilterGraph *fg, OutputFilter *ofilter,
                                    AVFilterInOut *out);
  int configure_output_audio_filter(FilterGraph *fg, OutputFilter *ofilter,
                                    AVFilterInOut *out);
  int configure_output_filter(FilterGraph *fg, OutputFilter *ofilter,
                              AVFilterInOut *out);
  bool check_filter_outputs(void);
  int sub2video_prepare(InputStream *ist, InputFilter *ifilter);
  int configure_input_video_filter(FilterGraph *fg, InputFilter *ifilter,
                                   AVFilterInOut *in);
  int configure_input_audio_filter(FilterGraph *fg, InputFilter *ifilter,
                                   AVFilterInOut *in);
  int configure_input_filter(FilterGraph *fg, InputFilter *ifilter,
                             AVFilterInOut *in);
  void cleanup_filtergraph(FilterGraph *fg);
  bool filter_is_buffersrc(const AVFilterContext *f);
  bool graph_is_meta(AVFilterGraph *graph);
  int configure_filtergraph(FilterGraph *fg);
  int ifilter_parameters_from_frame(InputFilter *ifilter, const AVFrame *frame);
  bool filtergraph_is_simple(FilterGraph *fg);
  void print_report(bool is_last_report, int64_t timer_start, int64_t cur_time);
  void print_final_stats(int64_t total_size);

  static int decode_interrupt_cb(void *ctx);
    
  void cleanup(bool succ);

 private:
  uint8_t *subtitle_out{nullptr};

  InputStream **input_streams{nullptr};
  int nb_input_streams{0};
  InputFile **input_files{nullptr};
  int nb_input_files{0};

  OutputStream **output_streams{nullptr};
  int nb_output_streams{0};
  OutputFile **output_files{nullptr};
  int nb_output_files{0};

  FilterGraph **filtergraphs{nullptr};
  int nb_filtergraphs{0};

  char *vstats_filename{nullptr};
  char *sdp_filename{nullptr};

  float audio_drift_threshold{0.1};
  float dts_delta_threshold{10};
  float dts_error_threshold{3600 * 30};

  int audio_volume{256};
  int audio_sync_method{0};
  VideoSyncMethod video_sync_method{VSYNC_AUTO};
  float frame_drop_threshold{0};
  bool do_benchmark{true};
  bool do_benchmark_all{false};
  int do_hex_dump{0};
  int do_pkt_dump{0};
  bool copy_ts{false};
  int start_at_zero{0};
  AVTimebaseSource copy_tb{AVFMT_TBCF_AUTO};
  bool debug_ts{false};
  bool exit_on_error{false};
  int abort_on_flags{0};
  int print_stats{-1};
  int64_t stats_period{500000};
  int qp_hist{0};
  AVIOContext *progress_avio{nullptr};
  float max_error_rate{2.0f / 3};

  char *filter_nbthreads{nullptr};
  int filter_complex_nbthreads{0};
  int vstats_version{2};
  int auto_conversion_filters{1};

  // TODO:用输入参数来支持option
  // const OptionDef options[];
  HWDevice *filter_hw_device;

  bool want_sdp{true};
  unsigned nb_output_dumped;
  int main_return_code;

  // 输入选项
  struct InputOption {
    ~InputOption();
    void Init();
    void UnInit();

    AVDictionary *format_opts{nullptr};
    AVDictionary *codec_opts{nullptr};

    /* input/output options */
    int64_t start_time{AV_NOPTS_VALUE};
    int64_t start_time_eof{AV_NOPTS_VALUE};  // value must be negative
    int seek_timestamp{0};
    std::string format;

    std::vector<CodecSpecifier> codec_names;
    int nb_audio_ch_layouts{0};
    int nb_audio_channels{0};
    int nb_audio_sample_rate{0};
    int nb_frame_rates{0};
    int nb_max_frame_rates{0};
    int nb_frame_sizes{0};
    int nb_frame_pix_fmts{0};

    int64_t input_ts_offset{0};
    int loop{0};
    bool rate_emu{false};
    float readrate{0};
    bool accurate_seek{true};
    int thread_queue_size{-1};
    int input_sync_ref{-1};

    int nb_ts_scale{0};
    int nb_dump_attachment{0};
    int nb_hwaccels{0};
    int nb_hwaccel_devices{0};
    int nb_hwaccel_output_formats{0};
    int nb_autorotate{0};

    int64_t recording_time{INT64_MAX};
    int64_t stop_time{INT64_MAX};
    uint64_t limit_filesize{UINT64_MAX};
    float mux_preload{0};
    float mux_max_delay{0.7};
    bool shortest{false};
    int bitexact{0};

    //
    bool video_disable{false};
    bool audio_disable{false};
    bool subtitle_disable{false};
    bool data_disable{false};

    //
    int64_t threads{0};
  } io;

  struct {
    /* indexed by output file stream index */
    int *streamid_map{nullptr};
    int nb_streamid_map{0};
    int nb_metadata{0};
    int nb_presets{0};
    int nb_reinit_filters{0};
    int nb_discard{0};
    // top=1/bottom=0/auto=-1 field first
    int top_field_first{1};
    int nb_top_field_first{0};
    int nb_hwaccels{0};
    int nb_hwaccel_devices{0};
    int nb_guess_layout_max{0};
    int nb_fix_sub_duration{0};
    int nb_canvas_sizes{0};
    int nb_autoscale{0};
    int nb_program{0};
    int nb_time_bases{0};
    int nb_enc_time_bases{0};
    int nb_max_frames{0};
    int nb_copy_prior_start{0};
    int nb_bitstream_filters{0};
    int nb_codec_tags{0};
    int nb_qscale{0};
    int nb_disposition{0};
    int nb_max_muxing_queue_size{0};
    int nb_muxing_queue_data_threshold{0};
    int nb_bits_per_raw_sample{0};
    int nb_copy_initial_nonkeyframes{0};
    int nb_metadata_map{0};
    bool recast_media{false};
  } o;

  struct OutputOption {
    ~OutputOption();
    void Init();
    void UnInit();

    AVDictionary *format_opts{nullptr};
    AVDictionary *codec_opts{nullptr};  // 设置码率等
    AVDictionary *sws_dict{nullptr};
    AVDictionary *swr_opts{nullptr};

    /* input/output options */
    int64_t start_time{AV_NOPTS_VALUE};  // microsecond
    int seek_timestam;
    std::string format{"mp4"};

    std::vector<CodecSpecifier> codec_names{
        {kVideoStream, -1, "h264"},
        {kAudioStream, -1, "copy"},
    };
    int nb_audio_ch_layouts{0};
    int nb_audio_channels{0};
    int nb_audio_sample_rate{0};
    int nb_frame_rates{0};
    int nb_max_frame_rates{0};
    int nb_frame_sizes{0};
    int nb_frame_pix_fmts{0};

    /* output options */
    StreamMap *stream_maps{nullptr};
    int nb_stream_maps{0};
    AudioChannelMap *audio_channel_maps{
        nullptr};                 /* one info entry per -map_channel */
    int nb_audio_channel_maps{0}; /* number of (valid) -map_channel settings */
    int metadata_global_manual{0};
    int metadata_streams_manual{0};
    int metadata_chapters_manual{0};
    const char **attachments{0};
    int nb_attachments{0};

    int chapters_input_file{INT_MAX};

    int64_t recording_time{INT64_MAX};  // microsecond
    int64_t stop_time{INT64_MAX};       // microsecond
    uint64_t limit_filesize{UINT64_MAX};
    float mux_preload{0};
    float mux_max_delay{0.7};
    bool shortest{false};
    int bitexact{0};

    //
    bool video_disable{false};
    bool audio_disable{false};
    bool subtitle_disable{false};
    bool data_disable{false};

    // video options
    bool autorotate{true};
    bool autoscale{true};

    // Set the number of video frames to output.
    int64_t max_frames{INT64_MAX};
    // Set frame rate (Hz value, fraction or abbreviation).
    // As an output option, duplicate or drop input frames to achieve constant
    // output frame rate fps.
    std::string frame_rate;
    // Set maximum frame rate (Hz value, fraction or abbreviation).
    std::string max_frame_rate;
    // Set the video display aspect ratio specified by aspect.
    // For example "4:3", "16:9", "1.3333", and "1.7777" are valid argument
    // values.
    std::string frame_aspect_ratio;
    // Set video sync method / framerate mode.
    std::string fps_mode;  // 'passthrough' 'cfr' 'vfr' 'drop' 'auto'

    //
    std::string vbitrate;
    std::string abitrate;
    int64_t threads{0};

    // video size
    std::string video_filters;  // scale=1920:-1 width or height
  } oo;

  bool do_psnr{false};
  bool input_stream_potentially_available{false};
  bool ignore_unknown_streams{false};
  bool copy_unknown_streams{false};
  bool find_stream_info{true};
  std::atomic_bool received_sigterm{false};

  int64_t nb_frames_dup{0};
  uint64_t dup_warning{1000};
  int64_t nb_frames_drop{0};
  int64_t decode_error_stat[2]{0};

  FILE *vstats_file{nullptr};

  struct BenchmarkTimeStamps {
    int64_t real_usec;
    int64_t user_usec;
    int64_t sys_usec;
  } current_time;
  // status
  BenchmarkTimeStamps get_benchmark_time_stamps(void);

  //
  std::unique_ptr<std::thread> worker_;
  static void Run(void *converter);
};
