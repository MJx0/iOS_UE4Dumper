#pragma once
#include <SCLAlertView/SCLAlertView.h>

#define ___ALERT_TITLE "Meow"

void showWaiting(NSString *msg, SCLAlertView* __strong *alert);
void dismisWaiting(SCLAlertView *al);

void showSuccess(NSString *msg);
void showInfo(NSString *msg, float duration);
void showError(NSString *msg);