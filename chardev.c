/*****************************************************************************
 *
 * M1522.000800
 * SYSTEM PROGRAMMING
 * 
 * Lab2. Kernel Lab
 *
 * chardev.c
 *  - handle reading pmu in kernel space.
 *
*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>

#include "chardev.h"

#define MSR_VEC_LIMIT 32
#define SUCCESS 0

MODULE_LICENSE("GPL");

static int Device_Open = 0;

static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
  printk(KERN_INFO "device_open(%p)\n", file);
#endif
  if (Device_Open)
    return -EBUSY;

  Device_Open++;
  
  try_module_get(THIS_MODULE);
  return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
  printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
#endif
  
  Device_Open--;
  
  module_put(THIS_MODULE);
  return SUCCESS;
}

static ssize_t device_read(struct file *file, 
                          char __user *buffer,
                          size_t length,
                          loff_t *offset)
{
  return 0;
}

static ssize_t device_write(struct file *file, 
                          char __user *buffer,
                          size_t length,
                          loff_t *offset)
{
  return 0;
}

static long long read_msr(unsigned int ecx) {
  /* Implement inline assembly for reading msr
   * YOUR CODE HERE */
  return 0;
}

static void write_msr(int ecx, unsigned int eax, unsigned int edx) {
  /* Implement inline assembly for writing msr
   * YOUR CODE HERE */
}

static long long read_tsc(void)
  /* reference for read_msr & write_msr
   * DON'T MODIFY THIS FUNCTION */
{
  unsigned eax, edx;
  long long result;
  __asm__ __volatile__("rdtsc" : "=a"(eax), "=d"(edx));
  result = eax | (unsigned long long)edx << 0x20;
#ifdef DEBUG
  printk(KERN_ALERT "Module msrdrv: Read 0x%016llx (0x%08x:0x%08x) from TSC\n", result, edx, eax);
#endif
  return result;
}


static long msrdrv_ioctl(struct file *f, unsigned int ioctl_num, unsigned long ioctl_param)
{
  /* implement ioctl for handling msrdrv commands
   * YOUR CODE HERE */
  return -1; 
}

int get_ptree(unsigned long ioctl_param) {
  /* implement get_ptree function
   * YOUR CODE HERE */
	struct PtreeInfo* ptreeinfo = (struct PtreeInfo*) ioctl_param;
	struct task_struct* current_task = pid_task(find_vpid(ptreeinfo->current_pid), PIDTYPE_PID);
	ptreeinfo->parent_pid = current_task->parent->pid;
	strncpy(ptreeinfo->current_name, current_task->comm, COMM_LEN_MAX);
	ptreeinfo->current_name[COMM_LEN_MAX-1] = NULL;
  return 0;
}

int device_ioctl(struct file *file, 
                unsigned int ioctl_num, 
                unsigned long ioctl_param)
{
  /* device_ioctl reference
   * DON'T MODIFY THIS FUNCTION */
  switch (ioctl_num) {
    case IOCTL_GET_PTREE:
      return get_ptree(ioctl_param);

    case IOCTL_MSR_CMDS:
      return msrdrv_ioctl(file, ioctl_num, ioctl_param);

    default:
      printk(KERN_ERR "not defined ioctl(%d)\n", ioctl_num);
      return -EINVAL;
  }

  return -EINVAL;
}

struct file_operations Fops = {
  .read =           device_read,
  .write =          device_write,
  .unlocked_ioctl = device_ioctl,
  .open =           device_open,
  .release =        device_release,
};

int init_module()
{
  int ret_val;

  ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops);

  if (ret_val < 0) {
    printk(KERN_ALERT "%s failed with %d\n",
        "Sorry, registering the character device", ret_val);
    return ret_val;
  }

  return SUCCESS;
}

void cleanup_module()
{
  unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}
