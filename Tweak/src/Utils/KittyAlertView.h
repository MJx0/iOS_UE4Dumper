#ifndef KittyAlertView_h
#define KittyAlertView_h

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, KittyAlertViewStyle) {
    KittyAlertViewStyleSuccess,
    KittyAlertViewStyleError,
    KittyAlertViewStyleWarning,
    KittyAlertViewStyleInfo,
    KittyAlertViewStyleEdit,
    KittyAlertViewStyleWaiting,
    KittyAlertViewStyleCustom
};

typedef NS_ENUM(NSInteger, KittyAlertViewAnimation) {
    KittyAlertViewAnimationFade,
    KittyAlertViewAnimationSlideFromTop,
    KittyAlertViewAnimationSlideFromBottom,
    KittyAlertViewAnimationScale,
    KittyAlertViewAnimationBounce,
    KittyAlertViewAnimationRotate
};

NS_ASSUME_NONNULL_BEGIN

@interface KittyAlertButton : UIButton

@property (nonatomic, copy, nullable) void (^action)(void);

@end

@interface KittyAlertSwitch : UIView

@property (nonatomic, strong) UILabel *switchLabel;
@property (nonatomic, strong) UISwitch *switchControl;
@property (nonatomic, copy, nullable) void (^action)(bool);

@end

@interface KittyAlertView : UIView

@property (nonatomic, assign) KittyAlertViewAnimation showAnimation;
@property (nonatomic, assign) KittyAlertViewAnimation hideAnimation;
@property (nonatomic, assign) BOOL shouldDismissOnTapOutside;
@property (nonatomic, assign) NSTimeInterval duration;
@property (nonatomic, assign) BOOL enablePulseEffect;
@property (nonatomic, assign) BOOL useLargeIcon;
@property (nonatomic, copy, nullable) void (^dismissAction)(void);
@property (nonatomic, readonly, assign) BOOL isShowing;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;
+ (instancetype)alloc NS_UNAVAILABLE;
+ (instancetype)allocWithZone:(struct _NSZone *)zone NS_UNAVAILABLE;

+ (instancetype)createWithParentView:(UIView *)view forStyle:(KittyAlertViewStyle)style;

- (void)showWithTitle:(nullable NSString *)title
         subtitle:(nullable NSString *)subtitle
 closeButtonTitle:(nullable NSString *)closeButtonTitle
             duration:(NSTimeInterval)duration;

//- (UITextField *)addTextFieldWithPlaceholder:(nullable NSString *)placeholder;

- (KittyAlertButton *)addButtonWithTitle:(NSString *)title action:(void (^ _Nullable)(void))action;

- (KittyAlertSwitch *)addSwitchWithTitle:(NSString *)title initialState:(BOOL)initialState action:(void (^ _Nullable)(BOOL))action;

- (void)dismiss;

- (void)setTitle:(nullable NSString *)title;
- (void)setSubtitle:(nullable NSString *)subtitle;

- (void)setTintColor:(nullable UIColor *)tintColor;
- (void)setBackgroundColor:(nullable UIColor *)backgroundColor;
- (void)setFontColor:(nullable UIColor *)fontColor;

@end

NS_ASSUME_NONNULL_END

#endif /* KittyAlertView_h */
