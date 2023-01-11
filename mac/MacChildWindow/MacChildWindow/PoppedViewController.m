//
//  ViewController.m
//  MacChildWindow
//
//  Created by 刘秀峰(Whitebrow.L) on 2023/1/5.
//

#import "PoppedViewController.h"

@interface PoppedViewController()

@property (nullable, weak) NSButton *closeButton;

@end

@implementation PoppedViewController

- (void)loadView {
    if (!self.closeButton) {
        // Do any additional setup after loading the view.
        self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 600, 400)];
        NSView *superview = self.view;
        NSRect frame = NSMakeRect(10, 10, 300, 100);
        NSButton *button = [[NSButton alloc] initWithFrame:frame];
        self.closeButton = button;
        [button setTitle:@"Click me close!"];
        [superview addSubview:button];
        
        [button setAction:@selector(closeAction:)];
        [button setTarget:self];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];
    
    // Update the view, if already loaded.
}

- (IBAction)closeAction:(id)sender {
    //[self.view.window endSheet:self.popedWindow];
    // hide poped window
    [self.view.window orderOut:nil];
}

@end
