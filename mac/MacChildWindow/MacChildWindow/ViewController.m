//
//  ViewController.m
//  MacChildWindow
//
//  Created by 刘秀峰(Whitebrow.L) on 2023/1/5.
//

#import "ViewController.h"
#import "PoppedViewController.h"

@interface ViewController()

@property (nullable, strong) NSWindow *popedWindow;
@property (nullable, strong) NSWindow *testPopedWindow;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Do any additional setup after loading the view.
    self.view.wantsLayer = YES;
    self.view.layer.backgroundColor = NSColor.systemRedColor.CGColor;
    [self.view window].appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];
    
    // Update the view, if already loaded.
}

- (void)attachChildWindow:(NSWindow*)child toParent:(NSWindow*)parent {
    [parent addChildWindow:child ordered:NSWindowAbove];
}
- (void)detachChildWindow:(NSWindow*)child fromParent:(NSWindow*)parent {
    [parent removeChildWindow:child];
}

- (IBAction)popedWindowCloseAction:(id)sender {
    //[self.view.window endSheet:self.popedWindow];
    [self detachChildWindow:self.popedWindow fromParent:self.view.window];
    // hide poped window
    [self.popedWindow orderOut:nil];
}

- (IBAction)testPopButtonAction:(id)sender {
    if (!self.testPopedWindow) {
        NSRect frame = self.view.window.frame;
        frame = CGRectInset(frame, 20, 20);
        NSViewController* testPopedController = [[PoppedViewController alloc] init];
        // test windowWithContentViewController
        self.testPopedWindow = [NSWindow windowWithContentViewController:testPopedController];
        [self.testPopedWindow setBackgroundColor:[NSColor colorWithRed:1.0 green:0 blue:0 alpha:0.8]];
    }
    NSRect frame = self.view.window.frame;
    frame = CGRectInset(frame, 20, 20);
    [self.testPopedWindow setFrame:frame display:YES];
    
    // fix: Window separation problem when sliding up with three fingers.
    [self attachChildWindow:self.testPopedWindow toParent:self.view.window];
}

- (IBAction)popButtonAction:(id)sender {
    if (!self.popedWindow) {
        NSRect frame = self.view.window.frame;
        frame = CGRectInset(frame, 20, 20);
        self.popedWindow  = [[NSWindow alloc] initWithContentRect:frame
                                                        styleMask:NSWindowStyleMaskBorderless
                                                          backing:NSBackingStoreBuffered
                                                            defer:NO];
        NSView *superview = [self.popedWindow contentView];
        frame = NSMakeRect(10, 10, 300, 100);
        NSButton *button = [[NSButton alloc] initWithFrame:frame];
        [button setTitle:@"Click me close!"];
        [superview addSubview:button];
        
        [button setAction:@selector(popedWindowCloseAction:)];
        [button setTarget:self];
        
        //self.popedWindow.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
        //[self.popedWindow setOpaque:NO];
        //[self.popedWindow setAlphaValue:0.5];
        [self.popedWindow setBackgroundColor:[NSColor colorWithRed:0.0 green:0 blue:1.0 alpha:0.8]];
    }
    NSRect frame = self.view.window.frame;
    frame = CGRectInset(frame, 20, 20);
    [self.popedWindow setFrame:frame display:YES];
    
#if 0
    [self.view.window beginSheet:self.popedWindow completionHandler:^(NSModalResponse returnCode) {
        NSLog(@"popclosed");
        [NSApp stopModalWithCode:returnCode];
    }];
#else
    // fix: Window separation problem when sliding up with three fingers.
    [self attachChildWindow:self.popedWindow toParent:self.view.window];
#endif
    
    //[NSApp runModalForWindow:self.popedWindow];
#if 0
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    //panel.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
    // 设置标题
    [panel setTitle:@"PopWindow"];
    [panel setMessage:@"PopWindow"];
    __block int chosen = NSModalResponseCancel;
    NSWindow* mainWindow = self.view.window;
    [panel beginSheetModalForWindow:mainWindow
                  completionHandler:^(NSInteger c) {
        chosen = c;
        [NSApp stopModal];
    }];
    [NSApp runModalForWindow:mainWindow];
    
    if (NSModalResponseOK == chosen) {
        NSArray* urls = [panel URLs];
        for (NSURL* url in urls) {
            if ([url isFileURL]) {
                NSLog(@"%@", [url path]);
            }
        }
    }
#endif
}

@end
