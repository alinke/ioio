
# create Makefiles from configurationss.xml files
prjMakefilesGenerator.sh firmware/libbtstack
echo "firmware/libbtstack DONE"

prjMakefilesGenerator.sh firmware/libconn
echo "firmware/libconn DONE"

prjMakefilesGenerator.sh firmware/libadb
echo "firmware/libadb DONE"

prjMakefilesGenerator.sh firmware/libsdcard
echo "firmware/libsdcard DONE"

prjMakefilesGenerator.sh firmware/libusb
echo "firmware/libusb DONE"

prjMakefilesGenerator.sh firmware/app_layer_v1
echo "firmware/app_layer_v1 DONE"

prjMakefilesGenerator.sh firmware/device_bootloader
echo "firmware/device_bootloader DONE"

