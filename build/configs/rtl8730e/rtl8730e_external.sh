#!/usr/bin/env bash
###########################################################################
#
# Copyright 2024 Samsung Electronics All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
###########################################################################
# rtl8730e_download.sh

TOOL_PATH=${TOP_PATH}/build/tools/amebasmart
IMG_TOOL_PATH=${TOOL_PATH}/image_tool

OS_PATH=${TOP_PATH}/os
CONFIG=${OS_PATH}/.config
source ${CONFIG}
APP_NUM=0

function pre_download()
{
	source ${TOP_PATH}/build/configs/${CONFIG_ARCH_BOARD}/board_metadata.txt
	if test -f "${BIN_PATH}/${EXTERNAL}.bin"; then
		cp -p ${BIN_PATH}/${EXTERNAL}.bin ${IMG_TOOL_PATH}/${EXTERNAL}.bin
	fi
}

function board_download()
{
	cd ${IMG_TOOL_PATH}
	if [ ! -f ${IMG_TOOL_PATH}/$3 ];then
		echo "$3 not present"
	else
		if [[ $TTYDEV == *"USB"* ]]; then
			#echo "UART download"
			./upload_image_tool_linux "download" $1 1 $2 $3
		fi

		if [[ $TTYDEV == *"ACM"* ]]; then
			#echo "USB download"
			echo "Save info to setting.txt"
			echo "$3" >> setting.txt
			echo "$2" >> setting.txt
			if [ "$3" == "$LAST_IMAGE" ]; then
				echo ""
				echo "=========================="
				echo "Start USB download"
				echo $(date)
				echo "=========================="
				./upload_image_tool_linux "download" $1
				echo "Complete USB download"
				echo $(date)
			fi
		fi
	fi
}

function board_erase()
{
	cd ${IMG_TOOL_PATH}
	./upload_image_tool_linux "erase" $1 1 $2 $3
}

function post_download()
{
	cd ${IMG_TOOL_PATH}
	if test -f "${RESOURCE_BIN_NAME}"; then
		[ -e  ] && rm ${RESOURCE_BIN_NAME}
	fi
	if test -f "${EXTERNAL}.bin"; then
		[ -e ${EXTERNAL}.bin ] && rm ${EXTERNAL}.bin
	fi

}


