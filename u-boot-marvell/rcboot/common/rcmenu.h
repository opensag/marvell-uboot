/***********************************************************************************
 * �� �� ��   : mboot.h
 * �� �� ��   : wjx
 * ��������   : 2017��9��13��
 * �ļ�����   : rcboot����ͷ�ļ�,
 * ��Ȩ˵��   : Copyright (c) 2017-2017   ��˹����Ƽ���չ�ɷ����޹�˾
 * ��    ��   : 
 * �޸���־   : 
***********************************************************************************/

#ifndef __MBOOT_H__
#define __MBOOT_H__
#include "common.h"




#if 0
/* image info ����������Ϣ ��ض��������**************************************************************/
typedef enum tag_RCIMAGE_MEADIUM_TYPE
{   /*����洢�������Ͷ���*/
    MEADIUM_SPI_FLASH = 1,
    MEADIUM_NOR_FLASH,
    MEADIUM_NAND_FLASH,
    MEADIUM_FS_FAT,
    MEADIUM_FS_YAFFAS,
    MEADIUM_FS_JIFFS,
    MEADIUM_CUSTOM_1,
    MEADIUM_CUSTOM_2,
    MEADIUM_CUSTOM_3,
    MEADIUM_MAX = 12,
}MEDIUM_TYPE;

#ifndef RCIMAGE_TYPE_CUSTOMER
typedef enum tag_RCIMAGE_TYPE
{   /*�������Ͷ���*/
    RCIMAGE_TYPE_SBOOT = 1,
    RCIMAGE_TYPE_MBOOT = 2,
    RCIMAGE_TYPE_SYS = 3,
    RCIMAGE_TYPE_BURN,
    RCIMAGE_TYPE_ENV,   
}RCIMAGE_TYPE;
#endif

typedef enum rcimage_status {
    /*����״̬����*/
    RCIMAGE_STATUS_OK = 0x0,
    RCIMAGE_STATUS_BAD_IO,
    RCIMAGE_STATUS_BAD_MAGIC,    
    RCIMAGE_STATUS_BAD_OID,
    RCIMAGE_STATUS_BAD_TYPE,
    RCIMAGE_STATUS_BAD_CRC,
}RCIMAGE_STATUS;



typedef struct tag_rcimage_info_cfg{
    /*������Ϣ������ö���*/
    char *name;  //IMG_END_FLAG �ǽ������ 
    unsigned int load;          // memory address to load          
    unsigned int image_start;   // image start address in medium
    unsigned int part_size;     // image max size, the part size
    unsigned int header_len;    // image header size
    unsigned int save_header;   // if save header to flash
    unsigned int oid;           // image oid in header, no set no check
    unsigned int magic;         // image magic in header, no set no check
    unsigned int type;          // image type, sboot, mboot, system..., no set no check
    unsigned int index;         // the index of the image, if the same type image more than one
    unsigned int medium;        // spi-flash, nor-flash, nand-flash...
    unsigned int check_crc;     // if check crc, 1 check, 0 not check
    RCIMAGE_STATUS (*img_check)(const struct tag_rcimage_info_cfg *pIf,  
                                uint addr, uint check_data_flag,
                                uint *pImageLen, const char **pImageName);
}RC_IMG_CFG;
extern RC_IMG_CFG g_Images_cfg[ ];
#define IMG_END_FLAG "image_end"/*�������ý������*/
#endif


/*menu �˵���ؽṹ�嶨��ͺ�������*********************************************************************/
typedef struct tag_MENU_INFOR_CFG
{/*�˵����ýṹ�嶨��*/
	unsigned short int num;		/* unique number 0 .. MAX_COUNT,default 1 */
	char key;			/* key identifier of number */
	char *title;			/* title of entry */
	char *command;			/* hush command of entry */
	char *password;
	int type;
}MENU_CFG;
extern MENU_CFG g_menu_cfg[];
#define MENU_ENTRY_END_FLAG 99 



#define ENTRY_DISPLAY         (1<<0)
#define ENTRY_HIDE            (1<<1)
#define ENTRY_NEED_PASSWD     (1<<2)
#define ENTRY_NEED_CONFIRM    (1<<3)




/*����*/
#define CTRL_CH(c)    ((c) - 'a' + 1)


//#define wjx_d printf("f=%s l=%d\n", __func__,__LINE__);

#if 0

typedef enum tagRcbootStage{
    RCBOOT_STAGE_INIT = 0,
    RCBOOT_STAGE_READ_IMAGE_HEAD = 1,
    RCBOOT_STAGE_LOAD = 2,
    RCBOOT_STAGE_IGNOR = 10000,
}RCBOOT_STAGE;
void mboot_stage_set(RCBOOT_STAGE stage);
RCBOOT_STAGE mboot_stage_get(void);
const char * mboot_type_str_get(const RC_IMG_CFG *pIf);
#endif
/*����*/
//#define RAISECOM_MENU_SWITCH /*��������Ʒ�˵�*/
//#define RAISECOM_HEAD_SWITCH /*��������Ʒͷ*/
//#define RAISECOM_MEDIUM_DRV_SPI_FLASH /*SPI flash����*/
//#define RAISECOM_MEDIUM_DRV_NAND_FLASH /* nand ����*/

#ifndef __weak
#define __weak	__attribute__((weak))
#endif

#endif
