#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>

#define OFF 0
#define ON 1
#define GPIOLEDCNT 8
#define GPIOKEYCNT 8

static int gpioLed[GPIOLEDCNT] = {6, 7, 8, 9, 10, 11, 12, 13};
static int gpioKey[GPIOKEYCNT] = {16, 17, 18, 19, 20, 21, 22, 23};

static int gpioLedInit(void);
static void gpioLedSet(long val);
static void gpioLedFree(void);

static int gpioKeyInit(void);
static int gpioKeyGet(void);
static void gpioKeyFree(void);

static int gpioKeyInit(void)
{
	int i;
	int ret = 0; // 정상종료 초기값 // 리턴 값이 있는 함수가 모인 코드 블럭을 함수화 할대는 그 형을 맞춰주자
	char gpioName[10];

	for (i = 0; i < GPIOKEYCNT; i++)
	{
		sprintf(gpioName, "key%d", gpioKey[i]);

		ret = gpio_request(gpioKey[i], gpioName); 
		if (ret < 0) 
		{
			printk("Failed gpio_request() gpio%d error\n", i); 
			return ret;
		}

		ret = gpio_direction_input(gpioKey[i]);
		if (ret < 0)
		{
			printk("Failed gpio_direction_input() gpio%d error\n", i);
			return ret;
		}
	}

	return ret;
}

static int gpioKeyGet(void)
{
	// 여기서 for문 돌면서 순회해서 찾는거네
	int ret;
	int i;
	int keyData = 0x00;

	for (i = 0; i < GPIOKEYCNT; i++)
	{
		ret = gpio_get_value(gpioKey[i]);
		keyData = keyData | (ret << i);
	}

	return keyData;
}

static void gpioKeyFree(void)
{
	int i;

	for (i = 0; i < GPIOKEYCNT; i++)
	{
		gpio_free(gpioKey[i]); 
	}	
}

static int gpioLedInit(void)
{
	int i;
	int ret = 0; // 정상종료 초기값 // 리턴 값이 있는 함수가 모인 코드 블럭을 함수화 할대는 그 형을 맞춰주자
	char gpioName[10];

	for (i = 0; i < GPIOLEDCNT; i++)
	{
		sprintf(gpioName, "led%d", i);

		ret = gpio_request(gpioLed[i], gpioName); 
		if (ret < 0) 
		{
			printk("Failed gpio_request() gpio%d error\n", i); 
			return ret;
		}

		ret = gpio_direction_output(gpioLed[i], OFF);
		if (ret < 0)
		{
			printk("Failed gpio_direction_output() gpio%d error\n", i);
			return ret;
		}
	}

	return ret;
}

static void gpioLedSet(long val)
{
	int i;

	for (i = 0; i < GPIOLEDCNT; i++)
	{
		gpio_set_value(gpioLed[i], (val >> i) & 0x01); 
	}
}

static void gpioLedFree(void)
{
	int i;

	for (i = 0; i < GPIOLEDCNT; i++)
	{
		gpio_free(gpioLed[i]); 
	}	
}


/* 모듈 코드는 꼭 맨 마지막에 와야한다 */
static int led_init(void)
{
	int ret;

	printk("Hello, world\n");

	// LED 관련
	ret = gpioLedInit();
	if (ret < 0)
		return ret;
	gpioLedSet(0xff);

	return 0;
}

static void led_exit(void)
{
	printk("Goodbye, world\n");

	// LED 관련
	gpioLedSet(0x00);
	gpioLedFree();
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("Dual BSD/GPL");
