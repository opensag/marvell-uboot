/***********************************************************************************
 * 文 件 名   : mboot_menu.c
 * 负 责 人   : wjx
 * 创建日期   : 2017年9月13日
 * 文件描述   : rcboot的菜单模块
 * 版权说明   : Copyright (c) 2017-2017   瑞斯康达科技发展股份有限公司
 * 其    他   : 
 * 修改日志   : 
***********************************************************************************/


#include <common.h>
#include <command.h>
#include <menu.h>
#include <watchdog.h>
#include <malloc.h>
#include <linux/string.h>
#include <version.h>
#include "rcmenu.h"
#include <linux/ctype.h>

#define CONFIG_RCBOOTDELAY 3

#ifndef DEFAULT_MENU_ENTRY_NUM
#define DEFAULT_MENU_ENTRY_NUM  1
#endif

#ifndef ENTER_MENU_KEY
#define ENTER_MENU_KEY 'Z' //定制进入菜单的命令 默认ctrl+B
#endif

#ifndef ENTER_MENU_NAME
#define ENTER_MENU_NAME "BOOT" 
#endif


//#ifndef MBOOT_SUPER_PASSWD
//#define MBOOT_SUPER_PASSWD "Bootrom!"
//#endif

#ifndef MBOOT_ENABLE_MENU_PASSWORD
#define MBOOT_ENABLE_MENU_PASSWORD 0 //1: 使能密码 0:不检查密码
#endif

#define ANSI_CURSOR_UP			"\e[%dA"
#define ANSI_CURSOR_DOWN		"\e[%dB"
#define ANSI_CURSOR_FORWARD		"\e[%dC"
#define ANSI_CURSOR_BACK		"\e[%dD"
#define ANSI_CURSOR_NEXTLINE		"\e[%dE"
#define ANSI_CURSOR_PREVIOUSLINE	"\e[%dF"
#define ANSI_CURSOR_COLUMN		"\e[%dG"
#define ANSI_CURSOR_POSITION		"\e[%d;%dH"
#define ANSI_CURSOR_SHOW		"\e[?25h"
#define ANSI_CURSOR_HIDE		"\e[?25l"
#define ANSI_CLEAR_CONSOLE		"\e[2J"
#define ANSI_CLEAR_LINE_TO_END		"\e[0K"
#define ANSI_CLEAR_LINE			"\e[2K"
#define ANSI_COLOR_RESET		"\e[0m"
#define ANSI_COLOR_REVERSE		"\e[7m"


typedef struct tag_rc_menu_data
{
	int delay;			/* delay for autoboot */
	int active;			/* active menu entry */
	int count;			/* total count of menu entries */
	int display_count;			/* total count of menu entries that can be display */
	char passwd[32];
	struct rc_menu_data_entry *first;	/* first menu entry */
}rc_menu_data_t;

struct rc_menu_data_entry
{
	unsigned short int num;		/* unique number 0 .. MENU_ENTRY_END_FLAG */
	char key[4];			/* string key identifier of number */
	char *title;			/* title of entry */
	char *command;			/* hush command of entry */
	char *password;
	int index;
	int type;
	rc_menu_data_t *menu;	/* this rc_menu_data_t */
	struct rc_menu_data_entry *next;	/* next menu entry (num+1) */
};

enum rc_menu_key
{
	KEY_NONE = 0,
	KEY_UP,
	KEY_DOWN,
	KEY_SELECT,
};
#define MAX_PASSWD_LEN 128

#define MBOOT_MENU_VERSION	"1.1"

/*交换机产品线标准菜单*/
#ifndef MBOOT_MENU_START_HEAD_STRING
#define MBOOT_MENU_START_HEAD_STRING	"        BOOT MENU V"MBOOT_MENU_VERSION        "\n"
#endif 

#define MBOOT_MENU_LINE_STATLINE (5) 
__weak const MENU_CFG *mboot_get_menu_CFG(void)
{
    const MENU_CFG *pRet = g_menu_cfg;
    return pRet;
}

int mboot_menu_printf_flag_get(void);
void mboot_menu_printf_flag_set(unsigned flag);
int mboot_menu_display_entry_num_get(void);


/*
菜单标记
1:输出全新菜单
0:先把老菜单清楚掉，再输出菜单
*/
static int g_rc_menu_printf_flag = 1;

/*****************************************************************************
 * 函 数 名  : mboot_menu_printf_flag_set
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 设置菜单打印标记
 * 输入参数  : unsigned flag  x
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
void mboot_menu_printf_flag_set(unsigned flag)
{
	g_rc_menu_printf_flag = flag;
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_printf_flag_get
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 获取菜单打印标记
 * 输入参数  : void  x
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
int mboot_menu_printf_flag_get(void)
{
    return g_rc_menu_printf_flag;
}


void menu_display_statusline(struct menu *m)
{
    struct rc_menu_data_entry *entry;

    if (menu_default_choice(m, (void *)&entry) < 0)
        return;

    if(0 == mboot_menu_printf_flag_get()){
        /*不是第一次显示统计菜单，需把历史菜单清楚掉，*/
        printf(ANSI_CURSOR_UP, MBOOT_MENU_LINE_STATLINE + mboot_menu_display_entry_num_get());
        printf(ANSI_CLEAR_LINE_TO_END);
    }else{	    
        mboot_menu_printf_flag_set(0);
    }
    
    printf("\n");
    printf(MBOOT_MENU_START_HEAD_STRING);
    printf("\n");
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_hide_entry_num_get
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 获取隐含菜单数目
 * 输入参数  : void  x
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
int mboot_menu_hide_entry_num_get(void)
{
    int i = 0;
    int hide_num = 0;
    const MENU_CFG *pMenuCfg = mboot_get_menu_CFG();
    for(i = 0; i < MENU_ENTRY_END_FLAG; i++)
    {
        if(pMenuCfg[i].num == MENU_ENTRY_END_FLAG)
        {
            break;
        }
        if(pMenuCfg[i].type & (ENTRY_HIDE))
        {
            hide_num++;
        }
    }
    return  hide_num; 
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_display_entry_num_get
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 获取需显示菜单的数目
 * 输入参数  : void  x
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
int mboot_menu_display_entry_num_get(void)
{
    int i = 0;
    int num = 0;
    const MENU_CFG *pMenuCfg = mboot_get_menu_CFG();
    for(i = 0; i < MENU_ENTRY_END_FLAG; i++)
    {
        if(pMenuCfg[i].num == MENU_ENTRY_END_FLAG)
        {
            break;
        }
        
        if(pMenuCfg[i].type & (ENTRY_HIDE))
        {
            num = num;
        }
        else
        {
            num++;
        }
    }
    return num; 
}

/*****************************************************************************
 * 函 数 名  : mboot_menu_passwd_get
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 获取用户输入的密码
 * 输入参数  : char *pPassBuff  x
               int Buflen       x
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
void mboot_menu_passwd_get(char *pPassBuff, int Buflen)
{
	unsigned char c;
	int i = 0;

	while ((c = getc()) != '\r')
	{
		if ((i < Buflen) && (c != '\b'))
		{
			pPassBuff[i++] = c;
			putc('*');
		}
		else if ((i > 0) && (c == '\b'))
		{
			--i;
			putc('\b');
			putc(' ');
			putc('\b');
		}
	}
	putc('\n');
	pPassBuff[i] = '\0';
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_rc_menu_getoption
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 获取rcmenu环境变量选项
 * 输入参数  : unsigned short int n  x
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static char *mboot_menu_rc_menu_getoption(unsigned short int n)
{
	char name[256];

	if (n > MENU_ENTRY_END_FLAG)
		return NULL;

	sprintf(name, "rc_menu_%d", n);
	return getenv(name);
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_get_entry_by_num
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 根据菜单数目获取rcmenu数据
 * 输入参数  : rc_menu_data_t *rc_menu  x
               int entry_num          x
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static struct rc_menu_data_entry *mboot_menu_get_entry_by_num(rc_menu_data_t *rc_menu, int entry_num)
{
	struct rc_menu_data_entry *iter;

	for (iter = rc_menu->first; iter; iter = iter->next)
	{
		if (iter->num == entry_num)
		{
			return iter;
		}
			
	}

	return NULL;
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_get_entry_by_index
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 根据序号获取rcmenu数据
 * 输入参数  : rc_menu_data_t *rc_menu  x
               int index              x
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static struct rc_menu_data_entry *mboot_menu_get_entry_by_index(rc_menu_data_t *rc_menu, int index)
{
	struct rc_menu_data_entry *iter;

	for (iter = rc_menu->first; iter; iter = iter->next)
	{
		if (iter->index == index)
			return iter;
	}

	return NULL;
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_print_entry
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 菜单输出函数
 * 输入参数  : void *data  x
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static void mboot_menu_print_entry(void *data)
{
	struct rc_menu_data_entry *entry = data;
	int reverse = (entry->menu->active == entry->index);

	if (entry->type & ENTRY_HIDE)
		return ;


	puts("  ");

	if (reverse)
		puts(ANSI_COLOR_REVERSE);

	puts(entry->title);
	puts("\n");

	if (reverse)
		puts(ANSI_COLOR_RESET);
}


/*****************************************************************************
 * 函 数 名  : mboot_menu_boot_loop
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 等待用户输入进菜单命令函数
 * 输入参数  : int delay  x
 * 输出参数  : 无
 * 返 回 值  : int <0  按下了ctrl+d ,0 没有按下ctrl+d 按键 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static int mboot_menu_boot_loop(int delay)
{
	int delay_ret = 0;
	int i, c;

	if (delay > 0)
	{
		printf("\r\nPress Ctrl+%c to enter %s menu: %2d ", 
										ENTER_MENU_KEY, ENTER_MENU_NAME, delay);
	}

	while (delay > 0)
	{
		for (i = 0; i < 100; ++i)
		{
			if (!tstc())
			{
				WATCHDOG_RESET();
				mdelay(10);
				continue;
			}

			c = getc();
			if (CTRL_CH(tolower(ENTER_MENU_KEY)) == c)
			{
				delay = -1;
				break;
			}
		}

		if (delay < 0)
			break;

		delay--;
		printf("\b\b\b%2d ", delay);
	}
	delay_ret = delay;

	printf("\n");

	return delay_ret;
}

/*****************************************************************************
 * 函 数 名  : mboot_menu_loop
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 菜单里面，等待用户输入函数
 * 输入参数  : rc_menu_data_t *menu  x
               unsigned char *key  x
               int *esc            x
 * 输出参数  : 无
 * 返 回 值  : void
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static void mboot_menu_loop(rc_menu_data_t *menu,
                         unsigned char *key, int *esc)
{
	int c;

	while (!tstc())
	{
		WATCHDOG_RESET();
		mdelay(10);
	}

	c = getc();

	switch (*esc)
	{
	case 0:
		/* First char of ANSI escape sequence '\e' */
		if (c == '\e')
		{
			*esc = 1;
			*key = KEY_NONE;
		}
		else
			*key = c;

		break;

	case 1:
		/* Second char of ANSI '[' */
		if (c == '[')
		{
			*esc = 2;
			*key = KEY_NONE;
		}
		else
		{
			*esc = 0;
		}

		break;

	case 2:
	case 3:
		/* Third char of ANSI (number '1') - optional */
		if (*esc == 2 && c == '1')
		{
			*esc = 3;
			*key = KEY_NONE;
			break;
		}

		*esc = 0;

		/* ANSI 'A' - key up was pressed */
		if (c == 'A')
			*key = KEY_UP;
		/* ANSI 'B' - key down was pressed */
		else if (c == 'B')
			*key = KEY_DOWN;
		/* other key was pressed */
		else
			*key = KEY_NONE;

		break;
	}

	/* enter key was pressed */
	if (c == '\r')
		*key = KEY_SELECT;
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_choice_entry
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 选择菜单回调函数
 * 输入参数  : void *data  x
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static char *mboot_menu_choice_entry(void *data)
{
	rc_menu_data_t *menu = data;
	struct rc_menu_data_entry *iter;
	unsigned char key = 0;
	char strkey[3];
	int esc = 0;
	int i;

	printf("\n");
	if(menu->active < mboot_menu_display_entry_num_get()){
		if((menu->active) == mboot_menu_display_entry_num_get()){
			printf("Press Up/Dwon or Number to move,Enter your choice: 0\n");
		}else{
			printf("Press Up/Dwon or Number to move,Enter your choice: %d\n",menu->active);
		}
	}
	
	while (1)
	{

		mboot_menu_loop(menu, &key, &esc);

		switch (key)
		{
		case KEY_UP:
			if (menu->active > 0)
			{
				--menu->active;
			}

			/* no menu key selected, regenerate menu */
			return NULL;

		case KEY_DOWN:
			if (menu->active < menu->display_count - 1)
			{
				++menu->active;
			}

			/* no menu key selected, regenerate menu */
			return NULL;

		case KEY_SELECT:
			if (0 == menu->delay)
			{
				iter = mboot_menu_get_entry_by_num(menu, DEFAULT_MENU_ENTRY_NUM);
			}
			else
			{
				iter = mboot_menu_get_entry_by_index(menu, menu->active);
				
			}

			return iter->key;

		default:
			iter = menu->first;
			sprintf(strkey, "%d", key);

			for (i = 0; i < menu->count; ++i)
			{
				if (!strcmp(iter->key, strkey))
				{
					if (iter->index < mboot_menu_display_entry_num_get()){
						menu->active = iter->index;
						return NULL;
					}
					printf("\n");
					printf("%s\n",iter->title);
					return iter->key;/*快捷键直接返回*/
				}
				iter = iter->next;
			}

			break;
		}
	}

	/* never happens */
	debug("rc_menu_data_t: this should not happen");
	return NULL;
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_destroy
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 释放rcboot菜单数据
 * 输入参数  : rc_menu_data_t *menu  x
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static void mboot_menu_destroy(rc_menu_data_t *menu)
{
	struct rc_menu_data_entry *iter = menu->first;
	struct rc_menu_data_entry *next;

	while (iter)
	{
		next = iter->next;
		free(iter->title);
		free(iter->command);
		free(iter);
		iter = next;
	}

	free(menu);
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_data_create
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 创建rcboot菜单数据
 * 输入参数  : int delay  x
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static rc_menu_data_t *mboot_menu_data_create(int delay)
{
	unsigned short int i = 0;
	rc_menu_data_t *menu;
	struct rc_menu_data_entry *iter = NULL;
	int len;
	struct rc_menu_data_entry *entry;
    const MENU_CFG *pMenuCfg = mboot_get_menu_CFG();

	menu = malloc(sizeof(rc_menu_data_t));

	if (!menu)
		return NULL;

	menu->delay = delay;
	menu->active = 0;
	menu->display_count = 0;
	menu->first = NULL;


	while (pMenuCfg[i].num != MENU_ENTRY_END_FLAG)
	{

		entry = malloc(sizeof(struct rc_menu_data_entry));

		if (!entry)
			goto cleanup;

		len = strlen(pMenuCfg[i].title);
		entry->title = malloc(len + 1);

		if (!entry->title)
		{
			free(entry);
			goto cleanup;
		}

		memcpy(entry->title, pMenuCfg[i].title, len);
		entry->title[len] = 0;
		len = strlen(pMenuCfg[i].command);
		entry->command = malloc(len + 1);

		if (!entry->command)
		{
			free(entry->title);
			free(entry);
			goto cleanup;
		}

		memcpy(entry->command, pMenuCfg[i].command, len);
		entry->command[len] = 0;

		sprintf(entry->key, "%d", pMenuCfg[i].key);

		entry->num = pMenuCfg[i].num;
		entry->type = pMenuCfg[i].type;
		entry->menu = menu;
		entry->next = NULL;
		entry->password = NULL;
		entry->index = 99;

		if (entry->type & ENTRY_NEED_PASSWD)
			entry->password = pMenuCfg[i].password;

		if (!(entry->type & ENTRY_HIDE))
			entry->index = menu->display_count++;

		if (!iter)
			menu->first = entry;
		else
			iter->next = entry;

		iter = entry;
		++i;

		if (i == MENU_ENTRY_END_FLAG - 1)
			break;
	}

	menu->count = i;

	return menu;

cleanup:
	mboot_menu_destroy(menu);
	return NULL;
}
/*****************************************************************************
 * 函 数 名  : mboot_menu_show
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : rcboot显示处理主函数
 * 输入参数  : int delay  x
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
void mboot_menu_show(int delay)
{
	int init = 0,  i = 0;
	char passwd[MAX_PASSWD_LEN] = {0};
	void *choice = NULL;
	char *title = NULL;
	char *command = NULL;
	struct menu *menu;
	rc_menu_data_t *p_rc_menu_data = NULL;
	struct rc_menu_data_entry *iter, *iter1, *itertmp;
	char *option, *sep;

	/* If delay is 0 do not create menu, just run first entry */
	if (delay == 0)
	{
		option = mboot_menu_rc_menu_getoption(0);

		if (!option)
		{
			printf("p_rc_menu_data option 0 was not found\n");
			return;
		}

		sep = strchr(option, '=');

		if (!sep)
		{
			printf("p_rc_menu_data option 0 is invalid\n");
			return;
		}

		run_command(sep + 1, 0);
		return;
	}

	p_rc_menu_data = mboot_menu_data_create(delay);

	if (!p_rc_menu_data)
		return;

#if 1
	menu = menu_create(NULL, p_rc_menu_data->delay, 1, mboot_menu_print_entry,
	                   mboot_menu_choice_entry, p_rc_menu_data);
#else
	menu = menu_create(NULL, p_rc_menu_data->delay, 0, mboot_menu_print_entry,
	                   mboot_menu_choice_entry, p_rc_menu_data);
#endif

	if (!menu)
	{
		mboot_menu_destroy(p_rc_menu_data);
		return;
	}

	iter1 = p_rc_menu_data->first;

	for (iter = p_rc_menu_data->first; iter; iter = iter->next)
	{
		if (!menu_item_add(menu, iter->key, iter))
			goto cleanup;
	}

	while (1)
	{
		mboot_menu_printf_flag_set(1);
		/* Default menu entry is always first */
		iter = mboot_menu_get_entry_by_num(p_rc_menu_data, DEFAULT_MENU_ENTRY_NUM);

		if (iter)
			menu_default_set(menu, iter->key);

		puts(ANSI_CURSOR_HIDE);

		init = 1;

		if (menu_get_choice(menu, &choice))
		{
			iter = choice;

			if (iter->password)
			{
				printf("Please input password: ");
				mboot_menu_passwd_get(passwd, sizeof(passwd) - 1);
				if (0 != strcmp(passwd, iter->password))
				{   /*密码错误*/
					continue;
				}
			}

			title = strdup(iter->title);
			command = strdup(iter->command);
		} 
		
		if (init)
		{
			puts(ANSI_CURSOR_SHOW);
		}

		if (title && command)
		{
			printf("Starting entry '%s'\n", title);
			free(title);

			if (!strcmp(command, "ubootrun"))
			{
				goto cleanup;
			}
			else if (!strcmp(command, "hprint"))
			{
				itertmp = iter1;
				printf(" Hide menu and key as follows:\n");
				printf("+--------------------------------------------+\n");

				for (i = 0; i < mboot_menu_hide_entry_num_get() + mboot_menu_display_entry_num_get(); i++)
				{
					if (i >= mboot_menu_display_entry_num_get())
					{
						printf("    %s \n", iter1->title);
					}
					if (NULL != iter1->next)
					{
						iter1 = iter1->next;
					}
				}
				iter1 = itertmp;
				printf("+--------------------------------------------+\n");
			}
			else if (!strcmp(command, "reset"))
			{
				run_command("reset", 0);
				goto cleanup;
			}
			else
			{
				printf("\n");
                    run_command(command, 0);
			}

			free(command);
		}
		p_rc_menu_data->delay = -1;
/*直接返回菜单，不再等待
		printf("\033[32m\r\npress any key to return menu\033[0m");
		getc();*/
	}

cleanup:
	menu_destroy(menu);
	mboot_menu_destroy(p_rc_menu_data);
}



/*****************************************************************************
 * 函 数 名  : boot_menu_mbootloop_default_choise
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : 不进入菜单时，要执行的操作
 * 输入参数  : void  x
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
__weak void boot_menu_mbootloop_default_choise(void)
{
	run_command("sysboot all", 0);
}
/*****************************************************************************
 * 函 数 名  : do_rc_menu
 * 负 责 人  : 
 * 创建日期  : 2017年9月14日
 * 函数功能  : rcboot的菜单入口函数
 * 输入参数  : cmd_tbl_t *cmdtp    x
               int flag            x
               int argc            x
               char *const argv[]  x
 * 输出参数  : 无
 * 返 回 值  : 
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
int do_rc_menu(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	char *delay_str = NULL;
	int delay = 3;

#if defined(CONFIG_RCBOOTDELAY) && (CONFIG_RCBOOTDELAY >= 0)
	delay = CONFIG_RCBOOTDELAY;
#endif
    //mboot_drv_image_init();

    if (0 == mboot_menu_boot_loop(delay))	//鏃堕棿鍒�0鎵ц榛樿鐨�
        boot_menu_mbootloop_default_choise();
    #if defined(MBOOT_ENABLE_MENU_PASSWORD) && (MBOOT_ENABLE_MENU_PASSWORD > 0)
    //mboot_menu_check_passwd(3);
    #endif

    if (argc >= 2)
        delay_str = argv[1];

    if (!delay_str)
        delay_str = getenv("rc_menu_delay");

    if (delay_str)
    {
		delay = (int)simple_strtol(delay_str, NULL, 10);
	}

    mboot_menu_show(delay);

    return 0;
}

U_BOOT_CMD(
    rcmenu, 2, 0, do_rc_menu,
    "ANSI terminal rcmenu",
    "[delay]\n"
    "    - show ANSI terminal rcmenu with autoboot delay"
);


