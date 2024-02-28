#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#pragma GCC diagnostic ignored "-Wunused-result" // scanf()쓸 때 리턴값 안받는걸로 경고 띄위지 말라는 것

#define ARR_SIZ 8
int main(int argc, char *argv[])
{
	char buff[ARR_SIZ];
	int i;
	int ledkey_fd;
    int key_data = 0, key_data_old = 0;
	unsigned long val = 0;

	// 예외처리1
	if(argc < 2)
	{
		printf("USAGE : %s ledVal[0x00~0xff]\n", argv[0]);
		return 1;
	}

	// 예외처리2
	val = strtoul(argv[1], NULL,16);
	if(val < 0 || 0xff < val)
	{
		printf("Usage : %s ledValue[0x00~0xff]\n", argv[0]);
		return 2;
	}

	// 실제 프로그램 실행 후 예외처리3
	ledkey_fd = open("/dev/ledkey", O_RDWR | O_NONBLOCK);
	if(ledkey_fd < 0)
	{
		perror("open()");
		return 3;
	}

	// 우선 LED 켜보기
	//buff = (char)val; // 버퍼에 val값을 담아서 led를 켜는 디바이스 드라이버 호출
	for(i = 0; i < ARR_SIZ; i++)
	{
		buff[i] = (val >> i) & 0x01;
	}
	//write(ledkey_fd, &buff, sizeof(buff)); // 여기서 &buff는 void* 파라미터에 대응하는 것이다.
	write(ledkey_fd, buff, sizeof(buff));

	do 
	{
		//usleep(100000);

		// 이 함수를 읽어오는거랑 쓰는걸로 찢어야 한다.
		// key_data = syscall(__NR_mysyscall, val);
		read(ledkey_fd, buff, sizeof(buff));
		//key_data = buff;
		key_data = 0;
		for(i = 0; i < ARR_SIZ; i++)
		{
/*			if(buff[i] == 1)
			{
				key_data = i+1;
				break;
			}
*/
			key_data = key_data | (buff[i] << i);
		}

		if(key_data != key_data_old)
		{
			key_data_old = key_data;
			if(key_data)
			{
				write(ledkey_fd, &buff, sizeof(buff)); // 여기서 &buff는 void* 파라미터에 대응하는 것이다.
				puts("0:1:2:3:4:5:6:7");
				for(i=0;i<8;i++)
				{
					if(key_data & (0x01 << i))
						putchar('O');
					else
						putchar('X');
					if(i != 7 )
						putchar(':');
					else
						putchar('\n');
				}
				putchar('\n');
			}
			if(key_data == 0x80)
				break;
		}
	} while(1);

	printf("mysyscall return value = %#04x\n",key_data);

	return 0;
}
