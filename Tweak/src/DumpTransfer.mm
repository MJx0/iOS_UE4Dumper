#include "DumpTransfer.h"

@implementation DumpTransfer

#define DUMP_TRANSFER_TAG 67

- (id)initWithFileAtPath:(NSString *)path
{
	if ((self = [super init]))
	{
		dispatch_queue_t mainQueue = dispatch_get_main_queue();
		asyncSocket = [[GCDAsyncSocket alloc] initWithDelegate:self delegateQueue:mainQueue];

		filePath = [path copy];
	}
	return self;
}

- (BOOL)transferToHost:(NSString *)host onPort:(UInt16)port error:(NSError **)error
{
	NSLog(@"Connecting to \"%@\" on port %hu...", host, port);
	return [asyncSocket connectToHost:host onPort:port error:error];
}

#pragma mark Socket Delegate

- (void)socket:(GCDAsyncSocket *)sock didConnectToHost:(NSString *)host port:(UInt16)port
{
	NSLog(@"socket:%p didConnectToHost:%@ port:%hu", sock, host, port);

	NSError *error = nil;
	NSData *data = [NSData dataWithContentsOfFile:filePath options:0 error:&error];
	if (data == nil)
	{
		NSLog(@"Error reading data from %@ with error: %@", filePath, error);
		showError([NSString stringWithFormat:@"Error reading transfer file: %@", error]);
		return;
	}
	showWaiting(@"Transfering...", &progressAlert);
	[asyncSocket writeData:data withTimeout:-1 tag:DUMP_TRANSFER_TAG];
}

- (void)socketDidSecure:(GCDAsyncSocket *)sock
{
	NSLog(@"socketDidSecure:%p", sock);
}

- (void)socket:(GCDAsyncSocket *)sock didWriteDataWithTag:(long)tag
{
	if (tag == DUMP_TRANSFER_TAG)
	{
		dismisWaiting(progressAlert);
		showSuccess(@"Transfer done. Wait for few seconds to make sure dump file has been fully received on listener device.");
	}
	NSLog(@"socket:%p didWriteDataWithTag:%ld", sock, tag);
}

- (void)socket:(GCDAsyncSocket *)sock didReadData:(NSData *)data withTag:(long)tag
{
	NSLog(@"socket:%p didReadData:withTag:%ld", sock, tag);
}

- (void)socketDidDisconnect:(GCDAsyncSocket *)sock withError:(NSError *)err
{
	dismisWaiting(progressAlert);
	showError([NSString stringWithFormat:@"Disconnected with error \"%@\"", err]);
	NSLog(@"socketDidDisconnect:%p with error: %@", sock, err);
}

@end