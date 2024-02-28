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
#include <linux/wait.h> // blockio

#define DEBUG 1

#define LEDKEY_DEV_NAME			"ledkey_dev"
#define LEDKEY_DEV_MAJOR		230 // 디바이스 주번호

#define OFF 0
#define ON 1
#define GPIOLEDCNT 8
#define GPIOKEYCNT 8

static int gpioLed[GPIOLEDCNT] = {6,7,8,9,10,11,12,13};
static int gpioKey[GPIOKEYCNT] = {16,17,18,19,20,21,22,23};

// 이건 전역변수가 아니다. 그저 구조체의 틀을 선언했을 뿐이다. 자료형을 선언했을 뿐이다.
// 구조체를 사용해야지만 인터럽트 서비스 루틴에다가 정보를 넘겨줄 때 여러개의 정보를 한번에 넘겨줄 수 있기 때문에 구조체를 선언한 것이다.
typedef struct {
	int key_irq[8];
	int keyNumber;
} keyData;

static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKeyInit(void);
//static int gpioKeyGet(void);
static void gpioKeyFree(void);
static int gpioKeyIrqInit(keyData* pKeyData);
static void gpioKeyIrqFree(keyData* pKeyData);
irqreturn_t key_isr(int irq, void *data);

DECLARE_WAIT_QUEUE_HEAD(WaitQueue_Read); // blockio
// 재울 때와 깨울 때 WaitQueue_Read 변수를 사용할 것이다.


irqreturn_t key_isr(int irq, void *data) // 이 함수는 '커널'에서 호출하는 함수이다.
{
	int i;
	keyData* pKeyData = (keyData*)data; // void포인터로 받아오기 때문에  크기정보가 없다. 그래서 크기정보를 주기 위해 형변환 해준다.

	for(i = 0; i < GPIOKEYCNT; i++)
	{
		if(irq == pKeyData->key_irq[i])
		{
			pKeyData->keyNumber = i + 1; // 키 번호를 0~7이 아니라 1~8번 범위로 보기 위함
			break;
		}
	}
#if DEBUG
	printk(KERN_DEBUG "key_isr() irq: %d, keyNumber: %d\n", irq, pKeyData->keyNumber);
#endif

	// 다시 한번 말하지만 이 key_isr()함수는 하드웨어 스위치를 누른것을 인식하여 '커널이' 호출하는 함수이다. 그래서 이 커널이 호출한 함수를 통해서 사용자 영역 프로세스를 깨울 수 있게 되는 것이다.(잠든 프로세스는 스스로 깨어나지 못한다. 그래서 다른 주체인 커널의 힘을 빌려서 프로세스를 깨우는 구조인 것이다. )
	wake_up_interruptible(&WaitQueue_Read); // 프로세스를 깨우는 함수
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
		// gpio핀의 기능을 외부 인터럽트 기능으로 설정하면서 그 값을 irq로 설정
		pKeyData->key_irq[i] = gpio_to_irq(gpioKey[i]); 

		if (pKeyData->key_irq[i] < 0)
		{
			printk("gpioKeyIrq() Failed gpio %d\n", gpioKey[i]);
			return pKeyData->key_irq[i]; // 오류 예외처리
		}
	}

	for (i = 0; i < GPIOKEYCNT; i++)
	{
		// 인터럽트 등록: request_irq(인터럽트 번호, 핸들러함수, 인터럽트타이밍(라이징엣지/폴링엣지)설정, 인터럽트 이름, 인터럽트서비스루틴에 전달할 변수)
		result = request_irq(pKeyData->key_irq[i], key_isr, IRQF_TRIGGER_RISING, irqName[i], pKeyData);
		if(result < 0) 
		{
			printk(KERN_ERR "request_irq() failed irq %d\n", pKeyData->key_irq[i]);
			return result;
		}
	}

	return 0; // 정상종료
}

static void gpioKeyIrqFree(keyData* pKeyData)
{
	int i;
	for(i = 0; i < GPIOKEYCNT; i++)
	{
		free_irq(pKeyData->key_irq[i], pKeyData); // 커널 지원 함수 사용
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

static int ledkey_open(struct inode *inode, struct file *filp)
{
	int result;
	int num0 = MINOR(inode->i_rdev); 
	int num1 = MAJOR(inode->i_rdev);

	// 동적 메모리 할당(36바이트의 keyData) 및 인터럽트 관련 각종 초기화 등록
	keyData* pKeyData = (keyData*)kmalloc(sizeof(keyData), GFP_KERNEL); 
	if(!pKeyData) return -ENOMEM; 
	pKeyData->keyNumber = 0;
	// GFP_KERNEL: 항상 메모리를 할당하도록 하는 매크로
	// 우리가 목적하는 것은 현재 이 함수의 지역변수로 선언된 pKeyData의 내용을 다른 시스템 콜 함수에서 사용할 수 있도록 하는 것이다. 그것을 위해서 *filp를 사용할 것이다.

	result = gpioLedInit();
	if(result < 0) return result;
	result = gpioKeyInit();
	if(result < 0) return result;
	result = gpioKeyIrqInit(pKeyData); // 동적할당 된 메모리를 넘겨준다.
	if(result < 0) return result;

	filp->private_data = pKeyData;
	// 우리는 file 구조체(`struct file *filp`)의 멤버변수인 `void  *private_data;`에다가 지역변수로 선언된 동적할당 지역변수 포인터를 넘겨주면 그렇게 함으로써 모든 디바이스 드라이버 함수에서 `*filp`를 통해서 해당 동적할당 메모리에 접근할 수 있게 되는 것이다.(file 구조체 내부는 fs.h 파일에 있다)

#if DEBUG
	printk("ledkey open-> minor : %d\n", num0);
	printk("ledkey open-> major : %d\n", num1);
#endif
	try_module_get(THIS_MODULE);

	return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	//char kbuff;
	keyData* pKeyData = (keyData*)filp->private_data;

	//kbuff = gpioKeyGet(); // 이렇게 폴링으로 하지 않고 인터럽트를 적용해야하는 부분
	//kbuff = pKeyData->keyNumber; // 전역변수 keyNumber 

	/*프로세스를 슬립모드로 보내는 경우*/
	if (pKeyData->keyNumber == 0) // 키가 눌리지 않았다면.. 블로킹 처리를 해야함
	{
		// 만약 open()할때 O_NONBLOCK 플래그를 주지 않고 열었다면 프로세스를 sleep상태로 만들어줘야함
		if ( !(filp->f_flags & O_NONBLOCK) )
		{
#if 1
			/* 인터럽트 콜백 함수를 이용하여 깨우는 방법(1) */
			wait_event_interruptible(WaitQueue_Read, pKeyData->keyNumber); // 이게 프로세스를 재우는 함수이다
			// 여기서 두번째 매개변수로 준 keyNumber의 값이 1이 되면 깨어나는 거고, 0이면 그대로 있는 것이다
#else
			/* timeout으로 일정 시간마다 깨우는 방법(2) */
			wait_event_interruptible_timeout(WaitQueue_Read, pKeyData->keyNumber, 100); // 1초마다 깨어남
			// 커널 타이머가 100hz이기 때문에 T = 1/100 = 0.01sec이고 0.01 * 100이므로 1sec
			// 이렇게 해도 인터럽트에 의해 깨어나는 것은 동일하다.(위의 상황에다가 timeout기능만 추가한 것)
#endif
		}
		// 사용자가 key를 누르면 '커널은' key_isr() 함수를 호출한다. 
		//그래서 우리는 커널히 호출하는 해당 함수쪽에서 프로세스를 깨우는 함수를 추가할 것이다.

	}

	//put_user(kbuff, buf);
	put_user(pKeyData->keyNumber, buf);
	//result = copy_to_user(buf, &(pKeyData->keyNumber), count);
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
	keyData* pKeyData = (keyData*)filp->private_data;

	printk("ledkey release \n");
	module_put(THIS_MODULE);

	gpioKeyIrqFree(pKeyData); // 파일구조체 포인터가 들고 있던 동적메모리 값을 인자로 던져줌
	gpioKeyFree();
	gpioLedFree();
	if (pKeyData) // 주소가 할당되어있다면..(nullptr이 아니라면..)
		kfree(pKeyData); // 혹시 다른 곳에서 동적메모리를 해제할 수도 있기 때문에 이렇게 구성함.

	return 0;
}

static struct file_operations ledkey_fops = {
	//.owner			= THIS_MODULE,
	.read			= ledkey_read,
	.write			= ledkey_write,
	.unlocked_ioctl	= ledkey_ioctl,
	.open			= ledkey_open,
	.release		= ledkey_release, // 저수준 입출력함수에서 close()가 .release에 대응한다.
};

static int ledkey_init(void)
{
	int result;

	printk("ledkey ledkey_init \n");

	result = register_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops); 
	if (result < 0) return result;

	return 0;
}

static void ledkey_exit(void)
{
	printk("ledkey ledkey_exit \n");

	unregister_chrdev(LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME);
}

module_init(ledkey_init);
module_exit(ledkey_exit);
MODULE_AUTHOR("KCCI");
MODULE_DESCRIPTION("Device Driver pracite~!");
MODULE_LICENSE("Dual BSD/GPL");

