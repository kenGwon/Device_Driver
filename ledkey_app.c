#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include "ioctl_test.h"

#define DEVICE_FILENAME "/dev/keyled_dev"

int main(int argc, char *argv[])
{
	int dev;
	char key_no;
	char led_no;
	char timer_val;
	int ret;
	int cnt = 0;
	int loopFlag = 1;
	struct pollfd Events[2];
	char inputString[80];
	keyled_data info;

	if(argc != 3)
	{
        printf("Usage : %s [led_val(0x00~0xff)] [timer_val(1/100)]\n",argv[0]);
		return 1;
	}
	led_no = (char)strtoul(argv[1],NULL,16);
	if(!((0 <= led_no) && (led_no <= 255)))
	{
		printf("Usage : %s [led_data(0x00~0xff)]\n",argv[0]);
		return 2;
	}
	printf("Author: KGH\n");
    timer_val = atoi(argv[2]);
	info.timer_val = timer_val;

//	dev = open(DEVICE_FILENAME, O_RDWR | O_NONBLOCK);
	dev = open(DEVICE_FILENAME, O_RDWR );
	if(dev < 0)
	{
		perror("open");
		return 2;
	}

	ioctl(dev,TIMER_VALUE,&info);
    write(dev,&led_no,sizeof(led_no));
    ioctl(dev,TIMER_START);

	memset( Events, 0, sizeof(Events));

	Events[0].fd = dev;
	Events[0].events = POLLIN;
	Events[1].fd = fileno(stdin);
	Events[1].events = POLLIN;

	while(loopFlag)
	{

		ret = poll(Events, 2, 1000);
		if(ret==0)
		{
//  		printf("poll time out : %d\n",cnt++);
			continue;
		}
		if(Events[0].revents & POLLIN)  //dev : keyled
		{
    		read(dev,&key_no,sizeof(key_no));
			printf("key_no : %d\n",key_no);
			switch(key_no) 
			{
				case 1:
            		printf("TIMER STOP! \n");
            		ioctl(dev,TIMER_STOP);
					break;
				case 2:
            		ioctl(dev,TIMER_STOP);
            		printf("Enter timer value! \n");
					break;
				case 3:
            		ioctl(dev,TIMER_STOP);
            		printf("Enter led value! \n");
					break;
				case 4:
            		printf("TIMER START! \n");
            		ioctl(dev,TIMER_START);
					break;
				case 8:
            		printf("APP CLOSE ! \n");
            		ioctl(dev,TIMER_STOP);
					loopFlag = 0;
				break;

			}
		}
		else if(Events[1].revents & POLLIN) //keyboard
		{
    		fflush(stdin);
			fgets(inputString,sizeof(inputString),stdin);
			if((inputString[0] == 'q') || (inputString[0] == 'Q'))
				break;
			inputString[strlen(inputString)-1] = '\0';
           
			if(key_no == 2) //timer value
			{
				timer_val = atoi(inputString);
				info.timer_val = timer_val;
				ioctl(dev,TIMER_VALUE,&info);
            	ioctl(dev,TIMER_START);
				
			}
			else if(key_no == 3) //led value
			{
				led_no = (char)strtoul(inputString,NULL,16);
    			write(dev,&led_no,sizeof(led_no));
            	ioctl(dev,TIMER_START);
			}
			key_no = 0;
		}
	}
	close(dev);
	return 0;
}
