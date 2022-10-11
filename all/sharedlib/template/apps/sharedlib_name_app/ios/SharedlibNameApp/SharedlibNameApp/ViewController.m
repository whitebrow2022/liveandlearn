//
//  ViewController.m
//  SharedlibNameApp
//
//  Created by %username% on %date%.
//

#import "ViewController.h"

#import "SharedlibNameUtil.h"

@interface ViewController ()

@property (weak) UITextView* textView;

@end

@implementation ViewController

- (void)loadView {
  [super loadView];
  //[self initViews];
}

- (void)viewDidLoad {
  [super viewDidLoad];
  // Do any additional setup after loading the view.
  [self initViews];
}

- (void)initViews {
  CGRect parentArea = self.view.frame;
  CGFloat topPadding = 0;
  CGFloat bottomPadding = 0;
  if (@available(iOS 11.0, *)) {
    UIWindow *window = UIApplication.sharedApplication.windows.firstObject;
    topPadding = window.safeAreaInsets.top;
    bottomPadding = window.safeAreaInsets.bottom;
  }
  UIStackView *stackView = [[UIStackView alloc] initWithFrame:CGRectMake(0, topPadding, parentArea.size.width, parentArea.size.height - topPadding - bottomPadding)];
  stackView.axis = UILayoutConstraintAxisVertical;
  stackView.spacing = 10.0;
  stackView.alignment = UIStackViewAlignmentFill;
  stackView.distribution = UIStackViewDistributionFill;
  [self.view addSubview:stackView];
  
  UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
  [button setTitle:@"Click Me!" forState:UIControlStateNormal];
  button.contentEdgeInsets = UIEdgeInsetsMake(2, 2, 2, 2);
  button.layer.borderWidth = 1;
  button.layer.borderColor = [UIColor redColor].CGColor;
  [button sizeToFit];
  [button addTarget:self action:@selector(buttonClicked) forControlEvents:UIControlEventTouchUpInside];
  [stackView addArrangedSubview:button];
  
  UITextView* textView = [[UITextView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  self.textView = textView;
  self.textView.contentInset = UIEdgeInsetsMake(2, 2, 2, 2);
  self.textView.layer.borderWidth = 1;
  self.textView.layer.borderColor = [UIColor redColor].CGColor;
  [stackView addArrangedSubview:textView];

  // layout
  CGFloat padding = 10;
  UIView* rootView = stackView;
  UIView* parentView = self.view;
  [rootView setTranslatesAutoresizingMaskIntoConstraints:NO];
  
  if (@available(iOS 11, *)) {
    UILayoutGuide * guide = parentView.safeAreaLayoutGuide;
    [rootView.leadingAnchor constraintEqualToAnchor:guide.leadingAnchor constant:padding].active = YES;
    [rootView.trailingAnchor constraintEqualToAnchor:guide.trailingAnchor constant:-padding].active = YES;
    [rootView.topAnchor constraintEqualToAnchor:guide.topAnchor constant:padding].active = YES;
    [rootView.bottomAnchor constraintEqualToAnchor:guide.bottomAnchor constant:-padding].active = YES;
  } else {
#if 0
    UILayoutGuide *margins = parentView.layoutMarginsGuide;
    [rootView.leadingAnchor constraintEqualToAnchor:margins.leadingAnchor constant:padding].active = YES;
    [rootView.trailingAnchor constraintEqualToAnchor:margins.trailingAnchor constant:-padding].active = YES;
    [rootView.topAnchor constraintEqualToAnchor:margins.topAnchor constant:padding].active = YES;
    [rootView.bottomAnchor constraintEqualToAnchor:margins.bottomAnchor constant:-padding].active = YES;
#else
    [parentView setTranslatesAutoresizingMaskIntoConstraints:YES];
    NSLayoutConstraint *left = [NSLayoutConstraint constraintWithItem:rootView attribute:NSLayoutAttributeLeft relatedBy:NSLayoutRelationEqual toItem:parentView attribute:NSLayoutAttributeLeft multiplier:1 constant:padding];
    NSLayoutConstraint *top = [NSLayoutConstraint constraintWithItem:rootView attribute:NSLayoutAttributeTop relatedBy:NSLayoutRelationEqual toItem:parentView attribute:NSLayoutAttributeTop multiplier:1 constant:padding];
    NSLayoutConstraint *right = [NSLayoutConstraint constraintWithItem:rootView attribute:NSLayoutAttributeRight relatedBy:NSLayoutRelationEqual toItem:parentView attribute:NSLayoutAttributeRight multiplier:1 constant:-padding];
    NSLayoutConstraint *bottom = [NSLayoutConstraint constraintWithItem:rootView attribute:NSLayoutAttributeBottom relatedBy:NSLayoutRelationGreaterThanOrEqual toItem:parentView attribute:NSLayoutAttributeBottom multiplier:1 constant:-padding];
    [parentView addConstraints:@[left, top, right, bottom]];
#endif
}

-(IBAction)buttonClicked {
  [self.textView setText:[SharedlibNameUtil version]];
}


@end
