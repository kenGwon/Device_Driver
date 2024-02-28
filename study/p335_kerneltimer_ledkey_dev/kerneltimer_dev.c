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

#define KERNELTIMER_DEV_NAME "kerneltimerdev"
#define KERNELTIMER_DEV_MAJOR 230

#define OFF 0
#define ON 1
#define GPIOLEDCNT 8
#define GPIOKEYCNT 8

static int gpioLed[GPIOLEDCNT] = {6,7,8,9,10,11,12,13};
static int gpioKey[GPIOKEYCNT] = {16,17,18,19,20,21,22,23};

static int timerVal = 100; // 커널타이머 f=100HZ, T=1/100=10ms, 100*10ms=1sec
static int ledVal = 0;
struct timer_list timerLed; // 이 타이머 구조체가 있어야 타이머를 가져다가 사용할 수 있음

module_param(timerVal, int, 0);
module_param(ledVal, int, 0);

static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKeyInit(void);
static int gpioKeyGet(void);
static void gpioKeyFree(void);

static void kerneltimer_registertimer(unsigned long timeover);
static void kerneltimer_func(struct timer_list *t);
static int kerneltimer_init(void);
static void kerneltimer_exit(void);

static int ledkey_open(struct inode *inode, struct file *filp);
static loff_t ledkey_llseek(struct file *filp, loff_t off, int whence);
static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t ledkey_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static long ledkey_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static int ledkey_release(struct inode *inode, struct file *filp);


static int gpioLedInit(void)
{
	int i;
	int ret=0;
	char gpioName[10];
	for(i=0;i<GPIOLEDCNT;i++)
	{
		sprintf(gpioName,"led%d",i);
		ret = gpio_request(gpioLed[i],gpioName);
		if(ret < 0) {
			printk("Failed gpio_request() gpio%d error \n",i);
			return ret;
		}

		ret = gpio_direction_output(gpioLed[i],OFF);
		if(ret < 0) {
			printk("Failed gpio_direction_output() gpio%d error \n",i);
			return ret;
		}
	}
	return ret;
}

static void gpioLedSet(long val)
{
	int i;
	for(i=0;i<GPIOLEDCNT;i++)
	{
		gpio_set_value(gpioLed[i],(val>>i) & 0x1);
	}
}
static void gpioLedFree(void)
{
	int i;
	for(i=0;i<GPIOLEDCNT;i++)
	{
		gpio_free(gpioLed[i]);
	}
}
static int gpioKeyInit(void)
{
	int i;
	int ret=0;
	char gpioName[10];
	for(i=0;i<GPIOKEYCNT;i++)
	{
		sprintf(gpioName,"key%d",gpioKey[i]);
		ret = gpio_request(gpioKey[i], gpioName);
		if(ret < 0) {
			printk("Failed Request gpio%d error\n", 6);
			return ret;
		}
	}
	for(i=0;i<GPIOKEYCNT;i++)
	{
		ret = gpio_direction_input(gpioKey[i]);
		if(ret < 0) {
			printk("Failed direction_output gpio%d error\n", 6);
       	 	return ret;
		}
	}
	return ret;
}
static int gpioKeyGet(void)
{
	int i;
	int ret;
	int keyData=0;
	for(i=0;i<GPIOKEYCNT;i++)
	{
//		ret=gpio_get_value(gpioKey[i]) << i;
//		keyData |= ret;
		ret=gpio_get_value(gpioKey[i]);
		keyData = keyData | ( ret << i );
	}
	return keyData;
}
static void gpioKeyFree(void)
{
	int i;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}

static void kerneltimer_registertimer(unsigned long timeover)
{
	timer_setup(&timerLed, kerneltimer_func, 0); // 일정시간마다 등록된 함수포인터를 실행하겠다는 뜻
	timerLed.expires = get_jiffies_64() + timeover; // 10ms * 100 = 1sec // 만료시간을 현재 틱 + 100으로 설정
	add_timer(&timerLed);
}

static void kerneltimer_func(struct timer_list *t)
{
#if DEBUG
	printk("ledVal : %#04x\n", (unsigned int)(ledVal));
#endif
	gpioLedSet(ledVal);

	ledVal = ~ledVal & 0xff; // 보수 취해주고 최하위 1바이트만 보겠다는 뜻
	mod_timer(t, get_jiffies_64() +  timerVal); // 타이머는 1회성이기 때문에 계속 쓸라면 갱신해줘야한다. 그때 쓰는 함수가 mod_timer()이다.
}

static int ledkey_open(struct inode *inode, struct file *filp)
{
	int num0 = MINOR(inode->i_rdev); 
	int num1 = MAJOR(inode->i_rdev);
	printk("ledkey open-> minor : %d\n", num0);
	printk("ledkey open-> major : %d\n", num1);

	try_module_get(THIS_MODULE);
	return 0;
}

static loff_t ledkey_llseek(struct file *filp, loff_t off, int whence)
{
	printk("ledkey llseek -> off : %08X, whence : %08X\n", (unsigned int)off, whence);
	return 0x23;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	printk("ledkey read -> buf : %08X, count : %08X \n", (unsigned int)buf, count);

	char kval;
	kval = (char)gpioKeyGet();
	printk("%d\n", kval);
	put_user(kval, buf);

	//result = copy_to_user(buf, &kbuf, count);

	return count;
}

static ssize_t ledkey_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	// 밖에서 저수준 입출력 함수로 write()할때는 void* 형으로 buf를 전달했는데, 이 함수와 연결되면서 buf가 const char* 형으로 강제 형변환되었다.
	// const char *buf라고 선언을 해둬야 여기 코드 안에서 실수로 *buf가 가리키는 공간을 바꿔버리는 것을 방지하기 위함이다.
	// 즉, 함수의 파라미터를 구성할 때 원본데이터가 되는 src를 const로 선언하여 수정되는 것을 방지해주는 것은 일반적인 방법이다. 아래의 strcpy() 함수를 보면 제대로 알 수 있다.
	/*
	char * strcpy(char * dest,const char *src)
	{
		char *tmp = dest;

		while ((*dest++ = *src++) != '\0')
			;
		return tmp;
	}
	*/
	// 이러한 개념 원리가 이 코드에도 적용되어 있는 것이다.
	// const로 선언 안하고 Readonly 영역에 write할려고 하면 그 순간 segmentation fault가 일어나면서 시스템이 죽어버린다. 너무나 fatal한 상황인 것이다. 하지만 코드상에 Readonly 영역 포인터를 const로 선언해놓고 거기에 무언가 값을 write하려고 하면 컴파일 순간에 컴파일 에러가 난다. 즉 프로그램이 돌면서 segmentaion fault가 나기 전에 컴파일 타임에 error를 발생시켜서 좀더 graceful하게 실수를 방지할 수 있다는 것이다.

	printk("ledkey write -> buf : %08X, count : %08X \n", (unsigned int)buf, count);

#if 0
	gpioLedSet(*buf); // 어플리케이션에서 할당한 값을 참조하여 불을 켬
	// buf라는 포인터 변수는 커널 공간에 할당된 것이 맞다. 
	// 하지만 buf라는 포인터 변수 안에 담긴 주소 값은 사용자 공간이라는 것이 핵심이다.
	// 근데 이렇게 직접접근 하지 말라고 했다. 반드시 값을 복사 받아서 사용하라고 했다.
	// 물론 그냥 이렇게 해도 실행이 되긴 한다. 근데 이러면 안된다는 것이다. 그 이유는 교재232쪽을 보면 알수 있다.
#else
	// 버퍼가 단 1바이트라면...
	char kbuff;
	get_user(kbuff, buf); // kbuff는 '일반 변수'이고, buf는 '포인터'이다.

	// 만약 버퍼가 1바이트가 아니라 여러 바이트였다면...
	/*  char kbuff[10];
		int i;
		for(i=0;i<count;i++)
			get_user(kbuff[i],buf++);
	 */
	/*  char kbuff[10];
		copy_from_user(kbuff,buf,count);
	 */
	/*  int result;
		result = copy_from_user(&kbuff, buf, count);
	 */

	//gpioLedSet(kbuff); // gpioLedSet은 long을 받기를 기대하고 있는데 char을 넣어줬다. 이건 문제가 안된다.
	
	ledVal = kbuff;
#endif
	return count; // 실제로 읽은 바이트 수를 리턴해야 한다. 
}

static long ledkey_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk("ledkey ioctl -> cmd : %08X, arg : %08X\n", cmd, (unsigned int)arg);
	return 0x53;
}

static int ledkey_release(struct inode *inode, struct file *filp)
{
	printk("ledkey release \n");

	module_put(THIS_MODULE);
	return 0;
}

// 구조체다
struct file_operations kerneltimer_fops = {
	//.owner			= THIS_MODULE,
	.llseek			= ledkey_llseek,
	.read			= ledkey_read,
	.write			= ledkey_write,
	.unlocked_ioctl	= ledkey_ioctl,
	.open			= ledkey_open,
	.release		= ledkey_release, // 저수준 입출력함수에서 close()가 .release에 대응한다.
};


static int kerneltimer_init(void)
{
	int ret;

#if DEBUG
	printk("timerVal : %#04x\n", (unsigned int)(timerVal));
#endif
	kerneltimer_registertimer(timerVal);

	ret = register_chrdev(KERNELTIMER_DEV_MAJOR, KERNELTIMER_DEV_NAME, &kerneltimer_fops);
	if (ret < 0)
		return ret;

	ret = gpioLedInit();
	if (ret < 0)
		return ret;
	
	ret = gpioKeyInit();
	if (ret < 0)
		return ret;

	return 0;
}

static void kerneltimer_exit(void)
{
	if (timer_pending(&timerLed)) // pending()은 타이머가 등록되어있는지 안등록되있는지 확인하는 함수
		del_timer(&timerLed); // 등록되어있다면 타이머 삭제

	/* 그냥 생짜로 del_timer()를 쓰지 않은 이유는... */
	// dnldp 39~40번째 라인사이에서 타이머가 종료되고 다시 생성되는 그 사이 순간에 순간적으로 타이머가 존재하지 않는 타이밍이 있기 때문이다. del_timer(NULL)이 들어가면 exception이 나면서 프로그램이 죽기 때문에, 위에처럼 if문으로 한번 감싸서 예외처리를 해준 것이다.

	unregister_chrdev(KERNELTIMER_DEV_MAJOR, KERNELTIMER_DEV_NAME);

	gpioLedSet(0x00);
	gpioLedFree();
	gpioKeyFree();
}

module_init(kerneltimer_init);
module_exit(kerneltimer_exit);

MODULE_AUTHOR("KCCI-AIOT KSH");
MODULE_DESCRIPTION("led key test module");
MODULE_LICENSE("Dual BSD/GPL");

