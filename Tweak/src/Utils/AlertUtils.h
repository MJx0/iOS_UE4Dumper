#pragma once

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "KittyAlertView.h"

UIWindow *GetMainWindow();
UIViewController *GetTopViewController();

typedef void (^ui_action_block_t)(void);
void execOnUIThread(ui_action_block_t block);

namespace Alert
{
    void dismiss(KittyAlertView *alert);

    KittyAlertView *showWaiting(NSString *title, NSString *msg);

    void showSuccess(NSString *title, NSString *msg, ui_action_block_t actionBlock = nil);
    
    void showInfo(NSString *title, NSString *msg, float durationSeconds=0, ui_action_block_t actionBlock=nil);
    
    void showError(NSString *title, NSString *msg, ui_action_block_t actionBlock = nil);
    
    void showNoOrYes(NSString *title, NSString *msg, ui_action_block_t noBlock, ui_action_block_t yesBlock);
}
