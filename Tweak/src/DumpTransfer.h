#pragma once

#import <UIKit/UIKit.h>

#import <GCDAsyncSocket/GCDAsyncSocket.h>

#include "AlertUtils.h"

@interface DumpTransfer : NSObject <GCDAsyncSocketDelegate>
{
    GCDAsyncSocket *asyncSocket;
    NSString *filePath;
    SCLAlertView *progressAlert;
}

- (id)initWithFileAtPath:(NSString *)path;

- (BOOL)transferToHost:(NSString *)host onPort:(UInt16)port error:(NSError **)error;

@end