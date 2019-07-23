/*
 * Copyright (C) 2016 Stefan Roese <sr@denx.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <i2c.h>
#include <phy.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/soc.h>
#include <power/regulator.h>
#ifdef CONFIG_BOARD_CONFIG_EEPROM
#include <mvebu_cfg_eeprom.h>
#endif
#include <asm-generic/gpio.h>


DECLARE_GLOBAL_DATA_PTR;

/* on Armada3700 rev2 devel-board, IO expander (with I2C address 0x22) bit
 * 14 is used as Serdes Lane 2 muxing, which could be used as SATA PHY or
 * USB3 PHY.
 */
enum COMPHY_LANE2_MUXING {
	COMPHY_LANE2_MUX_USB3,
	COMPHY_LANE2_MUX_SATA
};

/* IO expander I2C device */
#define I2C_IO_EXP_ADDR		0x22
#define I2C_IO_CFG_REG_0	0x6
#define I2C_IO_DATA_OUT_REG_0	0x2
#define I2C_IO_REG_0_SATA_OFF	2
#define I2C_IO_REG_0_USB_H_OFF	1
#define I2C_IO_COMPHY_SATA3_USB_MUX_BIT	14

/*
* For Armada3700 A0 chip, comphy serdes lane 2 could be used as PHY for SATA
* or USB3.
* For Armada3700 rev2 devel-board, pin 14 of IO expander PCA9555 with I2C
* address 0x22 is used as Serdes Lane 2 muxing; the pin needs to be set in
* output mode: high level is for SATA while low level is for USB3;
*/
static int board_comphy_usb3_sata_mux(enum COMPHY_LANE2_MUXING comphy_mux)
{
	int ret;
	u8 buf[8];
	struct udevice *i2c_dev;
	int i2c_byte, i2c_bit_in_byte;

	if (!of_machine_is_compatible("marvell,armada-3720-db-v2") &&
	    !of_machine_is_compatible("marvell,armada-3720-db-v3"))
		return 0;

	ret = i2c_get_chip_for_busnum(0, I2C_IO_EXP_ADDR, 1, &i2c_dev);
	if (ret) {
		printf("Cannot find PCA9555: %d\n", ret);
		return 0;
	}

	ret = dm_i2c_read(i2c_dev, I2C_IO_CFG_REG_0, buf, 2);
	if (ret) {
		printf("Failed to read IO expander value via I2C\n");
		return ret;
	}

	i2c_byte = I2C_IO_COMPHY_SATA3_USB_MUX_BIT / 8;
	i2c_bit_in_byte = I2C_IO_COMPHY_SATA3_USB_MUX_BIT % 8;

	/* Configure IO exander bit 14 of address 0x22 in output mode */
	buf[i2c_byte] &= ~(1 << i2c_bit_in_byte);
	ret = dm_i2c_write(i2c_dev, I2C_IO_CFG_REG_0, buf, 2);
	if (ret) {
		printf("Failed to set IO expander via I2C\n");
		return ret;
	}

	ret = dm_i2c_read(i2c_dev, I2C_IO_DATA_OUT_REG_0, buf, 2);
	if (ret) {
		printf("Failed to read IO expander value via I2C\n");
		return ret;
	}

	/* Configure output level for IO exander bit 14 of address 0x22 */
	if (comphy_mux == COMPHY_LANE2_MUX_SATA)
		buf[i2c_byte] |= (1 << i2c_bit_in_byte);
	else
		buf[i2c_byte] &= ~(1 << i2c_bit_in_byte);

	ret = dm_i2c_write(i2c_dev, I2C_IO_DATA_OUT_REG_0, buf, 2);
	if (ret) {
		printf("Failed to set IO expander via I2C\n");
		return ret;
	}

	return 0;
}

int board_early_init_f(void)
{
#ifdef CONFIG_BOARD_CONFIG_EEPROM
	cfg_eeprom_init();
#endif

#ifdef CONFIG_MVEBU_SYS_INFO
	/*
	 * Call this function to transfer data from address 0x4000000
	 * into a global struct, before code relocation.
	 */
	sys_info_init();
#endif
	return 0;
}

int board_usb3_vbus_init(void)
{
#if defined(CONFIG_DM_REGULATOR)
	struct udevice *regulator;
	int ret;

	/* lower usb vbus  */
	ret = regulator_get_by_platname("usb3-vbus", &regulator);
	if (ret) {
		debug("Cannot get usb3-vbus regulator\n");
		return 0;
	}

	ret = regulator_set_enable(regulator, false);
	if (ret) {
		error("Failed to turn OFF the VBUS regulator\n");
		return ret;
	}
#endif
	return 0;
}

int board_init(void)
{
	board_usb3_vbus_init();

	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
#ifdef CONFIG_OF_CONTROL
	printf("U-Boot DT blob at : %p\n", gd->fdt_blob);
#endif

	/* enable serdes lane 2 mux for sata phy */
	board_comphy_usb3_sata_mux(COMPHY_LANE2_MUX_SATA);

	return 0;
}

/* Board specific AHCI / SATA enable code */
int board_ahci_enable(struct udevice *dev)
{
#if defined(CONFIG_DM_REGULATOR)
	int ret;
	struct udevice *regulator;

	ret = device_get_supply_regulator(dev, "power-supply",
					  &regulator);
	if (ret) {
		debug("%s: No sata power supply\n", dev->name);
		return 0;
	}

	ret = regulator_set_enable(regulator, true);
	if (ret) {
		error("Error enabling sata power supply\n");
		return ret;
	}
#endif
	return 0;
}

bool check_if_need__recovery(void)
{
	#define  RESET_BUTTON_NUM 3
	#define  RESET_BUTTON_PRESS 0
	#define  RESET_BUTTON_CHECK_TIME 2
	int i = 0;
	
	int ret;

	int gpio = RESET_BUTTON_NUM;
	ret = gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return false;
	}
	
	gpio_direction_input(gpio);
	printf("begain test revover\n");
	
	for(i = 0; i < RESET_BUTTON_CHECK_TIME*5; i++)
	{
		int value[3];
		value[0] = gpio_get_value(gpio);
		mdelay(100);
		value[1] = gpio_get_value(gpio);
		mdelay(100);
		if( (RESET_BUTTON_PRESS != value[0]  ) && (RESET_BUTTON_PRESS != value[1]  ) )
		{
			gpio_free(gpio);
			return false;
		}
	}
	gpio_free(gpio);
	
	return true;
}
void usb_power_on(void)
{
	int ret;
	int gpio = 36;//gpio2_0

	
	ret = gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return ;
	}
	ret = gpio_direction_output(gpio, 0);
	gpio_free(gpio);
	return;
}
/*
gpio clear 17  ---打开WIFI电源
gpio set 17    ---关闭WIFI电源
gpio clear 39 ---复位WIFI芯片
gpio set 39   ---解复位WIFI芯片

*/
void wifi_power_on(void)
{
	int ret;
	int gpio_power = 17;//gpio1_17
	int gpio_reset = 39;//gpio2_3
	int gpio_power2=62;//gpio_2_26

	
	ret = gpio_request(gpio_power, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio_power);
		return ;
	}
	ret = gpio_request(gpio_reset, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		gpio_free(gpio_power);
		printf("gpio: requesting pin %u failed\n", gpio_reset);
		return ;
	}

	ret = gpio_request(gpio_power2, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		gpio_free(gpio_power2);
		printf("gpio: requesting pin %u failed\n", gpio_power2);
		return ;
	}


	ret = gpio_direction_output(gpio_power, 0);
	ret = gpio_direction_output(gpio_power2, 1);
	ret = gpio_direction_output(gpio_reset, 1);

	
	gpio_free(gpio_reset);
	gpio_free(gpio_power);
	gpio_free(gpio_power2);
	return;
}
/*
gpio2_20--->拉低；延时2s;  #电源
gpio2_5---->拉低1秒，在拉高；1秒  #reset
gpio2_2--->拉低；0.5s再拉高；15s  #开机
*/
void module_4G_powe_on(void)
{
	int ret;
	int gpio_power = 56;//gpio2_20
	int gpio_reset = 41;//gpio2_5
	int gpio_switch_on = 38;//gpio2_2

	
	ret = gpio_request(gpio_power, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio_power);
		return ;
	}
	ret = gpio_request(gpio_reset, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		gpio_free(gpio_power);
		printf("gpio: requesting pin %u failed\n", gpio_reset);
		return ;
	}
	ret = gpio_request(gpio_switch_on, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		gpio_free(gpio_power);
		gpio_free(gpio_reset);
		printf("gpio: requesting pin %u failed\n", gpio_switch_on);
		return ;
	}
	


	ret = gpio_direction_output(gpio_power, 0);
	mdelay(1000);
	ret = gpio_direction_output(gpio_reset, 0);
	mdelay(1000);
	ret = gpio_direction_output(gpio_reset, 1);
	mdelay(1000);
	ret = gpio_direction_output(gpio_switch_on, 0);
	mdelay(500);
	ret = gpio_direction_output(gpio_switch_on, 1);	

	
	gpio_free(gpio_reset);
	gpio_free(gpio_power);
	gpio_free(gpio_switch_on);
	
	return ;	
}
void switch_6176_reset(void)
{
	int ret;
	int gpio = 11;//gpio1_11

	
	ret = gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return ;
	}
	ret = gpio_direction_output(gpio, 1);
	mdelay(10);
	ret = gpio_direction_output(gpio, 0);
	gpio_free(gpio);
	//mdelay(500);
	return;
	
}

void cloudled_power_off(void)
{
	int ret;
	int gpio = 4;//gpio1_4

	
	ret = gpio_request(gpio, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n", gpio);
		return ;
	}
	ret = gpio_direction_output(gpio, 0);
	gpio_free(gpio);
	return;
}



extern int reset_sys_recovry(void);
int  board_late_init(void)
{
	bool need_recover;
	
 	need_recover = check_if_need__recovery();  
	printf("need_recover =%x\n", need_recover);
	if(need_recover)
	{
		printf("neeed reverver system\n");
		reset_sys_recovry();
	}
	else
	{
		mdelay(500);
	}

	cloudled_power_off();
	switch_6176_reset();
	wifi_power_on();
	usb_power_on();
	module_4G_powe_on();
	
	return 0;
}

void reset_phy(void)
{

	char cmd[2048];

	sprintf(cmd, "setenv disk_uuid 00042021-0408-4601-9dcc-a8c51255994f");
	run_command(cmd, 0);

	sprintf(cmd, "setenv reserve_uuid  8ef917d1-2c6f-4bd0-a5b2-331a19f91cb2");
	run_command(cmd, 0);

	sprintf(cmd, "setenv sys1_uuid  77877125-add0-4374-9e60-02cb591c9737");
	run_command(cmd, 0);


	sprintf(cmd, "setenv sys2_uuid  b4b84b8a-04e3-48ae-8536-aff5c9c495b1");
	run_command(cmd, 0);

	sprintf(cmd, "setenv partitions 'uuid_disk=${disk_uuid};"
		"name=sys1,start=1MiB,size=1000MiB,uuid=${sys1_uuid};"
		"name=reserve,start=1001MiB,size=2000MiB,uuid=${reserve_uuid};"
		"name=sys2,start=3001MiB,size=500MiB,uuid=${sys2_uuid};'  ");
	run_command(cmd, 0);


	sprintf(cmd, "setenv bootargs 'root=/dev/mmcblk1p1 rw rootwait  "
				"rootfstype=ext4  console=ttyMV0,%d earlycon=ar3700_uart,0xd0012000'", 
				CONFIG_BAUDRATE);
	run_command(cmd, 0);


	sprintf(cmd, "setenv ups 'tftp 0x5000000 rcsysups.ext4;mmc dev 1;"
					"mmc write 0x5000000 0x800 0x24050'");
	run_command(cmd, 0);



	sprintf(cmd, "setenv runl_mmc  'ext4load mmc 1:1 0x7000000 /boot/Image;"
				"ext4load mmc 1:1 0x6f00000 /boot/armada-3720-rc01.dtb;"
				"booti 0x7000000 - 0x6f00000'");
	run_command(cmd, 0);

	//sprintf(cmd, "setenv bootcmd 'run runl_mmc'");
	sprintf(cmd, "setenv bootcmd 'rcmenu'");
	run_command(cmd, 0);


	sprintf(cmd, "setenv make_part 'gpt write mmc 1 ${partitions}'");
	run_command(cmd, 0);
	
	//sprintf(cmd, "setenv serverip 192.168.1.7");
	//run_command(cmd, 0);

	//sprintf(cmd, "setenv ipaddr 192.168.1.6");
	//run_command(cmd, 0);
	
	return;
}

