# Stereognosis Protocol Main

Code for the main rig-controlling application for the Stereognosis project.

Locally interacts with the devices:
1. Light sensors in the armrest
2. 4 Motors:
    i. Translation Z - moves the object closer and further from the animal.
    ii. Tilt - tilts the object left and right
    iii and iv. Aperture - currently only using the left (iii) to control. Mechanical links of (iv) are damaged.
3. Reward system

Interacts with:
1. Servers controlling FLIR cameras via gRPC messages (see [repository](https://github.com/BensmaiaLab/cameras_server))
2. Neural recording via synchronizing TTL pulses
3. Tekscan pressure sensors via gRPC messages (see [repository](https://github.com/BensmaiaLab/tekscan_pressure_sensor_server))

## Order of Operations

How to run an experiment can be found [here](https://github.com/BensmaiaLab/stereognosis_protocol_main/blob/master/documentation/SOP.docx).

## Data Synchronization and Logs

The main and auxillary programs save extensive logs of what is happening. This section details the saved information and how to use it for synchronization of all data.

### Synchronization Concept

After the object is configured, messages are sent to cameras and pressure sensor (PS). This gives cameras and pressure sensor some time to start recording while the object is moving towards the animal. They record the time right when the recording starts. When the object is in position, a synchronization message is sent to PS and cameras together with TTL pulse for ePhys. Al of these messages and the pulse happen within 2 ms, assumed to record the time simultaneously and should be used as ZERO for trial alignment between devices. On the main protocol, it is saved as `log_sent_start_sync_messages(ms)`, on each device as `syncTrialStartTime(ms)`. At the end of the trial, another sync message (`log_sent_end_sync_messages(ms)` and `syncTrialEndTime(ms)`) is sent together with falling TTL pulse, which are also executed within 2 ms. Then the devices get a message to stop recording.

The periods between synchronization pulses were measured on main protocol, camera servers and pressure sensor. Their variability is < 1.5 ms.

To put the Main Protocol events to 0, subtract `log_sent_start_sync_messages(ms)` from each logged timepoint.

To put Pressure Sensor data in sync, one should add `startedRecording(ms)`-`syncTrialStartTime(ms)` to the internal time saved in the FSX files for that trial. It is expected for the some first samples to have negative values since recording starts before the sync pulse.

Images from Camera Servers can be aligned in the same way as Pressure Sensor, but there is a more precise way using trial-specific CSVs. For each trial for each camera, a CSV is saved with system and camera timestamps for each captured image. The `Global Time Stamp (msec)` time of the first image for the camera can be used instead of `startedRecording(ms)`. They are provided by the same clock, but `Global Time Stamp (msec)` corresponds to the actual first image captured, while `startedRecording(ms)` - to initialization of recording, so the former is more precise for synchronization. After the start of recording, all images from all cameras are synchronized via cable pulses, so only one camera timestamps need to be synchronized, all the rest can use the same exact timeline. List of which cameras connected to which PCs:

PC2: 19340298, 19340300, 19335177, 19194009
PC3: 19340396, 19194008, 19194005, 20050811

### Main Protocol

Log with name `<folder containing executable>/data/Protocol_<DATETIME>.txt` mostly contains debug information, like results of initialization, failures and warnings.

Log with name `<folder containing executable>/data/session_<DATETIME1>_from_<SESSION NAME>.csv` contains timestamps of different events during trial progress. In the beginning of the trial, values are set to 0, filled in during trial execution, and saved when trial finishes. All times are in internal clock in ms. Here is a list of all columns in order of them filling during a trial, with description of the main commands/functions being run between each saved timestamp. Note that the order does not match the file column exactly.

1. `trial_num` - number/index of the trial.
2. `repeating_trial` - 1 if trial with these parameters has ran before (usually repeated because the first one failed).
3. `reward` - 1 if reward was issued.

'Start Trial' button was pressed or timout happend if running on automatic loop. If light sensors are enabled, they have to be covered for the trial to start.

4. `trial_start_time(ms)` - when trial starts.

Messages with camera configs are sent to camera servers.

5. `log_sent_config_to_cameras(ms)`

Motors preshape the object (set tilt and aperture) and a message is sent to cameras to start recording. A separate thread monitors if monkey grabs the object before it is in position.

6. `log_started_camera_recording(ms)` - NOTICE that in the csv file this and following values are saved in column that follows `arm_liftoff_time(ms)`.

Message is sent to the pressure sensor to start recording.

7. `log_started_ps_recording(ms)`

Object approaches the animal and is put in position.

8. `object_in_position_time(ms)`

The thread monitoring early grab is stopped. If light sensors are enabled, wait until they are uncovered.

9. `arm_liftoff_time(ms)`

Send rising TTL pulse to EPhys signifying start of the trial. Preliminary measurements suggest that it takes < 1 ms.

10. `log_started_ephys_recording(ms)` 

Send START synchronization messages to both camera servers and the pressure sensor. Preliminary measurements suggest that it takes < 2 ms.

11. `log_sent_start_sync_messages(ms)` This timepoint should be used as 0 for all recordings.

Start monitoring pressure sensor for success conditions.

12. `log_started_monitoring_ps(ms)`

Sufficient force is exerted on both pressure sensors to identify a start of grab.

13. `started_touching_time(ms)`

Trial ends successfully or not. Stopping monitoring the pressure sensor for success conditions.

14. `trial_end_time(ms)`

Issue reward or play a failure sound. If the trial failed, it will be appended to the end of the session. Retreat the object away.

15. `log_starting_finishing_recordings(ms)`

Send a falling TTL pulse to EPhys and END synchronization messages to both cameras servers and pressure sensor server. In preliminary data, it happens within 3 ms.

16. `log_sent_end_sync_messages(ms)`

Send message to camera servers to stop recording.

17. `log_stopped_camera_recordings(ms)`

Send message to pressure sensors to stop recording.

18. `log_stopped_ps_recordings(ms)`

19. `trial_finished_time(ms)`

Save the timestamps into a CSV log.

20. `pos_translation_z(mm)`, `pos_tilt(deg)` and `pos_aperture(mm)` - specific object configuration.

### Pressure Server

Log with name `<data folder>/<experiment_DATETIME>/trial_log.csv` has the following columns:

1. `trial_num` - the trial number/index.
2. `startedRecording(ms)` - timestamp right before the recording starts.
3. `syncTrialStartTime(ms)` - time of sync pulse. This time should be used as ZERO for the purposes of trial synchronization.
4. `syncTrialEndTime(ms)` - second synchronization pulse.
5. `finishedRecording(ms)` - time when the recording was stopped.

### Cameras 

Log with name `<data folder>/<experiment_DATETIME>/trial_log.csv` has the same structure as the Pressure Sensor one.

In addition, a log is saved with each trial in: `<data folder>/<experiment_DATETIME>/<CAMERA_NAME>/trial<NUMBER>/<CAMERA_NAME>.csv`. It has the following columns:

1. `Frame #` - # of the frame.
2. `Time stamp (msec)` - time stamp using precise internal camera clock.
3. `Global Time Stamp (msec)` - time stamp using system clock.

I recommend using `Global Time Stamp (msec)` of frame 0 instead of `startedRecording(ms)`. All images within a trial are synchronized, so time offset needs to be calculated only once, and then can be applied to all cameras.

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

3. [NIDAQ 16.0+ Controller](https://www.ni.com/en-us/support/downloads/drivers/download.ni-daqmx.html#348669). For reward and light sensors working through a NIUSB card.

NIDAQ_HOME (C:\Program Files (x86)\National Instruments) must be added as an environment variable.

4. ClearView Motor Control Library from `\\BENSMAIA-LAB\LabSharing\Stereognosis\Rig Dependencies`.

5. Build for x64 RELEASE. For some reason, DEBUG makes either light sensor or motors break the execution. Possibly not an issue anymore.
