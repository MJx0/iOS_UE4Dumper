#include "AlertUtils.h"

void showWaiting(NSString *msg, SCLAlertView *__strong *alert)
{
  dispatch_async(dispatch_get_main_queue(), ^{
    *alert = [[SCLAlertView alloc] initWithNewWindow];
    (*alert).shouldDismissOnTapOutside = NO;
    (*alert).showAnimationType = SCLAlertViewShowAnimationSlideInFromTop;
    (*alert).hideAnimationType = SCLAlertViewHideAnimationSlideOutToBottom;
    [(*alert) showWaiting:@___ALERT_TITLE subTitle:msg closeButtonTitle:nil duration:0.0f];
  });
}

void dismisWaiting(SCLAlertView *alert)
{
  if (alert == nil) return;
  dispatch_async(dispatch_get_main_queue(), ^{
    [alert hideView];
  });
}

void showSuccess(NSString *msg)
{
  dispatch_async(dispatch_get_main_queue(), ^{
    SCLAlertView *alert = [[SCLAlertView alloc] initWithNewWindow];
    alert.shouldDismissOnTapOutside = YES;
    alert.showAnimationType = SCLAlertViewShowAnimationSlideInFromTop;
    alert.hideAnimationType = SCLAlertViewHideAnimationSlideOutToBottom;
    [alert showSuccess:@___ALERT_TITLE subTitle:msg closeButtonTitle:@"Ok" duration:0.0f];
  });
}

void showInfo(NSString *msg, float duration)
{
  dispatch_async(dispatch_get_main_queue(), ^{
    SCLAlertView *alert = [[SCLAlertView alloc] initWithNewWindow];
    alert.shouldDismissOnTapOutside = YES;
    alert.showAnimationType = SCLAlertViewShowAnimationSlideInFromTop;
    alert.hideAnimationType = SCLAlertViewHideAnimationSlideOutToBottom;
    [alert showInfo:@___ALERT_TITLE subTitle:msg closeButtonTitle:@"Ok" duration:duration];
  });
}

void showError(NSString *msg)
{
  dispatch_async(dispatch_get_main_queue(), ^{
    SCLAlertView *alert = [[SCLAlertView alloc] initWithNewWindow];
    alert.shouldDismissOnTapOutside = YES;
    alert.showAnimationType = SCLAlertViewShowAnimationSlideInFromTop;
    alert.hideAnimationType = SCLAlertViewHideAnimationSlideOutToBottom;
    [alert showError:@___ALERT_TITLE subTitle:msg closeButtonTitle:@"Ok" duration:0.0f];
  });
}