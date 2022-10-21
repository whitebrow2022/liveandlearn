// Created by liangxu on 2022/10/21.
//
// Copyright (c) 2022 The OpencvWrapper Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "opencv_wrapper/opencv_wrapper.h"

#include <opencv2/core/types_c.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/videoio/legacy/constants_c.h>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <opencv2/imgcodecs.hpp>

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

void ExtractFrames(const std::string &video_file_path, std::vector<cv::Mat>& frames) {
  try{
    
    //open the video file
    cv::VideoCapture cap(video_file_path); // open the video file
    if(!cap.isOpened())  // check if we succeeded
      CV_Error(CV_StsError, "Can not open Video file");

    //cap.get(CV_CAP_PROP_FRAME_COUNT) contains the number of frames in the video;
    for(int frameNum = 0; frameNum < cap.get(CV_CAP_PROP_FRAME_COUNT);frameNum++)
    {
      cv::Mat frame;
      cap >> frame; // get the next frame from video
      frames.push_back(frame);
    }
  }
  catch( cv::Exception& e ){
    //std::cerr << e.msg << endl;
    //exit(1);
  }
}

void SaveFrames(std::vector<cv::Mat>& frames, const std::string& outputDir){
  std::vector<int> compression_params;
  compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
  compression_params.push_back(100);

  long long frameNumber = 0;
  for(std::vector<cv::Mat>::iterator frame = frames.begin(); frame != frames.end(); ++frame){
    std::string filePath = outputDir + "/opencv_frame" + std::to_string(static_cast<long long>(frameNumber))+ ".jpg";
      cv::InputArray frameArr(*frame);
      if (!frameArr.empty()) {
    imwrite(filePath, frameArr, compression_params);
      }
      frameNumber++;
  }  
}

OPENCV_WRAPPER_API void ExtractVideoToFrames(const char* video_file_path, const char* output_dir) {
  std::vector<cv::Mat> frames;
  ExtractFrames(video_file_path, frames);
  SaveFrames(frames, output_dir);
}

END_NAMESPACE_OPENCV_WRAPPER
