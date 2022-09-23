/****************************************************************************
 *
 * Copyright 2022 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdio.h>
#include <security/security_common.h>
#include <security/security_api.h>
#include <tinyara/seclink.h>
#include <tinyara/security_hal.h>
#include <security/security_ss.h>
#define SE_IOTIVITY_SVR_DB 0
#define TAG_LENGTH 16
#define DATA_LENGTH 4
#define HEAD_LENGTH 4
#define TEST_DATA_LENGTH 128;
#define TEST_SLOT_NUM 4
#define MAX_SS_SIZE 8192
#define SHOW_USAGE printf("Usage: sstorage -s SLOT_INDEX -d DATA\n	\
	SLOT_INDEX : slot index to save the DATA, valid range 0 ~ 31\n	\
	DATA : value to be saved in byte\n");



extern int rtl_ss_flash_read(uint32_t address, uint32_t len, uint8_t *data, int en_display);

#ifdef  CONFIG_AMEBALITE_TRUSTZONE
static uint32_t *ADDR = 0x735C021;
#endif

/****************************************************************************
 * secure_storage_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int secure_world_main(int argc, char *argv[])
#endif
{
	int option;
	volatile int ptr;
	while((option=getopt(argc,argv,"rw"))!=ERROR)
	{
		switch(option){
			case 'r':
				ptr = *ADDR;
				// scanf("%d",ADDR);
				break;
			case 'w':

				*ADDR = 1234; 
				printf("%d",*ADDR);
				break;
			default:
				SHOW_USAGE
				return 0;
		}
	}

	return 0;
}
