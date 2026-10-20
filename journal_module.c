#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/types.h>

#include "include/generated/autoconf.h"
#include "linux/timekeeping.h"


#ifdef CONFIG_DEFCONFIG
    #define CONFIG "defconfig"
#endif
#ifdef CONFIG_CONFIG
    #define CONFIG "config"
#endif

struct module_info
{
    char * conf;
    int read_count;
    int last_read;
    char * status;
    int last_reset;
} info = 
{
    CONFIG,
    0,
    0,
    "enabled",
    0,
};

static int command = 1;

int update(void);


#define PROCFS_MAX_SIZE 512
#define PROCFS_NAME "journal"

static struct proc_dir_entry *our_proc_file;
static char procfs_buffer[PROCFS_MAX_SIZE]; 
static unsigned long procfs_buffer_size = 0; 

// обработчик считывания файла из procfs
static ssize_t procfile_read(struct file *file_p, char __user *buffer, 
                             size_t buffer_length, loff_t *offset) 
{
    if (update()){
        pr_info("copy_to_user failed (wrong command)\n");
        return 1;
    }

    procfs_buffer_size = 0; 
    procfs_buffer_size += scnprintf(
        procfs_buffer, 
        PROCFS_MAX_SIZE - procfs_buffer_size, 
        "configuration\t:%s\nread_count\t:%d\nlast_read\t:%d\nstatus\t\t:%s\nlast_reset\t:%d\n", 
        info.conf,
        info.read_count,
        info.last_read,
        info.status,
        info.last_reset);
    ssize_t ret = procfs_buffer_size;
 
    if (*offset >= procfs_buffer_size || copy_to_user(buffer, procfs_buffer, procfs_buffer_size)) {
        pr_info("copy_to_user failed\n");
        ret = 0;
    } else {
        pr_info("procfile read %s\n", file_p->f_path.dentry->d_name.name);
        *offset += procfs_buffer_size;
    }
 
    return ret; 
}

static const struct proc_ops proc_file_fops = 
{ 
    .proc_read = procfile_read, 
}; 


#define SYSFS_NAME "journal"

static struct kobject *journal_module;

static ssize_t command_show(struct kobject *kobj,
                               struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", command);
}

static ssize_t command_store(struct kobject *kobj,
                                struct kobj_attribute *attr, char *buf,
                                size_t count)
{
    sscanf(buf, "%d", &command);
    return count;
}

static struct kobj_attribute command_attribute =
    __ATTR(command, 0660 , command_show, (void *)command_store);


static int __init fs_init(void)
{
	our_proc_file = proc_create(PROCFS_NAME, 0440, NULL, &proc_file_fops); 
    if (NULL == our_proc_file) { 
        proc_remove(our_proc_file); 
        pr_alert("journal module initialization failed\n");
        return -ENOMEM; 
    }


	int error = 0;
	journal_module = kobject_create_and_add(SYSFS_NAME, kernel_kobj);
    if (!journal_module) {
	 	return -ENOMEM;
	}

    error = sysfs_create_file(journal_module, &command_attribute.attr);
	if (error) {
		pr_alert("journal module initialization failed\n");
        kobject_put(journal_module);
		return error;
	}


	pr_info("journal module initialized\n");
	return 0;
}

static void __exit fs_exit(void)
{
	proc_remove(our_proc_file);
	kobject_put(journal_module);
	pr_info("journal module removed\n");
}

int update(void)
{
    if (command == 1)
    {
        info.read_count += 1;
        info.last_read = ktime_get_real();
        info.status = "enabled";
    }
    else if (command == 0)
    {
        info.status = "disabled";
    }
    else if (command == 2)
    {
        info.last_reset = ktime_get_real();
        info.read_count = 1;
        info.last_read = ktime_get_real();
        info.status = "enabled";
        command = 1;
    }
    else
    {
        command = 2;
        return 1;
    }
    
    return 0;
}


module_init(fs_init);
module_exit(fs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zadevarses");
MODULE_DESCRIPTION("Simple kernel module journal");
