# UAV-SAR

### Project Architecture

We will be using Raspberry Pi to run PayloadSDK, which will be installed and connected to the gimbal. The radar module will be located on top of the drone, while the antennas will be installed on the gimbal platform facing forward.

### SDK Structure

The top-level file is `raspberry_pi/application/main.c`. By default it will call various test functions that are defined in `module_sample/`. You can learn the APIs through those sample files.

The `common/osal/` folder contains the *operating system abstraction layer* utilities. In particular, it defines the Linux standard for tasks like creating a thread, opening a file or managing MUTEX. 

In the same way, *hardware abstraction layer* utilities are defined in `raspberry_pi/hal`. Those functions are Linux specific. If this SDK is used on other platforms, those functions would be different, while keep the functionalities the same.

### Dependencies

* `wiringPi`: A library to control the GPIO pins of Raspberry Pi. Install by `sudo apt-get install wiringpi`. Check it by typing the following command `sudo gpio -v`.

### Terminology

- The compiler used for Raspiberry Pi environment is `arm-linux-gnueabihf-gcc`, where `hf` stands for hard-float.

### Raspberry Pi Develop Environment Setup

1. Plug SD card into your computer. Inside the `boot` directory, create a file `ssh` without extension. This will enable ssh connection to the Raspberry Pi. Open `cmdline.txt` file, add `ip=169.254.xxx.xxx` at the end of the line. This will be the fixed ip address of the Raspberry Pi.
2. Connect Raspberry Pi to your computer through network cable.
3. On your computer, use any terminal tools such as `MobaXterm` to connect to the ip address you defined.
4. Tada! You've successfully get access to the Raspberry Pi!
5. If you want internet connection on Raspberry Pi, use your phone as a router, then setup a local connection to it. In the terminal of Raspberry Pi, type `sudo raspi-config`, select `system` to reveal the option for wireless connection. In this way you can conncet to the WiFi of your phone. Once your computer is connnected to the same network, you can start a ssh link.
6. Important: If you are flashing your SD card, it is possible that the Raspbeery Pi will not create a default user automatically. You need to connect it to a monitor and a keyboard to manually create an user.

### Cross Compile

If you want to compile your code on your local machine, such as on an x86_64 Linux machine, use the following command to install the right compiler:

```bash
sudo apt-get install gcc-arm-linux-gnueabihf
sudo apt-get install g++-arm-linux-gnueabihf
```

#### Toubleshooting

If the linking process failed and errors like `undefined reference to 'DjiUserUtil_GetCurrentFileDirPath'` occured, try the following steps:

1. Check `CMakeLists.txt` to see if the corresponding `.C` file is included.

2. Make sure the missing function exists in the `.C` file that is not commented out or ignored by `ifndef`.

### Requirements

#### PaylaodSDK shoud do:
1. Get GPS/RTK localization information, time and velocity from the drone.
2. Control the gimbal to level when the drone maneuvers.
3. Transmit data to controller

#### Raspberry Pi should do:
1. Run PSDK.
2. Receive the audio data from the radar and store it locally.

### PSDK Explained




