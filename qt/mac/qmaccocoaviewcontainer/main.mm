#include <AppKit/AppKit.h>
#include <QtWidgets>
#include <QMacCocoaViewContainer>

class WindowWidget : public QWidget
{
public:
    WindowWidget()
    {
        QMacCocoaViewContainer *cocoaViewContainer = new QMacCocoaViewContainer(0, this);
        NSTextView *text = [[NSTextView alloc] init];
        cocoaViewContainer->setCocoaView(text);
        
        QVBoxLayout* main_layout = new QVBoxLayout(this);
        main_layout->addWidget(cocoaViewContainer);
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    WindowWidget widget;
    widget.setMinimumSize({800, 600});
    widget.show();
    
    return app.exec();
}
