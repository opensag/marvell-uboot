#include <common.h>
#include "../../common/rcmenu.h"
#include <menu.h>
#include <cli.h>

enum rc_entry_num
{   /*≤Àµ•À≥–Ú*/
	ENTRY_LOAD_SYS_FLASH = 1,
	ENTRY_LOAD_MBOOT_FLASH = 2,	
	ENTRY_UPDATE_SYSTEM,	
	ENTRY_UPDATE_MBOOT,
	ENTRY_UPDATE_MBOOT_SER,
	ENTRY_REBOOT,
	ENTRY_FACTORY_BURN,
	ENTRY_SET_SLIC,
	ENTRY_PIE,
	ENTRY_SET_RTC_TIME,
	ENTRY_UPDATE_SBOOT,
	ENTRY_LOAD_SYS_RAM,
	ENTRY_ERASE_ENV,
	ENTRY_UBOOT_SHELL,
	ENTRY_PRINT_CMD,
	ENTRY_SEL_SYS_IDX,
	ENTRY_SET_PASSWORD,
	ENTRY_SET_BAUDRATE,
	ENTRY_UPDATE_DOUBLESYS,
	ENTRY_BOOT_CHECK_VERSION,
	ENTRY_MAX = MENU_ENTRY_END_FLAG,
};
	

MENU_CFG g_menu_cfg[] = {
    //–Ú∫≈                      ∞¥º¸            œ‘ æ                                        √¸¡Ó                √‹¬Î           —°œÓ
    {ENTRY_LOAD_SYS_FLASH,    '0',          "0. Upgrade BOOT.",              "bootup",    NULL,         ENTRY_DISPLAY},
    {ENTRY_UPDATE_SYSTEM,     '1',          "1. Upgrade Main program.",      "sysup",        NULL,         ENTRY_DISPLAY},
    {ENTRY_UPDATE_MBOOT,      '2',          "2. Upgrade Recovry program.",   "sys_recovry",    NULL,         ENTRY_DISPLAY},
	{ENTRY_UPDATE_SBOOT,	  '3',			"3. Upgrade Opt data.",			 "data_update",	NULL,		  ENTRY_DISPLAY},
    {ENTRY_SEL_SYS_IDX,       '4',          "4. Scattered utilities.",       "make_part",        NULL,         ENTRY_DISPLAY},	
    {ENTRY_SET_BAUDRATE,      '5',          "5. SetSN utilities.",           "set_sn", NULL,         ENTRY_DISPLAY},
    {ENTRY_SET_PASSWORD,      '6',          "6. SetMac utilities.",          "set_mac",      NULL,         ENTRY_DISPLAY},
    {ENTRY_BOOT_CHECK_VERSION,'7',          "7. boot wifi check version.",   "boot_checkver",      NULL,         ENTRY_HIDE},
    //{ENTRY_REBOOT,            '0',          " 0: Reboot ",                            "reset",         NULL,         ENTRY_DISPLAY},
    //{ENTRY_LOAD_SYS_RAM,      '@',          " @: Download Program To RAM And Run ",   "sysrun menu",   NULL,         ENTRY_HIDE},
    //{ENTRY_UPDATE_SBOOT,      CTRL_CH('l'), " Ctrl+L: Update Sboot ",                 "sboot menu",   NULL,    ENTRY_HIDE | ENTRY_NEED_PASSWD},
    {ENTRY_UBOOT_SHELL,       CTRL_CH('u'), " Ctrl+U: Entry Uboot Shell ",   "ubootrun",     NULL,    ENTRY_HIDE | ENTRY_NEED_PASSWD},
    //{ENTRY_PRINT_CMD,         CTRL_CH('h'), " Ctrl+H: Print Hide Menu ",              "hprint",       NULL,    ENTRY_HIDE | ENTRY_NEED_PASSWD},
    //{ENTRY_UPDATE_DOUBLESYS,  CTRL_CH('t'), " Ctrl+T: Update Double System",          "sysup menu_all",      NULL,    ENTRY_HIDE | ENTRY_NEED_PASSWD},
    {ENTRY_MAX,               0,            NULL,                                      NULL,           NULL,         ENTRY_HIDE},
};
void boot_menu_mbootloop_default_choise(void)
{
	run_command("run runl_mmc", 0);
}

