#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h> // poll 기능을 위해 필요한 헤더
#include <string.h>

#define DEVICE_FILENAME "/dev/ledkey_dev"

int main(int argc, char *argv[])
{
	int dev;
	char buff;
	int ret;
	int num = 1;
	struct pollfd Events[2]; // poll 관련 구조체 선언... 처리하고 싶은 장치가 2개라서 배열크기 2로 선언
	char keyStr[80];

    if(argc != 2)
    {
        printf("Usage : %s [led_data(0x00~0xff)]\n",argv[0]);
        return 1;
    }
    buff = (char)strtoul(argv[1],NULL,16);
    if((buff < 0x00) || (0xff < buff))
    {
        printf("Usage : %s [led_data(0x00~0xff)]\n",argv[0]);
        return 2;
    }

//  dev = open(DEVICE_FILENAME, O_RDWR | O_NONBLOCK);
  	dev = open(DEVICE_FILENAME, O_RDWR );
	if(dev < 0)
	{
		perror("open");
		return 2;
	}
	write(dev,&buff,sizeof(buff));

	fflush(stdin); // 혹시 읽기 버퍼에 쓰레기 값이 있을 것을 고려하여 한번 청소
	memset( Events, 0, sizeof(Events)); 
  	Events[0].fd = fileno(stdin); // 표준입력장치의 file descriptor를 전달해준다.
  	Events[0].events = POLLIN; // 잠든다...
	Events[1].fd = dev; // 우리가 정의한 장치파일의 file descriptor를 전달해준다.
	Events[1].events = POLLIN; // 잠든다...
	while(1)
	{
		ret = poll(Events, 2, 2000); // poll()함수도 프로세스를 재우는 함수이다. 2000은 timeout이다(2초).
		// poll()은 저수준 입출력함수로 디바이스 드라이버의 file_operations에 연결된다.

		if(ret<0)
		{
			perror("poll");
			exit(1);
		}
		else if(ret==0) // 2초가 지나서 timeout이 된 경우
		{
  			printf("poll time out : %d Sec\n",2*num++);
			continue;
		}

		// timout이 되기 전에 이벤트가 발생한 경우
		if(Events[0].revents & POLLIN)  //stdin
		{
			fgets(keyStr,sizeof(keyStr),stdin);
			if(keyStr[0] == 'q')
				break;
			keyStr[strlen(keyStr)-1] = '\0';  //fgets()는 뉴라인"\n"이 붙어서 넘어오기 때문에 제거
			printf("STDIN : %s\n",keyStr);
			buff = (char)atoi(keyStr);
			write(dev,&buff,sizeof(buff));
		}
		else if(Events[1].revents & POLLIN) //ledkey
		{
			ret = read(dev,&buff,sizeof(buff));
			printf("key_no : %d\n",buff);
			if(buff == 8) 
				break;

			buff = 1 << buff-1;
			write(dev,&buff,sizeof(buff));
		}
	}
	close(dev);
	return 0;
}
