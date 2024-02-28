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
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include "ioctl_test.h"

#define DEBUG 1

#define LEDKEY_DEV_NAME			"keyled_dev"
#define LEDKEY_DEV_MAJOR		230

#define OFF 0
#define ON 1
#define GPIOLEDCNT 8
#define GPIOKEYCNT 8

static int timerVal = 100;
static int ledVal = 0;
struct timer_list timerLed;

module_param(timerVal, int, 0);
module_param(ledVal, int, 0);

static int gpioLed[GPIOLEDCNT] = {6,7,8,9,10,11,12,13};
static int gpioKey[GPIOKEYCNT] = {16,17,18,19,20,21,22,23};

typedef struct {
	int key_irq[8];
	int keyNumber;
} keyData;

/* block IO 매크로 */
DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Read);

/* 커널 타이머 */
static void kerneltimer_registertimer(unsigned long timeover);
static void kerneltimer_func(struct timer_list *t);

/* GPIO 제어 API */
static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKeyInit(void);
//static int gpioKeyGet(void);
static void gpioKeyFree(void);
static int gpioKeyIrqInit(keyData* pKeyData);
static void gpioKeyIrqFree(keyData* pKeyData);
irqreturn_t key_isr(int irq, void *data);

/* 디바이스 드라이버 file_operations 연결 함수 */
static int ledkey_open(struct inode *inode, struct file *filp);
static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t ledkey_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static long ledkey_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static __poll_t ledkey_poll(struct file *filp, struct poll_table_struct *wait);
static int ledkey_release(struct inode *inode, struct file *filp);
static int ledkey_init(void);
static void ledkey_exit(void);


static void kerneltimer_registertimer(unsigned long timeover)
{
    timer_setup(&timerLed, kerneltimer_func, 0); 
    timerLed.expires = get_jiffies_64() + timeover;
}

static void kerneltimer_func(struct timer_list *t)
{
#if DEBUG
    printk("ledVal : %#04x\n", (unsigned int)(ledVal));
	printk("current timerVal : %d\n", timerVal);
#endif
    gpioLedSet(ledVal);
    ledVal = ~ledVal & 0xff;

    mod_timer(t, get_jiffies_64() +  timerVal); 
}

irqreturn_t key_isr(int irq, void *data) 
{
	int i;
	keyData* pKeyData = (keyData*)data; 

	for(i = 0; i < GPIOKEYCNT; i++)
	{
		if(irq == pKeyData->key_irq[i])
		{
			pKeyData->keyNumber = i + 1; 
			break;
		}
	}
#if DEBUG
	printk(KERN_DEBUG "key_isr() irq: %d, keyNumber: %d\n", irq, pKeyData->keyNumber);
#endif

    wake_up_interruptible(&WaitQueue_Read);

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

#if 0	// <이제 폴링방식으로 key를 조사하지 않을 것이므로 필요없어진 함수...>
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
#endif

static void gpioKeyFree(void)
{
	int i;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}

static int gpioKeyIrqInit(keyData* pKeyData)
{
	int i;
	int result;
	char * irqName[8] = {
		"IrqKey0","IrqKey1","IrqKey2","IrqKey3",
		"IrqKey4","IrqKey5","IrqKey6","IrqKey7"
	};

	for(i = 0; i < GPIOKEYCNT; i++)
	{
		pKeyData->key_irq[i] = gpio_to_irq(gpioKey[i]); 

		if (pKeyData->key_irq[i] < 0)
		{
			printk("gpioKeyIrq() Failed gpio %d\n", gpioKey[i]);
			return pKeyData->key_irq[i];
		}
	}

	for (i = 0; i < GPIOKEYCNT; i++)
	{
		result = request_irq(pKeyData->key_irq[i], key_isr, IRQF_TRIGGER_RISING, irqName[i], pKeyData);
		if(result < 0) 
		{
			printk(KERN_ERR "request_irq() failed irq %d\n", pKeyData->key_irq[i]);
			return result;
		}
	}

	return 0;
}

static void gpioKeyIrqFree(keyData* pKeyData)
{
	int i;
	for(i = 0; i < GPIOKEYCNT; i++)
	{
		free_irq(pKeyData->key_irq[i], pKeyData); 
	}
}

static int ledkey_open(struct inode *inode, struct file *filp)
{
	int num0 = MINOR(inode->i_rdev); 
	int num1 = MAJOR(inode->i_rdev);
	int result;

	keyData* pKeyData = (keyData*)kmalloc(sizeof(keyData), GFP_KERNEL); 
	if(!pKeyData) return -ENOMEM; 
	pKeyData->keyNumber = 0;

	result = gpioKeyIrqInit(pKeyData);
	if(result < 0) return result;

	filp->private_data = pKeyData;

#if DEBUG
	printk("ledkey open-> minor : %d\n", num0);
	printk("ledkey open-> major : %d\n", num1);
#endif
	try_module_get(THIS_MODULE);

	return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	keyData* pKeyData = (keyData*)filp->private_data;

	if (pKeyData->keyNumber == 0)
		if ( !(filp->f_flags & O_NONBLOCK) )
			wait_event_interruptible(WaitQueue_Read, pKeyData->keyNumber);

	put_user(pKeyData->keyNumber, buf);
	if(pKeyData->keyNumber)
		pKeyData->keyNumber = 0;

#if DEBUG
	printk("ledkey read -> buf : %08X, count : %08X \n", (unsigned int)buf, count);
#endif
	return count;
}

static ssize_t ledkey_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuff;

	get_user(kbuff, buf); 
	ledVal = kbuff;

#if DEBUG
	printk("ledkey write -> buf : %08X, count : %08X \n", (unsigned int)buf, count);
#endif
	return count;
}

static long ledkey_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int err = 0;
	int size;
	keyled_data ctrl_info = {0};

	if (_IOC_TYPE(cmd) != TIMER_MAGIC) return -EINVAL;
	if (_IOC_NR(cmd) >= TIMER_MAXNR) return -EINVAL;

	size = _IOC_SIZE(cmd);
	if (size)
	{
		if(_IOC_DIR(cmd) & _IOC_READ)
			err = access_ok((void*)arg, size);
		if(_IOC_DIR(cmd) & _IOC_WRITE)
			err = access_ok((void*)arg, size);

		if(!err) return err;
	}

	switch(cmd)
	{
		case TIMER_START:
			if ( !timer_pending(&timerLed) )
				add_timer(&timerLed);
			break;

		case TIMER_STOP:
			if (timer_pending(&timerLed))
				del_timer(&timerLed);
			break;

		case TIMER_VALUE:
			err = copy_from_user((void *)&ctrl_info, (void *)arg, size);
			timerVal = ctrl_info.timer_val;
			break;

		default:
			err = -E2BIG;
			break;
	}
	return err;
}

static __poll_t ledkey_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	keyData * pKeyData = (keyData *)filp->private_data;
#ifdef DEBUG
	//printk("_key : %u\n",(wait->_key & POLLIN));
#endif

	if(wait->_key & POLLIN)
		poll_wait(filp, &WaitQueue_Read, wait);
	if(pKeyData->keyNumber > 0)
		mask = POLLIN;

	return mask;
}

static int ledkey_release(struct inode *inode, struct file *filp)
{
	keyData* pKeyData = (keyData*)filp->private_data;

	if(timer_pending(&timerLed))
		del_timer(&timerLed);

	gpioKeyIrqFree(pKeyData); 
	if (pKeyData) 
		kfree(pKeyData); 

	printk("ledkey release \n");
	module_put(THIS_MODULE);
	return 0;
}

static struct file_operations ledkey_fops = {
	.read			= ledkey_read,
	.write			= ledkey_write,
	.poll			= ledkey_poll,
	.unlocked_ioctl	= ledkey_ioctl,
	.open			= ledkey_open,
	.release		= ledkey_release,
};

static int ledkey_init(void)
{
	int result;

	kerneltimer_registertimer(timerVal);
	result = gpioLedInit();
	if(result < 0) 
		return result;
	result = gpioKeyInit();
	if(result < 0) 
		return result;
	result = register_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops); 
	if (result < 0) 
		return result;

	printk("ledkey ledkey_init \n");
	return 0;
}

static void ledkey_exit(void)
{
	unregister_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME);
	gpioKeyFree();
	gpioLedFree();
	if(timer_pending(&timerLed))
		del_timer(&timerLed);
	
	printk("ledkey ledkey_exit \n");
}

module_init(ledkey_init);
module_exit(ledkey_exit);
MODULE_AUTHOR("KCCI - kenGwon");
MODULE_DESCRIPTION("Led/Key Device Driver");
MODULE_LICENSE("Dual BSD/GPL");

