
#include <cstdio>
#include <string>
#include <fstream>
#include <utility>
#include <thread>
#include <vector>

#import <SSZipArchive/ZipArchive.h>

#import "Utils/AlertUtils.h"

#include <hash/hash.h>
#include "Core/Dumper.hpp"

#include "Core/GameProfiles/Farlight.hpp"
#include "Core/GameProfiles/DBD.hpp"
#include "Core/GameProfiles/ARK.hpp"
#include "Core/GameProfiles/PUBGM.hpp"
#include "Core/GameProfiles/PES.hpp"
#include "Core/GameProfiles/Distyle.hpp"
#include "Core/GameProfiles/Torchlight.hpp"
#include "Core/GameProfiles/MortalKombat.hpp"
#include "Core/GameProfiles/ArenaBreakout.hpp"
#include "Core/GameProfiles/BlackClover.hpp"

#define DUMP_DELAY_SEC 25
#define DUMP_FOLDER @"UEDump"

static IGameProfile *UE_Games[] =
{
    new FarlightProfile(),
    new DBDProfile(),
    new ArkProfile(),
    new PUBGMProfile(),
    new PESProfile(),
    new DistyleProfile(),
    new TorchlightProfile(),
    new MortalKombatProfile(),
    new ArenaBreakoutProfile(),
    new BlackCloverProfile()
};

void dump_thread();

__attribute__((constructor)) static void onLoad()
{
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSLog(@"======= I'm Loaded ========");
        std::thread(dump_thread).detach();
    });
}

void dump_thread()
{
    // wait for the application to finish initializing
    sleep(5);
    
    Alert::showInfo([NSString stringWithFormat:@"Dumping after %d seconds.", DUMP_DELAY_SEC], nil, DUMP_DELAY_SEC / 2.f);
    
    sleep(DUMP_DELAY_SEC);
    
    NSString *docDir = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)[0];
    
    NSString *nsAppName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"];
    std::string appName = ioutils::remove_specials(nsAppName.UTF8String);
    
    NSString *appID = [[[NSBundle mainBundle] infoDictionary] objectForKey:(id)kCFBundleIdentifierKey];
    
    NSString *dumpFolderName = [NSString stringWithFormat:@"%s_%@", appName.c_str(), DUMP_FOLDER];
    
    NSString *dumpPath = [NSString stringWithFormat:@"%@/%@", docDir, dumpFolderName];
    NSString *zipdumpPath = [NSString stringWithFormat:@"%@.zip", dumpPath];
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    
    KittyAlertView *waitingAlert = Alert::showWaiting(@"Dumping...", @"This could take some time...");
    
    Dumper::DumpStatus dumpStatus = Dumper::UE_DS_NONE;
    std::unordered_map<std::string, BufferFmt> buffersMap;
    
    auto dmpStart = std::chrono::steady_clock::now();
    
    for (auto &it : UE_Games)
    {
        for (auto &pkg : it->GetAppIDs())
        {
            if (pkg.compare(appID.UTF8String) == 0)
            {
                dumpStatus = Dumper::Dump(it, &buffersMap);
                goto done;
            }
        }
    }
done:
    
    if (dumpStatus == Dumper::UE_DS_NONE)
    {
        Alert::dismiss(waitingAlert);
        Alert::showError(@"Not Supported, Check AppID", nil);
        return;
    }
    
    if (buffersMap.empty())
    {
        Alert::dismiss(waitingAlert);
        Alert::showError(@"Dump Failed", [NSString stringWithFormat:@"Error <Buffers empty>.\nStatus <%s>", Dumper::DumpStatusToStr(dumpStatus).c_str()], nil);
        return;
    }
    
    execOnUIThread(^(){
        [waitingAlert setTitle:@"Saving Files..."];
    });
    
    if ([fileManager fileExistsAtPath:dumpPath])
    {
        [fileManager removeItemAtPath:dumpPath error:nil];
    }
    
    if ([fileManager fileExistsAtPath:zipdumpPath])
    {
        [fileManager removeItemAtPath:zipdumpPath error:nil];
    }
    
    NSError *error = nil;
    if (![fileManager createDirectoryAtPath:dumpPath withIntermediateDirectories:YES attributes:nil error:&error])
    {
        Alert::dismiss(waitingAlert);
        NSLog(@"Failed to create folders\nError: %@", error);
        Alert::showError(@"Failed to create folders", [NSString stringWithFormat:@"Error: %@", error]);
        return;
    }
    
    for (const auto &it : buffersMap)
    {
        if (!it.first.empty())
        {
            NSString *path = [NSString stringWithFormat:@"%@/%s", dumpPath, it.first.c_str()];
            it.second.writeBufferToFile(path.UTF8String);
        }
    }
    
    if ([SSZipArchive createZipFileAtPath:zipdumpPath withContentsOfDirectory:dumpPath] == YES)
    {
        [fileManager removeItemAtPath:dumpPath error:nil];
    }
    else
    {
        Alert::dismiss(waitingAlert);
        Alert::showError(@"Failed to zip dump folder", [NSString stringWithFormat:@"Folder: %@", dumpPath]);
        return;
    }
    
    auto dmpEnd = std::chrono::steady_clock::now();
    std::chrono::duration<float, std::milli> dmpDurationMS = (dmpEnd - dmpStart);
    
    Alert::dismiss(waitingAlert);

    ui_action_block_t shareAction = ^() {
        Alert::showNoOrYes(@"Share Dump", @"Do you want to share/transfer dump ZIP file?", nil, ^(){
            NSURL *zipFileURL = [NSURL fileURLWithPath:zipdumpPath];
            NSArray *activityItems = @[zipFileURL];
            UIActivityViewController *activityVC = [[UIActivityViewController alloc] initWithActivityItems:activityItems applicationActivities:nil];
            
            auto mainVC = GetTopViewController();
            if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
                activityVC.popoverPresentationController.sourceView = mainVC.view;
                activityVC.popoverPresentationController.sourceRect = CGRectMake(mainVC.view.bounds.size.width / 2, mainVC.view.bounds.size.height / 2, 1, 1);
            }
            [mainVC presentViewController:activityVC animated:YES completion:nil];
        });
    };
    
    if (dumpStatus == Dumper::UE_DS_SUCCESS)
    {
        Alert::showSuccess(@"Dump Succeeded", [NSString stringWithFormat:@"Duration: %.2fms\nPath:\n%@", dmpDurationMS.count(), zipdumpPath], shareAction);
    }
    else
    {
        Alert::showError(@"Dump Failed", [NSString stringWithFormat:@"Error <%s>.\nDuration: %.2fms\nDump Path:\n%@", Dumper::DumpStatusToStr(dumpStatus).c_str(), dmpDurationMS.count(), zipdumpPath], shareAction);
    }
}
