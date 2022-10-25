#ifndef VIDEO_CONVERTER_DIALOG_H
#define VIDEO_CONVERTER_DIALOG_H

#include <QDialog>

namespace Ui {
class VideoConverterDialog;
}

class VideoConverterDialog : public QDialog {
  Q_OBJECT

 public:
  explicit VideoConverterDialog(QWidget* parent = nullptr);
  ~VideoConverterDialog();

  void SetLastInputFile(const QString& inputfile);
  QString GetLastInputFile() const;
  QString GetLastFileFormat() const;
  QString GetLastVideoEncoder() const;
  QString GetLastAudioEncoder() const;
  int GetSelectedWidth() const;
  int GetSelectedHeight() const;
  uint32_t GetVideoStartTime() const;
  uint32_t GetVideoRecordTime() const;
  QString GetVideoBitrate() const;
  QString GetAudioBitrate() const;

 private slots:
  void OnInputFileChanged(const QString& inputfile);
  void OnInputFileBrowserClicked();

 private:
  Ui::VideoConverterDialog* ui_{nullptr};
};

#endif  // VIDEO_CONVERTER_DIALOG_H
