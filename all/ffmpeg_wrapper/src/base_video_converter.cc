#include "base_video_converter.h"

void BaseVideoConverter::PostConvertBegin(VideoConverter::Response r) {
  if (async_call_fun_) {
    std::weak_ptr<BaseVideoConverter> converter = this->weak_from_this();
    async_call_fun_([converter, r]() {
      if (auto conv = converter.lock()) {
        // lock delegate
        conv->delegate_->OnConvertBegin(r);
      }
    });
  } else if (auto delegate = delegate_) {
    delegate->OnConvertBegin(r);
  }
}
void BaseVideoConverter::PostConvertProgress(VideoConverter::Response r) {
  if (async_call_fun_) {
    std::weak_ptr<BaseVideoConverter> converter = this->weak_from_this();
    async_call_fun_([converter, r]() {
      if (auto conv = converter.lock()) {
        // lock delegate
        conv->delegate_->OnConvertProgress(r);
      }
    });
  } else if (auto delegate = delegate_) {
    delegate->OnConvertProgress(r);
  }
}
void BaseVideoConverter::PostConvertEnd(VideoConverter::Response r) {
  if (async_call_fun_) {
    std::weak_ptr<BaseVideoConverter> converter = this->weak_from_this();
    async_call_fun_([converter, r]() {
      if (auto conv = converter.lock()) {
        // lock delegate
        conv->delegate_->OnConvertEnd(r);
      }
    });
  } else if (auto delegate = delegate_) {
    delegate->OnConvertEnd(r);
  }
}
