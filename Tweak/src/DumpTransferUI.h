#pragma once

#import <UIKit/UIKit.h>

#include "AlertUtils.h"
#include "DumpTransfer.h"

@interface DumpTransferUI : NSObject <UITextFieldDelegate>
{
   DumpTransfer *dumpTransfer;
   NSString *filePath;
}

- (id)initWithFileAtPath:(NSString *)path;

- (void)show;

@end