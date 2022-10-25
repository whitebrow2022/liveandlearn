#include "video_converter_dialog.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QUrl>

#include "ffmpeg_wrapper/video_converter.h"
#include "ffmpeg_wrapper/video_info_capture.h"
#include "ui_video_converter_dialog.h"

namespace {
QImage ConvertCvImageToQImage(const VideoInfoCapture::Image& cvimage,
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
}  // namespace

VideoConverterDialog::VideoConverterDialog(QWidget* parent)
    : QDialog(parent), ui_(new Ui::VideoConverterDialog) {
  ui_->setupUi(this);
  connect(ui_->inputFilePath, &QLineEdit::textChanged, this,
          &VideoConverterDialog::OnInputFileChanged);
  connect(ui_->inputFileButton, &QPushButton::clicked, this,
          &VideoConverterDialog::OnInputFileBrowserClicked);

  auto file_formats = VideoConverter::AvailableFileFormats();
  int index = 0;
  for (const auto& file_format : file_formats) {
    ui_->fileFormatCombo->addItem(file_format.c_str());
    if (file_format == "mp4") {
      ui_->fileFormatCombo->setCurrentIndex(index);
    }
    index++;
  }

  auto vencoders = VideoConverter::AvailableVideoEncoders();
  index = 0;
  for (const auto& vencoder : vencoders) {
    ui_->videoEncoderCombo->addItem(vencoder.c_str());
    if (vencoder == "h264") {
      ui_->videoEncoderCombo->setCurrentIndex(index);
    }
    index++;
  }

  auto aencoders = VideoConverter::AvailableAudioEncoders();
  index = 0;
  for (const auto& aencoders : aencoders) {
    ui_->audioEncoderCombo->addItem(aencoders.c_str());
    if (aencoders == "aac") {
      ui_->audioEncoderCombo->setCurrentIndex(index);
    }
    index++;
  }
}

VideoConverterDialog::~VideoConverterDialog() { delete ui_; }

void VideoConverterDialog::SetLastInputFile(const QString& inputfile) {
  ui_->inputFilePath->setText(inputfile);
}
QString VideoConverterDialog::GetLastInputFile() const {
  return ui_->inputFilePath->text();
}
QString VideoConverterDialog::GetLastFileFormat() const {
  return ui_->fileFormatCombo->currentText();
}
QString VideoConverterDialog::GetLastVideoEncoder() const {
  return ui_->videoEncoderCombo->currentText();
}
QString VideoConverterDialog::GetLastAudioEncoder() const {
  return ui_->audioEncoderCombo->currentText();
}

int VideoConverterDialog::GetSelectedWidth() const {
  return ui_->videoWidth->text().toInt();
}
int VideoConverterDialog::GetSelectedHeight() const {
  return ui_->videoWidth->text().toInt();
}

uint32_t VideoConverterDialog::GetVideoStartTime() const {
  return ui_->videoStartTime->text().toInt();
}
uint32_t VideoConverterDialog::GetVideoRecordTime() const {
  return ui_->videoRecordTime->text().toInt();
}

QString VideoConverterDialog::GetVideoBitrate() const {
  return ui_->videoBitrateCombo->currentText();
}
QString VideoConverterDialog::GetAudioBitrate() const {
  return ui_->audioBitrateCombo->currentText();
}

void VideoConverterDialog::OnInputFileChanged(const QString& inputfile) {
  QFileInfo file_info(inputfile);
  if (!file_info.exists() && file_info.isFile()) {
    return;
  }
  unsigned int duration;
  auto cvimage = VideoInfoCapture::ExtractVideoFirstValidFrameToImageBuffer(
      inputfile.toStdString().c_str(), &duration);
  if (!cvimage) {
    return;
  }

  QImage qimage = ConvertCvImageToQImage(*cvimage, 0.1);
  ui_->imageHolder->setPixmap(QPixmap::fromImage(qimage));
  ui_->imageHolder->setMaximumSize(800, 600);

  if (cvimage) {
    const uint32_t dura_ms = duration * 1000;
    ui_->videoWidth->setValidator(
        new QIntValidator(0, cvimage->GetWidth(), this));
    ui_->videoHeight->setValidator(
        new QIntValidator(0, cvimage->GetHeight(), this));
    ui_->videoWidth->setText(QString("%1").arg(cvimage->GetWidth()));
    ui_->videoHeight->setText(QString("%1").arg(cvimage->GetHeight()));
    ui_->videoStartTime->setValidator(new QIntValidator(0, dura_ms, this));
    ui_->videoRecordTime->setValidator(new QIntValidator(0, dura_ms, this));
    ui_->videoStartTime->setText(QString("%1").arg(0));
    ui_->videoRecordTime->setText(QString("%1").arg(dura_ms));
  } else {
    ui_->videoWidth->setReadOnly(true);
    ui_->videoHeight->setReadOnly(true);
  }

  auto video_info =
      VideoInfoCapture::ExtractFileInfo(inputfile.toStdString().c_str());
  ui_->videoBitrateCombo->addItem("");
  for (const auto& vbitrate : video_info.video_stream_bitrates) {
    QString vbitratestr = QString("%1K").arg(vbitrate.second / 1000);
    ui_->videoBitrateCombo->addItem(vbitratestr);
  }
  ui_->audioBitrateCombo->addItem("");
  for (const auto& vbitrate : video_info.audio_stream_bitrates) {
    QString vbitratestr = QString("%1K").arg(vbitrate.second / 1000);
    ui_->audioBitrateCombo->addItem(vbitratestr);
  }
}

void VideoConverterDialog::OnInputFileBrowserClicked() {
  QFileDialog dialog(this);
  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setViewMode(QFileDialog::Detail);

  QStringList fileNames;
  if (dialog.exec()) {
    fileNames = dialog.selectedFiles();
  }

  if (!fileNames.isEmpty()) {
    SetLastInputFile(fileNames[0]);
  }
}
