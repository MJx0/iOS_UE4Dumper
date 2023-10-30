#include "DumpTransferUI.h"

@implementation DumpTransferUI

- (id)initWithFileAtPath:(NSString *)path
{
    if ((self = [super init]))
    {
        filePath = [path copy];
    }
    return self;
}

- (void)show
{
    SCLAlertView *alert = [[SCLAlertView alloc] init];
    alert.shouldDismissOnTapOutside = NO;
    alert.showAnimationType = SCLAlertViewShowAnimationSlideInFromTop;
    alert.hideAnimationType = SCLAlertViewHideAnimationSlideOutToBottom;

    UITextField *ipField = [alert addTextField:@"Device IP" setDefaultText:nil];
    UITextField *portField = [alert addTextField:@"Port number" setDefaultText:nil];
    [portField setDelegate:self];

    [alert addButton:@"Transfer"
         actionBlock:^(void) {
           NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
           UInt16 portNumber = [[formatter numberFromString:portField.text] unsignedShortValue];

           dumpTransfer = [[DumpTransfer alloc] initWithFileAtPath:filePath];
           NSError *error = nil;
           if (![dumpTransfer transferToHost:ipField.text onPort:portNumber error:&error])
           {
               showError([NSString stringWithFormat:@"Dump transfer Failed:\nErr: {%@}.", error]);
           };
         }];

    UIViewController *vc = [[[[UIApplication sharedApplication] delegate] window] rootViewController];
    [alert showEdit:vc title:@"Dump Transfer" subTitle:@"Enter receiver device IP and port" closeButtonTitle:@"Cancel" duration:0.0f];
}


#pragma mark Port UITextField Delegate

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    if ([string rangeOfCharacterFromSet:[[NSCharacterSet characterSetWithCharactersInString:@"1234567890"] invertedSet]].location != NSNotFound)
    {
        return NO;
    }

    // Prevent crashing undo bug – see note below.
    if (range.length + range.location > textField.text.length)
    {
        return NO;
    }

    NSString *stringForLen = [textField.text stringByReplacingCharactersInRange:range withString:string];
    return ([stringForLen length] <= 4);
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return true;
}

@end