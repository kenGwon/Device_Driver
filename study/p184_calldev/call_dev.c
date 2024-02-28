#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#define CALL_DEV_NAME	"call_dev"
#define CALL_DEV_MAJOR	230 // 디바이스 주번호

static int call_open(struct inode *inode, struct file *filp)
{
	int num = MINOR(inode->i_rdev); 
	printk("call open-> minor : %d\n", num);

	num = MAJOR(inode->i_rdev);
	printk("call open-> major : %d\n", num);
	try_module_get(THIS_MODULE);
	return 0;
}

static loff_t call_llseek(struct file *filp, loff_t off, int whence)
{
	printk("call llseek -> off : %08X, whence : %08X\n", (unsigned int)off, whence);
	return 0x23;
}

static ssize_t call_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	printk("call read -> buf : %08X, count : %08X \n", (unsigned int)buf, count);
	return 0x33;
}

static ssize_t call_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	printk("call write -> buf : %08X, count : %08X \n", (unsigned int)buf, count);
	return 0x43;
}

static long call_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk("call ioctl -> cmd : %08X, arg : %08X\n", cmd, (unsigned int)arg);
	return 0x53;
}

static int call_release(struct inode *inode, struct file *filp)
{
	printk("call release \n");
	module_put(THIS_MODULE);
	return 0;
}

// 구조체다
struct file_operations call_fops = {
	.owner			= THIS_MODULE,
	.llseek			= call_llseek,
	.read			= call_read,
	.write			= call_write,
	.unlocked_ioctl	= call_ioctl,
	.open			= call_open,
	.release		= call_release, // 저수준 입출력함수에서 close()가 .release에 대응한다.
};

static int call_init(void)
{
	int result;
	printk("call call_init \n");
	// 캐릭터 디바이스를 메모리에 적재시키는 명령
	result = register_chrdev(CALL_DEV_MAJOR, CALL_DEV_NAME, &call_fops); // call_fops의 주소값을 등록
	if (result < 0)
		return result;
	return 0;
}

static void call_exit(void)
{
	printk("call call_exit \n");
	unregister_chrdev(CALL_DEV_MAJOR, CALL_DEV_NAME);
}

module_init(call_init);
module_exit(call_exit);
MODULE_AUTHOR("KCCI");
MODULE_DESCRIPTION("Device Driver pracite~!");
MODULE_LICENSE("Dual BSD/GPL");
