# Stereognosis Protocol Main

Code for the main rig-controlling application for the Stereognosis project.

Locally interacts with the devices:
1. Light sensors
2. Motor (1, back and forth one)
3. Reward system

Remotely interacts with:
1. Servers controlling FLIR cameras (see nishbo/cameras_client repository)
2. Neural recording via TTL pulse (not implemented)
3. Pressure sensors (not implemented)

## Order of Operations

Assuming you have a compiled versions of all programs (located usually in "\ProtocolApp\App" directory).

1. Initialize camera servers:
    i. Start camera server App.
    ii. Init cameras.
    iii. Run intrinsic calibration and extrinsic calibration in NCams.
    iv. Start camera server.
    v. If you are launching the server on another computer, open command window (`cmd.exe`) and type `ipconfig`. You will need "IPv4 Address" under "Ethernet adapter Ethernet:". Write that ip address into the camera server IP field of THIS APP.
2. Main protocol:
    i. Start main protocol App.
    ii. Connect to camera servers. If a camera server is on a different PC, write in the correct IP and make sure that the port matches.
    iii. Make sure that the framerate, length of recording, and other parameters matches the desired.
    iv. Init Protocol.
    v. Run trials... While running trials make sure to wait until the 'Prepare' button on camera server becomes enabled - it means that the images were saved.

## Build

Required: Visual Studio 2019 with C++ development, .NET desktop development, Visual++ ATL, Visual++ MFC and MFC libraries for x64.

1. Install [vcpkg](https://github.com/microsoft/vcpkg) package installation system for Windows. Integrate it with Visual Studio:

```
>bootstrap-vcpkg.bat -disableMetrics
>vcpkg integrate install 
```

2. Using vcpkg install [gRPC](https://github.com/grpc/grpc) messenger system, for most projects we need x64 architecture, therefore the `--triplet` option. [CPP manual](https://github.com/grpc/grpc/tree/master/src/cpp).

```
vcpkg install grpc --triplet x64-windows
```

3. [NIDAQ 16.0+ Controller](https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html#348669). For motors and reward?

NIDAQ_HOME (C:\Program Files (x86)\National Instruments) must be added as an environment variable.

4. ClearView Motor Control Library from `\\BENSMAIA-LAB\LabSharing\Stereognosis\Rig Dependencies`.

5. Build for x64 RELEASE. For some reason, DEBUG makes either light sensor or motors break the execution.
