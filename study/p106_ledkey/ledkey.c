#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#define OFF 0
#define ON 1
#define GPIOLEDCNT 8
#define GPIOKEYCNT 8
static int gpioLed[GPIOLEDCNT] = {6,7,8,9,10,11,12,13};
static int gpioKey[GPIOKEYCNT] = {16,17,18,19,20,21,22,23};

static int gpioLedInit(void);
static void gpioLedSet(long);
static void gpioLedFree(void);
static int gpioKeyInit(void);
static int gpioKeyGet(void);
static void gpioKeyFree(void);

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

static int ledkey_init(void)
{
	int ret;
    int key_data, past_key_data;
	char strBuff[16] = " : : : : : : : ";

	printk("start ledkey~~\n");

	ret=gpioLedInit();
	if(ret < 0)
		return ret;

	ret=gpioKeyInit();
	if(ret < 0)
		return ret;

	gpioLedSet(0xff);
	past_key_data = gpioKeyGet();

    do
    {
		key_data = gpioKeyGet();

        if (key_data != past_key_data)
        {
            if (past_key_data == 0x00)
            {
                past_key_data = key_data;
            }
            else
            {
				mdelay(100);

                printk("0:1:2:3:4:5:6:7\n");

                for (int i = 0; i < 8; i++)
                {
                    int bit = (past_key_data >> i) & 1;
                    if (bit == 1)
						strBuff[i*2] = 'O';
                    else
						strBuff[i*2] = 'X';
                }
				strBuff[15] = '\0';
				printk("%s\n", strBuff);

				gpioLedSet(0x00);
				gpioLedSet(past_key_data);

                if (past_key_data == 0x80)
                {
                    break;
                }

                past_key_data = key_data;
            }
        }
    } while(1);

	return 0;
}
static void ledkey_exit(void)
{
	/*
	printk("Goodbye, world \n");
	gpioLedSet(0x00);
	printk("led all off \n");
    gpioLedFree();
	*/
	printk("end ledkey~~\n");
	gpioLedSet(0x00);
	gpioLedFree();
	gpioKeyFree();
}
module_init(ledkey_init);
module_exit(ledkey_exit);
MODULE_AUTHOR("KCCI-AIOT"); // 모듈 작성자를 넣어줄 수 있다.
MODULE_DESCRIPTION("led key test module"); // 모듈에 대한 설명을 넣어줄 수 있다.
MODULE_LICENSE("Dual BSD/GPL");

