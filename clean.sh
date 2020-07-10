# libbtstack
make -C firmware/libbtstack clean

# libconn
make -C firmware/libconn clean

# libadb
make -C firmware/libadb clean

# libsdcard
make -C firmware/libsdcard clean

# libusb
make -C firmware/libusb clean
make -C firmware/libusb DEFAULTCONF=PIC24FJ256GB206_CDC clean

# app_layer_v1
make -C firmware/app_layer_v1 clean

# device_bootloader
make -C firmware/device_bootloader clean


