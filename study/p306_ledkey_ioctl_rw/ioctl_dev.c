#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       

#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include "ioctl_test.h"

#define GPIOLEDCNT 8
#define GPIOKEYCNT 8
#define OFF 0
#define ON 1

#define   LEDKEY_DEV_NAME            "ioctldev"
#define   LEDKEY_DEV_MAJOR            230      
int gpioLed[GPIOLEDCNT] = {6,7,8,9,10,11,12,13};
int gpioKey[GPIOKEYCNT] = {16,17,18,19,20,21,22,23};
static int onevalue = 1;
static char * twostring = NULL;
module_param(onevalue, int ,0);
module_param(twostring,charp,0);


int gpioLedInit(void);
void gpioLedSet(long);
void gpioLedFree(void);
int gpioKeyInit(void);
int gpioKeyGet(void);
void gpioKeyFree(void);

int	gpioLedInit(void)
{
	int i;
	int ret = 0;
	char gpioName[10];
	for(i=0;i<GPIOLEDCNT;i++)
	{
		sprintf(gpioName,"led%d",i);
		ret = gpio_request(gpioLed[i], gpioName);
		if(ret < 0) {
			printk("Failed Request gpio%d error\n", 6);
			return ret;
		}
	}
	for(i=0;i<GPIOLEDCNT;i++)
	{
		ret = gpio_direction_output(gpioLed[i], OFF);
		if(ret < 0) {
			printk("Failed direction_output gpio%d error\n", 6);
       	 return ret;
		}
	}
	return ret;
}

void gpioLedSet(long val) 
{
	int i;
	for(i=0;i<GPIOLEDCNT;i++)
	{
		gpio_set_value(gpioLed[i], (val>>i) & 0x01);
	}
}
void gpioLedFree(void)
{
	int i;
	for(i=0;i<GPIOLEDCNT;i++)
	{
		gpio_free(gpioLed[i]);
	}
}

int gpioKeyInit(void) 
{
	int i;
	int ret=0;
	char gpioName[10];
	for(i=0;i<GPIOKEYCNT;i++)
	{
		sprintf(gpioName,"key%d",i);
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
int	gpioKeyGet(void) 
{
	int i;
	int ret;
	int keyData=0;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		ret=gpio_get_value(gpioKey[i]) << i;
		keyData |= ret;
	}
	return keyData;
}
void gpioKeyFree(void) 
{
	int i;
	for(i=0;i<GPIOKEYCNT;i++)
	{
		gpio_free(gpioKey[i]);
	}
}
static int ledkey_open (struct inode *inode, struct file *filp)
{
    int num0 = MAJOR(inode->i_rdev); 
    int num1 = MINOR(inode->i_rdev); 
    printk( "call open -> major : %d\n", num0 );
    printk( "call open -> minor : %d\n", num1 );

	try_module_get(THIS_MODULE);
    return 0;
}

static ssize_t ledkey_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
	kbuf = gpioKeyGet();
//  put_user(kbuf,buf);
    ret=copy_to_user(buf,&kbuf,count);
    return count;
}

static ssize_t ledkey_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	char kbuf;
	int ret;
//  get_user(kbuf,buf);
    ret=copy_from_user(&kbuf,buf,count);
    gpioLedSet(kbuf);

    return count;
//	return -EFAULT;
}

static long ledkey_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{

	ioctl_test_info ctrl_info = {0,{0}};
	int err=0, size;
	if( _IOC_TYPE( cmd ) != IOCTLTEST_MAGIC ) return -EINVAL; // 만약 들어온 커맨드의 매직넘버가 다르다면..
	if( _IOC_NR( cmd ) >= IOCTLTEST_MAXNR ) return -EINVAL; // 커맨드의 구분번호가 잘못된 범위에 해당한다면..

	size = _IOC_SIZE( cmd );
	if( size ) // size가 0이 아니라면 진입...(읽기/쓰기 작업이 발생하는 경우에만 구조체의 크기인 132를 물고 진입하게 된다.)
	{
		if( _IOC_DIR( cmd ) & _IOC_READ )
			// 현재 arg에는 구조체의 시작주소의 '값'이 들어있다고 했다.(어떤 자료형인지에 대한 정보는 유실된 상태)
			// 그래서 그걸 void 포인터로 받아서 `주소`로 다시 형변환 해주고 해당 주소로부터 사이즈 132까지의 메모리를 읽고 싶은 상황이다.(어떤 자료형에 대한 주소인지 모르기 때문에 void로 받고 size로 최대 크기 지정하는 것임)
			//그래서 해당 주소에서 해당 사이즈 만큼 접근하는 것이 가능한 상황인지를 access_ok()함수를 통해서 확인하고 있다.
			err = access_ok( (void *) arg, size ); 
		if( _IOC_DIR( cmd ) & _IOC_WRITE )
			err = access_ok( (void *) arg, size );
		if( !err ) return err;
	}
	switch( cmd )
	{
		char buf;
		case IOCTLTEST_KEYLEDINIT :  // 원래 모듈을 insmod 할 때 호출되던 init()에 있던 초기화를 어플리케이션 코드의 명령을 받아서 실행될 수 있도록 구조를 바꾼 것이다. 
            gpioLedInit();
            gpioKeyInit();
            break;
		case IOCTLTEST_KEYLEDFREE :  // 원래 모듈을 rmmod 할 때 호출되던 exit()에 있던 해제루틴을 어필리케이션 코드의 명령을 받아서 실행될 수 있도록 구조를 바꾼 것이다.
            gpioLedFree();
            gpioKeyFree();
            break;
		case IOCTLTEST_LEDOFF :
			gpioLedSet(0);
			break;
		case IOCTLTEST_LEDON :
			gpioLedSet(255);
			break;
		case IOCTLTEST_GETSTATE :
			buf = gpioKeyGet();
			return buf;  // 여기서 리턴되는 값은 어플리케이션으로 리턴됨
		case IOCTLTEST_READ :
  			ctrl_info.buff[0] = gpioKeyGet();
			if(ctrl_info.buff[0] != 0)
				ctrl_info.size=1; // 키가 눌러지면 우선 사이즈에 1을 넣어준다.
			err = copy_to_user((void *)arg,(const void *)&ctrl_info,size); // 원래의 주소로 읽은 값을 복사하여 전달해준다.
			break;

		case IOCTLTEST_WRITE :
			err = copy_from_user((void *)&ctrl_info,(void *)arg,size); // void 포인터로 arg를 형변환하고, cmd에 들어가있는 size 정보로 올바른 주소에서 올바른 양의 데이터를 읽엇거 복사한다는게 포인트이다.
			if(ctrl_info.size == 1)
				gpioLedSet(ctrl_info.buff[0]);
			break;
		case IOCTLTEST_WRITE_READ :
			err = copy_from_user((void *)&ctrl_info,(void *)arg,size);
			if(ctrl_info.size == 1)
				gpioLedSet(ctrl_info.buff[0]);

  			ctrl_info.buff[0] = gpioKeyGet();
			if(ctrl_info.buff[0] != 0)
				ctrl_info.size=1;
			else
				ctrl_info.size=0; // 읽은 값이 없다는 의미

			err = copy_to_user((void *)arg,(const void *)&ctrl_info,size);
			break;
		default:
			err =-E2BIG;
			break;
	}	
	return err;
}

static int ledkey_release (struct inode *inode, struct file *filp)
{
    printk( "call release \n" );
	module_put(THIS_MODULE);
    return 0;
}

struct file_operations ledkey_fops =
{
    //.owner     		= THIS_MODULE,
    .read			= ledkey_read,     
    .write    		= ledkey_write,    
	.unlocked_ioctl = ledkey_ioctl,
    .open    		= ledkey_open,     
    .release  		= ledkey_release,  
};

static int ledkey_init(void)
{
    int result;

    printk( "call ledkey_init \n" );    

    result = register_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME, &ledkey_fops);
    if (result < 0) return result;

//	gpioLedInit();
//	gpioKeyInit();
    return 0;
}

static void ledkey_exit(void)
{
    printk( "call ledkey_exit \n" );    
    unregister_chrdev( LEDKEY_DEV_MAJOR, LEDKEY_DEV_NAME );
//	gpioLedFree();
//	gpioKeyFree();
}

module_init(ledkey_init);
module_exit(ledkey_exit);

MODULE_AUTHOR("KCCI-AIOT KSH");
MODULE_DESCRIPTION("led key test module");
MODULE_LICENSE("Dual BSD/GPL");
