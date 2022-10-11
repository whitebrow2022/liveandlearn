#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class SharedlibNameFrame;
}
QT_END_NAMESPACE

class SharedlibNameFrame : public QMainWindow {
  Q_OBJECT

 public:
  SharedlibNameFrame(QWidget *parent = nullptr);
  ~SharedlibNameFrame();

 private slots:
  void PopWindow();
  void Exit();
  void About();

 private:
  Ui::SharedlibNameFrame *ui_;
};
