/* Based on example from: http://tuxthink.blogspot.com/2011/02/kernel-thread-creation-1.html
   Modified and commented by: Luis Rivera			
   
   Compile using the Makefile
*/

#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif
   
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>	// for kthreads
#include <linux/sched.h>	// for task_struct
#include <linux/time.h>		// for using jiffies 
#include <linux/timer.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>


MODULE_LICENSE("GPL");

#define GPIO_BASE 0x3F200000U
//#define GPIO_SET_BASE (*((unsigned long *)(GPIO_BASE + 0x1CU)))
#define GPIO_SET_BASE GPIO_BASE + 0x1CU
#define GPIO_CLR_BASE GPIO_BASE + 0x28U
#define GPIO_GPED_BASE GPIO_BASE + 0x40U

#define GPEDS_OFFSET 0x10

// structure for the kthread.
static struct task_struct *kthread1;
int mydev_id;

#define MSG_SIZE 50
#define CDEV_NAME "Lab6"	// "YourDevName"

MODULE_LICENSE("GPL");
 
static int major; 
static char msg[MSG_SIZE];



static irqreturn_t button_isr(int irq, void *dev_id)
{
	// In general, you want to disable the interrupt while handling it.
	disable_irq_nosync(79);

	unsigned long * gpsel0_ptr = (unsigned long*) ioremap(GPIO_BASE, 4096);
	unsigned long * gpeds0_ptr = gpsel0_ptr + GPEDS_OFFSET;
	unsigned long * gpset0_ptr = (gpsel0_ptr + 7);
	unsigned long * gpclr0_ptr = (gpsel0_ptr + 10);

	// printk("GPESD PTR: %X \n", gpeds0_ptr);



	// setting leds to output
	iowrite32(0x00009240 , gpsel0_ptr);

	unsigned long high_pin = ioread32(gpeds0_ptr);
	// printk("high pins: 0x%X", high_pin);
	// F8000
	unsigned long high_pin_mask = high_pin & 0b00000000000011111000000000000000;
	// printk("high pins: 0x%X", high_pin_mask);

	switch (high_pin_mask)
	{
		case 0x10000:
			printk("button 1 \n");
			iowrite32(0x00000004U, gpset0_ptr);
			break;
		case 0x20000:
			iowrite32(0x00000008U, gpset0_ptr);
			break;
		case 0x40000:
			iowrite32(0x000000010U, gpset0_ptr);
			break;
		case 0x80000:
			iowrite32(0x000000020U, gpset0_ptr);
			break;
		case 0x0:
			iowrite32(0x0000003CU, gpclr0_ptr);
			printk("button 5 \n");
			break;
	}

	iowrite32(0x001F0000, gpeds0_ptr);
	enable_irq(79);		// re-enable interrupt
	printk("Interrupt handled\n");	
	
    return IRQ_HANDLED;
}


// Function to be associated with the kthread; what the kthread executes.
int kthread_fn(void *ptr)
{
	/* GPIO Ptrs */
	unsigned long * gpsel0_ptr = (unsigned long*) ioremap(GPIO_BASE, 4096);
	unsigned long * gpeds0_ptr = gpsel0_ptr + GPEDS_OFFSET;
	unsigned long * gpset0_ptr = (gpsel0_ptr + 7);
	unsigned long * gpclr0_ptr = (gpsel0_ptr + 10);

	// setting spkr to output (and LEDS for now for testing)
	iowrite32(0x00049240 , gpsel0_ptr);
	/* E.S. */

	unsigned long j0, j1;
	int count = 0;

	printk("In kthread1\n");
	j0 = jiffies;		// number of clock ticks since system started;
						// current "time" in jiffies
	j1 = j0 + 10*HZ;	// HZ is the number of ticks per second, that is
						// 1 HZ is 1 second in jiffies
	
	while(time_before(jiffies, j1))	// true when current "time" is less than j1
        schedule();		// voluntarily tell the scheduler that it can schedule
						// some other process
	
	printk("Before loop\n");
	
	// The ktrhead does not need to run forever. It can execute something
	// and then leave.
	while(1)
	{
		// iowrite32(0x00000004U, gpset0_ptr);
		// // ON
		// iowrite32(0x000000020U, gpset0_ptr);
		msleep(50);	// good for > 10 ms

		// //OFF
		// iowrite32(0x0000003CU, gpclr0_ptr);
		// msleep(50);

		//msleep_interruptible(1000); // good for > 10 ms
		//udelay(unsigned long usecs);	// good for a few us (micro s)
		//usleep_range(unsigned long min, unsigned long max); // good for 10us - 20 ms
		
		
		// In an infinite loop, you should check if the kthread_stop
		// function has been called (e.g. in clean up module). If so,
		// the kthread should exit. If this is not done, the thread
		// will persist even after removing the module.
		if(kthread_should_stop()) {
			do_exit(0);
		}
				
		// comment out if your loop is going "fast". You don't want to
		// printk too often. Sporadically or every second or so, it's okay.
		if(count % 10 == 0)
			printk("Count: %d\n", ++count);
	}
	
	return 0;
}

int thread_init(void)
{
	/* Button & Speaker Setup Section */
	unsigned long * gpsel_ptr = ioremap(GPIO_BASE, 4096);
	unsigned long * gpsel1_ptr = (gpsel_ptr + 1);
	unsigned long * gpsel2_ptr = (gpsel_ptr + 2);
	unsigned long * gppud_ptr = (gpsel_ptr + 37);
	unsigned long * gppudclk0_ptr = (gpsel_ptr + 38);
	unsigned long * gpafen0_ptr = (gpsel_ptr + 34);
	
	// set as input
	iowrite32( 0x00000000 , gpsel1_ptr);
	iowrite32( 0x00000000 , gpsel2_ptr);

	// set as pull down
	iowrite32( 0x00000001, gppud_ptr);

	// delay
	udelay(100);

	// write 1 for bits 16-20 to gppudclk reg
	iowrite32( 0x001F0000, gppudclk0_ptr);

	// delay
	udelay(100);

	iowrite32(0x00000000, gppud_ptr);
	iowrite32(0x00000000, gppudclk0_ptr);
	
	// set bits 16-20 for gparen0
	iowrite32(0x001F0000 , gpafen0_ptr);

	// bind
	int dummy = 0;
	dummy = request_irq(79, button_isr, IRQF_SHARED, "Button_handler", &mydev_id);
	/* E.S. */

	/* CDEV Init Section */

	// // register the Characted Device and obtain the major (assigned by the system)
	// major = register_chrdev(0, CDEV_NAME, &fops);
	// if (major < 0) {
    //  		printk("Registering the character device failed with %d\n", major);
	//      	return major;
	// }
	// printk("Lab6_cdev_kmod example, assigned major: %d\n", major);
	// printk("Create Char Device (node) with: sudo mknod /dev/%s c %d 0\n", CDEV_NAME, major);

	/* E.S. */

	char kthread_name[11]="my_kthread";	// try running  ps -ef | grep my_kthread
										// when the thread is active.
	printk("In init module\n");
    	    
    kthread1 = kthread_create(kthread_fn, NULL, kthread_name);

	
    if((kthread1))	// true if kthread creation is successful
    {
        printk("Inside if\n");
		// kthread is dormant after creation. Needs to be woken up
        wake_up_process(kthread1);
    }

    return 0;
}

void thread_cleanup(void) {
	int ret;

	unsigned long * gpclr_ptr = ioremap(GPIO_CLR_BASE, 4096);
	iowrite32(0x00000004U, gpclr_ptr);
	iowrite32(0x00000008U, gpclr_ptr);
	iowrite32(0x00000010U, gpclr_ptr);
	iowrite32(0x00000020U, gpclr_ptr);


	// // Once unregistered, the Character Device won't be able to be accessed,
	// // even if the file /dev/YourDevName still exists. Give that a try...
	// unregister_chrdev(major, CDEV_NAME);
	// printk("Char Device /dev/%s unregistered.\n", CDEV_NAME);

	// the following doesn't actually stop the thread, but signals that
	// the thread should stop itself (with do_exit above).
	// kthread should not be called if the thread has already stopped.
	ret = kthread_stop(kthread1);
								
	if(!ret)
		printk("Kthread stopped\n");
}

module_init(thread_init);
module_exit(thread_cleanup);
