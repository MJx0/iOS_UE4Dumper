
#include <cstdio>
#include <string>
#include <fstream>
#include <utility>
#include <thread>

#import <SCLAlertView/SCLAlertView.h>
#import <SSZipArchive/ZipArchive.h>

#include "AlertUtils.h"
#include "DumpTransferUI.h"

DumpTransferUI *dumpTransferUI = nil;

#include <hash/hash.h>
#include "Core/Dumper.hpp"

#define WAIT_TIME_SEC 15
#define DUMP_FOLDER @"UE4Dumper"

#include "Core/GameProfiles/Apex.hpp"
#include "Core/GameProfiles/DBD.hpp"
#include "Core/GameProfiles/ARK.hpp"
#include "Core/GameProfiles/PUBGM.hpp"
#include "Core/GameProfiles/PES.hpp"
#include "Core/GameProfiles/Distyle.hpp"

// DumpArgs
// 1 string dump_dir, 2 string dump_headers_dir (App documents folder)
// 3 bool dump_objects, 4 bool dump_full, 5 bool dump_headers, 6 bool gen_functions_script;
  
static std::pair<Dumper::DumpArgs, IGameProfile *> UE_Games[]
{
    {{{}, {}, true, true, true, true},
     new ApexProfile()},

    {{{}, {}, true, true, true, true},
     new DBDProfile()},

    {{{}, {}, true, true, true, true},
     new ArkProfile()},

    {{{}, {}, true, true, true, true},
     new PUBGMProfile()},

    {{{}, {}, true, true, true, true},
     new PESProfile()},

    {{{}, {}, true, true, true, true},
     new DistyleProfile()},
};

void *dump_thread(void *);

__attribute__((constructor)) static void onLoad()
{
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    NSLog(@"======= I'm Loaded ========");
    pthread_t pthread;
    pthread_create(&pthread, nullptr, dump_thread, nullptr);
  });
}

void *dump_thread(void *)
{
  // wait for the application to finish initializing
  sleep(5);

  showInfo([NSString stringWithFormat:@"Dumping after %d seconds.", WAIT_TIME_SEC], WAIT_TIME_SEC / 2);

  sleep(WAIT_TIME_SEC);

  NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];

  NSString *appName = [[[NSBundle mainBundle] infoDictionary] objectForKey:(id)kCFBundleNameKey];
  NSString *appID = [[[NSBundle mainBundle] infoDictionary] objectForKey:(id)kCFBundleIdentifierKey];

  NSString *dumpFolderName = [NSString stringWithFormat:@"%@_%@", [appName stringByReplacingOccurrencesOfString:@" " withString:@""], DUMP_FOLDER];

  NSString *dumpPath = [NSString stringWithFormat:@"%@/%@", docDir, dumpFolderName];
  NSString *headersdumpPath = [NSString stringWithFormat:@"%@/%@/%@", docDir, dumpFolderName, @"headers_dump"];

  NSLog(@"UE4DUMP_PATH: %@", dumpPath);

  NSFileManager *fileManager = [NSFileManager defaultManager];

  [fileManager removeItemAtPath:dumpPath error:nil];
  [fileManager removeItemAtPath:[NSString stringWithFormat:@"%@.zip", dumpPath] error:nil];

  NSError *error = nil;

  if (![fileManager createDirectoryAtPath:headersdumpPath withIntermediateDirectories:YES attributes:nil error:&error])
  {
    NSLog(@"Failed to create folders.\nError: %@", error);
    showError([NSString stringWithFormat:@"Failed to create folders.\nError: %@", error]);
    return nullptr;
  }

  SCLAlertView *waitingAlert = nil;
  showWaiting(@"Dumping...", &waitingAlert);

  Dumper::DumpStatus dumpStatus = Dumper::UE_DS_NONE;

  for(auto &it: UE_Games)
  {
    if(strcmp(appID.UTF8String, it.second->GetAppID().c_str()) == 0)
    {
      it.first.dump_dir = dumpPath.UTF8String; 
      it.first.dump_headers_dir = headersdumpPath.UTF8String;
      dumpStatus = Dumper::Dump(&it.first, it.second);
      break;
    }
  }


  NSString *zipPath = [NSString stringWithFormat:@"%@.zip", dumpPath];
  if ([fileManager fileExistsAtPath:dumpPath])
  {
    [SSZipArchive createZipFileAtPath:zipPath withContentsOfDirectory:dumpPath];
    [fileManager removeItemAtPath:dumpPath error:nil];
  }

  dismisWaiting(waitingAlert);

  if (dumpStatus != Dumper::UE_DS_SUCCESS)
  {
    if (dumpStatus == Dumper::UE_DS_NONE)
    {
      showError(@"Not Supported | Check AppID.");
    }
    else
    {
      std::string dumpStatusStr = Dumper::DumpStatusToStr(dumpStatus);
      showError([NSString stringWithFormat:@"Dump Failed: Err: {%s}.\nLogs at: \n%@", dumpStatusStr.c_str(), dumpPath]);
    }
    return nullptr;
  }

  NSLog(@"Dump finished.");

  dispatch_async(dispatch_get_main_queue(), ^{
    UIViewController *vc = [[[[UIApplication sharedApplication] delegate] window] rootViewController];

    SCLAlertView *okAlert = [[SCLAlertView alloc] init];
    okAlert.shouldDismissOnTapOutside = YES;
    okAlert.showAnimationType = SCLAlertViewShowAnimationSlideInFromTop;
    okAlert.hideAnimationType = SCLAlertViewHideAnimationSlideOutToBottom;

    [okAlert alertIsDismissed:^{
      SCLAlertView *transferAlert = [[SCLAlertView alloc] init];
      transferAlert.shouldDismissOnTapOutside = YES;
      transferAlert.showAnimationType = SCLAlertViewShowAnimationSlideInFromTop;
      transferAlert.hideAnimationType = SCLAlertViewHideAnimationSlideOutToBottom;

      [transferAlert addButton:@"Yes"
                   actionBlock:^(void) {
                     dumpTransferUI = [[DumpTransferUI alloc] initWithFileAtPath:zipPath];
                     [dumpTransferUI show];
                   }];
      [transferAlert showEdit:vc title:@___ALERT_TITLE subTitle:@"Do you want to transfer dump over IP?" closeButtonTitle:@"No" duration:0.0f];
    }];

    [okAlert showSuccess:vc title:@___ALERT_TITLE subTitle:[NSString stringWithFormat:@"Dump at: \n%@", zipPath] closeButtonTitle:@"Ok" duration:0.0f];
  });

  return nullptr;
}