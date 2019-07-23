#! /bin/bash

#
# This script generates the image files used in the ARM Trusted Firmware Reset
# Design document from the 'reset_code_flow.dia' file.
#
# The PNG files in the present directory have been generated using Dia version
# 0.97.2, which can be obtained from https://wiki.gnome.org/Apps/Dia/Download
#

set -e

# Usage: generate_image <layers> <image_filename>
function generate_image
{
	dia				\
		--show-layers=$1	\
		--filter=png		\
		--export=$2		\
		reset_code_flow.dia

}

# The 'reset_code_flow.dia' file is organized in several layers.
# Each image is generated by combining and exporting the appropriate set of
# layers.
generate_image								\
	Frontground,Background,cpu_type_check,boot_type_check		\
	default_reset_code.png

generate_image								\
	Frontground,Background,no_cpu_type_check,boot_type_check	\
	reset_code_no_cpu_check.png

generate_image								\
	Frontground,Background,cpu_type_check,no_boot_type_check	\
	reset_code_no_boot_type_check.png

generate_image								\
	Frontground,Background,no_cpu_type_check,no_boot_type_check	\
	reset_code_no_checks.png
