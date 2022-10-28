# iOS Unreal Engine 4 Dumper / UE4 Dumper

MobileSubstrate Tweak to dump Unreal Engine 4 games on iOS.

The dumper is based on [UE4Dumper-4.25](https://github.com/guttir14/UnrealDumper-4.25)
project.

## Features
* Supports ARM64 & ARM64e
* CodeSign friendly, you can use this as a jailed tweak
* Dumps UE4 classes, structs, enums and functions
* Generates function names json script to use with IDA & Ghidra
* pattern scanning to find the GUObjectArray, GNames and FNamePoolData addresses automatically
* Transfer dump files over IP

## Currently Supported Games
* Dead by Daylight Mobile
* Apex Legends Mobile
* PUBGM International
* ARK Survival
* eFootBall 2023 (PES Mobile)
* Distyle

## Usage
Download the pre-compiled deb or compile it by yourself.
Install the debian package on your device with Filza or by any method you prefer.
Open one of the supported games and wait for the message pop-up to appear.
It will say that the dumping will begin soon.
Wait for the dumper to complete the process.
Another pop-up will appear showing the dump result and the dump files location.
After this a third pop-up will appear showing you the optional function to directly transfer the dump files to your pc.

## Output-Files

##### headers_dump
* C++ headers that you can use in your source, however the headers might not compile directly without a change

##### FullDump.hpp
* An all-in-one dump file

##### logs.txt
* Logfile containing dump process logs

##### objects_dump.txt
* ObjObjects dump

##### script.json
* If you are familiar with Il2cppDumper script.json, this is similar
* It contains a json array of function names and addresses

## How to transfer the dump from the device to the pc without SSH connection
The transfer process uses TCP to connect to an ip on a port. You can use netcat
for example on your pc to listen on a specific port.
make sure your pc and iOS device connected to the same wifi.
in the dumper transfer UI type your pc ip and the
port you chose. You need to open a port on
your pc before clicking "transfer".

## NetCat for Windows
NetCat comes with NMap and you can download it from [here](https://nmap.org/book/inst-windows.html)
After you install it, make sure NMap binary folder is in your windows
environment path. Now open the command line and type:
```
ncat -l <port> > <filename>.zip
```

## NetCat for Linux and Mac
You can download NetCat from your terminal directly. Make sure the incomming
connections are not blocked before using this.
```
nc -l <port> > <filename>.zip
```
## Adding a new game to the Dumper
Follow the prototype in Tweak/src/Core/GameProfiles<br/>
You can also use the provided patterns to find GUObjectArray, GNames or NamePoolData.


## Credits & Thanks
- [UE4Dumper-4.25](https://github.com/guttir14/UnrealDumper-4.25)
- [Il2cppDumper](https://github.com/Perfare/Il2CppDumper/blob/master/README.md)
- @Katzi for testing and writing this README for me XD
