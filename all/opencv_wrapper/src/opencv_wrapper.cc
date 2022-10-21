// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "opencv_wrapper/opencv_wrapper.h"

#include <opencv2/core/types_c.h>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <opencv2/videoio/legacy/constants_c.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

BEGIN_NAMESPACE_OPENCV_WRAPPER

OPENCV_WRAPPER_API const char* GetVersion() { return "1.0.0.001"; }
OPENCV_WRAPPER_API const char* GetOsName() {
#if defined(OS_WINDOWS)
  return "Windows";
#elif defined(OS_ANDROID)
  return "Android";
#else
  return "Unknown system type";
#endif
}
OPENCV_WRAPPER_API const char* GetName() { return "OpencvWrapper"; }

void ExtractFrames(const std::string& video_file_path,
                   std::vector<cv::Mat>* frames) {
  if (!frames) {
    return;
  }
  try {
    // open the video file
    cv::VideoCapture cap(video_file_path);  // open the video file
    if (!cap.isOpened())                    // check if we succeeded
      CV_Error(CV_StsError, "Can not open Video file");

    // cap.get(CV_CAP_PROP_FRAME_COUNT) contains the number of frames in the
    // video;
    for (int frameNum = 0; frameNum < cap.get(CV_CAP_PROP_FRAME_COUNT);
         frameNum++) {
      cv::Mat frame;
      cap >> frame;  // get the next frame from video
      frames->push_back(frame);
    }
  } catch (cv::Exception& e) {
    // std::cerr << e.msg << endl;
    // exit(1);
  }
}

void SaveFrames(const std::vector<cv::Mat>& frames,
                const std::string& outputDir) {
  std::vector<int> compression_params;
  compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
  compression_params.push_back(100);

  uint64_t frameNumber = 0;
  for (std::vector<cv::Mat>::const_iterator frame = frames.begin();
       frame != frames.end(); ++frame) {
    std::string filePath = outputDir + "/frame_" +
                           std::to_string(static_cast<uint64_t>(frameNumber)) +
                           ".jpg";
    cv::InputArray frameArr(*frame);
    if (!frameArr.empty()) {
      cv::imwrite(filePath, frameArr, compression_params);
    }
    frameNumber++;
  }
}

OPENCV_WRAPPER_API void ExtractVideoToFrames(const char* video_file_path,
                                             const char* output_dir) {
  std::vector<cv::Mat> frames;
  ExtractFrames(video_file_path, &frames);
  SaveFrames(frames, output_dir);
}

OPENCV_WRAPPER_API bool ExtractVideoFirstValidFrameToImage(
    const char* video_file_path, const char* image_file_path) {
  // open the video file
  cv::VideoCapture cap(video_file_path);  // open the video file
  if (!cap.isOpened()) {                  // check if we succeeded
    CV_Error(CV_StsError, "Can not open Video file");
    return false;
  }

  std::vector<int> compression_params;
  compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
  compression_params.push_back(100);

  // cap.get(CV_CAP_PROP_FRAME_COUNT) contains the number of frames in the
  // video;
  for (int frame_num = 0; frame_num < cap.get(CV_CAP_PROP_FRAME_COUNT);
       frame_num++) {
    cv::Mat frame;
    cap >> frame;  // get the next frame from video
    cv::InputArray frame_arr(frame);
    if (!frame_arr.empty()) {
      cv::imwrite(image_file_path, frame_arr, compression_params);
      break;
    }
  }
  return true;
}

std::unique_ptr<CvImage> ConvertCvMatToCvImage(const cv::Mat& frame) {
  if (frame.empty()) {
    return nullptr;
  }
  std::unique_ptr<CvImage> cvimage =
      std::make_unique<CvImage>(frame.cols, frame.rows);
  for (int y = 0; y < frame.rows; ++y) {
    for (int x = 0; x < frame.cols; ++x) {
      cv::Vec3b col = frame.at<cv::Vec3b>(cv::Point(x, y));
      cvimage->SetColor(x, y, col[2], col[1], col[0], 255);
    }
  }
  return cvimage;
}

OPENCV_WRAPPER_API std::unique_ptr<CvImage>
ExtractVideoFirstValidFrameToImageBuffer(const char* video_file_path,
                                         unsigned int* duration) {
  // https://stackoverflow.com/questions/53552698/opencv-videocapture-removes-alpha-channel-from-video
  // Unfortunately, OpenCV does not supports fetching video frames with Alpha
  // channel.
  std::unique_ptr<CvImage> image_buf;
  // open the video file
  cv::VideoCapture cap(video_file_path);  // open the video file
  if (!cap.isOpened()) {                  // check if we succeeded
    CV_Error(CV_StsError, "Can not open Video file");
    return image_buf;
  }

  std::vector<int> compression_params;
  compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
  compression_params.push_back(100);

  // cap.get(CV_CAP_PROP_FRAME_COUNT) contains the number of frames in the
  // video;
  for (int frame_num = 0; frame_num < cap.get(CV_CAP_PROP_FRAME_COUNT);
       frame_num++) {
    cv::Mat frame;
    cap >> frame;  // get the next frame from video
    cv::InputArray frame_arr(frame);
    if (!frame_arr.empty()) {
      image_buf = ConvertCvMatToCvImage(frame);
      break;
    }
  }

  double fps = cap.get(CV_CAP_PROP_FPS);
  double frame_count = cap.get(CV_CAP_PROP_FRAME_COUNT);
  *duration = static_cast<unsigned int>(frame_count / fps);
  return image_buf;
}

OPENCV_WRAPPER_API bool ConvertVideoToMp4(const char* src_file_path,
                                          const char* dest_file_path) {
  cv::VideoCapture cap(src_file_path);  // open the video file
  if (!cap.isOpened()) {                // check if we succeeded
    CV_Error(CV_StsError, "Can not open Video file");
    return false;
  }
  int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
  int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
  cv::Size video_sz(width, height);
  double fps = cap.get(CV_CAP_PROP_FPS);

#if defined(_WIN32)
  int fourcc = cv::VideoWriter::fourcc('M', 'P', '4', 'V');
#else
  // TODO: need to fix 'mp4v'
  int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
#endif
  cv::VideoWriter output_video;
  if (output_video.open(dest_file_path, fourcc, fps, video_sz)) {
    // cap.get(CV_CAP_PROP_FRAME_COUNT) contains the number of frames in the
    // video;
    for (int frame_num = 0; frame_num < cap.get(CV_CAP_PROP_FRAME_COUNT);
         frame_num++) {
      cv::Mat frame;
      cap >> frame;  // get the next frame from video
      output_video << frame;
    }
    output_video.release();
  }
  cap.release();
  return true;
}

END_NAMESPACE_OPENCV_WRAPPER
