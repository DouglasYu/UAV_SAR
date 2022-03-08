# UAV-SAR

### Architecture

We will be using Raspberry Pi to run PayloadSDK, which will be installed and connected to the gimbal. The radar module will be located on top of the drone, while the antennas will be installed on the gimbal platform facing forward.

### SDK Structure

The top-level file is `raspberry_pi/application/main.c`. By default it will call various test functions that are defined in `module_sample/`. You can learn the APIs through those sample files. The `common/osal/` folder contains the *operating system abstraction layer* utilities. In particular, it defines the Linux standard for tasks like creating a thread, opening a file or managing MUTEX. In the same way, *hardware abstraction layer* utilities are defined in `raspberry_pi/hal`. Those functions are Linux specific. If this SDK is used on other platforms, those functions would be different, while keep the functionalities the same.
