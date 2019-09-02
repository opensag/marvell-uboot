#include <common.h>
#include "image.h"
#include <cli.h>
#include <menu.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include "../rcboot/common/rcmenu.h"
  

/*
sbootup
mbootup
sysup
sysboot
ubootcmd
menuhelp
*/
#define STR_TMP 512
#define SYS_LEN  0x40000
#define DATA_LEN  0x40000
#define WIFI_CHECK_LEN  0x40000
#define LOAD_ADDR  0x5000000
#define DELAY_COUNT_M 5
#define ENTER_MENU_KEY 'Z'
#define ERROR_EXIT(a) {int b = a; if(b) return b;}

typedef struct mmc_part{

	 unsigned long long start_lpa;
	 unsigned long long end_lpa;
	char part_name[PARTNAME_SZ+1];
} part_entry;

extern part_entry efi_part[];
extern int part_efi(struct blk_desc *dev_desc);

static int get_part_info(void)
{
	int dev = 0;
	char *ep;
	int ret = 0;
	struct blk_desc *blk_dev_desc = NULL;

	dev = (int)simple_strtoul("1", &ep, 10);
	if (!ep || ep[0] != '\0') {
		return CMD_RET_USAGE;
	}
	
	blk_dev_desc = blk_get_dev("mmc", dev);
	if (!blk_dev_desc) {
		return CMD_RET_FAILURE;
	}

	ret = part_efi(blk_dev_desc);

	return ret;

}


__weak int mboot_run_command(const char *cmd, int flag)
{
	return run_command(cmd, flag);
}

static int printf_result(int ret)
{
	
	if(0 == ret)
	{
		printf("\nsucess\r\n");
	}
	else
	{
		printf("\nfailed\r\n");
	}
	return 0;
	
}

/*****************************************************************************
 * 函 数 名  : tftp_get_file
 * 负 责 人  : 
 * 创建日期  : 2017年10月11日
 * 函数功能  : 通过tftp获取某个文件
 * 输入参数  : char *pcname  x
               uint addr     x
               uint *pLen    x
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static int tftp_get_file(char *pcname, uint addr, uint *pLen)
{
    int ret = 0;
    char cmd[STR_TMP] = {0};
	extern u32 net_boot_file_size;
	
    sprintf(cmd, "tftpboot 0x%x %s", addr, pcname);
	
	printf("Now, begin download program through Tftp \r\n");
	
    ret = mboot_run_command(cmd, CMD_FLAG_BOOTD);
    if(0 != ret)
    {
        printf("ERROR: tftp image %s fail.\n", pcname);
		return CMD_RET_FAILURE;
    }
    
    if(NULL != pLen)
    {
        *pLen = (uint)net_boot_file_size;
    }

    return CMD_RET_SUCCESS;
}

static int mboot_func_CheckIp( char *str_addr )
{
	int count;
	unsigned long temp;
	char* colon;
	char* pBuf, addrBuf[20];
	char *endptr = NULL;

	if(strlen(str_addr) > 20)
		return -1;

	strcpy(addrBuf, str_addr);
	pBuf = addrBuf;
	for(count = 0; count < 3; count++)
	{
		colon = strchr (pBuf, '.');
		if (colon == NULL)
			return -1;
		*colon = '\0';
		temp = simple_strtoul(pBuf, &endptr, 10);
		if (*endptr != '\0' || temp > 256)
			return -1;
		pBuf = colon + 1;
	}
	temp = simple_strtoul(pBuf, &endptr, 10);
	if (*endptr != '\0' || temp > 256)
		return -1;
		
	return 0;
}

static int mboot_bootline_para_set(int force)
{
	char inputBuf[128] = {0};
	int len = 0;
	int i = 0;
	int flag = 1;
	int press_key = 0;
	char env[128] = {0};
	char *namelst[] = {"Plz enter the Local IP:", "Plz enter the Remote IP:", "Plz enter the Filename:"};
	char *paralst[] = {"ipaddr", "serverip", "filename"};
	char cmdBuf[128] = {0};

	for (i = 0; i < 3; i++)
	{
		inputBuf[0] = 0;
		sprintf(env, "%-10s[%s]:", namelst[i], getenv(paralst[i]));
		len = cli_readline_into_buffer(env, inputBuf, 0);

		if (len > 0)
		{
			if((0 == i) || (1 == i)){
				if(mboot_func_CheckIp(inputBuf)) 
				{
					printf("ERROR:invalid ip!\n");                
					return -1;
				}
			}else{
				if(strlen(inputBuf) > STR_TMP)
				{
					printf("ERROR:input too large!\n");                
					return -1;
				}				
			}
			sprintf(cmdBuf, "setenv %s %s", paralst[i], inputBuf);
			mboot_run_command(cmdBuf, 0);
		}
		else if(len < 0){
			return -1;
		}
	}

	if (1 == flag)
		saveenv();

	if (!force)
	{
		printf("\r\nPress y to confirm execution: ");
		press_key = getc();
		printf("%c\r\n", press_key);

		if ('y' != press_key && 'Y' != press_key)
			return -1;
	}
	return 0;
}

static int get_file_name(int argc, char *const argv[], char* filename)
{
    char *pName = NULL;
    
    if(1 > argc)
    {
        return CMD_RET_USAGE;        
    }

    /*boot 需要重新配置下网络参数*/
    if (mboot_bootline_para_set(1))
    {
        return CMD_RET_FAILURE;
    }
    pName = getenv("filename");

    if(NULL == pName )
    {
        return CMD_RET_FAILURE;
    }
    strncpy(filename, pName, STR_TMP);
    return CMD_RET_SUCCESS;
}

int reset_sys_recovry(void)
{
	uint file_add = LOAD_ADDR;
	int ret = 0;
	char cmdBuf[128] = {0};

	ret = get_part_info();
	if (ret != 0){
		printf("Partition Table is wrong, Please execute 3. Scattered utilities.\r\n");
		return -1;
	}	
	ret = run_command("mmc dev 1", 0);

	printf("\r\nstarting read system2");
	memset(cmdBuf, 0 , 128);
	sprintf(cmdBuf, "mmc read 0x%x 0x%llx 0x%x", file_add, efi_part[2].start_lpa, SYS_LEN);
	ret += run_command(cmdBuf, 0);
	
	printf("\r\nstarting write system1");
	memset(cmdBuf, 0 , 128);
	sprintf(cmdBuf, "mmc write 0x%x 0x%llx 0x%x", file_add, efi_part[0].start_lpa, SYS_LEN);
	ret += run_command(cmdBuf, 0);

	ret += run_command("reset", 0);
	
	printf_result(ret);

	return ret;
	
}

static int do_bootup(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char file_name[STR_TMP] = {0};
	int ret = 0;
	char cmdBuf[128] = {0};
   
	ERROR_EXIT( get_file_name(argc, argv, file_name) );
	
	sprintf(cmdBuf, "bubt %s", file_name);
	ret += run_command(cmdBuf, 0);
	
	printf_result(ret);
	
	return ret;
}

U_BOOT_CMD(
	bootup,	5,	1,	do_bootup,
	"boot update\n",
	"boot update\n"
);

static int do_sysup(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret = 0;
	
	ret = run_command("setenv ipaddr 192.168.0.1; setenv serverip 192.168.0.100; setenv image_name openwrt-mvebu-cortexa53-sag-100wm-initramfs-kernel.bin; setenv fdt_name armada-3720-sag-100wm.dtb; tftpboot $kernel_addr $image_name; tftpboot $fdt_addr $fdt_name;setenv bootargs $console; booti $kernel_addr - $fdt_addr;", 0);

	printf_result(ret);
	
	return ret;
}

U_BOOT_CMD(
	sysup,	5,	1,	do_sysup,
	"system update \n",
	"system update \n"
);


static int do_sys_recovry(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	char file_name[STR_TMP] = {0};
	uint file_len = 0;
	uint file_add = LOAD_ADDR;
	char cmdBuf[128] = {0};
	int ret = 0;

	ret = get_part_info();
	if (ret != 0){
		printf("Partition Table is wrong, Please execute 3. Scattered utilities.\r\n");
		return -1;
	}

	ERROR_EXIT( get_file_name(argc, argv, file_name) );
	ERROR_EXIT( tftp_get_file(file_name, file_add, &file_len) );

	ret = run_command("mmc dev 1", 0);

	memset(cmdBuf, 0, 128);//update part 3
	sprintf(cmdBuf, "mmc write 0x%x 0x%llx 0x%x", file_add, efi_part[2].start_lpa, SYS_LEN);
	ret += run_command(cmdBuf, 0);

	printf_result(ret);
	return ret;
}

U_BOOT_CMD(
	sys_recovry, 5,	1,	do_sys_recovry,
	"update backup system ",
	"update backup system \n"
	"update backup system \n"
);


static int do_data_update(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	char file_name[STR_TMP] = {0};
	uint file_len = 0;
	uint file_add = LOAD_ADDR;
	char cmdBuf[128] = {0};
	int ret = 0;

	ret = get_part_info();
	if (ret != 0){
		printf("Partition Table is wrong !!!\r\n");
		return -1;
	}

	ERROR_EXIT( get_file_name(argc, argv, file_name) );
	ERROR_EXIT( tftp_get_file(file_name, file_add, &file_len) );

	ret = run_command("mmc dev 1", 0);

	memset(cmdBuf, 0, 128); //update part 2
	sprintf(cmdBuf, "mmc write 0x%x 0x%llx 0x%x", file_add, efi_part[1].start_lpa, DATA_LEN);
	ret += run_command(cmdBuf, 0);

	printf_result(ret);
	return ret;
}

U_BOOT_CMD(
	data_update,	5,	1,	do_data_update,
	"update reserve ",
	"update reserve \n"
	"update reserve \n"
);

static int do_sysdef(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	#if 0
	uint cur_Default_Sys = get_commit_or_active_system_num(RC_SYS_ENV_ADDR_COMMITE);
	uint next_default_sys = (1 == cur_Default_Sys)?2:1;
	char temp_str[STR_TMP];
	char input_buff[256] = {0};
	int ret = 0;
	printf("current default system is %d\n", cur_Default_Sys);
	sprintf(temp_str, "change default system to %d, please inpute [n/y/yes]", 
															next_default_sys);
	ret = cli_readline_into_buffer(temp_str, input_buff, 0);
	if((ret > 0) )
	{
		if( (0 == strcmp("y", input_buff) )
			|| (0 == strcmp("yes", input_buff) ) 
		)
		{
			if(0 == set_commit_or_active_sys_num(next_default_sys, RC_SYS_ENV_ADDR_COMMITE))	
			{
				printf("change success\n");
				printf("now default boot system is %d\n", 
					  get_commit_or_active_system_num(RC_SYS_ENV_ADDR_COMMITE));
			}
			else 
			{
				printf("change failed\n");
				printf("default boot system is %d\n", 
					get_commit_or_active_system_num(RC_SYS_ENV_ADDR_COMMITE));
			}
		}
	}
	#endif
	return 0;
	
}


U_BOOT_CMD(
	sysdef,	5,	1,	do_sysdef,
	"sysdef -- 1/2/all boot system 1/2/all",
	"sysdef -- 1 boot system 1 \n"
	"sysdef -- 2 boot system 2 \n"
	"sysdef -- all boot system 1 or 2 \n"
);



static int do_set_sn(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char inputBuf[1024] = {0};
	int ret = 0;
	unsigned int length = 0;
	char cmdBuf[128] = {0};

	memset(inputBuf, 0, sizeof(inputBuf));
	
	printf("Please input sn: ");
	
	length = cli_readline_into_buffer(NULL, inputBuf, 0);

	printf("\r\n");
	
	inputBuf[++length] = '\0';

	sprintf(cmdBuf, "ext4write mmc 1:2 0x%p /etc/box.sn 0x%x 0", inputBuf, length);

	ret += run_command(cmdBuf, 0);
	
	printf_result(ret);
	
	return ret;
	
}

U_BOOT_CMD(
	set_sn,	5,	1,	do_set_sn,
	"set sn ",
	"modify sn value \n"
	"modify sn value \n"
);

static int do_set_mac(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char macBuf[STR_TMP] = {0};
	int len = 0;
	int ret = 0;
		
	printf("\r\nPlease input mac address:[XX:XX:XX:XX:XX:XX]: ");
	
	len = cli_readline_into_buffer(NULL, macBuf, 0);
	if (len > 0)
	{	
		ret = setenv("ethaddr", macBuf);
		if (0 == ret)
		{
			saveenv();
		}
	}

	return 0;
}


U_BOOT_CMD(
	set_mac, 5,	1,	do_set_mac,
	"set mac_addr",
	"modify mac_addr value \n"
);


static int make_part_loop(int delay)
{
	int ret = 0;
	int i = 0;
	int press_key = 0;

	while (delay > 0)
	{
		for (i = 0; i < 100; ++i)
		{
			if (!tstc())
			{
				mdelay(10);
				continue;
			}

			press_key = getc();
			if (CTRL_CH(tolower(ENTER_MENU_KEY)) == press_key)
			{
				return 0;
			}
			printf("%c\r\n", press_key);

			if ('y' == press_key ||'Y' == press_key){
				
				ret += run_command("mmc dev 1", 0);
				ret += run_command("mmc erase 0 0x800", 0);
				ret += run_command("mmc erase 0x800 0x10", 0);
				ret += run_command("mmc erase 0x001f4800 0x10", 0);
				ret += run_command("mmc erase 0x006d6800 0x10", 0);
				ret += run_command("run make_part", 0); 

				printf_result(ret);
				return 0;
			
			}else if('n' == press_key ||'N' == press_key){
				
				return 0;
			}

		}

		if (delay < 0)
			break;

		delay--;

	}

	printf("\r\n");

	return ret;
}

static int do_make_part(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{	

	printf("Whether to format partitions:[y/n]: ");

	make_part_loop(DELAY_COUNT_M);

	return 0;

}

U_BOOT_CMD(
	make_part,	5,	1,	do_make_part,
	"make mmc partitions ",
	"Enter yes, divide partitions \n"
	"enter no or others, will do nothing \n"
);


static int do_check_up(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{	

	char file_name[STR_TMP] = {0};
	uint file_len = 0;
	uint file_add = LOAD_ADDR;
	char cmdBuf[128] = {0};
	int ret = 0;
	
	printf("\r\n*** update dtb !!! *** \r\n");
	
	ERROR_EXIT( get_file_name(argc, argv, file_name) );
	ERROR_EXIT( tftp_get_file(file_name, file_add, &file_len) );

	ret += run_command("mmc dev 1", 0);
	memset(cmdBuf, 0, 128);        //update wifi check version dtb
	sprintf(cmdBuf, "mmc write 0x%x 0x%x 0x%x", file_add, 0x700000, 0x800);
	ret += run_command(cmdBuf, 0);

	
	printf("\r\n*** update system !!! *** \r\n");
	
	ERROR_EXIT( get_file_name(argc, argv, file_name) );
	ERROR_EXIT( tftp_get_file(file_name, file_add, &file_len) );

	ret = run_command("mmc dev 1", 0);
	memset(cmdBuf, 0, 128);        //update wifi check version
	sprintf(cmdBuf, "mmc write 0x%x 0x%x 0x%x", file_add, 0x700000+0x800, WIFI_CHECK_LEN);
	ret += run_command(cmdBuf, 0);

	printf_result(ret);
	return ret;

}

U_BOOT_CMD(
	check_up,	5,	1,	do_check_up,
	"update check version ",
	"update check version \n"
	"update check version \n"
);


static int do_boot_checkver(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{	
	uint file_add = 0;
	char cmdBuf[128] = {0};
	int ret = 0;

	file_add = 0x6f00000;
	ret += run_command("mmc dev 1", 0);
	memset(cmdBuf, 0, 128);        //update wifi check version
	sprintf(cmdBuf, "mmc read 0x%x 0x%x 0x%x", file_add, 0x700000, 0x800);
	ret += run_command(cmdBuf, 0);

	file_add = 0x7000000;
	ret += run_command("mmc dev 1", 0);
	memset(cmdBuf, 0, 128);        //update wifi check version
	sprintf(cmdBuf, "mmc read 0x%x 0x%x 0x%x", file_add, 0x700000+0x800, WIFI_CHECK_LEN);
	ret += run_command(cmdBuf, 0);
	
	setenv("runlinux", "setenv bootargs 'console=ttyMV0,115200n8 earlycon=ar3700_uart,0xd0012000'; booti 0x7000000 - 0x6f00000"); 
	ret += run_command("run runlinux", 0);
	
	printf_result(ret);
	return ret;

}

U_BOOT_CMD(
	boot_checkver,	5,	1,	do_boot_checkver,
	"update check version ",
	"update check version \n"
	"update check version \n"
);

static int do_eraseenv(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{	
	int ret = 0;

	ret = run_command("mmc dev 1 1;mmc erase 0x1f80 0x80", 0);
	
	printf_result(ret);
	
	return ret;

}

U_BOOT_CMD(
	eraseenv,	5,	1,	do_eraseenv,
	"erase env ",
	"erase env \n"
	"erase env \n"
);



