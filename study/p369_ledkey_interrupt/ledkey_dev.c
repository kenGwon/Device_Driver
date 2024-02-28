#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gpio.h> // gpio
#include <linux/interrupt.h>
#include <linux/irq.h>

#define DEBUG 1

#define LEDKEY_DEV_NAME			"ledkey_dev"
#define LEDKEY_DEV_MAJOR		230 // 디바이스 주번호

#define OFF 0
#define ON 1
#define GPIOLEDCNT 8
#define GPIOKEYCNT 8

static int gpioLed[GPIOLEDCNT] = {6,7,8,9,10,11,12,13};
static int gpioKey[GPIOKEYCNT] = {16,17,18,19,20,21,22,23};

static int key_irq[8] = {0};
static int keyNumber = 0;

static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKeyInit(void);
//static int gpioKeyGet(void);
static void gpioKeyFree(void);
static int gpioKeyIrqInit(void);
static void gpioKeyIrqFree(void);
irqreturn_t key_isr(int irq, void *data);

irqreturn_t key_isr(int irq, void *data)
{
	int i;
	for(i = 0; i < GPIOKEYCNT; i++)
	{
		if(irq == key_irq[i])
		{
			keyNumber = i + 1; // 전역변수, 키 번호를 0~7이 아니라 1~8번 범위로 보기 위함
			break;
		}
	}
#if DEBUG
	printk(KERN_DEBUG "key_isr() irq: %d, keyNumber: %d\n", irq, keyNumber);
#endif
	return IRQ_HANDLED;
}

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
/* <이제 폴링방식으로 key를 조사하지 않을 것이므로 필요없어진 함수...>
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
*/
static void gpioKeyFree(void)
{
	int i;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}

static int gpioKeyIrqInit(void)
{
	int i;
	for(i = 0; i < GPIOKEYCNT; i++)
	{
		key_irq[i] = gpio_to_irq(gpioKey[i]); // gpio핀의 기능을 외부 인터럽트 기능으로 설정

		if (key_irq[i] < 0)
		{
			printk("gpioKeyIrq() Failed");
			return key_irq[i]; // 오류 예외처리
		}
	}

	return 0; // 정상종료
}

static void gpioKeyIrqFree(void)
{
	int i;
	for(i = 0; i < GPIOKEYCNT; i++)
	{
		free_irq(key_irq[i], NULL); // 커널 지원 함수 사용
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////

static int ledkey_open(struct inode *inode, struct file *filp)
{
	int num0 = MINOR(inode->i_rdev); 
	int num1 = MAJOR(inode->i_rdev);
	printk("ledkey open-> minor : %d\n", num0);
	printk("ledkey open-> major : %d\n", num1);

	try_module_get(THIS_MODULE);
	return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuff;

	//kbuff = gpioKeyGet(); // 이렇게 폴링으로 하지 않고 인터럽트를 적용해야하는 부분
	kbuff = keyNumber; // 전역변수 keyNumber 
	put_user(kbuff, buf);
	if(keyNumber)
		keyNumber = 0;

#if DEBUG
	printk("%d\n", kbuff);
	printk("ledkey read -> buf : %08X, count : %08X \n", (unsigned int)buf, count);
#endif
	return count;
}

static ssize_t ledkey_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuff;

	get_user(kbuff, buf); 
	gpioLedSet(kbuff);

#if DEBUG
	printk("ledkey write -> buf : %08X, count : %08X \n", (unsigned int)buf, count);
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

struct file_operations ledkey_fops = {
	//.owner			= THIS_MODULE,
	.read			= ledkey_read,
	.write			= ledkey_write,
	.unlocked_ioctl	= ledkey_ioctl,
	.open			= ledkey_open,
	.release		= ledkey_release, // 저수준 입출력함수에서 close()가 .release에 대응한다.
};

static int ledkey_init(void)
{
	int i;
	int result;
	char * irqName[8] = {
		"IrqKey0","IrqKey1","IrqKey2","IrqKey3",
		"IrqKey4","IrqKey5","IrqKey6","IrqKey7"
	};

	printk("ledkey ledkey_init \n");

	result = gpioLedInit();
	if(result < 0) return result;
	result = gpioKeyInit();
	if(result < 0) return result;

	result = gpioKeyIrqInit();
	if(result < 0) return result;

	for (i = 0; i < GPIOKEYCNT; i++)
	{
		// 인터럽트 등록: request_irq(irq, 핸들러함수, 인터럽트타이밍(라이징엣지/폴링엣지)설정, 인터럽트 이름, 인터럽트서비스루틴에 전달할 변수)
		result = request_irq(key_irq[i], key_isr, IRQF_TRIGGER_RISING, irqName[i], NULL);
		if(result < 0) 
		{
			printk(KERN_ERR "request_irq() failed irq %d\n", key_irq[i]);
			return result;
		}
	}

	result = register_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops); 
	if (result < 0) return result;

	return 0;
}

static void ledkey_exit(void)
{
	printk("ledkey ledkey_exit \n");

	unregister_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME);
	gpioKeyIrqFree();
	gpioKeyFree();
	gpioLedFree();
}

module_init(ledkey_init);
module_exit(ledkey_exit);
MODULE_AUTHOR("KCCI");
MODULE_DESCRIPTION("Device Driver pracite~!");
MODULE_LICENSE("Dual BSD/GPL");

