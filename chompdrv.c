#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include<stdbool.h>

//global variables
int fd;
struct uinput_user_dev uinp;

unsigned char *return_byte (unsigned char source);

/*
This function is used to setup the uinput device /dev/uinput. It open a uinput file, setup uinput device, add joystick variables
*/ 
int setup_uinput_device() {
    int err=0;
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd < 0) {
        printf("Input device uinput open failed.\n");
        exit(-1);
    }
    
    //setup uinput device, add event types
    err = ioctl(fd, UI_SET_EVBIT, EV_KEY);
    if(err < 0) {
        printf("ioctl key evbit setup error.");
    }
    err = ioctl(fd, UI_SET_EVBIT, EV_SYN);
    if(err < 0) {
        printf("ioctl evebit setup error\n");
    }
    ioctl(fd, UI_SET_KEYBIT, BTN_A);

    //Setting up the JOYSTICK variables
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);

    //Setting up the vendor id and product id of virtual joystick device
    memset(&uinp, 0, sizeof(uinp));
    snprintf(uinp.name, UINPUT_MAX_NAME_SIZE, "ChompStick");
    uinp.id.bustype = BUS_USB;
    uinp.id.vendor = 0x9A7A; //Chompapp vendor id
    uinp.id.product = 0xBA17; //Chompapp product id
    uinp.id.version = 1;

    err = write(fd, &uinp, sizeof(uinp));
    if(err < 0) {
        printf("uinput write error.\n");
    }

    err = ioctl(fd, UI_DEV_CREATE);
    if(err < 0) {
        printf("ioctl dev create error.\n");
        exit(0);
    }
    return 0;
}

int closeDEV() {
    if(ioctl(fd, UI_DEV_DESTROY) < 0) {
        printf("device cloase error.\n");
        exit(0);
    }
    close(fd);
    return 1;
}


void emit(int fd, int type, int code, int val)
{
    struct input_event ie;
    
    ie.type = type;
    ie.code = code;
    ie.value = val;
    //timestamp values below are ignored
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;
    
    write(fd, &ie, sizeof(ie));
}

void user_input(int x, int y, int button) {
    if (x == 1) {
        emit(fd, EV_ABS, ABS_X, -32767);
        emit(fd, EV_SYN, SYN_REPORT,0);
    }
    else if (x == 3) {
        emit(fd, EV_ABS, ABS_X, 32767);
        emit(fd, EV_SYN, SYN_REPORT,0);
    }
    else if (x == 2) {
        emit(fd, EV_ABS, ABS_X, 0);
        emit(fd, EV_SYN, SYN_REPORT, 0);
    }
    if (button == 1) {
        emit(fd, EV_KEY, BTN_A, 1);
        emit(fd, EV_SYN, SYN_REPORT, 0);
    }
    else if (button == 0) {
        emit(fd, EV_KEY, BTN_A, 0);
        emit(fd, EV_SYN, SYN_REPORT, 0);
    }
    if (y == 1) {
        emit(fd, EV_ABS, ABS_Y, 32767);
        emit(fd, EV_SYN, SYN_REPORT,0);
    }
    else if (y == 3) {
        emit(fd, EV_ABS, ABS_Y, -32767);
        emit(fd, EV_SYN, SYN_REPORT,0);
    }
    else if (y == 2) {
        emit(fd, EV_ABS, ABS_Y, 0);
        emit(fd, EV_SYN, SYN_REPORT, 0);
    }
}

void error(char* s, int err) { //Error function recommended in an online guide for libusb.
    printf("%s", s);
    printf("Error: ");
    printf("%s", libusb_error_name(err));
    printf("\n");
    exit(err);
}

int main() {

    //Step 1: Setup Device
    int i = 0;
    i = setup_uinput_device();
    if(i < 0) {
        printf("uinput device setup failed.\n");
        return -1;
    }
    
    //Step 2: LibUSB init
    int transfer_size;
    int err;
    unsigned char data[1];
    libusb_device_handle* dev;
    libusb_init(NULL);

    dev = libusb_open_device_with_vid_pid(NULL, 0x9A7A, 0xBA17);
    if (dev == NULL) {
        printf("Device open failed.\n");
        return -1;
    }

    //Step 3: Lock the interface for read / write operations.
    err = libusb_claim_interface(dev, 0);
	if (err) {
	error("Cannot claim interface!", err);
	}

    err = libusb_set_interface_alt_setting(dev,0,0);
	if (err) {
		error("Cannot set alternate setting!", err);
	}

    while (1) {

	//Step 4: Waiting for interrupt from virtual joystick device.
        err = libusb_interrupt_transfer(dev, 0x81, data, sizeof(data), &transfer_size, 1000);
        if (err) {
            error("Interrupt IN Transfer Failed!", err);
        }

	//Step 5: Read and parse the byte received from device as interrupt data
	unsigned char bitsarray[8];
	unsigned char *bits = bitsarray; //data to readchar bits[8];
	bits = return_byte(data[0]);

        int x, y, button;
        button = bits[4] - '0';
        x = (bits[2] - '0') + 2*(bits[3] - '0');
        y = (bits[0] - '0') + 2*(bits[1] - '0');

	//printf("\nButton : %d, X : %d, Y : %d", button, x, y);

	//Step 6: Publish the state of buttons and axis as per device specifications.
        user_input(x, y, button);
	//sleep(5);
    }
    err = closeDEV();

    return (0);
}

//Function to convert the unsigned char to bits
unsigned char *return_byte (unsigned char source)
{
    static unsigned char byte[CHAR_BIT];
    unsigned char *p = byte, i;

    for(i = 0; i < CHAR_BIT; i++)
        p[i] = '0' + ((source & (1 << i)) > 0);

    //printf("Bits in function: %s", p);

    return p;
}
