#import "AlertUtils.h"

UIWindow *GetMainWindow()
{
    for (UIScene *scene in [UIApplication sharedApplication].connectedScenes) {
        if (scene && [scene isKindOfClass:[UIWindowScene class]]) {
            UIWindowScene *windowScene = (UIWindowScene *)scene;
            for (UIWindow *window in windowScene.windows) {
                if (window && window.isKeyWindow) {
                    return window;
                }
            }
        }
    }
    
    return nil;
}

UIViewController *GetTopViewController()
{
    UIWindow *w = GetMainWindow();
    if(!w || !w.rootViewController) return nil;
    
    UIViewController *con = w.rootViewController;
    while(con.presentedViewController)
        con = con.presentedViewController;
    
    return con;
}

void execOnUIThread(ui_action_block_t block)
{
    if (block) {
        if ([NSThread isMainThread]) {
            block();
        } else {
            dispatch_sync(dispatch_get_main_queue(), ^{ block(); });
        }
    }
}

namespace Alert
{
    void dismiss(KittyAlertView *alert)
    {
        execOnUIThread(^(){
            if (alert) [alert dismiss];
        });
    }
    
    KittyAlertView *showWaiting(NSString *title, NSString *msg)
    {
        __block KittyAlertView *waitingAlert = nil;
        execOnUIThread(^{
            waitingAlert = [KittyAlertView createWithParentView:GetMainWindow() forStyle:KittyAlertViewStyleWaiting];
            
            waitingAlert.shouldDismissOnTapOutside = NO;
            waitingAlert.enablePulseEffect = YES;
            waitingAlert.useLargeIcon = YES;
            waitingAlert.showAnimation = KittyAlertViewAnimationBounce;
            waitingAlert.hideAnimation = KittyAlertViewAnimationRotate;
            
            [waitingAlert showWithTitle:title
                               subtitle:msg
                       closeButtonTitle:nil
                               duration:0];
        });
        return waitingAlert;
    }
    
    void showSuccess(NSString *title, NSString *msg, ui_action_block_t actionBlock)
    {
        execOnUIThread(^{
            KittyAlertView *alertView = [KittyAlertView createWithParentView:GetMainWindow() forStyle:KittyAlertViewStyleSuccess];
            
            alertView.shouldDismissOnTapOutside = NO;
            alertView.enablePulseEffect = YES;
            alertView.useLargeIcon = YES;
            alertView.showAnimation = KittyAlertViewAnimationBounce;
            alertView.hideAnimation = KittyAlertViewAnimationRotate;
            alertView.dismissAction = actionBlock;
            
            [alertView showWithTitle:title
                            subtitle:msg
                    closeButtonTitle:@"Ok"
                            duration:0];
        });
    }
    
    void showInfo(NSString *title, NSString *msg, float durationSeconds, ui_action_block_t actionBlock)
    {
        execOnUIThread(^{
            KittyAlertView *alertView = [KittyAlertView createWithParentView:GetMainWindow() forStyle:KittyAlertViewStyleInfo];
            
            alertView.shouldDismissOnTapOutside = NO;
            alertView.enablePulseEffect = YES;
            alertView.useLargeIcon = YES;
            alertView.showAnimation = KittyAlertViewAnimationBounce;
            alertView.hideAnimation = KittyAlertViewAnimationRotate;
            alertView.dismissAction = actionBlock;
            
            [alertView showWithTitle:title
                            subtitle:msg
                    closeButtonTitle:@"Ok"
                            duration:durationSeconds];
        });
    }
    
    void showError(NSString *title, NSString *msg, ui_action_block_t actionBlock)
    {
        execOnUIThread(^{
            KittyAlertView *alertView = [KittyAlertView createWithParentView:GetMainWindow() forStyle:KittyAlertViewStyleError];
            
            alertView.shouldDismissOnTapOutside = NO;
            alertView.useLargeIcon = YES;
            alertView.enablePulseEffect = YES;
            alertView.showAnimation = KittyAlertViewAnimationBounce;
            alertView.hideAnimation = KittyAlertViewAnimationRotate;
            alertView.dismissAction = actionBlock;
            
            [alertView showWithTitle:title
                            subtitle:msg
                    closeButtonTitle:@"Ok"
                            duration:0];
        });
    }
    
    void showNoOrYes(NSString *title, NSString *msg, ui_action_block_t noBlock, ui_action_block_t yesBlock)
    {
        execOnUIThread(^{
            KittyAlertView *alertView = [KittyAlertView createWithParentView:GetMainWindow() forStyle:KittyAlertViewStyleEdit];
            
            alertView.shouldDismissOnTapOutside = NO;
            alertView.useLargeIcon = YES;
            alertView.enablePulseEffect = YES;
            alertView.showAnimation = KittyAlertViewAnimationBounce;
            alertView.hideAnimation = KittyAlertViewAnimationRotate;
            
            [alertView addButtonWithTitle:@"No" action:noBlock];
            [alertView addButtonWithTitle:@"Yes" action:yesBlock];
            
            [alertView showWithTitle:title
                            subtitle:msg
                    closeButtonTitle:nil
                            duration:0];
        });
    }
}
