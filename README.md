libhowler
====================

Device drivers for the [Wolfware Howler Arcade Controller](http://www.wolfwareeng.com/)

### Requirements

* [CMake](www.cmake.org) (Version 2.8 or greater)
* [libusb](www.libusb.org) (Version 1.0 or greater)

### Before using the library

The library interfaces to the Howler Controller via libusb. By default, when
attaching the Howler Controller, the kernel will identify it as a HID device
and create the corresponding mount points in `/dev/input`. In order to interface
to an application and write libraries that can use the controller, we use libusb.
In order to properly interface with the device, we must have the proper permissions
to send instructions to the device. For that reason, there is an included
permissions file to be used with `udev` for allowing all users to read/write the
controller:

    70-howler-controller.rules

Depending on your version of Ubuntu, the syntax of the `.rules` file may not
correspond to the one provided in this file. For Ubuntu 12.04 the following commands
will properly setup the USB permission to be used with `libusb`:

    $ sudo cp 70-howler-controller.rules /lib/udev/rules.d/
    $ sudo udevadm control --reload-rules && sudo udevadm trigger
    
To check whether or not the permissions have been set properly, you can look at
the filesystem permissions of the mounted file:

    $ lsusb -d 03eb:
    Bus 004 Device 004: ID 03eb:6800 Atmel Corp.
    
    # From the previous return value, we can find the bus and usb value that
    # are associated with any connected Howler Controllers, and then we can
    # check the associated mounted file by looking at the following files:
    #                    bus dev
    $ ls -l /dev/bus/usb/004/004
    crw-rw-rw- 1 root root 189, 387 Apr 24 02:56 /dev/bus/usb/004/004
    
If anything went wrong, make sure that the rules are being loaded properly:

    $ udevadm test /dev/bus/usb/004/004

### Build the library and examples

    $ mkdir howler && cd howler
    $ git clone https://github.com/Mokosha/howler-drivers-linux.git src
    $ mkdir build && cd build
    $ cmake ../src -DCMAKE_BUILD_TYPE=Release
    $ make
    
    # Run the example...
    $ ./howlerex
