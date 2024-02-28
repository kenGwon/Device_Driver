#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "ioctl_test.h"
#define DEVICE_FILENAME "/dev/ioctldev"
int main()
{
	ioctl_test_info info={0,{0}};
	int dev;
	int state;
	int cnt;
	int ret;
	int oldState=0;

	printf("info size : %d\n",sizeof(info));
	dev = open( DEVICE_FILENAME, O_RDWR|O_NDELAY );
	if( dev >= 0 )
	{ 
		ret = ioctl(dev, IOCTLTEST_KEYLEDINIT); // 어플리케이션 명령을 통해서 하드웨어를 초기화했다.
		if(ret < 0)
		{
			perror("ioctl()");
			return ret;
		}

		printf("IOCTLTEST_KEYLEDINIT : %#010x\n",IOCTLTEST_KEYLEDINIT);
		printf( "wait... input1\n" );
		ioctl(dev, IOCTLTEST_LEDON );
		while(1)
		{
			state = ioctl(dev, IOCTLTEST_GETSTATE );//key값 리턴
			if((state != 0) && (oldState != state))
			{
				printf("key : %#04x\n",state);
				oldState = state;
				if(state == 0x80) break;
			}
		}
		ioctl(dev, IOCTLTEST_LEDOFF );
		sleep(1);
		printf( "wait... input2\n" );
		while(1)
		{
			info.size = 0;
			ioctl(dev, IOCTLTEST_READ, &info );
			if( info.size > 0 )
			{
				printf("key : %#x\n",info.buff[0]);

				if(info.buff[0] == 1) break; // 1번 버튼을 눌러서 탈출
			}
		}
		info.size = 1;
		info.buff[0] = 0x0F;
		for( cnt=0; cnt<10; cnt++ )
		{
			ioctl(dev, IOCTLTEST_WRITE, &info );
			info.buff[0] = ~info.buff[0] & 0xff; // 최하위 1바이트만 관심있게 보겠다는 의미(0x000000ff)
			// 불빛을 이쁘게 깜빡거리기 위한 코드임11111
			usleep( 500000 );
		}
		printf( "wait... input3\n" );
		cnt = 0;
		state = 0xFF;
		while(1)
		{
			info.size = 1;
			info.buff[0] = (char)state;
			ret = ioctl(dev, IOCTLTEST_WRITE_READ, &info );
			if(ret < 0)
			{
				printf("ret : %d\n",ret);
				perror("ioctl()");
			}
			if( info.size > 0 )
			{
				printf("key : %#x\n",info.buff[0]);
				if(info.buff[0] == 1) break;
			}
			state = ~state;
			// 불빛을 이쁘게 깜빡거리기 위한 코드임22222
			usleep( 100000 );
		}
		ioctl(dev, IOCTLTEST_LEDOFF );
		ioctl(dev, IOCTLTEST_KEYLEDFREE );
  		close(dev);
	}
	else
	{
		perror("open");
		return 1;
	}
	return 0;
}
