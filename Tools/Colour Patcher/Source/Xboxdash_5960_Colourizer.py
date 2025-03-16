# Rocky5 2024
from __future__ import print_function
from ftplib import FTP
import argparse
import colorsys
import hashlib
import os
import shutil
import sys
import time

'''

Good site for looking at colours - https://traditionalcolors.com

If 'apply' is set to 1, the line will be processed.

If 'flip' is set to 1, the value is converted to little-endian format.
Values are stored in big-endian format by default, as this is the standard way to view hex colours.

If 'patch_type' is set to 0, the bytes are written as-is.
If 'patch_type' is set to 1, the bytes are added to the sequence "C6400C**C6400D**C6400E**" and then written.
If 'patch_type' is set to 2, the bytes are added to the sequence "C6400F**C6400C**C6400D**C6400E**" and then written.


Original MSDash Colours

01: B2D000  02: FFFF80  03: FEFFBC  04: F3FF6B  05: 19E100
06: 14C000  07: BEFA5E  08: 28D414  09: 8CC919  10: 8BC818
11: DDD078  12: 808080  13: F99819  14: 64C819  15: FFFFFF
16: 4CA200  17: 295700  18: 032C00  19: 4DE039  20: A0FC00
21: B6F560  22: 11FF22  23: 12C10A  24: 0A751C  25: 123700
26: 092900  27: 1EA000  28: CBCD55  29: 052305  30: 475345
31: 566452  32: 202020  33: 404040  34: 000003  35: 7DC622
36: F2FA99  37: 076800  38: 3CC643  39: C7E800  40: 617200
41: FCFF00  42: BFFF6B  43: 00FF12  44: 1EFF00  45: FD1E00
46: F21C00  47: FFAD6B  48: F6FF00  49: E5E5E5  50: 041400
51: 0E2E07  52: 9D6DC2  53: 0B2000  54: 062100

'''

def clear_screen():
	if os.name == 'nt':
		os.system('cls')
	else:
		os.system('clear')

def hex_to_argb(hex_color):
	if len(hex_color) == 6:
		hex_color = 'FF' + hex_color
	return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4, 6))

def hex_to_rgb(hex_color):
	return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

def argb_to_hex(argb_color):
	return '{:02X}{:02X}{:02X}{:02X}'.format(*argb_color)

def rgb_to_hex(rgb_color):
	return '{:02X}{:02X}{:02X}'.format(*rgb_color)

def adjust_color_to_target(color, target_color, brightness_factor):
	if len(color) == 8:  # ARGB
		a, r, g, b = hex_to_argb(color)
		h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
		# print('ARGB - Original Lightness: {}'.format(l))
		target_r, target_g, target_b = hex_to_rgb(target_color)
		target_h, _, target_s = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
		l = min(float(l) * brightness_factor, 1.0)
		# print('ARGB - Adjusted Lightness: {}'.format(l))
		new_r, new_g, new_b = colorsys.hls_to_rgb(target_h, l, target_s)
		adjusted_color = argb_to_hex((a, int(new_r * 255), int(new_g * 255), int(new_b * 255)))
	elif len(color) == 6:  # RGB
		r, g, b = hex_to_rgb(color)
		h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
		# print('RGB - Original Lightness: {}'.format(l))
		target_r, target_g, target_b = hex_to_rgb(target_color)
		target_h, _, target_s = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
		l = min(float(l) * brightness_factor, 1.0)
		# print('RGB - Adjusted Lightness: {}'.format(l))
		new_r, new_g, new_b = colorsys.hls_to_rgb(target_h, l, target_s)
		adjusted_color = rgb_to_hex((int(new_r * 255), int(new_g * 255), int(new_b * 255)))
	else:
		# print('Skipping color: {}'.format(color))
		return color
	# print('Original: {}\tAdjusted: {}'.format(color, adjusted_color))
	return adjusted_color

def adjust_colors_to_target(colors, target_color, brightness_factor=1.0):
	adjusted_colors = []
	for color in colors:
		adjusted_color = adjust_color_to_target(color, target_color, brightness_factor)
		adjusted_colors.append(adjusted_color)
	return adjusted_colors

def flip_bytes(hex_str):
	if len(hex_str) not in [6, 8]:
		return hex_str
	flipped = ''.join([hex_str[i:i+2] for i in range(0, len(hex_str), 2)][::-1])
	return flipped

def insert_bytes(small_value):
	large_value = 'C6400C**C6400D**C6400E**'
	bytes_list = [small_value[i:i+2] for i in range(0, len(small_value), 2)]
	for byte in bytes_list:
		large_value = large_value.replace('**', byte, 1)
	return large_value

def patch_list_to_support_ARGB(small_value):
	large_value = 'C6400F**C6400C**C6400D**C6400E**'
	bytes_list = [small_value[i:i+2] for i in range(0, len(small_value), 2)]
	for byte in bytes_list:
		large_value = large_value.replace('**', byte, 1)
	return large_value

def patch_file(file_path, patches, target_color='FFFFFF', brightness_factor=1.0, override=0):
	global export_colours
	export_colours = []
	name = ''
	with open(file_path, 'r+b') as f:
		for data in patches:
			try:
				name = data['name']
			except: pass
			process, address, patch, flip, value = data['apply'], data['address'], data['patch_type'], data['flip'], data['value']
			if process == '1':
				if not override and len(value) in [6, 8] and target_color.lower() != 'stock':
					value = adjust_color_to_target(value, target_color, float(brightness_factor))
				
				if name != '':
					export_colours.append('{}={}'.format(name, value))
				
				if patch == '0' and flip == '1' and len(value) in [6, 8]:
					value = flip_bytes(value)
				elif patch == '1' and len(value) in [6]:
					value = insert_bytes(value)
				elif patch == '2' and len(value) in [8]:
					value = patch_list_to_support_ARGB(value)
				
				if python_mode == 3:
					value = bytes.fromhex(value)
				else:
					value = value.decode('hex')
				
				f.seek(address)
				f.write(value)

def calculate_md5(file_path):
	with open(file_path, 'rb') as file:
		file_contents = file.read()
		md5_hash = hashlib.md5(file_contents).hexdigest()
	return md5_hash

def check_for_partition_patch(file_path, position, byte):
	with open(file_path, 'r+b') as f:
		f.seek(position)
		current_byte = f.read(1)
		if current_byte != byte:
			return 1
		else:
			return 0

def read_external_patch_list(file_path):
	with open(file_path, 'r') as file:
		lines = file.readlines()
	target_color = lines[1].strip().split('=')[1]
	brightness_factor = lines[2].strip().split('=')[1]
	external_patches = []
	for line in lines[4:]:
		line = line.strip()
		if not line.startswith(';') and '=' in line:
			external_patches.append(line)
	return target_color, brightness_factor, external_patches

def update_patches(patches, external):
	external_dict = {}
	for item in external:
		key, value = item.split('=')
		external_dict[key.strip()] = value.strip().lower()
	external_patches = []
	for patch in patches:
		if 'name' in patch:
			if patch['name'] in external_dict:
				updated_patch = patch.copy()
				updated_patch['value'] = external_dict[patch['name']]
				external_patches.append(updated_patch)
	return external_patches

def set_window_size(title, color='00', width=100, height=100):
	if os.name == 'nt':
		os.system('mode con: cols={} lines={}'.format(width, height))
		os.system('title {}'.format(title))
		os.system('color {}'.format(color))
	else:
		os.system('echo "\033]0;{}\007"'.format(title))

def show_error(error):
	if os.name == 'nt':
		set_window_size('Error', '0B', 70, 11)
	else:
		set_window_size('Error')
	
	print('{}'.format(error))
	time.sleep(10)
	sys.exit()

def check_length_of_colour(value):
	if value == 'stock':
		return
	if len(value) != 6:
		show_error("\n Error: The colour value must be exactly 6 characters long.")

def check_for_float(value):	
	try:
		float_value = float(value)
	except ValueError:
		show_error("\n Error: The value provided is not a valid float.")

def read_settings(file_path):
	settings = {}
	with open(file_path, 'r') as file:
		for line in file:
			if line.strip() and not line.startswith('#') and '=' in line:
				key, value = line.split('=', 1)
				settings[key.strip()] = value.strip()
	return settings

def upload_file(ftp_server, server_path, file_path, ftp_username='xbox', ftp_password='xbox'):
	global ftp
	max_retries = 4
	wait_time = 5
	for attempt in range(max_retries):
		try:
			# Connect to the FTP server
			ftp = FTP(ftp_server)
			ftp.login(user=ftp_username, passwd=ftp_password)
			ftp.cwd(server_path)
			if attempt > 0:
				print('\n Reconnected, uploading:')
			directory, filename = os.path.split(file_path)
			with open(file_path, 'rb') as file:
				ftp.storbinary('STOR {}'.format(filename), file)
			print("  - {} uploaded successfully.".format(filename))
			break
		except:
			clear_screen()
			if attempt < max_retries - 1:
				wait_counter = wait_time
				while True:
					time.sleep(1)
					clear_screen()
					print("\n\n Connection failed\n  - retrying in {}".format(wait_counter))
					wait_counter = wait_counter - 1
					if wait_counter == 0:
						break
			else:
				clear_screen()
				print("\n\n Can't connect to xbox.\n\n If this persists disable ftp support.")
				time.sleep(10)
				sys.exit()

fg_patches = [
	# Updated 3rd September 2024 thanks BigJx and to the original patch creator, you know who you're.
	# Description: F/G Support
	{'apply': '1', 'address': 0x00000840, 'patch_type': '0', 'flip': '0', 'value': '07'},
	{'apply': '1', 'address': 0x0001DCA2, 'patch_type': '0', 'flip': '0', 'value': '77C31B00E855FDFFFF31C0C3'},
	# F & G HDD0 - E, F, G HDD1
	{'apply': '1', 'address': 0x001C9000, 'patch_type': '0', 'flip': '0', 'value': '5589E583EC2C8B450C8A45080FBE45088D55E98D0DD09F1E00891424894C240489442408E8578FE8FF8D45E98D4DF0890C2489442404FF150C20010083EC088B450C8D4DE0890C2489442404FF150C20010083EC088D4DF08D45E0890C2489442404FF152420010083EC0883C42C5DC35C3F3F5C25633A005C4465766963655C486172646469736B305C506172746974696F6E360000000000005C4465766963655C486172646469736B305C506172746974696F6E37000000000000005589E568D89F1E006A4EE834FFFFFF83C4085D5589E568FA9F1E006A4FE821FFFFFF83C4085D5589E5686CA01E006A50E80EFFFFFF83C4085D5589E5688FA01E006A51E8FBFEFFFF83C4085DEB47C35C4465766963655C486172646469736B315C506172746974696F6E36000000000000005C4465766963655C486172646469736B315C506172746974696F6E37000000000000005589E568C8A01E006A52E89FFEFFFF83C4085D31C0C35C4465766963655C486172646469736B315C506172746974696F6E3100000000000000'}
]
	
other_patches = [
	# Description: Bypass Xip checks and allow external ones
	{'apply': '1', 'address': 0x0002D4C8, 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': 0x0002D520, 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': 0x0002D59D, 'patch_type': '0', 'flip': '0', 'value': '9090'},
	{'apply': '1', 'address': 0x0002D5B8, 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': 0x0002D863, 'patch_type': '0', 'flip': '0', 'value': '909090909090'},
	{'apply': '1', 'address': 0x0002D880, 'patch_type': '0', 'flip': '0', 'value': '9090'},
	{'apply': '1', 'address': 0x0002EB13, 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': 0x0002EBE7, 'patch_type': '0', 'flip': '0', 'value': 'E98400'},
	{'apply': '1', 'address': 0x0002EC54, 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	# No DVD region patch (by sylver) still requires the DVD dongle
	{'apply': '1', 'address': 0x000567F7, 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': 0x00056833, 'patch_type': '0', 'flip': '0', 'value': 'C64424120133C98B87440100008A4C241233D2B20149D2E2F6D2526A4450E802FC12008B8F440100008DB748010000566A4451E857FB12008BD081E20000008081FA00000080894424187527FE442412807C24120676B090909090909090909090909090909090909090909090909090909090'}
]

colour_patches = [
	# Description: Pulse highlight
	# Colour type: RGB
	# Original Bytes: 00D0B2
	{'name': 'pulse_highlight', 'apply': '1', 'address': 0x00046FAC, 'patch_type': '0', 'flip': '1', 'value': 'B2D000'},

	# Description: Pulsing Selection/focus
	# Colour type: RGB
	# Original Bytes: 80FFFF
	{'name': 'pulsing_selection', 'apply': '1', 'address': 0x00046FE1, 'patch_type': '0', 'flip': '1', 'value': 'FFFF80'},

	# Description: Button press (A, B etc...) indicator
	# Colour type: RGB
	# Original Bytes: BCFFFE
	{'name': 'egg_button_press', 'apply': '1', 'address': 0x000470A8, 'patch_type': '0', 'flip': '1', 'value': 'FEFFBC'},

	# Description: Keyboard button focus
	# Colour type: RGB
	# Original Bytes: 00D0B2
	{'name': 'kb_button_focus', 'apply': '1', 'address': 0x00047177, 'patch_type': '0', 'flip': '1', 'value': 'B2D000'},

	# Description: Keyboard button focus highlight
	# Colour type: ARGB
	# Original Bytes: 00D0B2FF
	{'name': 'kb_button_focus_highlight', 'apply': '1', 'address': 0x0004717E, 'patch_type': '0', 'flip': '1', 'value': 'FFB2D000'},

	# Description: Keyboard button no focus highlight
	# Colour type: ARGB
	# Original Bytes: 6BFFF3FF
	{'name': 'kb_button_large_no_focus_highlight', 'apply': '1', 'address': 0x000471F6, 'patch_type': '0', 'flip': '1', 'value': 'FFF3FF6B'},

	# Description: Keyboard left side left no focus buttons
	# Colour type: RGB
	# Original Bytes: 00E119
	{'name': 'kb_button_large_no_focus', 'apply': '1', 'address': 0x000471FD, 'patch_type': '0', 'flip': '1', 'value': '19E100'},

	# Description: Keyboard button left no focus highlight
	# Colour type: ARGB
	# Original Bytes: 6BFFF3DE
	{'name': 'kb_button_no_focus_highlight', 'apply': '1', 'address': 0x00047206, 'patch_type': '0', 'flip': '1', 'value': 'DEF3FF6B'},

	# Description: Keyboard no focus button back
	# Colour type: RGB
	# Original Bytes: 00C014
	{'name': 'kb_button_no_focus', 'apply': '1', 'address': 0x0004720D, 'patch_type': '0', 'flip': '1', 'value': '14C000'},

	# Description: Keyboard no focus letters
	# Colour type: RGB
	# Original Bytes: 5EFABE
	{'name': 'kb_letters_no_focus', 'apply': '1', 'address': 0x00047278, 'patch_type': '0', 'flip': '1', 'value': 'BEFA5E'},

	# Description: InnerWall_01 (transition)
	# Colour type: ARGB
	# Original Bytes: 6BFFF3FF
	{'name': 'innerwall_01_transition', 'apply': '1', 'address': 0x00047CC1, 'patch_type': '0', 'flip': '1', 'value': 'FFF3FF6B'},

	# Description: InnerWall_01
	# Colour type: ARGB
	# Original Bytes: 14D42814
	{'name': 'innerwall_01', 'apply': '1', 'address': 0x00047CF8, 'patch_type': '0', 'flip': '1', 'value': '1428D414'},

	# Description: InnerWall_02
	# Colour type: ARGB
	# Original Bytes: 00C01414
	{'name': 'innerwall_02', 'apply': '1', 'address': 0x00047D3F, 'patch_type': '0', 'flip': '1', 'value': '1414C000'},

	# Description: Material #1334
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C7400400980200895808C700F48B02
	# Original Bytes: C6400C0BC6400D2088580EC6400FC0
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00047F6A, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C7400400980200895808C700F48B0200'},
	{'name': 'material_#1334', 'apply': '1', 'address': 0x00047F8F, 'patch_type': '2', 'flip': '0', 'value': 'FF0b2000'},

	# Description: XBOXgreendark
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004E4970200895808C700F48B02
	# Original Bytes: C6400C06C6400D2188580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00047F29, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004E4970200895808C700F48B0200'},
	{'name': 'xboxgreendark', 'apply': '1', 'address': 0x00047F4E, 'patch_type': '2', 'flip': '0', 'value': 'FF062100'},

	# Description: XBOXgreen (Sub Menu A/B lower text)
	# Colour type: RGB
	# Original Bytes: C6400C8CC6400DC9C6400E19
	{'name': 'egg_xboxgreen_sub', 'apply': '1', 'address': 0x00047FD1, 'patch_type': '1', 'flip': '0', 'value': '8cc919'},

	# Description: XBoxGreen2
	# Colour type: RGB
	# Original Bytes: C6400C8BC6400DC8C6400E18
	{'name': 'xboxgreen2', 'apply': '1', 'address': 0x00048013, 'patch_type': '1', 'flip': '0', 'value': '8BC818'},

	# Description: GameHilite33
	# Colour type: RGB
	# Original Bytes: C6400CDDC6400DD0C6400E78
	{'name': 'gamehilite33', 'apply': '1', 'address': 0x00048055, 'patch_type': '1', 'flip': '0', 'value': 'DDD078'},

	# Description: Nothing
	# Colour type: RGB
	# Original Bytes: C6400C80C6400D80C6400E80
	{'name': 'nothing', 'apply': '1', 'address': 0x00048097, 'patch_type': '1', 'flip': '0', 'value': '808080'},

	# Description: NavType (Text)
	# Colour type: RGB
	# Original Bytes: C6400CBEC6400DFAC6400E5E
	{'name': 'navtype', 'apply': '1', 'address': 0x000480D9, 'patch_type': '1', 'flip': '0', 'value': 'BEFA5E'},

	# Description: OrangeNavType
	# Colour type: RGB
	# Original Bytes: C6400CF9C6400D98C6400E19
	{'name': 'orangenavtype', 'apply': '1', 'address': 0x0004811B, 'patch_type': '1', 'flip': '0', 'value': 'F99819'},

	# Description: XBOXGreen (Main menu A/B lower text)
	# Colour type: RGB
	# Original Bytes: C6400C8BC6400DC8C6400E18
	{'name': 'egg_xboxgreen', 'apply': '1', 'address': 0x0004819F, 'patch_type': '1', 'flip': '0', 'value': '8BC818'},

	# Description: Type
	# Colour type: RGB
	# Original Bytes: C6400C64C6400DC8C6400E19
	{'name': 'type', 'apply': '1', 'address': 0x000481E1, 'patch_type': '1', 'flip': '0', 'value': '64C819'},

	# Description: Typesdsafsda
	# Colour type: RGB
	# Original Bytes: C6400CFFC6400DFFC6400EFF
	{'name': 'typesdsafsda', 'apply': '1', 'address': 0x00048223, 'patch_type': '1', 'flip': '0', 'value': 'FFFFFF'},

	# Description: Material #1335
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004D8960200895808C700F48B02
	# Original Bytes: C6400C4CC6400DA288580EC6400FC8
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x0004827E, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004D8960200895808C700F48B0200'},
	{'name': 'material_#1335', 'apply': '1', 'address': 0x000482A3, 'patch_type': '2', 'flip': '0', 'value': 'FF4CA200'},

	# Description: Material #133511
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004B4960200895808C700F48B02
	# Original Bytes: C6400C29C6400D5788580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x000482BF, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004B4960200895808C700F48B0200'},
	{'name': 'material_#133511', 'apply': '1', 'address': 0x000482E4, 'patch_type': '2', 'flip': '0', 'value': 'FF295700'},
	
	# Description: HilightedType
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C7400478960200895808C700F48B02
	# Original Bytes: C6400CCCC6400D33C6400EFFC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x0004833F, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C7400478960200895808C700F48B0200'},
	{'name': 'hilightedtype', 'apply': '1', 'address': 0x00048364, 'patch_type': '2', 'flip': '0', 'value': 'FF032C00'},

	# Description: XBoxGreenq
	# Colour type: RGB
	# Original Bytes: C6400C8BC6400DC8C6400E18
	{'name': 'xboxgreenq', 'apply': '1', 'address': 0x000483A6, 'patch_type': '1', 'flip': '0', 'value': '8BC818'},

	# Description: CellEgg/Partsw
	# Colour type: RGB
	# Original Bytes: C6400C4DC6400DE0C6400E39
	{'name': 'cellegg_partsw', 'apply': '1', 'address': 0x00048466, 'patch_type': '1', 'flip': '0', 'value': '4DE039'},

	# Description: Material #108
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004E8950200897808C700F48B02
	# Original Bytes: C6400CA0C6400DFC88580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x000484C3, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004E8950200897808C700F48B0200'},
	{'name': 'material_#108', 'apply': '1', 'address': 0x000484E8, 'patch_type': '2', 'flip': '0', 'value': 'FFA0FC00'},

	# Description: ItemsType (UIX Config Title)
	# Colour type: RGB
	# Original Bytes: C6400CB6C6400DF5C6400E60
	{'name': 'itemstype', 'apply': '1', 'address': 0x0004852A, 'patch_type': '1', 'flip': '0', 'value': 'B6F560'},

	# Description: GameHiliteMemory
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004B0950200895808C700F48B02
	# Original Bytes: C6400CB2C6400DD088580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00048546, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004B0950200895808C700F48B0200'},
	{'name': 'gamehilitememory', 'apply': '1', 'address': 0x0004856B, 'patch_type': '2', 'flip': '0', 'value': 'FFB2D000'},

	# Description: white
	# Colour type: RGB
	# Original Bytes: C6400CFFC6400DFFC6400EFF
	{'name': 'white', 'apply': '1', 'address': 0x000485ED, 'patch_type': '1', 'flip': '0', 'value': 'FFFFFF'},

	# Description: solid green 1
	# Colour type: RGB
	# Original Bytes: C6400C11C6400DFFC6400E22
	{'name': 'solid_green_1', 'apply': '1', 'address': 0x00048634, 'patch_type': '1', 'flip': '0', 'value': '11FF22'},

	# Description: solid green 2
	# Colour type: RGB
	# Original Bytes: C6400C12C6400DC1C6400E0A
	{'name': 'solid_green_2', 'apply': '1', 'address': 0x00048676, 'patch_type': '1', 'flip': '0', 'value': '12C10A'},

	# Description: solid green 3
	# Colour type: RGB
	# Original Bytes: C6400C0AC6400D75C6400E1C
	{'name': 'solid_green_3', 'apply': '1', 'address': 0x000486B8, 'patch_type': '1', 'flip': '0', 'value': '0A751C'},

	# Description: solid green 4
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C740042C950200897008C700F48B02
	# Original Bytes: C6400C12C6400D3788580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x000486D4, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C740042C950200897008C700F48B0200'},
	{'name': 'solid_green_4', 'apply': '1', 'address': 0x000486F9, 'patch_type': '2', 'flip': '0', 'value': 'FF123700'},

	# Description: dark green panels
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C7400408950200897008C700F48B02
	# Original Bytes: C6400C09C6400D2988580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00048715, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C7400408950200897008C700F48B0200'},
	{'name': 'dark_green_panels', 'apply': '1', 'address': 0x0004873A, 'patch_type': '2', 'flip': '0', 'value': 'FF092900'},

	# Description: FlatSurfaces (Panel highlights)
	# Colour type: ARGB
	# Original Bytes: 6BFFF3C0
	{'name': 'flatsurfaces_highlights', 'apply': '1', 'address': 0x0004875A, 'patch_type': '0', 'flip': '1', 'value': 'C0F3FF6B'},
	# Description: FlatSurfaces (Menu Button Backs)
	# Colour type: RGB
	# Original Bytes: 00C014
	{'name': 'flatsurfaces_back', 'apply': '1', 'address': 0x0004875F, 'patch_type': '0', 'flip': '1', 'value': '14C000'},

	# Description: FlatSurfacesSelected (Menu header text colour)
	# Colour type: ARGB
	# Original Bytes: 00FF1E80
	{'name': 'flatsurfacesselected', 'apply': '1', 'address': 0x000487DA, 'patch_type': '0', 'flip': '1', 'value': '801EFF00'},

	# Description: FlatSurfacesMemory (Memory menu save back panel)
	# Colour type: ARGB
	# Original Bytes: 00A01E80
	{'name': 'flatsurfacesmemory', 'apply': '1', 'address': 0x0004882D, 'patch_type': '0', 'flip': '1', 'value': '801EA000'},

	# Description: DarkSurfaces
	# Colour type: ARGB
	# Original Bytes: 55CDCB5A
	{'name': 'darksurfaces', 'apply': '1', 'address': 0x00048859, 'patch_type': '0', 'flip': '1', 'value': '5ACBCD55'},

	# Description: dark green panels falloff
	# Colour type: ARGB
	# Original Bytes: 052305FF
	{'name': 'dark_green_panels_falloff', 'apply': '1', 'address': 0x0004905D, 'patch_type': '0', 'flip': '1', 'value': 'FF052305'},

	# Description: grey
	# Colour type: RGB
	# Original Bytes: 455347
	{'name': 'grey_1', 'apply': '1', 'address': 0x000490AD, 'patch_type': '0', 'flip': '1', 'value': '475345'},
	# Description: grey
	# Colour type: ARGB
	# Original Bytes: 526456FF
	{'name': 'grey_2', 'apply': '1', 'address': 0x000490B4, 'patch_type': '0', 'flip': '1', 'value': 'FF566452'},

	# Description: grill grey
	# Colour type: RGB
	# Original Bytes: 202020
	{'name': 'grill_grey_1', 'apply': '1', 'address': 0x00049104, 'patch_type': '0', 'flip': '1', 'value': '202020'},
	# Description: grill grey
	# Colour type: ARGB
	# Original Bytes: 404040FF
	{'name': 'grill_grey_2', 'apply': '1', 'address': 0x0004910B, 'patch_type': '0', 'flip': '1', 'value': 'FF404040'},

	# Description: Wireframe
	# Colour type: ARGB
	# Original Bytes: 03000000
	{'name': 'wireframe_1', 'apply': '1', 'address': 0x0004915C, 'patch_type': '0', 'flip': '1', 'value': '00000003'},
	# Description: Wireframe
	# Colour type: ARGB
	# Original Bytes: 22C67D64
	{'name': 'wireframe_2', 'apply': '1', 'address': 0x00049163, 'patch_type': '0', 'flip': '1', 'value': '647DC622'},
	# Description: Wireframe
	# Colour type: RGB
	# Original Bytes: 22C67D
	{'name': 'wireframe_3', 'apply': '1', 'address': 0x0004916A, 'patch_type': '0', 'flip': '1', 'value': '7DC622'},

	# Description: Tubes (behind button tubes)
	# Colour type: ARGB
	# Original Bytes: 99FAF2D7
	{'name': 'tubes_1', 'apply': '1', 'address': 0x0004917E, 'patch_type': '0', 'flip': '1', 'value': 'D7F2FA99'},
	# Description: Tubes (lower tubes)
	# Colour type: ARGB
	# Original Bytes: 00680725
	{'name': 'tubes_2', 'apply': '1', 'address': 0x00049183, 'patch_type': '0', 'flip': '1', 'value': '25076800'},

	# Description: MemoryHeader (Memory menu save back panel header outline)
	# Colour type: ARGB
	# Original Bytes: 43C63C7A
	{'name': 'memoryheader', 'apply': '1', 'address': 0x00049392, 'patch_type': '0', 'flip': '1', 'value': '7A3CC643'},

	# Description: MemoryHeaderHilite (Memory menu save panel header hightlight)
	# Colour type: ARGB
	# Original Bytes: 00E8C7F0
	{'name': 'memoryheaderhilite_1', 'apply': '1', 'address': 0x000493E9, 'patch_type': '0', 'flip': '1', 'value': 'F0C7E800'},
	# Description: MemoryHeaderHilite (Memory menu save back panel header no focus)
	# Colour type: ARGB
	# Original Bytes: 00726182
	{'name': 'memoryheaderhilite_2', 'apply': '1', 'address': 0x000493F0, 'patch_type': '0', 'flip': '1', 'value': '82617200'},

	# Description: EggGlow
	# Colour type: ARGB
	# Original Bytes: BCFFFEE4
	{'name': 'eggglow_1', 'apply': '1', 'address': 0x0004941E, 'patch_type': '0', 'flip': '1', 'value': 'E4FEFFBC'},
	# Description: EggGlow
	# Colour type: RGB
	# Original Bytes: 00FFFC
	{'name': 'eggglow_2', 'apply': '1', 'address': 0x00049423, 'patch_type': '0', 'flip': '1', 'value': 'FCFF00'},

	# Description: Gradient
	# Colour type: ARGB
	# Original Bytes: 6BFFBF33
	{'name': 'gradient_1', 'apply': '1', 'address': 0x000494D2, 'patch_type': '0', 'flip': '1', 'value': '33BFFF6B'},
	# Description: Gradient
	# Colour type: RGB
	# Original Bytes: 12FF00
	{'name': 'gradient_2', 'apply': '1', 'address': 0x000494D7, 'patch_type': '0', 'flip': '1', 'value': '00FF12'},
	
	# Description: CellEgg/Parts
	# Colour type: ARGB
	# Original Bytes: 6BFFF3B2
	{'name': 'cellegg_parts_1', 'apply': '1', 'address': 0x0004966C, 'patch_type': '0', 'flip': '1', 'value': 'B2F3FF6B'},
	# Description: CellEgg/Parts
	# Colour type: RGB
	# Original Bytes: 00FF1E
	{'name': 'cellegg_parts_2', 'apply': '1', 'address': 0x00049671, 'patch_type': '0', 'flip': '1', 'value': '1EFF00'},

	# Description: FlatSurfaces2sided3
	# Colour type: ARGB
	# Original Bytes: 001EFDFF
	{'name': 'flatsurfaces2sided3_1', 'apply': '1', 'address': 0x0004973C, 'patch_type': '0', 'flip': '1', 'value': 'FFFD1E00'},
	# Description: FlatSurfaces2sided3
	# Colour type: RGB
	# Original Bytes: 001CF2
	{'name': 'flatsurfaces2sided3_2', 'apply': '1', 'address': 0x00049743, 'patch_type': '0', 'flip': '1', 'value': 'F21C00'},

	# Description: console_hilite
	# Colour type: ARGB
	# Original Bytes: 6BADFFFF
	{'name': 'console_hilite_1', 'apply': '1', 'address': 0x00049793, 'patch_type': '0', 'flip': '1', 'value': 'FFFFAD6B'},
	# Description: console_hilite
	# Colour type: RGB
	# Original Bytes: 00FFF6
	{'name': 'console_hilite_2', 'apply': '1', 'address': 0x0004979A, 'patch_type': '0', 'flip': '1', 'value': 'F6FF00'},

	# Description: Metal_Chrome (highlight shading)
	# Colour type: RGB
	# Original Bytes: E5E5E5
	{'name': 'metal_chrome_1', 'apply': '1', 'address': 0x00049AAF, 'patch_type': '0', 'flip': '1', 'value': 'E5E5E5'},
	# Description: Metal_Chrome (hightlight)
	# Colour type: ARGB
	# Original Bytes: E5E5E5FF
	{'name': 'metal_chrome_2', 'apply': '1', 'address': 0x00049AB4, 'patch_type': '0', 'flip': '1', 'value': 'FFE5E5E5'},

	# Description: PanelBacking_01
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004FC8D0200895808C700F88B02
	# Original Bytes: C6400C04C6400D1488580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00049B36, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004FC8D0200895808C700F88B0200'},
	{'name': 'panelbacking_01', 'apply': '1', 'address': 0x00049B5B, 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: PanelBacking_03
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004BC8D0200895808C700F88B0200
	# Original Bytes: C6400C04C6400D1488580EC6400FF0
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00049BB6, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004BC8D0200895808C700F88B0200'},
	{'name': 'panelbacking_03', 'apply': '1', 'address': 0x00049BDB, 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: PanelBacking_04 (Music Menu)
	# Colour type: RGB
	# Original Bytes: C6400C0EC6400D2EC6400E07
	{'name': 'panelbacking_04', 'apply': '1', 'address': 0x00049C1D, 'patch_type': '1', 'flip': '0', 'value': '0E2E07'},

	# Description: DarkenBacking
	# Original Bytes: 74378B0D54E7170089048DB8C6170041890D54E71700C74004808D0200C740080A000000C700F88B02
	# Original Bytes: C6400C04C6400D3288580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00049C39, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004808D0200C740080A000000C700F88B0200'},
	{'name': 'darkenbacking', 'apply': '1', 'address': 0x00049C62, 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: button
	# Colour type: RGB
	# Original Bytes: C6400C9DC6400D6DC6400EC2
	{'name': 'button', 'apply': '1', 'address': 0x00049CEC, 'patch_type': '1', 'flip': '0', 'value': '9D6DC2'},

	# Description: image
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C740044C8D0200897008C700EC8B02
	# Original Bytes: C6400C04C6400D1488580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00049D08, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C740044C8D0200897008C700EC8B0200'},
	{'name': 'image', 'apply': '1', 'address': 0x00049D2D, 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: live header
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004348D0200897008C700EC8B02
	# Original Bytes: C6400C04C6400D1488580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00049D49, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004348D0200897008C700EC8B0200'},
	{'name': 'live_header', 'apply': '1', 'address': 0x00049D6E, 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: highlight
	# Colour type: RGB
	# Original Bytes: C6400C9DC6400D6DC6400EC2
	{'name': 'highlight', 'apply': '1', 'address': 0x00049DB0, 'patch_type': '1', 'flip': '0', 'value': '9D6DC2'},

	# Description: footer
	# Colour type: RGB
	# Original Bytes: 9D6DC2
	{'name': 'footer', 'apply': '1', 'address': 0x00049DF2, 'patch_type': '1', 'flip': '0', 'value': '9D6DC2'},

	# Description: LiveChrome
	# Colour type: RGB
	# Original Bytes: 9D6DC2
	{'name': 'livechrome', 'apply': '1', 'address': 0x00049E34, 'patch_type': '1', 'flip': '0', 'value': '9D6DC2'},

	# Description: GameHilite
	# Colour type: RGB
	# Original Bytes: FFFFFF
	{'name': 'gamehilite', 'apply': '1', 'address': 0x00049EAA, 'patch_type': '1', 'flip': '0', 'value': 'FFFFFF'},

	# Description: PanelBacking
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C740045C8C0200897008C700EC8B02
	# Original Bytes: C6400C04C6400D1488580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': 0x00049F6B, 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C740045C8C0200897008C700EC8B0200'},
	{'name': 'panelbacking', 'apply': '1', 'address': 0x00049F90, 'patch_type': '2', 'flip': '0', 'value': 'FF041400'}
]

if __name__ == '__main__':
	version = 1.2
	if os.name == 'nt':
		set_window_size('Xboxdash Colourizer v{}'.format(version), '0B', 70, 11)
	else:
		set_window_size('Xboxdash Colourizer v{}'.format(version))

	global python_mode
	python_mode = 3
	if sys.version_info[0] < 3:
		python_mode = 2
	
	target_color = ''
	brightness_factor = ''
	override = 0
	export = 0
	
	# Check for settings.ini and make one if it doesn't exist
	if not os.path.isfile('settings.ini'):
		settings_template = '''[FTP]
enable=false
server=0.0.0.0
username=xbox
password=xbox'''
		with open('settings.ini', 'w') as settings:
			settings.write(settings_template)
	
	# Read settings
	settings = read_settings('settings.ini')
	ftp_enabled = settings.get('enable')
	ftp_server = settings.get('server')
	ftp_username = settings.get('username', 'xbox')
	ftp_password = settings.get('password', 'xbox')

	# Check for xbe files
	if not os.path.isdir('xbe file'):
		os.makedirs('xbe file')
	xboxdash = os.path.join('xbe file','xboxdash.xbe')
	xb0xdash = os.path.join('xbe file','xb0xdash.xbe')
	if os.path.isfile(xboxdash):
		file_path = xboxdash
	elif os.path.isfile(xb0xdash):
		file_path = xb0xdash
	else:
		show_error("\n Error: Cannot find valid xbe file:\n\n Looking for:\n  - xbe file\\xboxdash.xbe\n  - xbe file\\xb0xdash.xbe\n\n Please ensure the file exists and try again.")
	
	parser = argparse.ArgumentParser(description='Patch Xboxdash colours')
	parser.add_argument('--colour_file', type=str, help='Path to the file')
	parser.add_argument('--target_color', type=str, help='Target color')
	parser.add_argument('--brightness_factor', type=float, help='Brightness factor')

	args = parser.parse_args()

	# Assign the arguments to variables, prompting the user if necessary
	if python_mode == 3:
		colour_file = args.colour_file if args.colour_file else input('\n Use an "theme".ini file to override specific colours.\n (optional enter export to output a "theme.ini" file)\n Enter filename: ') or ''
		colour_file = os.path.join('skins', colour_file)
		
		# So you can enter filename without extension
		if not colour_file.endswith('.ini'):
			colour_file += '.ini'
		
		if os.path.isfile(colour_file):
			override = 1
			target_color, brightness_factor, external_patches = read_external_patch_list(colour_file)
		elif not os.path.isfile(colour_file) and not "export.ini":
			print("\n Couldn't fine the file {}".format(colour_file))
		
		if target_color == '':
			target_color = args.target_color if args.target_color else input('\n Will restore Stock colours if left blank.\n Enter RGB Hex colour value: ') or 'stock'
		else:
			print('\n Colour set to: {}'.format(target_color))
		
		check_length_of_colour(target_color)
		
		if brightness_factor == '':
			brightness_factor = args.brightness_factor if args.brightness_factor else float(input('\n Default to 1.0 if left blank.\n Enter brightness factor (defaults to 1.0): ') or 1.0)
		else:
			print(' Brightness facter set to: {}'.format(brightness_factor))
	
		check_for_float(brightness_factor)
	
	else: # Python 2
		colour_file = args.colour_file if args.colour_file else raw_input('\n Use an "theme".ini file to override specific colours.\n (optional enter export to output a "theme.ini" file)\n Enter filename: ') or ''
		colour_file = os.path.join('skins', colour_file)
		
		# So you can enter filename without extension
		if colour_file != '' and not colour_file.endswith('.ini'):
			colour_file += '.ini'
		
		if os.path.isfile(colour_file):
			override = 1
			target_color, brightness_factor, external_patches = read_external_patch_list(colour_file)
		elif not os.path.isfile(colour_file) and not "export.ini":
			print("\n Couldn't fine the file {}".format(colour_file))
		
		if target_color == '':
			target_color = args.target_color if args.target_color else raw_input('\n Will restore Stock colours if left blank.\n Enter RGB Hex colour value: ') or 'stock'
		else:
			print('\n Colour set to: {}'.format(target_color))
		
		check_length_of_colour(target_color)
		
		if brightness_factor == '':
			brightness_factor = args.brightness_factor if args.brightness_factor else float(raw_input('\n Default to 1.0 if left blank.\n Enter brightness factor (defaults to 1.0): ') or 1.0)
		else:
			print(' Brightness facter set to: {}'.format(brightness_factor))
	
		check_for_float(brightness_factor)

	if target_color != 'stock':
		target_a, target_r, target_g, target_b = hex_to_argb(target_color)
		target_hue, target_lightness, target_saturation = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
	
	# Pause so you can see info for external files
	time.sleep(0.5)
	
	# Create output folder, copy xbe from xbe file folder so source file is left intact.
	tran_folder = 'transfer to xbox'
	if not os.path.isdir(tran_folder):
		os.makedirs(tran_folder)
	else:
		shutil.rmtree(tran_folder)
		os.makedirs(tran_folder)
	
	shutil.copyfile(file_path, os.path.join(tran_folder, os.path.basename(file_path)))
	# Set new file path
	file_path = os.path.join(tran_folder, os.path.basename(file_path))
	
	# Check if xbe is clean & patch extended partitions support.
	if calculate_md5(file_path) == '08d3a6f99184679aa13008d6397bacce':
		patch_file(file_path, fg_patches, '', '', 1)
	
	# Patch out xip stuff
	patch_file(file_path, other_patches, '', '', 1)
	
	# Patch colours.
	patch_file(file_path, colour_patches, target_color, brightness_factor, 0)
	
	# If override file is found
	if override or target_color == 'stock':
		if target_color != 'stock':
			external_patches = update_patches(colour_patches, external_patches)
			patch_file(file_path, external_patches, '', '', 1)
			ini_name = os.path.basename(os.path.splitext(colour_file)[0])
			xip_file = os.path.splitext(colour_file)[0] + '.xip'
		else:
			ini_name = 'stock'
			xip_file = 'skins\\stock.xip'
		
		if os.path.isfile(xip_file):	
			xbdash_folder = os.path.join(tran_folder, 'xboxdashdata.185ead00')
			if not os.path.isdir(xbdash_folder):
				os.makedirs(xbdash_folder)
			shutil.copyfile(xip_file, os.path.join(xbdash_folder, 'skin.xip'))

		colour = ini_name
	else:
		colour = '#{}'.format(target_color)
	
	# Export colours
	if 'export' in colour_file:
		with open('exported {}.ini'.format(target_color), "w") as file:
			template = '[These are mandatory]\ncolour=%s\nbrightness=%s\n[Override colours with your own]'
			file.write(template % (target_color, brightness_factor) + "\n")
			for line in export_colours:
				file.write(line + "\n")
	
	clear_screen()
	if target_color.lower() == 'stock':
		print('\n\n Stock colours restored\n')
	else:
		print('\n\n Colour {}\n Applied successfully\n'.format(colour))
	
	if ftp_enabled.lower() in ['1', 'yes', 'true']:
		if file_path:
			print(' Uploading:')
			upload_file(ftp_server, '/C/', file_path, ftp_username, ftp_password)
			try:
				skin_xip = os.path.join(xbdash_folder, 'skin.xip')
				if skin_xip:
					upload_file(ftp_server, '/C/xboxdashdata.185ead00/', skin_xip, ftp_username, ftp_password)
			except:
				pass
		ftp.quit()
		print(" Done.")
		shutil.rmtree(tran_folder)

time.sleep(2)
