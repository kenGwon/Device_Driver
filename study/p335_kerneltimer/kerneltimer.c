#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

#define DEBUG 1

static int timerVal = 100; // 커널타이머 f=100HZ, T=1/100=10ms, 100*10ms=1sec
static int ledVal = 0;
struct timer_list timerLed; // 이 타이머 구조체가 있어야 타이머를 가져다가 사용할 수 있음

module_param(timerVal, int, 0);
module_param(ledVal, int, 0);

void kerneltimer_registertimer(unsigned long timeover);
void kerneltimer_func(struct timer_list *t);

void kerneltimer_registertimer(unsigned long timeover)
{
	timer_setup(&timerLed, kerneltimer_func, 0); // 일정시간마다 등록된 함수포인터를 실행하겠다는 뜻
	timerLed.expires = get_jiffies_64() + timeover; // 10ms * 100 = 1sec // 만료시간을 현재 틱 + 100으로 설정
	add_timer(&timerLed);
}

void kerneltimer_func(struct timer_list *t)
{
#if DEBUG
	printk("ledVal : %#04x\n", (unsigned int)(ledVal));
#endif
	ledVal = ~ledVal & 0xff; // 보수 취해주고 최하위 1바이트만 보겠다는 뜻
	mod_timer(t, get_jiffies_64() +  timerVal); // 타이머는 1회성이기 때문에 계속 쓸라면 갱신해줘야한다. 그때 쓰는 함수가 mod_timer()이다.
}

int kerneltimer_init(void)
{
#if DEBUG
	printk("timerVal : %#04x\n", (unsigned int)(timerVal));
#endif
	kerneltimer_registertimer(timerVal);
	return 0;
}

void kerneltimer_exit(void)
{
	if (timer_pending(&timerLed)) // pending()은 타이머가 등록되어있는지 안등록되있는지 확인하는 함수
		del_timer(&timerLed); // 등록되어있다면 타이머 삭제

	/* 그냥 생짜로 del_timer()를 쓰지 않은 이유는... */
	// dnldp 39~40번째 라인사이에서 타이머가 종료되고 다시 생성되는 그 사이 순간에 순간적으로 타이머가 존재하지 않는 타이밍이 있기 때문이다. del_timer(NULL)이 들어가면 exception이 나면서 프로그램이 죽기 때문에, 위에처럼 if문으로 한번 감싸서 예외처리를 해준 것이다.
}

module_init(kerneltimer_init);
module_exit(kerneltimer_exit);

MODULE_AUTHOR("KCCI-AIOT KSH");
MODULE_DESCRIPTION("led key test module");
MODULE_LICENSE("Dual BSD/GPL");

