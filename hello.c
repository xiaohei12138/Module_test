#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/rcupdate.h>
#include <linux/ioctl.h>

#define cdev_Num 3
#define HELLO_MAJOR 174


#define dev_men_size 100
#ifndef DEG
	#define DBG(fmt,args...) printk(KERN_ALERT  fmt,##args)
#endif
MODULE_LICENSE("Dual BSD/GPL");     


static struct hello_dev{
	struct cdev cdev;
	unsigned char *men;
};

struct hello_dev *helloDev;
static int hello_MAJOR = HELLO_MAJOR;
static int hello_open(struct inode *inode, struct file *file)
{
	struct hello_dev *dev;
	dev = container_of(inode->i_cdev,struct hello_dev,cdev);
	DBG("this is open, file->flags : %u    dev = %u\n",file->f_flags,dev);
	file->private_data = dev;
	return 0;
}
static ssize_t hello_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	unsigned long p = *f_pos;
	int ret =0;
	struct hello_dev *dev = filp->private_data;
	if(p >= dev_men_size)
		return 0;	
	if(count > dev_men_size-p)
		count =  dev_men_size-p;
	
	if(copy_to_user(buf,dev->men+p,count))
	{
		DBG("copy to user fail\n");
		return - EFAULT;
	}
	else
	{
		*f_pos += count;
		ret = count;
	}
	return ret;




}
static ssize_t hello_write(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	
	unsigned long p = *f_pos;
	int ret =0;
	struct hello_dev *dev = filp->private_data;
	if(dev->men == NULL)
	{
		dev->men = kmalloc(dev_men_size,GFP_KERNEL);
	}
	else
		DBG(" dev->men :%s\n", dev->men);
	
	
	if(p >= dev_men_size)
		return 0;
	if(count > dev_men_size-p)
		count =  dev_men_size-p;
	if(copy_from_user(dev->men+p,buf,count))
	{
		DBG("copy to user fail\n");
		return - EFAULT;
	}
	else
	{
		*f_pos += count;
		ret = count;
	}
	DBG("user have write :%s\n",dev->men);
	return ret;

}
loff_t hello_llseek(struct file *filp, loff_t f_pos,int orig)
{
       
#if 1
	 loff_t ret;
	int offset = (int ) f_pos;
	int CurSet = (int ) filp->f_pos;
//	filp->private_data = &helloDev;
	switch(orig)
	{
		case 0 :
			if(offset < 0) 
				CurSet = 0;
			else if(offset >=dev_men_size) 
				CurSet= dev_men_size-1;	
			else 
				CurSet = offset;	
			break;
		case 1:
			if(offset+CurSet >=dev_men_size) 
				CurSet = dev_men_size -1;
			else if(offset+filp->f_op< 0)
				CurSet = 0;
			else
				CurSet += offset;
			break;

		case 2:
			if(offset > 0) 			
				{CurSet = dev_men_size-1;printk("***********offset :: %d\n",offset);}		
			else if(offset <0-dev_men_size)
				CurSet = 0;
			else 
				CurSet = dev_men_size-1+offset;
			break;
		default:
			printk("fail to lseek\n");
			return -1;
	}
	
	filp->f_pos =(loff_t )CurSet;
	return CurSet;
#endif
}
	
int hello_ioctl(struct inode *inode,struct file *filp,unsigned int cmd,unsigned long org) 
{
	DBG("ioctl function\n");
	DBG("cmd : %u\n  org: %u\n",cmd,(unsigned int)org);
	switch(cmd)
	{
		case 1:
			DBG("function  1\n");break;
		case 2:
			DBG("function  2\n");break;
		default:
			DBG("fail function\n");
			return -1;
	}
	return 0;
}

static const struct file_operations hello_devices_fileops = {
	.owner          = THIS_MODULE,
	.open           = hello_open,
	.read		= hello_read,
	.write		= hello_write,
	.llseek		= hello_llseek,
	.ioctl		= hello_ioctl,
};
static void hello_setup_cdev(struct hello_dev *dev,int index)
{
	int devno = MKDEV(hello_MAJOR,index);
	int err;
	cdev_init(&dev->cdev,&hello_devices_fileops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &hello_devices_fileops;
	err = cdev_add(&dev->cdev,devno,1);
	if(err)
		DBG("fail cdev_add : err : %d  index : %d",err,index);
}

static int hello_init(void) //有的上面定义的是init_modules(void)是通不过编译的
{ 

	int ret,i ;
	dev_t denvo ;
	//获取设备号 初始化设备结构体  注册设备
	
	DBG("initing ...\n");
	ret = alloc_chrdev_region(&denvo,0,cdev_Num,"hello");
	if(ret<0)
		DBG("fail alloc_chrdev_region\n");
	DBG("denov : %u    %u  \n",MAJOR(denvo),MINOR(denvo));
	hello_MAJOR = MAJOR(denvo);
	helloDev = kmalloc(cdev_Num*sizeof(struct hello_dev),GFP_KERNEL);
	if(!helloDev)
		DBG("申请失败\n");
	memset(helloDev,0,cdev_Num*sizeof(struct hello_dev));
	for(i = 0;i < cdev_Num ; i++)
		hello_setup_cdev(helloDev+i,i);	
	DBG("init success\n");
	return 0;
	
}

static void hello_exit(void) 
{
	int i;
	DBG(KERN_DEBUG "this is exit\n"); 
#if 1
	for(i = 0;i<cdev_Num;i++)
	{	
		cdev_del(&helloDev[i].cdev);
		if(helloDev[i].men != NULL)
		{
			kfree(helloDev[i].men);
			DBG("free men success\n");
		}
	}
	kfree(helloDev);
	unregister_chrdev_region(MKDEV(hello_MAJOR,0),cdev_Num);
#endif
}

module_init(hello_init); 
module_exit(hello_exit); 
