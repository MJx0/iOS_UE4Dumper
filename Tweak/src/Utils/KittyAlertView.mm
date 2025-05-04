#import "KittyAlertView.h"
#import <QuartzCore/QuartzCore.h>

// Color macro
#define kColorHex(RGBA) [UIColor colorWithRed:((RGBA >> 24) & 0xFF)/255.0 green:((RGBA >> 16) & 0xFF)/255.0 blue:((RGBA >> 8) & 0xFF)/255.0 alpha:((RGBA) & 0xFF)/255.0]

// Default colors
#define kAlertBackgroundColorHex 0xFFFFFFFF
#define kDimmingViewColorHex 0x00000080
#define kFontColorHex 0x000000FF
#define kShadowColorHex 0x000000FF
#define kIconLetterColorHex 0xFFFFFFFF
#define kSubtitleBorderColorHex 0x000000FF
#define kSuccessTintColorHex 0x00A000FF
#define kErrorTintColorHex 0xCA0000FF
#define kWarningTintColorHex 0xFF8C00FF
#define kInfoTintColorHex 0x0088CCFF
#define kEditTintColorHex 0x800080FF
#define kWaitingTintColorHex 0x009090FF
#define kCustomTintColorHex 0xC800C8FF

// Default sizing values
#define kPaddingSmallScreen 12.0
#define kPaddingLargeScreen 20.0
#define kElementSpacing 8.0
#define kIconSizeSmallPhone 36.0
#define kIconSizeSmallTablet 48.0
#define kIconSizeLargePhone 56.0
#define kIconSizeLargeTablet 72.0
#define kButtonHeightPhone 40.0
#define kButtonHeightTablet 48.0
#define kAlertMaxWidthPhone 320.0
#define kAlertMaxWidthTablet 400.0
#define kAlertMaxHeightRatio 0.8
#define kSubtitleMaxHeight 150.0
#define kMinFontScale 0.8
#define kMinScreenWidth 1.0
#define kTitleMaxHeightThreshold 100.0
#define kLargeScreenThreshold 768.0
#define kAlertCornerRadius 12.0
#define kButtonCornerRadiusDivisor 2.0
#define kAlertShadowOffsetX 0.0
#define kAlertShadowOffsetY 4.0
#define kAlertShadowRadius 8.0
#define kAlertShadowOpacity 0.3

// Default font sizing
#define kFontSizeTitle [UIFont systemFontSize]
#define kFontSizeSubtitle [UIFont smallSystemFontSize]
#define kFontSizeButton [UIFont buttonFontSize]
#define kFontSizeIcon ([UIFont systemFontSize]+([UIFont systemFontSize]*0.25))
#define kIconLineWidthRatio 0.1
#define kIconInsetRatio 0.25
#define kSubtitleTextContainerInset 8.0
#define kSubtitleBorderWidth 1.0
#define kSubtitleBorderAlpha 0.7

// Default animation values
#define kAnimationShowHideDuration 0.3
#define kAnimationBounceDuration 0.5
#define kAnimationPulseDuration 0.8
#define kAnimationButtonHoverDuration 0.1
#define kAnimationGradientRotationDuration 2.0
#define kAnimationPulseScaleFactor 1.1
#define kAnimationSpringDamping 0.6
#define kAnimationSpringVelocity 0.5

// Animation keys
#define kPulseAnimationKey @"kitty.alert.pulse"
#define kGradientRotationKey @"kitty.alert.gradientRotation"

@implementation KittyAlertButton
@end

@implementation KittyAlertSwitch
@end

@interface KittyAlertView ()

@property (nonatomic, weak) UIView *alertParentView;
@property (nonatomic, strong) UIView *alertDimmingView;
@property (nonatomic, strong) UIView *alertContentView;
@property (nonatomic, strong) UIImageView *alertIconView;
@property (nonatomic, strong) CAGradientLayer *waitingIconGradientLayer;
@property (nonatomic, strong) UILabel *alertTitleLabel;
@property (nonatomic, strong) UITextView *alertSubtitleTextView;
@property (nonatomic, strong) UIScrollView *elementsScrollView;
@property (nonatomic, strong) NSMutableArray<UITextField *> *alertTextFields;
@property (nonatomic, strong) NSMutableArray<KittyAlertButton *> *alertButtons;
@property (nonatomic, strong) NSMutableArray<KittyAlertSwitch *> *alertSwitches;
@property (nonatomic, strong, nullable) NSTimer *dismissTimer;
@property (nonatomic, strong) UITapGestureRecognizer *dismissTapGesture;

@property (nonatomic, assign) KittyAlertViewStyle alertStyle;

@property (nonatomic, strong, nullable) UIColor *tintColor;
@property (nonatomic, strong, nullable) UIColor *backgroundColor;
@property (nonatomic, strong, nullable) UIColor *fontColor;

@property (nonatomic, assign) BOOL isShowing;

@end

@implementation KittyAlertView

+ (instancetype)createWithParentView:(UIView *)view forStyle:(KittyAlertViewStyle)style
{
    if (!view)
    {
        NSLog(@"KittyAlertView: parentView cannot be nil");
        return nil;
    }
    
    KittyAlertView *av = [[KittyAlertView alloc] initWithFrame:view.bounds];
    if (av) {
        av.alertParentView = view;
        av.alertStyle = style;
        av.alertTextFields = [NSMutableArray array];
        av.alertButtons = [NSMutableArray array];
        av.alertSwitches = [NSMutableArray array];
        av.shouldDismissOnTapOutside = YES;
        av.showAnimation = KittyAlertViewAnimationFade;
        av.hideAnimation = KittyAlertViewAnimationFade;
        av.duration = 0;
        av.isShowing = NO;
        av.enablePulseEffect = YES;
        av.useLargeIcon = NO;
        [av setupUI];
        [av setupRotationHandling];
    }
    return av;
}

- (void)setupUI
{
    self.tintColor = [self tintColorForStyle:self.alertStyle];
    self.backgroundColor = [self backgroundColorForStyle:self.alertStyle];
    self.fontColor = [self fontColorForStyle:self.alertStyle];
    
    self.alertDimmingView = [[UIView alloc] initWithFrame:self.bounds];
    self.alertDimmingView.backgroundColor = kColorHex(kDimmingViewColorHex);
    self.alertDimmingView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self addSubview:self.alertDimmingView];
    
    self.alertContentView = [[UIView alloc] init];
    self.alertContentView.backgroundColor = kColorHex(kAlertBackgroundColorHex);
    self.alertContentView.layer.cornerRadius = kAlertCornerRadius;
    self.alertContentView.layer.masksToBounds = NO;
    self.alertContentView.layer.shadowColor = kColorHex(kShadowColorHex).CGColor;
    self.alertContentView.layer.shadowOpacity = kAlertShadowOpacity;
    self.alertContentView.layer.shadowOffset = CGSizeMake(kAlertShadowOffsetX, kAlertShadowOffsetY);
    self.alertContentView.layer.shadowRadius = kAlertShadowRadius;
    [self addSubview:self.alertContentView];
    
    self.alertIconView = [[UIImageView alloc] init];
    self.alertIconView.contentMode = UIViewContentModeScaleAspectFit;
    self.alertIconView.image = [self iconForStyle:self.alertStyle];
    self.alertIconView.tintColor = self.tintColor ?: [self tintColorForStyle:self.alertStyle];
    [self.alertContentView addSubview:self.alertIconView];
    
    self.waitingIconGradientLayer = [CAGradientLayer layer];
    UIColor *waitTint = [self tintColorForStyle:KittyAlertViewStyleWaiting];
    UIColor *waitDarkerTint = [self darkerColor:waitTint];
    self.waitingIconGradientLayer.colors = @[
        (id)waitDarkerTint.CGColor,
        (id)waitTint.CGColor,
        (id)waitDarkerTint.CGColor
    ];
    self.waitingIconGradientLayer.startPoint = CGPointMake(0.0, 0.5);
    self.waitingIconGradientLayer.endPoint = CGPointMake(1.0, 0.5);
    [self.alertIconView.layer addSublayer:self.waitingIconGradientLayer];
    
    self.alertTitleLabel = [[UILabel alloc] init];
    self.alertTitleLabel.font = [UIFont boldSystemFontOfSize:kFontSizeTitle];
    self.alertTitleLabel.textAlignment = NSTextAlignmentCenter;
    self.alertTitleLabel.numberOfLines = 0;
    self.alertTitleLabel.textColor = kColorHex(kFontColorHex);
    self.alertTitleLabel.backgroundColor = [UIColor clearColor];
    [self.alertContentView addSubview:self.alertTitleLabel];
    
    self.alertSubtitleTextView = [[UITextView alloc] init];
    self.alertSubtitleTextView.font = [UIFont systemFontOfSize:kFontSizeSubtitle];
    self.alertSubtitleTextView.textAlignment = NSTextAlignmentCenter;
    self.alertSubtitleTextView.textColor = kColorHex(kFontColorHex);
    self.alertSubtitleTextView.backgroundColor = [UIColor clearColor];
    self.alertSubtitleTextView.editable = NO;
    self.alertSubtitleTextView.selectable = YES;
    self.alertSubtitleTextView.showsVerticalScrollIndicator = YES;
    self.alertSubtitleTextView.textContainerInset = UIEdgeInsetsMake(kSubtitleTextContainerInset, kSubtitleTextContainerInset, kSubtitleTextContainerInset, kSubtitleTextContainerInset);
    self.alertSubtitleTextView.layer.borderWidth = kSubtitleBorderWidth;
    self.alertSubtitleTextView.layer.borderColor = kColorHex(kSubtitleBorderColorHex).CGColor;
    [self.alertContentView addSubview:self.alertSubtitleTextView];
    
    self.elementsScrollView = [[UIScrollView alloc] init];
    self.elementsScrollView.showsVerticalScrollIndicator = YES;
    [self.alertContentView addSubview:self.elementsScrollView];
    
    self.dismissTapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTap:)];
    [self.alertDimmingView addGestureRecognizer:self.dismissTapGesture];
}

- (void)setupRotationHandling
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(deviceOrientationDidChange:)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
}

- (void)deviceOrientationDidChange:(NSNotification *)notification
{
    self.frame = self.alertParentView.bounds;
    if (self.isShowing) {
        [self setNeedsLayout];
    }
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    
    if (!self.superview || self.bounds.size.width <= 0)
        return;
    
    self.alertDimmingView.frame = self.bounds;
    
    CGFloat screenWidth = MAX(self.bounds.size.width, kMinScreenWidth);
    CGFloat screenHeight = self.bounds.size.height;
    BOOL isLargeScreen = screenWidth >= kLargeScreenThreshold;
    CGFloat alertMaxWidth = isLargeScreen ? kAlertMaxWidthTablet : kAlertMaxWidthPhone;
    CGFloat alertMaxHeight = screenHeight * kAlertMaxHeightRatio;
    CGFloat padding = isLargeScreen ? kPaddingLargeScreen : kPaddingSmallScreen;
    CGFloat iconSize = self.useLargeIcon ? (isLargeScreen ? kIconSizeLargeTablet : kIconSizeLargePhone) : (isLargeScreen ? kIconSizeSmallTablet : kIconSizeSmallPhone);
    CGFloat buttonHeight = isLargeScreen ? kButtonHeightTablet : kButtonHeightPhone;
    CGFloat currentY = padding;
    
    UIEdgeInsets insets = UIEdgeInsetsZero;
    if (@available(iOS 11.0, *)) {
        if (self.alertParentView) {
            insets = self.alertParentView.safeAreaInsets;
        }
    }
    
    self.alertIconView.hidden = self.alertIconView.image == nil;
    if (self.alertIconView.image) {
        self.alertIconView.frame = CGRectMake((alertMaxWidth - iconSize) / 2, currentY, iconSize, iconSize);
        currentY += iconSize + kElementSpacing;
    }
    
    if (self.alertStyle == KittyAlertViewStyleWaiting) {
        self.waitingIconGradientLayer.hidden = NO;
        self.waitingIconGradientLayer.frame = self.alertIconView.bounds;
        self.waitingIconGradientLayer.mask = [self clockMaskLayerForSize:iconSize];
    } else {
        self.waitingIconGradientLayer.hidden = YES;
    }
    
    CGFloat titleFontSize = kFontSizeTitle;
    CGFloat subtitleFontSize = kFontSizeSubtitle;
    
    if (self.alertTitleLabel.text && self.alertTitleLabel.text.length > 0) {
        NSString *title = self.alertTitleLabel.text;
        CGSize titleSize = [title boundingRectWithSize:CGSizeMake(alertMaxWidth - 2 * padding, CGFLOAT_MAX)
                                               options:NSStringDrawingUsesLineFragmentOrigin
                                            attributes:@{NSFontAttributeName: [UIFont boldSystemFontOfSize:titleFontSize]}
                                               context:nil].size;
        if (titleSize.height > kTitleMaxHeightThreshold) {
            titleFontSize *= kMinFontScale;
            self.alertTitleLabel.font = [UIFont boldSystemFontOfSize:titleFontSize];
        }
        titleSize = [title boundingRectWithSize:CGSizeMake(alertMaxWidth - 2 * padding, CGFLOAT_MAX)
                                        options:NSStringDrawingUsesLineFragmentOrigin
                                     attributes:@{NSFontAttributeName: self.alertTitleLabel.font}
                                        context:nil].size;
        self.alertTitleLabel.frame = CGRectMake(padding, currentY, alertMaxWidth - 2 * padding, titleSize.height);
        currentY += titleSize.height + kElementSpacing;
    } else {
        self.alertTitleLabel.frame = CGRectZero;
    }
    
    if (self.alertSubtitleTextView.text.length > 0) {
        NSString *subtitle = self.alertSubtitleTextView.text ?: @"";
        CGSize subtitleSize = [subtitle boundingRectWithSize:CGSizeMake(alertMaxWidth - 2 * padding - 2 * kSubtitleTextContainerInset, CGFLOAT_MAX)
                                                     options:NSStringDrawingUsesLineFragmentOrigin
                                                  attributes:@{NSFontAttributeName: [UIFont systemFontOfSize:subtitleFontSize]}
                                                     context:nil].size;
        BOOL isScrollable = subtitleSize.height > kSubtitleMaxHeight;
        if (isScrollable) {
            subtitleFontSize *= kMinFontScale;
            self.alertSubtitleTextView.font = [UIFont systemFontOfSize:subtitleFontSize];
            subtitleSize.height = kSubtitleMaxHeight;
        }
        self.alertSubtitleTextView.scrollEnabled = isScrollable;
        self.alertSubtitleTextView.frame = CGRectMake(padding, currentY, alertMaxWidth - 2 * padding, MIN(subtitleSize.height + 2 * kSubtitleTextContainerInset, kSubtitleMaxHeight));
        self.alertSubtitleTextView.layer.borderWidth = kSubtitleBorderWidth;
        self.alertSubtitleTextView.layer.borderColor = kColorHex(kSubtitleBorderColorHex).CGColor;
        currentY += self.alertSubtitleTextView.frame.size.height + kElementSpacing;
    } else {
        self.alertSubtitleTextView.frame = CGRectZero;
        self.alertSubtitleTextView.layer.borderWidth = 0.0;
    }
    
    self.elementsScrollView.frame = CGRectMake(padding, currentY, alertMaxWidth - 2 * padding, alertMaxHeight - currentY - padding);
    CGFloat scrollContentY = 0;
    
    for (UITextField *textField in self.alertTextFields) {
        if (textField) {
            textField.frame = CGRectMake(0, scrollContentY, alertMaxWidth - 2 * padding, buttonHeight);
            scrollContentY += buttonHeight + kElementSpacing;
        }
    }
    
    for (KittyAlertButton *button in self.alertButtons) {
        if (button) {
            button.frame = CGRectMake(0, scrollContentY, alertMaxWidth - 2 * padding, buttonHeight);
            scrollContentY += buttonHeight + kElementSpacing;
        }
    }
    
    for (KittyAlertSwitch *switchCon in self.alertSwitches) {
        if (switchCon) {
            switchCon.frame = CGRectMake(0, scrollContentY, alertMaxWidth - 2 * padding, buttonHeight);
            scrollContentY += buttonHeight + kElementSpacing;
            
            if (switchCon.switchLabel && switchCon.switchControl) {
                float w = switchCon.frame.size.width - switchCon.switchControl.frame.size.width - kElementSpacing;
                float x = switchCon.switchControl.frame.size.width + kElementSpacing;
                
                switchCon.switchControl.frame = CGRectMake(0, 0, 0, buttonHeight);
                
                [switchCon.switchLabel sizeToFit];
                switchCon.switchLabel.frame = CGRectMake(x, 0, w, buttonHeight);
            }
        }
    }
    
    self.elementsScrollView.contentSize = CGSizeMake(alertMaxWidth - 2 * padding, scrollContentY);
    
    CGFloat alertHeight = MIN(currentY + self.elementsScrollView.contentSize.height + padding, alertMaxHeight);
    self.alertContentView.frame = CGRectMake((screenWidth - alertMaxWidth) / 2,
                                             (screenHeight - alertHeight - insets.top - insets.bottom) / 2 + insets.top,
                                             alertMaxWidth,
                                             alertHeight);
}

- (void)startPulseAnimation
{
    if (!self.alertIconView || !self.alertIconView.layer)
        return;
    
    [self stopPulseAnimation];
    CABasicAnimation *pulse = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
    pulse.duration = kAnimationPulseDuration;
    pulse.fromValue = @(1.0);
    pulse.toValue = @(kAnimationPulseScaleFactor);
    pulse.autoreverses = YES;
    pulse.repeatCount = HUGE_VALF;
    [self.alertIconView.layer addAnimation:pulse forKey:kPulseAnimationKey];
}

- (void)stopPulseAnimation
{
    if (self.alertIconView && self.alertIconView.layer && [self.alertIconView.layer animationForKey:kPulseAnimationKey]) {
        [self.alertIconView.layer removeAnimationForKey:kPulseAnimationKey];
    }
}

- (void)startWaitingGradientAnimation
{
    if (!self.waitingIconGradientLayer || !self.waitingIconGradientLayer.superlayer)
        return;
    
    [self stopWaitingGradientAnimation];
    CABasicAnimation *rotation = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
    rotation.fromValue = @(0);
    rotation.toValue = @(2 * M_PI);
    rotation.duration = kAnimationGradientRotationDuration;
    rotation.repeatCount = HUGE_VALF;
    [self.waitingIconGradientLayer addAnimation:rotation forKey:kGradientRotationKey];
}

- (void)stopWaitingGradientAnimation
{
    if (!self.waitingIconGradientLayer)
        return;
    
    if ([self.waitingIconGradientLayer animationForKey:kGradientRotationKey]) {
        [self.waitingIconGradientLayer removeAnimationForKey:kGradientRotationKey];
    }
}

- (void)showWithTitle:(nullable NSString *)title
         subtitle:(nullable NSString *)subtitle
 closeButtonTitle:(nullable NSString *)closeButtonTitle
         duration:(NSTimeInterval)duration {
    
    if (self.isShowing)
        [self dismiss];
    
    self.alertIconView.hidden = self.alertIconView.image == nil;

    if (self.alertStyle == KittyAlertViewStyleWaiting) {
        self.waitingIconGradientLayer.hidden = NO;
        [self startWaitingGradientAnimation];
    } else {
        self.waitingIconGradientLayer.hidden = YES;
        [self stopWaitingGradientAnimation];
    }
    
    self.alertTitleLabel.text = title;
    self.alertSubtitleTextView.text = subtitle;
    
    if (closeButtonTitle.length > 0) {
        [self addButtonWithTitle:closeButtonTitle action:nil];
    }
    
    self.duration = duration;
    [self show];
}

/*
- (UITextField *)addTextFieldWithPlaceholder:(nullable NSString *)placeholder
{
    UITextField *textField = [[UITextField alloc] init];
    textField.borderStyle = UITextBorderStyleRoundedRect;
    textField.placeholder = placeholder;
    [self.elementsScrollView addSubview:textField];
    [self.alertTextFields addObject:textField];
    [self setNeedsLayout];
    return textField;
}
 */

- (KittyAlertButton *)addButtonWithTitle:(NSString *)title action:(void (^ _Nullable)(void))action
{
    if (!title.length) {
        NSLog(@"KittyAlertView: Button title cannot be empty");
        return nil;
    }
    KittyAlertButton *button = [KittyAlertButton buttonWithType:UIButtonTypeCustom];
    [button setTitle:title forState:UIControlStateNormal];
    button.titleLabel.font = [UIFont systemFontOfSize:kFontSizeButton weight:UIFontWeightMedium];
    [button setTitleColor:self.fontColor ?: kColorHex(kFontColorHex) forState:UIControlStateNormal];
    button.backgroundColor = self.tintColor ?: [self tintColorForStyle:self.alertStyle];
    [button addTarget:self action:@selector(touchDown:) forControlEvents:UIControlEventTouchDown];
    [button addTarget:self action:@selector(touchUpInside:) forControlEvents:UIControlEventTouchUpInside];
    [button addTarget:self action:@selector(touchUpOutside:) forControlEvents:UIControlEventTouchUpOutside];
    [button addTarget:self action:@selector(buttonTapped:) forControlEvents:UIControlEventTouchUpInside];
    
    [self.elementsScrollView addSubview:button];
    [self.alertButtons addObject:button];
    if (action) {
        button.action = [action copy];
    } else {
        button.action = (^(){});
    }
    [self setNeedsLayout];
    return button;
}

- (KittyAlertSwitch *)addSwitchWithTitle:(NSString *)title initialState:(BOOL)initialState action:(void (^ _Nullable)(BOOL))action
{
    if (!title.length) {
        NSLog(@"KittyAlertView: Switch title cannot be empty");
        return nil;
    }
    
    KittyAlertSwitch *switchContainer = [[KittyAlertSwitch alloc] init];
    
    switchContainer.switchLabel = [[UILabel alloc] init];
    switchContainer.switchLabel.text = title;
    switchContainer.switchLabel.font = [UIFont systemFontOfSize:kFontSizeButton];
    switchContainer.switchLabel.textColor = self.fontColor ?: kColorHex(kFontColorHex);
    switchContainer.switchLabel.numberOfLines = 1;
    switchContainer.switchLabel.textAlignment = NSTextAlignmentLeft;
    switchContainer.switchLabel.lineBreakMode = NSLineBreakByTruncatingTail;
    switchContainer.switchLabel.baselineAdjustment = UIBaselineAdjustmentAlignCenters;
    switchContainer.switchLabel.minimumScaleFactor = 0.5f;
    switchContainer.switchLabel.adjustsFontSizeToFitWidth = YES;

    [switchContainer addSubview:switchContainer.switchLabel];
    
    switchContainer.switchControl = [[UISwitch alloc] init];
    switchContainer.switchControl.on = initialState;
    switchContainer.switchControl.tintColor = self.tintColor ?: [self tintColorForStyle:self.alertStyle];
    switchContainer.switchControl.onTintColor = self.tintColor ?: [self tintColorForStyle:self.alertStyle];
    [switchContainer.switchControl addTarget:self action:@selector(switchChanged:) forControlEvents:UIControlEventValueChanged];
    
    [switchContainer addSubview:switchContainer.switchControl];
    
    [self.elementsScrollView addSubview:switchContainer];
    [self.alertSwitches addObject:switchContainer];
    if (action) {
       switchContainer.action = [action copy];
    } else {
       switchContainer.action = (^(BOOL){});
    }
    [self setNeedsLayout];
    return switchContainer;
}

- (void)show
{
    if (self.isShowing || !self.alertParentView) return;
    
    [self.alertParentView addSubview:self];
    self.isShowing = YES;

    self.dismissTapGesture.enabled = self.shouldDismissOnTapOutside;
    
    self.alertContentView.alpha = 1.0;
    self.alertContentView.transform = CGAffineTransformIdentity;
    
    __weak KittyAlertView *weakSelf = self;
    void (^completion)(BOOL) = ^(BOOL) {
        __strong KittyAlertView *strongSelf = weakSelf;
        if (strongSelf) {
            [strongSelf startPulseAnimation];
        }
    };
    
    if (self.showAnimation == KittyAlertViewAnimationFade) {
        self.alertContentView.alpha = 0;
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.alpha = 1;
        } completion:completion];
    } else if (self.showAnimation == KittyAlertViewAnimationSlideFromTop) {
        self.alertContentView.transform = CGAffineTransformMakeTranslation(0, -self.bounds.size.height);
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformIdentity;
        } completion:completion];
    } else if (self.showAnimation == KittyAlertViewAnimationSlideFromBottom) {
        self.alertContentView.transform = CGAffineTransformMakeTranslation(0, self.bounds.size.height);
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformIdentity;
        } completion:completion];
    } else if (self.showAnimation == KittyAlertViewAnimationScale) {
        self.alertContentView.transform = CGAffineTransformMakeScale(0.1, 0.1);
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformIdentity;
        } completion:completion];
    } else if (self.showAnimation == KittyAlertViewAnimationBounce) {
        self.alertContentView.transform = CGAffineTransformMakeScale(0.1, 0.1);
        [UIView animateWithDuration:kAnimationBounceDuration
                              delay:0
             usingSpringWithDamping:kAnimationSpringDamping
              initialSpringVelocity:kAnimationSpringVelocity
                            options:UIViewAnimationOptionCurveEaseInOut
                         animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformIdentity;
        } completion:completion];
    } else if (self.showAnimation == KittyAlertViewAnimationRotate) {
        self.alertContentView.transform = CGAffineTransformMakeRotation(M_PI);
        self.alertContentView.alpha = 0;
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformIdentity;
            weakSelf.alertContentView.alpha = 1;
        } completion:completion];
    }
    
    if (self.duration > 0) {
        [self.dismissTimer invalidate];
        self.dismissTimer = [NSTimer scheduledTimerWithTimeInterval:self.duration
                                                             target:self
                                                           selector:@selector(dismiss)
                                                           userInfo:nil
                                                            repeats:NO];
    }
}

- (void)dismiss
{
    if (!self.isShowing) return;
    
    if (self.dismissTimer) {
        [self.dismissTimer invalidate];
        self.dismissTimer = nil;
    }
    
    [self stopPulseAnimation];
    [self stopWaitingGradientAnimation];
    
    __weak KittyAlertView *weakSelf = self;
    void (^completion)(BOOL) = ^(BOOL) {
        __strong KittyAlertView *strongSelf = weakSelf;
        if (strongSelf) {
            [strongSelf removeFromSuperview];
            strongSelf.isShowing = NO;
            if (strongSelf.dismissAction) {
                strongSelf.dismissAction();
            }
        }
    };
    
    if (self.hideAnimation == KittyAlertViewAnimationFade) {
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.alpha = 0;
        } completion:completion];
    } else if (self.hideAnimation == KittyAlertViewAnimationSlideFromTop) {
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformMakeTranslation(0, -weakSelf.bounds.size.height);
        } completion:completion];
    } else if (self.hideAnimation == KittyAlertViewAnimationSlideFromBottom) {
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformMakeTranslation(0, weakSelf.bounds.size.height);
        } completion:completion];
    } else if (self.hideAnimation == KittyAlertViewAnimationScale) {
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformMakeScale(0.1, 0.1);
        } completion:completion];
    } else if (self.hideAnimation == KittyAlertViewAnimationBounce) {
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformMakeScale(0.1, 0.1);
        } completion:completion];
    } else if (self.hideAnimation == KittyAlertViewAnimationRotate) {
        [UIView animateWithDuration:kAnimationShowHideDuration animations:^{
            weakSelf.alertContentView.transform = CGAffineTransformMakeRotation(M_PI);
            weakSelf.alertContentView.alpha = 0;
        } completion:completion];
    }
}

- (void)handleTap:(UITapGestureRecognizer *)gesture {
    if (self.shouldDismissOnTapOutside && self.alertContentView && ![self.alertContentView pointInside:[gesture locationInView:self.alertContentView] withEvent:nil]) {
        [self dismiss];
    }
}

- (void)touchDown:(KittyAlertButton *)button
{
    if (!button) return;
    
    [UIView animateWithDuration:kAnimationButtonHoverDuration animations:^{
        button.transform = CGAffineTransformMakeScale(kAnimationPulseScaleFactor, kAnimationPulseScaleFactor);
    }];
}

- (void)touchUpInside:(KittyAlertButton *)button
{
    if (!button) return;
    
    [UIView animateWithDuration:kAnimationShowHideDuration
                          delay:0
         usingSpringWithDamping:kAnimationSpringDamping
          initialSpringVelocity:kAnimationSpringVelocity
                        options:UIViewAnimationOptionCurveEaseInOut
                     animations:^{
        button.transform = CGAffineTransformIdentity;
    } completion:nil];
}

- (void)touchUpOutside:(KittyAlertButton *)button
{
    if (!button) return;
    
    [UIView animateWithDuration:kAnimationShowHideDuration
                          delay:0
         usingSpringWithDamping:kAnimationSpringDamping
          initialSpringVelocity:kAnimationSpringVelocity
                        options:UIViewAnimationOptionCurveEaseInOut
                     animations:^{
        button.transform = CGAffineTransformIdentity;
    } completion:nil];
}

- (void)buttonTapped:(KittyAlertButton *)sender
{
    if (sender)
    {
        NSUInteger index = [self.alertButtons indexOfObject:sender];
        if (index != NSNotFound)
        {
            if (sender.action) sender.action();
            [self dismiss];
        }
    }
}

- (void)switchChanged:(UISwitch *)sender
{
    if (sender && sender.superview && [sender.superview isKindOfClass:[KittyAlertSwitch class]])
    {
        KittyAlertSwitch *container = (KittyAlertSwitch*)sender.superview;
        NSUInteger index = [self.alertSwitches indexOfObject:container];
        if (index != NSNotFound)
        {
            if (container.action) container.action(sender.on);
        }
    }
}

- (void)setTitle:(nullable NSString *)title
{
    self.alertTitleLabel.text = title;
    [self setNeedsLayout];
}

- (void)setSubtitle:(nullable NSString *)subtitle
{
    self.alertSubtitleTextView.text = subtitle;
    [self setNeedsLayout];
}

- (void)setTintColor:(nullable UIColor *)tintColor
{
    _tintColor = tintColor ?: [self tintColorForStyle:self.alertStyle];
    
    self.alertIconView.tintColor = _tintColor;
    
    UIColor *waitDarkerTint = [self darkerColor:_tintColor];
    self.waitingIconGradientLayer.colors = @[
        (id)waitDarkerTint.CGColor,
        (id)_tintColor.CGColor,
        (id)waitDarkerTint.CGColor
    ];
    
    for (KittyAlertButton *button in self.alertButtons) {
        if (button) {
            button.backgroundColor = _tintColor;
        }
    }
    
    for (KittyAlertSwitch *switchCon in self.alertSwitches) {
        if (switchCon && switchCon.switchControl) {
            switchCon.switchControl.tintColor = _tintColor;
            switchCon.switchControl.onTintColor = _tintColor;
        }
    }
    
    self.alertIconView.image = [self iconForStyle:self.alertStyle];
    self.alertIconView.tintColor = _tintColor;
}

- (void)setBackgroundColor:(nullable UIColor *)backgroundColor
{
    _backgroundColor = backgroundColor ?: [self backgroundColorForStyle:self.alertStyle];
    self.alertContentView.backgroundColor = _backgroundColor;
}

- (void)setFontColor:(nullable UIColor *)fontColor
{
    _fontColor = fontColor ?: [self fontColorForStyle:self.alertStyle];
    
    self.alertTitleLabel.textColor = _fontColor;
    self.alertSubtitleTextView.textColor = _fontColor;
    self.alertSubtitleTextView.layer.borderColor = _fontColor.CGColor;
    
    for (KittyAlertButton *button in self.alertButtons) {
        if (button) {
            [button setTitleColor:_fontColor forState:UIControlStateNormal];
        }
    }
    
    for (KittyAlertSwitch *switchCon in self.alertSwitches) {
        if (switchCon && switchCon.switchLabel) {
            switchCon.switchLabel.textColor = _fontColor;
        }
    }
}

- (void)setUseLargeIcon:(BOOL)useLargeIcon
{
    _useLargeIcon = useLargeIcon;
    if (self.isShowing) {
        [self setNeedsLayout];
    }
}

- (void)setEnablePulseEffect:(BOOL)enablePulseEffect
{
    _enablePulseEffect = enablePulseEffect;
    if (self.isShowing) {
        [self startPulseAnimation];
    }
}

- (void)setShouldDismissOnTapOutside:(BOOL)shouldDismissOnTapOutside
{
    _shouldDismissOnTapOutside = shouldDismissOnTapOutside;
    if (self.isShowing) {
        self.dismissTapGesture.enabled = shouldDismissOnTapOutside;
    }
}

- (void)setDuration:(NSTimeInterval)duration
{
    _duration = duration;
    
    if (self.dismissTimer) {
        [self.dismissTimer invalidate];
        self.dismissTimer = nil;
    }
    
    if (duration > 0 && self.isShowing) {
        self.dismissTimer = [NSTimer scheduledTimerWithTimeInterval:duration
                                                             target:self
                                                           selector:@selector(dismiss)
                                                           userInfo:nil
                                                            repeats:NO];
    }
}

- (void)setShowAnimation:(KittyAlertViewAnimation)showAnimation
{
    _showAnimation = showAnimation;
}

- (void)setHideAnimation:(KittyAlertViewAnimation)hideAnimation
{
    _hideAnimation = hideAnimation;
}

- (void)setDismissAction:(void (^ _Nullable)(void))dismissAction
{
    if (dismissAction)
        _dismissAction = [dismissAction copy];
    else
        _dismissAction = ^(){};
}

- (UIColor *)darkerColor:(UIColor *)color
{
    CGFloat r, g, b, a;
    if ([color getRed:&r green:&g blue:&b alpha:&a]) {
        return [UIColor colorWithRed:MAX(r * 0.6, 0.0)
                               green:MAX(g * 0.6, 0.0)
                                blue:MAX(b * 0.6, 0.0)
                               alpha:a];
    }
    return color;
}

- (CALayer *)clockMaskLayerForSize:(CGFloat)size
{
    CAShapeLayer *mask = [CAShapeLayer layer];
    UIBezierPath *path = [UIBezierPath bezierPath];
    [path addArcWithCenter:CGPointMake(size / 2, size / 2)
                    radius:size / 2 - size * kIconLineWidthRatio
                startAngle:0
                  endAngle:2 * M_PI
                 clockwise:YES];
    [path addArcWithCenter:CGPointMake(size / 2, size / 2)
                    radius:size * kIconInsetRatio
                startAngle:0
                  endAngle:2 * M_PI
                 clockwise:NO];
    mask.path = path.CGPath;
    mask.fillRule = kCAFillRuleEvenOdd;
    return mask;
}

- (UIColor *)tintColorForStyle:(KittyAlertViewStyle)style
{
    switch (style) {
        case KittyAlertViewStyleSuccess: return kColorHex(kSuccessTintColorHex);
        case KittyAlertViewStyleError: return kColorHex(kErrorTintColorHex);
        case KittyAlertViewStyleWarning: return kColorHex(kWarningTintColorHex);
        case KittyAlertViewStyleInfo: return kColorHex(kInfoTintColorHex);
        case KittyAlertViewStyleEdit: return kColorHex(kEditTintColorHex);
        case KittyAlertViewStyleWaiting: return kColorHex(kWaitingTintColorHex);
        case KittyAlertViewStyleCustom: return kColorHex(kCustomTintColorHex);
    }
}

- (UIColor *)backgroundColorForStyle:(KittyAlertViewStyle)style
{
    return kColorHex(kAlertBackgroundColorHex);
}

- (UIColor *)fontColorForStyle:(KittyAlertViewStyle)style
{
    return kColorHex(kFontColorHex);
}

- (UIImage *)iconForStyle:(KittyAlertViewStyle)style
{
    if (style == KittyAlertViewStyleCustom) return nil;
    
    CGFloat screenWidth = [UIScreen mainScreen].bounds.size.width;
    CGFloat iconSize = self.useLargeIcon ? (screenWidth >= kLargeScreenThreshold ? kIconSizeLargeTablet : kIconSizeLargePhone) : (screenWidth >= kLargeScreenThreshold ? kIconSizeSmallTablet : kIconSizeSmallPhone);
    
    if (@available(iOS 10.0, *)) {
        UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(iconSize, iconSize)];
        return [renderer imageWithActions:^(UIGraphicsImageRendererContext * _Nonnull context) {
            UIColor *fillColor = self.tintColor ?: [self tintColorForStyle:style];
            [fillColor setFill];
            [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(0, 0, iconSize, iconSize)] fill];
            
            if (style != KittyAlertViewStyleWaiting) {
                NSString *letter;
                switch (style) {
                    case KittyAlertViewStyleSuccess: letter = @"✓"; break;
                    case KittyAlertViewStyleError: letter = @"✕"; break;
                    case KittyAlertViewStyleWarning: letter = @"!"; break;
                    case KittyAlertViewStyleInfo: letter = @"i"; break;
                    case KittyAlertViewStyleEdit: letter = @"✎"; break;
                    default: letter = @""; break;
                }
                
                NSDictionary *attributes = @{
                    NSFontAttributeName: [UIFont systemFontOfSize:kFontSizeIcon weight:UIFontWeightBold],
                    NSForegroundColorAttributeName: kColorHex(kIconLetterColorHex)
                };
                CGSize textSize = [letter sizeWithAttributes:attributes];
                [letter drawAtPoint:CGPointMake((iconSize - textSize.width) / 2, (iconSize - textSize.height) / 2) withAttributes:attributes];
            } else {
                UIColor *darkerFillColor = [self darkerColor:fillColor];
                [kColorHex(kIconLetterColorHex) setFill];
                [darkerFillColor setStroke];
                CGContextSetLineWidth(context.CGContext, iconSize * kIconLineWidthRatio * 2);
                [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(iconSize * kIconInsetRatio / 2, iconSize * kIconInsetRatio / 2, iconSize - iconSize * kIconInsetRatio, iconSize - iconSize * kIconInsetRatio)] stroke];
                CGContextMoveToPoint(context.CGContext, iconSize * 0.5, iconSize * 0.5);
                CGContextAddLineToPoint(context.CGContext, iconSize * 0.5, iconSize * kIconInsetRatio);
                CGContextStrokePath(context.CGContext);
            }
        }];
    } else {
        UIGraphicsBeginImageContextWithOptions(CGSizeMake(iconSize, iconSize), NO, [UIScreen mainScreen].scale);
        CGContextRef context = UIGraphicsGetCurrentContext();
        if (!context) {
            NSLog(@"KittyAlertView: Failed to get graphics context for icon");
            UIGraphicsEndImageContext();
            return nil;
        }
        
        UIColor *fillColor = self.tintColor ?: [self tintColorForStyle:style];
        CGContextSetFillColorWithColor(context, fillColor.CGColor);
        CGContextAddEllipseInRect(context, CGRectMake(0, 0, iconSize, iconSize));
        CGContextFillPath(context);
        
        if (style != KittyAlertViewStyleWaiting) {
            NSString *letter;
            switch (style) {
                case KittyAlertViewStyleSuccess: letter = @"✓"; break;
                case KittyAlertViewStyleError: letter = @"✕"; break;
                case KittyAlertViewStyleWarning: letter = @"!"; break;
                case KittyAlertViewStyleInfo: letter = @"i"; break;
                case KittyAlertViewStyleEdit: letter = @"✎"; break;
                default: letter = @""; break;
            }
            
            NSDictionary *attributes = @{
                NSFontAttributeName: [UIFont systemFontOfSize:kFontSizeIcon weight:UIFontWeightBold],
                NSForegroundColorAttributeName: kColorHex(kIconLetterColorHex)
            };
            CGSize textSize = [letter sizeWithAttributes:attributes];
            CGContextSaveGState(context);
            [letter drawAtPoint:CGPointMake((iconSize - textSize.width) / 2, (iconSize - textSize.height) / 2) withAttributes:attributes];
            CGContextRestoreGState(context);
        } else {
            UIColor *darkerFillColor = [self darkerColor:fillColor];
            CGContextSetFillColorWithColor(context, kColorHex(kIconLetterColorHex).CGColor);
            CGContextSetStrokeColorWithColor(context, darkerFillColor.CGColor);
            CGContextSetLineWidth(context, iconSize * kIconLineWidthRatio * 2);
            CGContextAddEllipseInRect(context, CGRectMake(iconSize * kIconInsetRatio / 2, iconSize * kIconInsetRatio / 2, iconSize - iconSize * kIconInsetRatio, iconSize - iconSize * kIconInsetRatio));
            CGContextStrokePath(context);
            CGContextMoveToPoint(context, iconSize * 0.5, iconSize * 0.5);
            CGContextAddLineToPoint(context, iconSize * 0.5, iconSize * kIconInsetRatio);
            CGContextStrokePath(context);
        }
        
        UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
        UIGraphicsEndImageContext();
        return image ? [image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate] : nil;
    }
}

- (void)dealloc
{
    if (self.dismissTimer) {
        [self.dismissTimer invalidate];
        self.dismissTimer = nil;
    }
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self stopPulseAnimation];
    [self stopWaitingGradientAnimation];
    [self.alertDimmingView removeGestureRecognizer:self.dismissTapGesture];
    NSLog(@"KittyAlertView deallocated");
}

@end
