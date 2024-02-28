#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define DEVICE_FILENAME  "/dev/ledkey_dev"

void print_OX(unsigned char);
int main(int argc,char * argv[])
{
    char buff = 0;
	char oldBuff = 0;
    int ret;
    int dev;
	unsigned long val;

	if(argc < 2)
	{
        printf("Usage : %s ledValue[0x00~0xff]\n",argv[0]);
		return 1;
	}

	val = strtoul(argv[1],0,16);
	if(val < 0 || 0xff < val)
    {
        printf("Usage : %s ledValue[0x00~0xff]\n",argv[0]);
        return 2;
    }
	buff = val;

    dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
	if(dev<0)
	{
		perror("open()");
		return 2;
	}
    ret = write(dev,&buff,sizeof(buff));
	if(ret < 0)
	{
		perror("write()");
		return 3;
	}
	
	buff = 0;
	do {
		read(dev,&buff,sizeof(buff));
		if(oldBuff != buff)
		{
			if(buff != 0)
			{
				printf("key : %d\n",buff);
				print_OX(buff);
    			write(dev,&buff,sizeof(buff));
				if(buff == 8) //key:8
					break;
			}
			oldBuff = buff;
		}
	} while(1);


    close(dev);
    return 0;
}
void print_OX(unsigned char led)
{
	int i;
	led = 1 << led-1; // 커널에서 키 넘버를 보낼 때 1~8범위로 보냈기 때문에 0~7 범위로 바꿔준 뒤 비트시프트
	puts("1:2:3:4:5:6:7:8");
	for(i=0;i<=7;i++)
	{
		if(led & (0x01 << i))
			putchar('O');
		else
			putchar('X');
		if(i < 7 )
			putchar(':');
		else
			putchar('\n');
	}
	return;
}
