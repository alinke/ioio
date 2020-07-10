# libbtstack
make -C firmware/libbtstack

# libconn
make -C firmware/libconn

# libadb
make -C firmware/libadb

# libsdcard
make -C firmware/libsdcard

# libusb
make -C firmware/libusb
make -C firmware/libusb DEFAULTCONF=PIC24FJ256GB206_CDC

# app_layer_v1
make -C firmware/app_layer_v1

# device_bootloader
make -C firmware/device_bootloader
