/********************************************** SEMA EC GPIO interrupt handler driver ***********************************************/

/*
=============================================
EC GPIO Interrupt Pin numbers
=============================================

1. Linux kernel 5.15 to 5.19
  a. Express-TL   - 689 (pin 79 SPI1_CSB)
  b. cExpress-TL  - 825 (pin 100 ISH_GP_1)
  c. Express-RLP  - 837 (pin 112 ISH_UART0_RXD)
  d. cExpress-MTL  - Unsupported
  e. cExpress-RLP - 837 (pin 112 ISH_UART0_RXD)
  
2. Linux kernel > 6.0/ Redhat OS
  a. Express-TL   - 640 (pin 79 SPI1_CSB)
  b. cExpress-TL  - 673 (pin 100 ISH_GP_1)
  c. Express-RLP  - 685 (pin 112 ISH_UART0_RXD)
  d. cExpress-MTL - 644 (pin 82 GPP_E_4)
  e. cExpress-RLP - 685 (pin 112 ISH_UART0_RXD)
=============================================
=============================================
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/irq.h>
#include <linux/string.h>
#include <linux/utsname.h>

#define BOARD_NAME_FILE "/sys/class/dmi/id/board_name"
#define MAX_BOARD_NAME_LEN 128

static int irq_number, gpio_pin=-1, redhat = 0, ubuntu = 0, debian = 0, suse = 0, fedora = 0;

static unsigned int new_trigger_type = IRQF_TRIGGER_NONE;  // Default: No trigger (None)
static char *trigger_type = "none";
module_param(trigger_type, charp, 0644);

void delay(unsigned long int ticks);
static int wait_for_ec_resp(uint8_t port, uint8_t mask, uint8_t cond);

static void write_ec(uint8_t reg, uint8_t value){

        outb(0x91, 0x6c);
        wait_for_ec_resp(0x6c, 0x02, 0x00);
        
        outb(reg, 0x68);
        wait_for_ec_resp(0x6c, 0x02, 0x00);
        
        outb(value, 0x68);
        wait_for_ec_resp(0x6c, 0x02, 0x00);
}

static int read_ec(uint8_t reg){
	uint8_t value;
	
        outb(0x90, 0x6c);
	wait_for_ec_resp(0x6c, 0x02, 0x00);

	outb(reg, 0x68);
	wait_for_ec_resp(0x6c, 0x03, 0x01);

	value=inb(0x68);

	return value;
}


static int os_release(void)
{
    struct new_utsname *uts;

    uts = &init_uts_ns.name;  // Access the system name information
    
    if (strstr(utsname()->release, "el") != NULL){
        printk(KERN_INFO "Red Hat or Cent OS\n");
    	redhat = 1;
    	return 0;
    	}
    else if (strstr(utsname()->release, "generic") != NULL){
        printk(KERN_INFO "Ubuntu\n");
    	ubuntu = 1;
    	return 0;
    	}
    else if (strstr(utsname()->release, "debian") != NULL){
        printk(KERN_INFO "Debian\n");
    	debian = 1;
    	return 0;
    	}
    else if (strstr(utsname()->release, "default") != NULL){
        printk(KERN_INFO "SUSE\n");
    	suse = 1;
    	return 0;
    	}
    else if (strstr(utsname()->release, "fc") != NULL){
        printk(KERN_INFO "Fedora\n");
        fedora = 1;
        return 0;
        }
    else{
        printk(KERN_INFO "OS not supported!\n");
        return -1;
        }

}

void delay(unsigned long int ticks)
{
	udelay(ticks);
}


static int wait_for_ec_resp(uint8_t port, uint8_t mask, uint8_t cond)
{
	uint32_t i = 0;

	while(1)
	{
		/* Maximum of 5msec timeout */
		if(i > 500)
		{
			pr_err("Error: EC GPIO IRQ EC_TIMEOUT!!!\n");
			return -ETIMEDOUT;
		}

		if((inb(port) & mask) == cond)
			return 0;

		delay(1000);
		i++;
	}

	return 0;
}

static irqreturn_t gpio_irq_handler(int irq, void *dev_id) {
    
    /**clearing the interrupt flag***/
    write_ec(0x68, 0x00);
    return IRQ_HANDLED;
}

static void trim_string_inplace(char *str)
{
    char *end;

    // Trim leading space
    while (isspace(*str)) {
        str++;
    }

    // If the string is empty, return
    if (*str == 0) {
        return;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        end--;
    }

    // Null-terminate the string
    *(end + 1) = 0;
}

static int check_board_kernel(void)
{
    char board_name[MAX_BOARD_NAME_LEN];
    struct file *file;
    loff_t pos = 0;
    ssize_t bytes_read;

    // Open the file /sys/class/dmi/id/board_name
    file = filp_open(BOARD_NAME_FILE, O_RDONLY, 0);
    if (IS_ERR(file)) {
        pr_err("Failed to open %s\n", BOARD_NAME_FILE);
        return PTR_ERR(file);
    }

    // Read the board name into the buffer
    bytes_read = kernel_read(file, board_name, MAX_BOARD_NAME_LEN - 1, &pos);
    if (bytes_read < 0) {
        pr_err("Failed to read board name\n");
        filp_close(file, NULL);
        return bytes_read;
    }

    // Null-terminate the string
    board_name[bytes_read] = '\0';
    trim_string_inplace(board_name);

    if(ubuntu){
	    if(LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) )
	    {
	      if (strcmp(board_name, "cExpress-MTL") == 0) {
		  gpio_pin = 644;
		  pr_info("cExpress-MTL, GPIO Pin = %d\n",gpio_pin);
	      } 
	      else if (strcmp(board_name, "Express-TL") == 0) {
		  gpio_pin = 640;
		  pr_info("Express-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if (strcmp(board_name, "cExpress-TL") == 0) {
		  gpio_pin = 673;
		  pr_info("cExpress-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if ((strcmp(board_name, "Express-RLP") == 0) || strstr(board_name, "cExpress-RLP")) {
		  gpio_pin = 685;
		  pr_info("%s, GPIO Pin = %d\n",board_name,gpio_pin);
	      }
	      else {
		  pr_info("Board not supported\n");
		  filp_close(file, NULL);
		  return -1;
	      }
	    }
	    else if((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0)))
	    {
	      // Check if the board name contains "Express-TL"
	      if (strcmp(board_name, "Express-TL") == 0) {
		  gpio_pin = 689;
		  pr_info("Express-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if (strcmp(board_name, "cExpress-TL") == 0) {
		  gpio_pin = 825;
		  pr_info("cExpress-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if ((strcmp(board_name, "Express-RLP") == 0) || strstr(board_name, "cExpress-RLP")) {
		  gpio_pin = 837;
		  pr_info("%s, GPIO Pin = %d\n",board_name,gpio_pin);
	      }
	      else {
		  pr_info("Board not supported\n");
		  filp_close(file, NULL);
		  return -1;
	      }
	    }
	    else{
		pr_info("Linux Kernel version not supported\n");
		filp_close(file, NULL);
		return -1;
	    }
    }
    else if(redhat){
    	if (strcmp(board_name, "cExpress-MTL") == 0) {
		  gpio_pin = 644;
		  pr_info("cExpress-MTL, GPIO Pin = %d\n",gpio_pin);
	      } 
	      else if (strcmp(board_name, "Express-TL") == 0) {
		  gpio_pin = 640;
		  pr_info("Express-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if (strcmp(board_name, "cExpress-TL") == 0) {
		  gpio_pin = 673;
		  pr_info("cExpress-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if ((strcmp(board_name, "Express-RLP") == 0) || strstr(board_name, "cExpress-RLP")) {
		  gpio_pin = 685;
		  pr_info("%s, GPIO Pin = %d\n",board_name,gpio_pin);
	      }
	      else {
		  pr_info("Board not supported\n");
		  filp_close(file, NULL);
		  return -1;
	      }
	}
    else if(debian){
    	if (strcmp(board_name, "Express-TL") == 0) {
		  gpio_pin = 689;
		  pr_info("Express-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if (strcmp(board_name, "cExpress-TL") == 0) {
		  gpio_pin = 825;
		  pr_info("cExpress-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if ((strcmp(board_name, "Express-RLP") == 0) || strstr(board_name, "cExpress-RLP")) {
		  gpio_pin = 837;
		  pr_info("%s, GPIO Pin = %d\n",board_name,gpio_pin);
	      }
	      else {
		  pr_info("Board not supported\n");
		  filp_close(file, NULL);
		  return -1;
	      }
    }
    else if(suse){
    	if (strcmp(board_name, "cExpress-MTL") == 0) {
		  gpio_pin = 644;
		  pr_info("cExpress-MTL, GPIO Pin = %d\n",gpio_pin);
	      } 
	      else if (strcmp(board_name, "Express-TL") == 0) {
		  gpio_pin = 640;
		  pr_info("Express-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if (strcmp(board_name, "cExpress-TL") == 0) {
		  gpio_pin = 673;
		  pr_info("cExpress-TL, GPIO Pin = %d\n",gpio_pin);
	      }
	      else if ((strcmp(board_name, "Express-RLP") == 0) || strstr(board_name, "cExpress-RLP")) {
		  gpio_pin = 685;
		  pr_info("%s, GPIO Pin = %d\n",board_name,gpio_pin);
	      }
	      else {
		  pr_info("Board not supported\n");
		  filp_close(file, NULL);
		  return -1;
	      }
	}
    else if(fedora){
        if (strcmp(board_name, "cExpress-MTL") == 0) {
                  gpio_pin = 644;
                  pr_info("cExpress-MTL, GPIO Pin = %d\n",gpio_pin);
              }
              else if (strcmp(board_name, "Express-TL") == 0) {
                  gpio_pin = 640;
                  pr_info("Express-TL, GPIO Pin = %d\n",gpio_pin);
              }
              else if (strcmp(board_name, "cExpress-TL") == 0) {
                  gpio_pin = 673;
                  pr_info("cExpress-TL, GPIO Pin = %d\n",gpio_pin);
              }
              else if ((strcmp(board_name, "Express-RLP") == 0) || strstr(board_name, "cExpress-RLP")) {
                  gpio_pin = 685;
                  pr_info("%s, GPIO Pin = %d\n",board_name,gpio_pin);
              }
              else {
                  pr_err("Board not supported\n");
                  filp_close(file, NULL);
                  return -1;
              }
        }
    else{
    		pr_info("OS not supported\n");
    		filp_close(file, NULL);
    		return -1;
    		}

    // Close the file
    filp_close(file, NULL);

    return 0;
}


static int __init gpio_init(void) {
    int ret, reg;
    
    ret = os_release();
    if (ret) {
        return ret;
    }
    
    ret = check_board_kernel();
    if (ret) {
        return ret;
    }
  
    // Request the GPIO pin
    ret = gpio_request(gpio_pin, "ec_gpio_irq");
    if (ret) {
        pr_err("Failed to request GPIO %d\n", gpio_pin);
        return ret;
    }

    // Get the IRQ number for the GPIO pin
    irq_number = gpio_to_irq(gpio_pin);
    pr_info("irq number = %d\n", irq_number);
    if (irq_number < 0) {
        pr_err("Failed to get IRQ number for GPIO %d\n", gpio_pin);
        gpio_free(gpio_pin);
        return irq_number;
    }
    
    if (strncasecmp(trigger_type, "edge", strlen("edge")) == 0) {
      new_trigger_type = IRQF_TRIGGER_FALLING;  // Edge trigger
     reg = 0x01;
    } else if (strncasecmp(trigger_type, "low-level", strlen("low-level")) == 0) {
      new_trigger_type = IRQF_TRIGGER_LOW;  // Level trigger
      reg = 0x04;
    }
    else if (strncasecmp(trigger_type, "high-level", strlen("high-level")) == 0) {
      new_trigger_type = IRQF_TRIGGER_LOW;  // Level trigger
      reg = 0x08;
    }
    else if (strncasecmp(trigger_type, "none", strlen("none")) == 0) {
      new_trigger_type = IRQF_TRIGGER_NONE; //No trigger
      reg = 0x00;
    }
    else
    {
      pr_err("Invalid trigger trype. Please enter edge or low-level or high-level or none as the trigger type\n");
      new_trigger_type = IRQF_TRIGGER_NONE;
      reg = 0x00;
      return -1;
    }
    

    // Request IRQ with the current trigger type
    ret = request_irq(irq_number, gpio_irq_handler, new_trigger_type, "ec_gpio_irq", NULL);
    if (ret) {
        pr_err("Failed to request IRQ %d\n", irq_number);
        gpio_free(gpio_pin);
        return ret;
    }
    
    irq_set_irq_type(irq_number, new_trigger_type);
    
     /**clearing the interrupt flag***/
	write_ec(0x68, 0x00);
    
    /**Enabling the EC interrupts**/
     write_ec(0x64, reg);
     reg = read_ec(0x0F);
     reg |= 0x01;
     write_ec(0x0F, reg);

     return 0;
}

static void __exit gpio_exit(void) {
     write_ec(0x64, 0x00);

    // Free IRQ and GPIO resources
    free_irq(irq_number, NULL);
    gpio_free(gpio_pin);
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("ADLINK");
MODULE_DESCRIPTION("ADLINK EC GPIO Interrupt Driver");

