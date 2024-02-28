#ifndef __IOCTL_H__
#define __IOCTL_H__

#define TIMER_MAGIC '6' 
typedef struct
{
	unsigned long timer_val;
} __attribute__((packed)) keyled_data; // 132바이트 구조체

#define TIMER_START			 	_IO(TIMER_MAGIC, 0) 
#define TIMER_STOP			 	_IO(TIMER_MAGIC, 1)
#define TIMER_VALUE				_IOW(TIMER_MAGIC, 2, keyled_data)
#define TIMER_MAXNR			3
#endif
