//
//  ViewController.m
//  OpencvWrapperApp
//
//  Created by liangxu on 2022/10/21.
//

#import "ViewController.h"
#import "OpencvWrapperUtil.h"

@interface ViewController ()

@property (weak) NSTextView *textView;

@end

@implementation ViewController

- (void)loadView {
  [super loadView];
}

- (void)viewDidLoad {
  [super viewDidLoad];

  // Do any additional setup after loading the view.
  [self initViews];
}

- (void)initViews {
  CGRect rootArea = self.view.frame;
  NSStackView *verticalView = [[NSStackView alloc] initWithFrame:rootArea];
  verticalView.orientation = NSUserInterfaceLayoutOrientationVertical;
  [self.view addSubview:verticalView];
  
  NSButton *button = [[NSButton alloc] init];
  [button setTitle:@"Click Me!"];
  [button sizeToFit];
  button.action = @selector(buttonClicked);
  [verticalView addArrangedSubview:button];
  
  NSTextView *textView = [[NSTextView alloc] init];
  self.textView = textView;
  [verticalView addArrangedSubview:textView];

  // layout
  CGFloat padding = 10;
  NSView* rootView = verticalView;
  NSView* parentView = self.view;
  [rootView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [parentView setTranslatesAutoresizingMaskIntoConstraints:YES];
  NSLayoutConstraint *left = [NSLayoutConstraint constraintWithItem:rootView attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:parentView attribute:NSLayoutAttributeLeft multiplier:1 constant:padding];
  NSLayoutConstraint *top = [NSLayoutConstraint constraintWithItem:rootView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:parentView attribute:NSLayoutAttributeTop multiplier:1 constant:padding];
  NSLayoutConstraint *right = [NSLayoutConstraint constraintWithItem:rootView attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:parentView attribute:NSLayoutAttributeRight multiplier:1 constant:-padding];
  NSLayoutConstraint *bottom = [NSLayoutConstraint constraintWithItem:rootView attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationGreaterThanOrEqual toItem:parentView attribute:NSLayoutAttributeBottom multiplier:1 constant:-padding];
  [parentView addConstraints:@[left, top, right, bottom]];
}

- (IBAction)buttonClicked {
  [self.textView setString:[OpencvWrapperUtil version]];
}

- (void)setRepresentedObject:(id)representedObject {
  [super setRepresentedObject:representedObject];

  // Update the view, if already loaded.
}


@end
