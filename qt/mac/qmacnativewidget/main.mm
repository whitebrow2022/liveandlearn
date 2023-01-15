#import <AppKit/AppKit.h>

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <QMacNativeWidget>

class RedWidget : public QWidget
{
public:
    RedWidget() {
        
    }
    
    void resizeEvent(QResizeEvent *)
    {
        qDebug() << "RedWidget::resize" << size();
    }
    
    void paintEvent(QPaintEvent *event)
    {
        QPainter p(this);
        Q_UNUSED(event);
        QRect rect(QPoint(0, 0), size());
        qDebug() << "Painting geometry" << rect;
        p.fillRect(rect, QColor(133, 50, 50));
    }
};

namespace {
int qtArgc = 0;
char **qtArgv;
QApplication *qtApp = 0;
}

@interface WindowCreator : NSObject <NSApplicationDelegate>
@end

@implementation WindowCreator

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationWillFinishLaunching", notification.name,  notification.userInfo.description);
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationDidFinishLaunching", notification.name,  notification.userInfo.description);
    
    // Qt widgets rely on a QApplication being alive somewhere
    qtApp = new QApplication(qtArgc, qtArgv);
    
    // Create the NSWindow
    NSRect frame = NSMakeRect(500, 500, 500, 500);
    NSWindow* window  = [[NSWindow alloc] initWithContentRect:frame
                                                    styleMask:NSWindowStyleMaskTitled |  NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                                      backing:NSBackingStoreBuffered
                                                        defer:NO];
    [window setTitle:@"NSWindow"];
    
    // Create widget hierarchy with QPushButton and QLineEdit
    QMacNativeWidget *nativeWidget = new QMacNativeWidget();
    // Get the NSView for QMacNativeWidget and set it as the content view for the NSWindow
    [window setContentView:nativeWidget->nativeView()];
    
    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addWidget(new QPushButton("Push", nativeWidget));
    hlayout->addWidget(new QLineEdit(nativeWidget));
    
    QVBoxLayout *vlayout = new QVBoxLayout();
    vlayout->addLayout(hlayout);
    
    RedWidget *redWidget = new RedWidget;
    vlayout->addWidget(redWidget);
    
    nativeWidget->setLayout(vlayout);
    
    // show() must be called on nativeWiget to get the widgets int he correct state.
    nativeWidget->show();
    
    // Show the NSWindow
    [window makeKeyAndOrderFront:NSApp];
}

- (void)applicationWillBecomeActive:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationWillBecomeActive", notification.name,  notification.userInfo.description);
}

- (void)applicationDidBecomeActive:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationDidBecomeActive", notification.name,  notification.userInfo.description);
}

- (void)applicationWillResignActive:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationWillResignActive", notification.name,  notification.userInfo.description);
}

- (void)applicationDidResignActive:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationDidResignActive", notification.name,  notification.userInfo.description);
}

- (void)applicationWillHide:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationWillHide", notification.name,  notification.userInfo.description);
}

- (void)applicationDidHide:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationDidHide", notification.name,  notification.userInfo.description);
}

- (void)applicationWillUnhide:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationWillUnhide", notification.name,  notification.userInfo.description);
}

- (void)applicationDidUnhide:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationDidUnhide", notification.name,  notification.userInfo.description);
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    NSLog(@"[NSApplicationDelegate] %@, %@, %@", @"applicationWillTerminate", notification.name,  notification.userInfo.description);
    
    delete qtApp;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    NSLog(@"[NSApplicationDelegate] %@", @"applicationShouldTerminate");
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    NSLog(@"[NSApplicationDelegate] %@", @"applicationShouldTerminateAfterLastWindowClosed");
    
    return YES;
}

@end

int main(int argc, char *argv[])
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    Q_UNUSED(pool);
    
    // Normally, we would use let the main bundle instanciate and set
    // the application delegate, but we set it manually for conciseness.
    WindowCreator *windowCreator= [WindowCreator alloc];
    [[NSApplication sharedApplication] setDelegate:windowCreator];
    
    // Save these for QApplication
    qtArgc = argc;
    qtArgv = argv;
    
    // Other than the few lines above, it's business as usual...
    return NSApplicationMain(argc, (const char **)argv);
}
