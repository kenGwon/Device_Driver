#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE_FILENAME "/dev/calldev"

int main(void)
{
	int dev;
	char buff[128];
	int ret;

	printf("(!) device file open\n");
	
	dev = open(DEVICE_FILENAME, O_RDWR | O_NDELAY);
	printf("dev: %d\n", dev);

	if(dev < 0)
	{
		perror("open");
		return 1;
	}


	while(1)
		/* nothing */;


	printf("(2) seek function call dev: %d\n", dev);
	ret = lseek(dev, 0x20, SEEK_SET);
	printf("ret = %08X\n", ret);

	printf("(3) read function call\n");
	ret = read(dev, (char *)0x30, 0x31);
	printf("ret = %08X\n", ret);

	printf("(4) write function call\n");
	ret = write(dev, (char *)0x40, 0x41);
	printf("ret = %08X\n", ret);

	printf("(5) ioctl function call\n");
	ret = ioctl(dev, 0x51, 0x52);	
	printf("ret = %08X\n", ret);

	printf("(6) device file close\n");
	ret = close(dev);
	printf("ret = %08X\n", ret);

	return 0;
}

