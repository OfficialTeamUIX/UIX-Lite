# -*- coding: utf-8 -*-
# Rocky5 2025
from __future__ import print_function, unicode_literals
from binascii import unhexlify
from tkinter import colorchooser, filedialog, messagebox, simpledialog
import tkinter as tk
import argparse, colorsys, configparser, ftplib, hashlib, io, os, re, shutil, struct, sys, time, zipfile

'''

Good site for looking at colours - https://traditionalcolors.com

If 'apply' is set to 1, the line will be processed.

If 'flip' is set to 1, the value is converted to little-endian format.
Values are stored in big-endian format by default, as this is the standard way to view hex colours.

If 'patch_type' is set to 0, the bytes are written as-is.
If 'patch_type' is set to 1, the bytes are added to the sequence "C6400C**C6400D**C6400E**" and then written.
If 'patch_type' is set to 2, the bytes are added to the sequence "C6400F**C6400C**C6400D**C6400E**" and then written.
If 'patch_type' is set to 3, floats for xonlinedash.
If 'patch_type' is set to 4, insert bytes at offset.
If 'patch_type' is set to 5, insert a byte and duplicate as many time as you want.


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

def hex_to_argb(hex_colour):
	if len(hex_colour) == 6:
		hex_colour = 'FF' + hex_colour
	return tuple(int(hex_colour[i:i+2], 16) for i in (0, 2, 4, 6))

def hex_to_rgb(hex_colour):
	return tuple(int(hex_colour[i:i+2], 16) for i in (0, 2, 4))

def argb_to_hex(argb_colour):
	return '{:02X}{:02X}{:02X}{:02X}'.format(*argb_colour)

def rgb_to_hex(rgb_colour):
	return '{:02X}{:02X}{:02X}'.format(*rgb_colour)

def adjust_colour_to_target(colour, target_colour, float_type, brightness_factor):
	if len(colour) == 8: # ARGB
		a, r, g, b = hex_to_argb(colour)
		h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
		# print('ARGB - Original Lightness: {}'.format(l))
		target_r, target_g, target_b = hex_to_rgb(target_colour)
		target_h, _, target_s = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
		l = min(float(l) * brightness_factor, 1.0)
		# print('ARGB - Adjusted Lightness: {}'.format(l))
		new_r, new_g, new_b = colorsys.hls_to_rgb(target_h, l, target_s)
		adjusted_colour = argb_to_hex((a, int(new_r * 255), int(new_g * 255), int(new_b * 255)))
	elif len(colour) == 6: # RGB
		r, g, b = hex_to_rgb(colour)
		h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
		# print('RGB - Original Lightness: {}'.format(l))
		target_r, target_g, target_b = hex_to_rgb(target_colour)
		target_h, _, target_s = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
		l = min(float(l) * brightness_factor, 1.0)
		# print('RGB - Adjusted Lightness: {}'.format(l))
		new_r, new_g, new_b = colorsys.hls_to_rgb(target_h, l, target_s)
		adjusted_colour = rgb_to_hex((int(new_r * 255), int(new_g * 255), int(new_b * 255)))
	elif len(colour) == 4: # RG-RB-GB
		if float_type == "R1B":
			r, g, b = hex_to_rgb("{}FF{}".format(colour[:2], colour[2:]))
		elif float_type == "0GB":
			r, g, b = hex_to_rgb("00{}".format(colour))
		elif float_type == "1GB":
			r, g, b = hex_to_rgb("FF{}".format(colour))
		h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
		# print('RGB - Original Lightness: {}'.format(l))
		target_r, target_g, target_b = hex_to_rgb(target_colour)
		target_h, _, target_s = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
		l = min(float(l) * brightness_factor, 1.0)
		# print('RGB - Adjusted Lightness: {}'.format(l))
		new_r, new_g, new_b = colorsys.hls_to_rgb(target_h, l, target_s)
		adjusted_colour = rgb_to_hex((int(new_r * 255), int(new_g * 255), int(new_b * 255)))
		adjusted_colour = adjusted_colour[2:]
	elif len(colour) == 2: # R, G, B
		if float_type == "R":
			r, g, b = hex_to_rgb("{}FFFF".format(colour[:2]))
		if float_type == "G":
			r, g, b = hex_to_rgb("{}FF{}".format(colour[:2], colour[2:]))
		if float_type == "B":
			r, g, b = hex_to_rgb("FFFF{}".format(colour[2:]))
		h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
		# print('RGB - Original Lightness: {}'.format(l))
		target_r, target_g, target_b = hex_to_rgb(target_colour)
		target_h, _, target_s = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
		l = min(float(l) * brightness_factor, 1.0)
		# print('RGB - Adjusted Lightness: {}'.format(l))
		new_r, new_g, new_b = colorsys.hls_to_rgb(target_h, l, target_s)
		adjusted_colour = rgb_to_hex((int(new_r * 255), int(new_g * 255), int(new_b * 255)))
		if float_type == "R":
			adjusted_colour = adjusted_colour[:2]
		if float_type == "G":
			adjusted_colour = "{}{}".format(adjusted_colour[:2], adjusted_colour[2:])
		if float_type == "B":
			adjusted_colour = adjusted_colour[2:]
	else:
		# print('Skipping colour: {}'.format(colour))
		return colour

	# print('Original: {}\tAdjusted: {}'.format(colour, adjusted_colour))
	return adjusted_colour

def adjust_colours_to_target(colours, target_colour, brightness_factor=1.0):
	adjusted_colours = []
	for colour in colours:
		adjusted_colour = adjust_colour_to_target(colour, target_colour, brightness_factor)
		adjusted_colours.append(adjusted_colour)
	return adjusted_colours

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

def patch_file(file_path, patches, target_colour='FFFFFF', brightness_factor=1.0, override=0):
	with open(file_path, 'r+b') as xbeData:
		for data in patches:
			name = data.get('name', '')
			float_type = data.get('float_type', '')
			process, addresses, patch, flip, value = data['apply'], data['address'], data['patch_type'], data['flip'], data['value']

			if isinstance(addresses, str) and ',' in addresses:
				addresses = addresses.split(',')
				addresses = [addr.strip() for addr in addresses]

			if not isinstance(addresses, (list, tuple)):
				addresses = [addresses]

			addresses = [int(addr, 16) if isinstance(addr, str) and addr.startswith("0x") else addr for addr in addresses]
			
			if process == '1':
				if not override and len(value) in [2, 4, 6, 8] and target_colour.lower() != 'stock':
					value = adjust_colour_to_target(value, target_colour, float_type, float(brightness_factor))

				if patch == '0' and flip == '1' and len(value) in [6, 8]:
					value = flip_bytes(value)
				elif patch == '1' and len(value) in [6]:
					value = insert_bytes(value)
				elif patch == '2' and len(value) in [8]:
					value = patch_list_to_support_ARGB(value)

				if patch == '3':
					fill_values = [value[i:i+2] for i in range(0, len(value), 2)]
					value_list = []
					for fill_value in fill_values:
						normalized_value = int(fill_value, 16) / 255.0
						float_bytes = struct.pack('<f', normalized_value)
						value_list.append(float_bytes.hex().upper())

				# Used to convert floats into hex values
				# hex_values = "".split()
				# for hex_val in hex_values:
					# float_value = struct.unpack('<f', bytes.fromhex(hex_val))[0]
					# hex_result = struct.pack('<f', float_value).hex().upper()
					# color_value = int(float_value * 255)

					# print(f"{hex_val} -> {round(float_value, 4)} -> {color_value:02X}")
					# os.system('pause')

				if patch == '5':
					fill_value, repeat_size = value.split('|')
					repeat_size = int(repeat_size)
					value = (fill_value * ((repeat_size * 2) // len(fill_value) + 1))[:repeat_size * 2]

				value = bytes.fromhex(value)

				for idx, address in enumerate(addresses):
					xbeData.seek(address)
					if patch == '3' and idx < len(value_list):
						xbeData.write(bytes.fromhex(value_list[idx]))
					else:
						xbeData.write(value)

				if patch in ['4', '5']:
					xbeData.seek(0)
					xbe_bytes = xbeData.read()
					beginning = xbe_bytes[:addresses[0]]
					end = xbe_bytes[addresses[0]:]
					xbe_bytes = beginning + value + end
					xbeData.seek(0)
					xbeData.write(xbe_bytes)
					xbeData.truncate()

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
	target_colour = lines[1].strip().split('=')[1]
	brightness_factor = lines[2].strip().split('=')[1]
	external_patches = []
	for line in lines[4:]:
		line = line.strip()
		if not line.startswith(';') and '=' in line:
			external_patches.append(line)
	return target_colour, brightness_factor, external_patches

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

def set_window_size(title, colour='00', width=100, height=100):
	if os.name == 'nt':
		os.system('mode con: cols={} lines={}'.format(width, height))
		os.system('title {}'.format(title))
		os.system('color {}'.format(colour))
	else:
		os.system('echo "\033]0;{}\007"'.format(title))

def show_error(error):
	if os.name == 'nt':
		set_window_size('Error', '4F', 70, 21)
	else:
		set_window_size('Error')
	
	print('{}'.format(error))
	input("\n Press Enter to exit.")
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

def upload_file(ftp_server, server_path, path, ftp_username='xbox', ftp_password='xbox', single_file=False):
	global ftp
	max_retries = 4
	wait_time = 5

	def upload_recursive(local_path, remote_path):
		for filename in os.listdir(local_path):
			file_path = os.path.join(local_path, filename)
			server_file_path = os.path.join(remote_path, filename).replace("\\", "/")

			if os.path.isdir(file_path):
				try:
					ftp.cwd(server_file_path)
				except ftplib.error_perm:
					ftp.mkd(server_file_path)
				# print("   - Creating directory: {}".format(server_file_path))

				upload_recursive(file_path, server_file_path)
			else:
				with open(file_path, "rb") as file:
					ftp.storbinary(f"STOR {server_file_path}", file)
			
			if os.path.isfile(file_path): 
				print("   - {} uploaded.".format(filename))

	for attempt in range(max_retries):
		try:
			# Connect to the FTP server
			ftp = ftplib.FTP(ftp_server)
			ftp.login(user=ftp_username, passwd=ftp_password)
			ftp.cwd(server_path)

			if attempt > 0:
				print('\n  Reconnected, uploading:')

			if single_file:
				filename = os.path.basename(path)
				with open(path, 'rb') as file:
					ftp.storbinary('STOR {}'.format(filename), file)
				print("   - {} uploaded successfully.".format(filename))
			else:
				upload_recursive(path, server_path)

			break
		except:
			clear_screen()
			if attempt < max_retries - 1:
				wait_counter = wait_time
				while True:
					time.sleep(1)
					clear_screen()
					print("\n\n  Connection failed\n   - retrying in {}".format(wait_counter))
					wait_counter -= 1
					if wait_counter == 0:
						break
			else:
				clear_screen()
				print("\n\n  Can't connect to xbox.\n\n  If this persists disable FTP support.")
				time.sleep(10)
				sys.exit()

fg_patches = [
	# New patch position 01/04/2025 BigJx, I and to the original patch creator, you know who you're.
	# Header sections count and offset/size
	{'apply': '1', 'address': '0x0000010D', 'patch_type': '0', 'flip': '0', 'value': 'E31E00840100003D856840840101001A0000007003010008000000DCA2FAA8B42801000000010000001000'},
	{'apply': '1', 'address': '0x0000014C', 'patch_type': '0', 'flip': '0', 'value': '400B0100610B01001C0B0100B6606C5B000000000E0000003C0A01007C0A01003C0A0100740B0100B20200000000000000000000000000'},
	# sections data (.hack)
	{'apply': '1', 'address': '0x00000384', 'patch_type': '0', 'flip': '0', 'value': '580901000000000020090100220901005A0D6C2C59FBF9AA8E3CB4AD8FE9EC75273FAB360700000020F40C00E059020000000C00D85902005E090100000000002209010024090100A974B91B76A0C446440F89214C5715F5AAE71B6507000000004E0F00A484000000600E004482000063090100000000002409010026090100BC2C38531E85D8A6B6F907CE05290DC8F196E17E07000000C0D20F002495010000F00E00249501006A090100000000002609010028090100C38F8CFE3D5BE8308F42AAB405E15811B32B9D562600000000681100BC9E010000901000BC9E01007109010000000000280901002A090100BF655CA4A371C032DD501C988BD3573163AD979B16000000C006130078250100003012007825010079090100000000002A0901002C09010096E49224CD0699B30AF51CA8B524F3AF3175A3F507000000402C14004C460100006013007C0D01007E090100000000002C0901002E0901006544C534AFDA6CA2C02B79B23861FB3F985D245307000000A0721500781D000000701400741D000082090100000000002E0901003009010009FBE39DB0FEC712C63D10A98D489A7AAB0846400700000020901500E897000000901400E89700008809010000000000300901003209010007418A8C78F415963F80A5A9E464308D413D30060700000020281600ACB4020000301500F03F01008C090100000000003209010034090100BEC4FEA336329A535D22964166C26FC9118B93EB26000000E0DC180080710000007016006C71000092090100000000003409010036090100C8C9142DCCBD31BDCA9A2D94E0ACC47C2048D61516000000604E1900EC13000000F01600EC130000980901000000000036090100380901001738DA1C0F4234ADFBF6846178573D6480DA7D3307000000606219005C040000001017005C0400009F09010000000000380901003A090100C30E38F79B11C31FDC254E0BE103DF8DE19C759907000000C0661900701D000000201700701D0000A8090100000000003A0901003C090100386FD794D32434FC9D81FC24082EEBFF956DF78C0A00000040841900706900000040170070690000AF090100000000003C0901003E090100D2F9E272F9AF5D2E6015E5B1F93F70EE4641E6A109000000C0ED190076AF000000B0170076AF0000B4090100000000003E090100400901000939C1222F8EE32A45A62AB9FC3599FF7B2BDF5209000000409D1A00FE90000000601800FE900000C10901000000000040090100420901009BFE4CF251FAFAB330C7ED757E9EF69C2F28B06909000000402E1B00A8BA000000001900A8BA0000CF0901000000000042090100440901001CB60A80A2064D6C2410FF495A805FD802EAD4FA0900000000E91B00B2BA000000C01900B2BA0000DB0901000000000044090100460901002419B2B0C43C7C7F5B7E4BF5FAB5374BAC755B2A09000000C0A31C00A0B9000000801A00A0B90000E70901000000000046090100480901007D6611F78514F60A89220E70939B5DE1A8DB0BAC09000000605D1D00C8B5000000401B00C8B50000F409010000000000480901004A09010072FA6B132CB1FD40996531D3F5A1111A04C2E4F00900000040131E00028C000000001C00028C0000010A0100000000004A0901004C0901004E4FB4E76A1A6935442549214E2C8327BB96893309000000609F1E003A81000000901C003A8100000D0A0100000000004C0901004E090100A166ED7F85F0BDEC78D4C5454F7708B7DA31570009000000A0201F00D4B5000000201D00D4B500001B0A0100000000004E090100500901005671EF6B923682EABB2CB6E49B25D71BEFB68AF30800000080D61F00A008000000E01D00A00800002B0A01000000000050090100520901001AEDA73A30C27FAAE259AB82A3F562A5D9E9A30D0700000000E01F000004000000F01D0000040000320A0100000000005209010054090100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002E7465787400443344580044534F554E4400574D4144454300584F4E4C494E4500584E45540044334400584752504800585050002E6461746100444F4C425900584F4E5F5244004456445448554E4B002E6461746131005849505300456E676C697368586C617465004A6170616E657365586C617465004765726D616E586C617465004672656E6368586C617465005370616E697368586C617465004974616C69616E586C617465004B6F7265616E586C61746500544368696E657365586C61746500506F7274756775657365586C617465002E58544C4944002E6861636B7300000000584150494C49420001000000D9160140584150494C42500001000000D916012058424F584441534801000100481701504C4942434D54000001000000D916014058424F584B524E4C01000000D9160140443344583800000001000000D916014044534F554E44000001000000D916014058564F494345000001000000D9160140584B45590000000001000000D9160120584F4E4C494E455301000000D91603204C494243504D540001000000D9160140443344380000000001000000D9160140443344384C54434701000000D9160140584752415048434C01000000D9160140780062006F00780064006100730068005F0032003000300032002E006500780065000000643A5C78626F785C707269766174655C75695C786170705C6F626A5C693338365C78626F78646173685F323030322E65786500000733AD030753AD03A903EA000373A7033200B3FD030503FDD343F9EA0003E3F93347332200FF030573FD7373A773EA0073F7D373E3F7430F03FF0305FF332E00037A00035200F93303F9030F33FF030343FF13F913050393A3F7730773A7632353A30513A3D3F77303071373E3F7E3A3130903A3F7E39313054353F95373F9B333032363030593FF4303E3FF33F9030343E3FDB305F9D3F5D303A3FFE303037326F0130523E3FF630336F0132333C36305D3FF45FFA393F7E3036326F04303FF73D3F9B3F97313F95313E3F7930323F9E3D3F9D336F00323638363030322F043E3FF43D3F79323F9A30323F7B33332F0130353F7E343F7E305A3F7A303E3F7D305D3F7C303D3F75303F913071333030313F9D3F7A3F7D3F913F943D3F9030303F993F9E32323F9730503F963F9D34313030773F92305A3F903F92303F9030F53F973FDE3B3F913F923F9730FD3F7E3030393F9030533F913E3FFD30303E3F7D307A3F913F90343F7D32200A3F923FD43E3F7D343F943F9030D03F95305F9E30743F90303B3FFE303F9A307D3F953F7E30393F7932200FB03FBE303F973A3F7D343F90305030723F90305F9A307D3F39323090333D3FB23F9430513F973B3F79303E3F7330F03F9D303FB4323F933E3F79343F90513F963F907F9B3054322F0430573F923F97305A3F913F94303F9030F33F97303F9E30353F913F94313F97343E3F773B3F7B30793F96323E3F7D3F9A30303A3F903D3F7E32373F96303F90313F9B3A30D73F92303F94303B3F923F90303D322F0B303F973071326F0E303D32AF073032326F0A30343F90343FB930DD3F90303F7E305F9A353F90503E3FDB30303F9330923FF73030303E322F0630763FDE34305A3F7B305D3F9330D4B0547130303491373A793070343A3C3A37313051349030B0353D3F3E3731309034373A3D3F3E373130B03235347030749130749030D'},
	# Jump to patch data
	{'apply': '1', 'address': '0x0001DCA2', 'patch_type': '0', 'flip': '0', 'value': 'CA031D00E855FDFFFF31C0C3'},
	# Add patch space (F & G HDD0 - E, F, G HDD1)
	{"apply": "1", "address": 0x001DF000, "patch_type": "5", "flip": "0", "value": '0000|4096'},
	{"apply": "1", "address": 0x001DF000, "patch_type": "0", "flip": "0", "value": '5589E583EC2C8B450C8A45080FBE45088D55E98D0D00E11F00891424894C240489442408E8B74EE7FF8D45E98D4DF0890C2489442404FF150C20010083EC088B450C8D4DE0890C2489442404FF150C20010083EC088D4DF08D45E0890C2489442404FF152420010083EC0883C42C5DC35589E56808E11F006A4EE881FFFFFF83C4085D5589E5682AE11F006A4FE86EFFFFFF83C4085D5589E56871E11F006A50E85BFFFFFF83C4085D5589E56894E11F006A51E848FFFFFF83C4085D5589E5684DE11F006A52E835FFFFFF83C4085D31C0C300000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000'},
	{"apply": "1", "address": 0x001DF100, "patch_type": "0", "flip": "0", "value": '5C3F3F5C25633A005C4465766963655C486172646469736B305C506172746974696F6E360000000000005C4465766963655C486172646469736B305C506172746974696F6E37000000000000005C4465766963655C486172646469736B315C506172746974696F6E3100000000000000005C4465766963655C486172646469736B315C506172746974696F6E36000000000000005C4465766963655C486172646469736B315C506172746974696F6E37'}
]
	
other_patches = [
	# 64MB limit disabled
	{'apply': '1', 'address': '0x00000124', 'patch_type': '0', 'flip': '0', 'value': '08'},
	# 720p patch by Team Resurgent (Phantom) (moved to offset 0x001DF344 so it in the hacks section)
	{'apply': '1', 'address': '0x001DF344', 'patch_type': '0', 'flip': '0', 'value': '5589E550518B75088B05E4AE170083F804745AB863C50600FFD083F801754EB81AC50600FFD083E00383F803753FB86BC50600FFD083F802740583F801752E8B462883E0DF83C840894628C70600050000C74604D0020000C7461000000000C7462C00000000C7463000000080595856BBD0821400FFD3C9C204'},
	{'apply': '1', 'address': '0x0001CE70', 'patch_type': '0', 'flip': '0', 'value': 'D0141D00'},
	# Description: Bypass Xip checks and allow external ones
	{'apply': '1', 'address': '0x0002D4C8', 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': '0x0002D520', 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': '0x0002D59D', 'patch_type': '0', 'flip': '0', 'value': '9090'},
	{'apply': '1', 'address': '0x0002D5B8', 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': '0x0002D863', 'patch_type': '0', 'flip': '0', 'value': '909090909090'},
	{'apply': '1', 'address': '0x0002D880', 'patch_type': '0', 'flip': '0', 'value': '9090'},
	{'apply': '1', 'address': '0x0002EB13', 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': '0x0002EBE7', 'patch_type': '0', 'flip': '0', 'value': 'E98400'},
	{'apply': '1', 'address': '0x0002EC54', 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	# No DVD region patch (by sylver) still requires the DVD dongle
	{'apply': '1', 'address': '0x000567F7', 'patch_type': '0', 'flip': '0', 'value': 'EB'},
	{'apply': '1', 'address': '0x00056833', 'patch_type': '0', 'flip': '0', 'value': 'C64424120133C98B87440100008A4C241233D2B20149D2E2F6D2526A4450E802FC12008B8F440100008DB748010000566A4451E857FB12008BD081E20000008081FA00000080894424187527FE442412807C24120676B090909090909090909090909090909090909090909090909090909090'}
]
	
xonline_patches = [
	# 64MB limit disabled
	{'apply': '1', 'address': '0x00000124', 'patch_type': '0', 'flip': '0', 'value': '08'},
	# xonlinedash 720p patch by Team Resurgent (Phantom)
	{'apply': '1', 'address': '0x00000F00', 'patch_type': '0', 'flip': '0', 'value': '56B8FAA90700FFD083F8017552B8B1A90700FFD083E00383F8037543B888A90700FFD083F802740583F80175328B7424148B462883E0DF83C840894628C70600050000C74604D0020000C7461000000000C7462C00000000C74630000000005EB8E0660E00FFD0B92CD70400FFE1B8FAA90700FFD083F801752EB8B1A90700FFD083E00383F803751FB888A90700FFD083F802740583F801750EC745FC0000A044C745F800003444FF35C4351900B8A88E0900FFE0'},
	{'apply': '1', 'address': '0x0003D727', 'patch_type': '0', 'flip': '0', 'value': 'E9D437FCFF'},
	{'apply': '1', 'address': '0x00088EA2', 'patch_type': '0', 'flip': '0', 'value': 'E9C780F7FF'},
	{'apply': '1', 'address': '0x0003D669', 'patch_type': '0', 'flip': '0', 'value': '9090'},
	{'apply': '1', 'address': '0x0006E3D7', 'patch_type': '0', 'flip': '0', 'value': '9090'},
	{'apply': '1', 'address': '0x000833F8', 'patch_type': '0', 'flip': '0', 'value': '90909090909090'},
	{'apply': '1', 'address': '0x00083548', 'patch_type': '0', 'flip': '0', 'value': '9090'},
	{'apply': '1', 'address': '0x0008354D', 'patch_type': '0', 'flip': '0', 'value': '9090'},
	{'apply': '1', 'address': '0x00083587', 'patch_type': '0', 'flip': '0', 'value': '9090'}
]

xonline_colour_patches = [
	# float patches

	# dark green panels (This is a float and we can only change green and blue values as transparency and red are set to 1.0 and 0.0)
	# Colour type: GB
	# Original Bytes: 4FAF143D E6AE253E
	{'name': 'xonline_dark_green_panels', 'apply': '1', 'address': '0x00014730,0x0001472C', 'patch_type': '3', 'float_type': '0GB', 'flip': '0', 'value': '2909'},
	
	# XboxGreen
	# Colour type: RGB
	# Original Bytes: 39B4C83D 7AC7493F 448B0C3F
	{'name': 'xonline_XBoxGreen', 'apply': '1', 'address': '0x00014D84,0x00014D80,0x00014D7C', 'patch_type': '3', 'flip': '0', 'value': '18C88B'},

	# options footer
	# Colour type: RGB
	# Original Bytes: 9E9D1D3F DBDADA3E C3C2423F
	{'name': 'xonline_options_footer', 'apply': '1', 'address': '0x00014808,0x00014804,0x00014800', 'patch_type': '3', 'flip': '0', 'value': '9D6DC2'},

	# float unknown 2
	# Colour type: R
	# Original Bytes: D0D5D63E
	{'name': 'xonline_float_unknown_2', 'apply': '1', 'address': '0x00014C64', 'patch_type': '3', 'float_type': 'R', 'flip': '0', 'value': '6A'},

	# float unknown 3
	# Colour type: R
	# Original Bytes: ADAC2C3F
	{'name': 'xonline_float_unknown_3', 'apply': '1', 'address': '0x000149D8', 'patch_type': '3', 'float_type': 'R', 'flip': '0', 'value': 'AC'},

	# NavType
	# Colour type: ARGB
	# Original Bytes: 21B0323F 7F6ABC3E 48E17A3F 52B83E3F
	{'name': 'xonline_NavType', 'apply': '1', 'address': '0x00014DBC,0x00014DB8,0x00014DB4,0x00014DB0', 'patch_type': '3', 'flip': '0', 'value': 'B15DF9BD'},

	# Material #13
	# Colour type: RGB
	# Original Bytes: C5C4443E B0AF2F3F 8D8C8C3E
	{'name': 'xonline_Material_#13', 'apply': '1', 'address': '0x00014A38,0x00014A34,0x00014A30', 'patch_type': '3', 'flip': '0', 'value': '31AF46'},

	# Pulse_3d_Trans
	# Colour type: RGB
	# Original Bytes: CDCC4C3F DDDC5C3F 2222A23F
	{'name': 'xonline_Pulse_3d_Trans', 'apply': '1', 'address': '0x000149C4,0x000149C0,0x000149BC', 'patch_type': '3', 'flip': '0', 'value': 'CCDCFF'},

	# solid green 1
	# Colour type: RB
	# Original Bytes: 8988083E 8988883D
	{'name': 'xonline_solid_green_1', 'apply': '1', 'address': '0x000148E8,0x000148E4', 'patch_type': '3', 'float_type': 'R1B', 'flip': '0', 'value': '2211'},

	# solid green2
	# Colour type: RGB
	# Original Bytes: A1A0203D C2C1413F 9190903D
	{'name': 'xonline_solid_green2', 'apply': '1', 'address': '0x000148D0,0x000148CC,0x000148C8', 'patch_type': '3', 'flip': '0', 'value': '0AC112'},

	# solid green 3
	# Colour type: RGB
	# Original Bytes: EBEAEA3E E1E0E03D A1A0203D
	{'name': 'xonline_solid_green_3', 'apply': '1', 'address': '0x000148B4,0x000148B0,0x000148D0', 'patch_type': '3', 'flip': '0', 'value': '1C750A'},

	# solid green 4 (This is a float and we can only change green and blue values as transparency and red are set to 1.0 and 0.0)
	# Colour type: GB
	# Original Bytes: DDDC5C3E 9190903D
	{'name': 'xonline_solid_green_4', 'apply': '1', 'address': '0x0001489C,0x000148C8', 'patch_type': '3', 'float_type': '0GB', 'flip': '0', 'value': '3712'},

	# Hex colours

	# Colour type: ARGB
	# Original Bytes: 00C0140A
	{'name': 'xonline_flatsurfaces_back', 'apply': '1', 'address': '0x0008400C', 'patch_type': '0', 'flip': '1', 'value': '0A14C000'},
	# Colour type: ARGB
	# Original Bytes: 6BFFF3FF
	{'name': 'xonline_flatsurfaces_highlights', 'apply': '1', 'address': '0x00084011', 'patch_type': '0', 'flip': '1', 'value': 'FFF3FF6B'},

	# Colour type: ARGB
	# Original Bytes: BCFFFEE4
	{'name': 'xonline_CellEgg/Transition', 'apply': '1', 'address': '0x0008408F', 'patch_type': '0', 'flip': '1', 'value': 'E4FEFFBC'},
	# Colour type: ARGB
	# Original Bytes: 00FFFC
	{'name': 'xonline_CellEgg/Pulse', 'apply': '1', 'address': '0x00084094', 'patch_type': '0', 'flip': '1', 'value': 'FCFF00'},
	
	# Colour type: RGB
	# Original Bytes: 00C014
	{'name': 'xonline_FlatSurfaces2sided3_1', 'apply': '1', 'address': '0x00084156', 'patch_type': '0', 'flip': '1', 'value': '14C000'},
	# Colour type: ARGB
	# Original Bytes: 6BFFF3C0
	{'name': 'xonline_FlatSurfaces2sided3_2', 'apply': '1', 'address': '0x0008415C', 'patch_type': '0', 'flip': '1', 'value': 'C0F3FF6B'},
	
	# Colour type: ARGB
	# Original Bytes: 6BFFF3C0
	{'name': 'xonline_IconParts', 'apply': '1', 'address': '0x000841D2', 'patch_type': '0', 'flip': '1', 'value': 'C0F3FF6B'},
	
	# Colour type: ARGB
	# Original Bytes: 6BFFF3B2
	{'name': 'xonline_CellEgg/Parts', 'apply': '1', 'address': '0x00084242', 'patch_type': '0', 'flip': '1', 'value': 'B2F3FF6B'},
	
	# Colour type: RGB
	# Original Bytes: 12FF00
	{'name': 'xonline_CellWallStructure_1', 'apply': '1', 'address': '0x0008432C', 'patch_type': '0', 'flip': '1', 'value': '00FF12'},
	# Colour type: ARGB
	# Original Bytes: 6BFFBF33
	{'name': 'xonline_CellWallStructure_2', 'apply': '1', 'address': '0x00084331', 'patch_type': '0', 'flip': '1', 'value': '33BFFF6B'},
	
	# Colour type: ARGB
	# Original Bytes: 6BFFF338
	{'name': 'xonline_GamePod', 'apply': '1', 'address': '0x0008439D', 'patch_type': '0', 'flip': '1', 'value': '38F3FF6B'},
	
	# Colour type: ARGB
	# Original Bytes: 6BFF7F77
	{'name': 'xonline_InnerWall_02', 'apply': '1', 'address': '0x00084406', 'patch_type': '0', 'flip': '1', 'value': '777FFF6B'},
	
	# Colour type: ARGB
	# Original Bytes: 14D42814
	{'name': 'xonline_InnerWall_01_1', 'apply': '1', 'address': '0x000844E8', 'patch_type': '0', 'flip': '1', 'value': '1428D414'},
	# Colour type: ARGB
	# Original Bytes: 6BFFF37F
	{'name': 'xonline_InnerWall_01_2', 'apply': '1', 'address': '0x000844ED', 'patch_type': '0', 'flip': '1', 'value': '7FF3FF6B'},
	
	# Colour type: ARGB
	# Original Bytes: E5E5E5FF
	{'name': 'xonline_metal_chrome_1', 'apply': '1', 'address': '0x00084558', 'patch_type': '0', 'flip': '1', 'value': 'FFE5E5E5'},
	# Colour type: RGB
	# Original Bytes: E5E5E5
	{'name': 'xonline_metal_chrome_2', 'apply': '1', 'address': '0x0008455D', 'patch_type': '0', 'flip': '1', 'value': 'E5E5E5'},

	# Colour type: ARGB
	# Original Bytes: 00680725
	{'name': 'xonline_Tubes', 'apply': '1', 'address': '0x000845CF', 'patch_type': '0', 'flip': '1', 'value': '25076800'},
	# Colour type: ARGB
	# Original Bytes: 99FAF2D7
	{'name': 'xonline_Tubes_Hightlight', 'apply': '1', 'address': '0x000845D4', 'patch_type': '0', 'flip': '1', 'value': 'D7F2FA99'},
	
	# Colour type: ARGB
	# Original Bytes: 6BFFF3C0
	{'name': 'xonline_Shell', 'apply': '1', 'address': '0x000846BA', 'patch_type': '0', 'flip': '1', 'value': 'C0F3FF6B'},
	
	# Colour type: RGB
	# Original Bytes: 12FF00
	{'name': 'xonline_FlatSrfc/PodParts_1', 'apply': '1', 'address': '0x00084725', 'patch_type': '0', 'flip': '1', 'value': '00FF12'},
	# Colour type: ARGB
	# Original Bytes: 6BFFBF33
	{'name': 'xonline_FlatSrfc/PodParts_2', 'apply': '1', 'address': '0x0008472A', 'patch_type': '0', 'flip': '1', 'value': '33BFFF6B'},
	
	# Colour type: RGB
	# Original Bytes: 12FF00
	{'name': 'xonline_Cell_Light_1', 'apply': '1', 'address': '0x00084A7A', 'patch_type': '0', 'flip': '1', 'value': '00FF12'},
	# Colour type: ARGB
	# Original Bytes: 6BFFBF33
	{'name': 'xonline_Cell_Light_2', 'apply': '1', 'address': '0x00084A7F', 'patch_type': '0', 'flip': '1', 'value': '33BFFF6B'},
	
	# Colour type: ARGB
	# Original Bytes: 6BFFF3C0
	{'name': 'xonline_dual_buttons_hightlight', 'apply': '1', 'address': '0x00084AF5', 'patch_type': '0', 'flip': '1', 'value': 'C0F3FF6B'},

	# Colour type: ARGB
	# Original Bytes: 00C01414
	{'name': 'xonline_Unknown_10', 'apply': '1', 'address': '0x00084B6A', 'patch_type': '0', 'flip': '1', 'value': '1414C000'},
	# Colour type: ARGB
	# Original Bytes: 6BFFF3FF
	{'name': 'xonline_Unknown_11', 'apply': '1', 'address': '0x00084B6F', 'patch_type': '0', 'flip': '1', 'value': 'FFF3FF6B'},

	# Colour type: ARGB
	# Original Bytes: 00C01410
	{'name': 'xonline_Background', 'apply': '1', 'address': '0x00084BEA', 'patch_type': '0', 'flip': '1', 'value': '1014C000'},

	# Colour type: ARGB
	# Original Bytes: 276F68BB
	{'name': 'xonline_innerwall_01_transition', 'apply': '1', 'address': '0x00084BEF', 'patch_type': '0', 'flip': '1', 'value': 'BB686F27'},

	# Colour type: RGB
	# Original Bytes: 0069EB
	{'name': 'xonline_Unknown_14', 'apply': '1', 'address': '0x00084C57', 'patch_type': '0', 'flip': '1', 'value': '686F27'},
	# Colour type: ARGB
	# Original Bytes: 000CF8FF
	{'name': 'xonline_Unknown_15', 'apply': '1', 'address': '0x00084C5C', 'patch_type': '0', 'flip': '1', 'value': 'FFF80C00'},
	# Colour type: ARGB
	# Original Bytes: 00FF1E20
	{'name': 'xonline_Unknown_16', 'apply': '1', 'address': '0x00084CC4', 'patch_type': '0', 'flip': '1', 'value': '201EFF00'},
	# Colour type: ARGB
	# Original Bytes: 6AECA7FF
	{'name': 'xonline_Unknown_17', 'apply': '1', 'address': '0x00084CC9', 'patch_type': '0', 'flip': '1', 'value': 'FFA7EC6A'},

	# Colour type: ARGB
	# Original Bytes: 0C80E04C
	{'name': 'xonline_EggGlowOange_1', 'apply': '1', 'address': '0x00085492', 'patch_type': '0', 'flip': '1', 'value': '4CE0800C'},
	# Colour type: RGB
	# Original Bytes: 0C64C8
	{'name': 'xonline_EggGlowOange_2', 'apply': '1', 'address': '0x00085497', 'patch_type': '0', 'flip': '1', 'value': 'C8640C'},

	# Colour type: ARGB
	# Original Bytes: 2D4D580C
	{'name': 'xonline_FlatSurfacesOrange_1', 'apply': '1', 'address': '0x00085502', 'patch_type': '0', 'flip': '1', 'value': '0C584D2D'},
	# Colour type: ARGB
	# Original Bytes: 2D99C8AC
	{'name': 'xonline_FlatSurfacesOrange_2', 'apply': '1', 'address': '0x00085507', 'patch_type': '0', 'flip': '1', 'value': 'ACC8992D'},

	# Colour type: RGB
	# Original Bytes: 12FF00
	{'name': 'xonline_Cell_Lighter_1', 'apply': '1', 'address': '0x00085572', 'patch_type': '0', 'flip': '1', 'value': '00FF12'},
	# Colour type: ARGB
	# Original Bytes: 6BFFBFE4
	{'name': 'xonline_Cell_Lighter_2', 'apply': '1', 'address': '0x00085577', 'patch_type': '0', 'flip': '1', 'value': 'E4BFFF6B'},

	# Colour type: RGB
	# Original Bytes: 000CFF
	{'name': 'xonline_FlatSurfaces_Tube_1', 'apply': '1', 'address': '0x00085629', 'patch_type': '0', 'flip': '1', 'value': 'FF0C00'},
	# Colour type: ARGB
	# Original Bytes: 00DEFF6B
	{'name': 'xonline_FlatSurfaces_Tube_2', 'apply': '1', 'address': '0x0008562E', 'patch_type': '0', 'flip': '1', 'value': '6BFFDE00'},

	# Colour type: RGB
	# Original Bytes: 800680
	{'name': 'xonline_Pulse_3d_1', 'apply': '1', 'address': '0x00085674', 'patch_type': '0', 'flip': '1', 'value': '800680'},
	# Colour type: ARGB
	# Original Bytes: 7D6F7D6B
	{'name': 'xonline_Pulse_3d_2', 'apply': '1', 'address': '0x00085679', 'patch_type': '0', 'flip': '1', 'value': '6B7D6F7D'},

	# Colour type: RGB
	# Original Bytes: FF0640
	{'name': 'xonline_Unknown_18', 'apply': '1', 'address': '0x000856BF', 'patch_type': '0', 'flip': '1', 'value': '4006FF'},
	# Colour type: ARGB
	# Original Bytes: FF6F7D6B
	{'name': 'xonline_Unknown_19', 'apply': '1', 'address': '0x000856C4', 'patch_type': '0', 'flip': '1', 'value': '6B7D6FFF'},
	# Colour type: RGB
	# Original Bytes: FF0C00
	{'name': 'xonline_Unknown_20', 'apply': '1', 'address': '0x00085707', 'patch_type': '0', 'flip': '1', 'value': '000CFF'},
	# Colour type: ARGB
	# Original Bytes: FFDE006B
	{'name': 'xonline_Unknown_21', 'apply': '1', 'address': '0x0008570C', 'patch_type': '0', 'flip': '1', 'value': '6B00DEFF'},

	# Colour type: ARGB
	# Original Bytes: BCFFFEE4
	{'name': 'xonline_XBOX_Green_1', 'apply': '1', 'address': '0x00085D79', 'patch_type': '0', 'flip': '1', 'value': 'E4FEFFBC'},
	# Colour type: RGB
	# Original Bytes: 00FFFC
	{'name': 'xonline_XBOX_Green_2', 'apply': '1', 'address': '0x00085D7E', 'patch_type': '0', 'flip': '1', 'value': 'FCFF00'},

	# Colour type: ARGB
	# Original Bytes: BC00FEE4
	{'name': 'xonline_Lensflare_Text_1', 'apply': '1', 'address': '0x00085DF3', 'patch_type': '0', 'flip': '1', 'value': 'E4FE00BC'},
	# Colour type: RGB
	# Original Bytes: 0000FC
	{'name': 'xonline_Lensflare_Text_2', 'apply': '1', 'address': '0x00085DF8', 'patch_type': '0', 'flip': '1', 'value': 'FC0000'},

	# Colour type: ARGB
	# Original Bytes: 00C0140A
	{'name': 'xonline_FlatSurfaces_wire_1', 'apply': '1', 'address': '0x00086059', 'patch_type': '0', 'flip': '1', 'value': '0A14C000'},
	# Colour type: RGB
	# Original Bytes: 3B9C33FF
	{'name': 'xonline_FlatSurfaces_wire_2', 'apply': '1', 'address': '0x0008605E', 'patch_type': '0', 'flip': '1', 'value': 'FF339C3B'},

	# Colour type: ARGB
	# Original Bytes: 525347FF
	{'name': 'xonline_grey_1', 'apply': '1', 'address': '0x00086446', 'patch_type': '0', 'flip': '1', 'value': 'FF475352'},
	# Colour type: RGB
	# Original Bytes: 455347
	{'name': 'xonline_grey_2', 'apply': '1', 'address': '0x0008644B', 'patch_type': '0', 'flip': '1', 'value': '475345'},

	# Colour type: ARGB
	# Original Bytes: 404040FF
	{'name': 'xonline_grill_grey', 'apply': '1', 'address': '0x000864C0', 'patch_type': '0', 'flip': '1', 'value': 'FF404040'},

	# Colour type: ARGB
	# Original Bytes: 043616FF
	{'name': 'xonline_green_falloff_1', 'apply': '1', 'address': '0x0008653B', 'patch_type': '0', 'flip': '1', 'value': 'FF163604'},
	# Colour type: ARGB
	# Original Bytes: 000000FF
	{'name': 'xonline_green_falloff_2', 'apply': '1', 'address': '0x00086540', 'patch_type': '0', 'flip': '1', 'value': 'FF000000'}
]

colour_patches = [
	# Description: Pulse highlight
	# Colour type: RGB
	# Original Bytes: 00D0B2
	{'name': 'pulse_highlight', 'apply': '1', 'address': '0x00046FAC', 'patch_type': '0', 'flip': '1', 'value': 'B2D000'},

	# Description: Pulsing Selection/focus
	# Colour type: RGB
	# Original Bytes: 80FFFF
	{'name': 'pulsing_selection', 'apply': '1', 'address': '0x00046FE1', 'patch_type': '0', 'flip': '1', 'value': 'FFFF80'},

	# Description: Button press (A, B etc...) indicator
	# Colour type: RGB
	# Original Bytes: BCFFFE
	{'name': 'egg_button_press', 'apply': '1', 'address': '0x000470A8', 'patch_type': '0', 'flip': '1', 'value': 'FEFFBC'},

	# Description: Keyboard button focus
	# Colour type: RGB
	# Original Bytes: 00D0B2
	{'name': 'kb_button_focus', 'apply': '1', 'address': '0x00047177', 'patch_type': '0', 'flip': '1', 'value': 'B2D000'},

	# Description: Keyboard button focus highlight
	# Colour type: ARGB
	# Original Bytes: 00D0B2FF
	{'name': 'kb_button_focus_highlight', 'apply': '1', 'address': '0x0004717E', 'patch_type': '0', 'flip': '1', 'value': 'FFB2D000'},

	# Description: Keyboard button no focus highlight
	# Colour type: ARGB
	# Original Bytes: 6BFFF3FF
	{'name': 'kb_button_large_no_focus_highlight', 'apply': '1', 'address': '0x000471F6', 'patch_type': '0', 'flip': '1', 'value': 'FFF3FF6B'},

	# Description: Keyboard left side left no focus buttons
	# Colour type: RGB
	# Original Bytes: 00E119
	{'name': 'kb_button_large_no_focus', 'apply': '1', 'address': '0x000471FD', 'patch_type': '0', 'flip': '1', 'value': '19E100'},

	# Description: Keyboard button left no focus highlight
	# Colour type: ARGB
	# Original Bytes: 6BFFF3DE
	{'name': 'kb_button_no_focus_highlight', 'apply': '1', 'address': '0x00047206', 'patch_type': '0', 'flip': '1', 'value': 'DEF3FF6B'},

	# Description: Keyboard no focus button back
	# Colour type: RGB
	# Original Bytes: 00C014
	{'name': 'kb_button_no_focus', 'apply': '1', 'address': '0x0004720D', 'patch_type': '0', 'flip': '1', 'value': '14C000'},

	# Description: Keyboard no focus letters
	# Colour type: RGB
	# Original Bytes: 5EFABE
	{'name': 'kb_letters_no_focus', 'apply': '1', 'address': '0x00047278', 'patch_type': '0', 'flip': '1', 'value': 'BEFA5E'},

	# Description: InnerWall_01 (transition)
	# Colour type: ARGB
	# Original Bytes: 6BFFF3FF
	{'name': 'innerwall_01_transition', 'apply': '1', 'address': '0x00047CC1', 'patch_type': '0', 'flip': '1', 'value': 'FFF3FF6B'},

	# Description: InnerWall_01
	# Colour type: ARGB
	# Original Bytes: 14D42814
	{'name': 'innerwall_01', 'apply': '1', 'address': '0x00047CF8', 'patch_type': '0', 'flip': '1', 'value': '1428D414'},

	# Description: InnerWall_02
	# Colour type: ARGB
	# Original Bytes: 00C01414
	{'name': 'innerwall_02', 'apply': '1', 'address': '0x00047D3F', 'patch_type': '0', 'flip': '1', 'value': '1414C000'},

	# Description: Material #1334
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C7400400980200895808C700F48B02
	# Original Bytes: C6400C0BC6400D2088580EC6400FC0
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00047F6A', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C7400400980200895808C700F48B0200'},
	{'name': 'material_#1334', 'apply': '1', 'address': '0x00047F8F', 'patch_type': '2', 'flip': '0', 'value': 'FF0b2000'},

	# Description: XBOXgreendark
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004E4970200895808C700F48B02
	# Original Bytes: C6400C06C6400D2188580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00047F29', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004E4970200895808C700F48B0200'},
	{'name': 'xboxgreendark', 'apply': '1', 'address': '0x00047F4E', 'patch_type': '2', 'flip': '0', 'value': 'FF062100'},

	# Description: XBOXgreen (Sub Menu A/B lower text)
	# Colour type: RGB
	# Original Bytes: C6400C8CC6400DC9C6400E19
	{'name': 'egg_xboxgreen_sub', 'apply': '1', 'address': '0x00047FD1', 'patch_type': '1', 'flip': '0', 'value': '8CC919'},

	# Description: XBoxGreen2
	# Colour type: RGB
	# Original Bytes: C6400C8BC6400DC8C6400E18
	{'name': 'xboxgreen2', 'apply': '1', 'address': '0x00048013', 'patch_type': '1', 'flip': '0', 'value': '8BC818'},

	# Description: GameHilite33
	# Colour type: RGB
	# Original Bytes: C6400CDDC6400DD0C6400E78
	{'name': 'gamehilite33', 'apply': '1', 'address': '0x00048055', 'patch_type': '1', 'flip': '0', 'value': 'DDD078'},

	# Description: Nothing
	# Colour type: RGB
	# Original Bytes: C6400C80C6400D80C6400E80
	{'name': 'nothing', 'apply': '1', 'address': '0x00048097', 'patch_type': '1', 'flip': '0', 'value': '808080'},

	# Description: NavType (Text)
	# Colour type: RGB
	# Original Bytes: C6400CBEC6400DFAC6400E5E
	{'name': 'navtype', 'apply': '1', 'address': '0x000480D9', 'patch_type': '1', 'flip': '0', 'value': 'BEFA5E'},

	# Description: OrangeNavType
	# Colour type: RGB
	# Original Bytes: C6400CF9C6400D98C6400E19
	{'name': 'orangenavtype', 'apply': '1', 'address': '0x0004811B', 'patch_type': '1', 'flip': '0', 'value': 'F99819'},

	# Description: XBOXGreen (Main menu A/B lower text)
	# Colour type: RGB
	# Original Bytes: C6400C8BC6400DC8C6400E18
	{'name': 'egg_xboxgreen', 'apply': '1', 'address': '0x0004819F', 'patch_type': '1', 'flip': '0', 'value': '8BC818'},

	# Description: Type
	# Colour type: RGB
	# Original Bytes: C6400C64C6400DC8C6400E19
	{'name': 'type', 'apply': '1', 'address': '0x000481E1', 'patch_type': '1', 'flip': '0', 'value': '64C819'},

	# Description: Typesdsafsda
	# Colour type: RGB
	# Original Bytes: C6400CFFC6400DFFC6400EFF
	{'name': 'typesdsafsda', 'apply': '1', 'address': '0x00048223', 'patch_type': '1', 'flip': '0', 'value': 'FFFFFF'},

	# Description: Material #1335
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004D8960200895808C700F48B02
	# Original Bytes: C6400C4CC6400DA288580EC6400FC8
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x0004827E', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004D8960200895808C700F48B0200'},
	{'name': 'material_#1335', 'apply': '1', 'address': '0x000482A3', 'patch_type': '2', 'flip': '0', 'value': 'FF4CA200'},

	# Description: Material #133511
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004B4960200895808C700F48B02
	# Original Bytes: C6400C29C6400D5788580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x000482BF', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004B4960200895808C700F48B0200'},
	{'name': 'material_#133511', 'apply': '1', 'address': '0x000482E4', 'patch_type': '2', 'flip': '0', 'value': 'FF295700'},
	
	# Description: HilightedType
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C7400478960200895808C700F48B02
	# Original Bytes: C6400CCCC6400D33C6400EFFC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x0004833F', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C7400478960200895808C700F48B0200'},
	{'name': 'hilightedtype', 'apply': '1', 'address': '0x00048364', 'patch_type': '2', 'flip': '0', 'value': 'FF032C00'},

	# Description: XBoxGreenq
	# Colour type: RGB
	# Original Bytes: C6400C8BC6400DC8C6400E18
	{'name': 'xboxgreenq', 'apply': '1', 'address': '0x000483A6', 'patch_type': '1', 'flip': '0', 'value': '8BC818'},

	# Description: CellEgg/Partsw
	# Colour type: RGB
	# Original Bytes: C6400C4DC6400DE0C6400E39
	{'name': 'cellegg_partsw', 'apply': '1', 'address': '0x00048466', 'patch_type': '1', 'flip': '0', 'value': '4DE039'},

	# Description: Material #108
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004E8950200897808C700F48B02
	# Original Bytes: C6400CA0C6400DFC88580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x000484C3', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004E8950200897808C700F48B0200'},
	{'name': 'material_#108', 'apply': '1', 'address': '0x000484E8', 'patch_type': '2', 'flip': '0', 'value': 'FFA0FC00'},

	# Description: ItemsType (UIX Config Title)
	# Colour type: RGB
	# Original Bytes: C6400CB6C6400DF5C6400E60
	{'name': 'itemstype', 'apply': '1', 'address': '0x0004852A', 'patch_type': '1', 'flip': '0', 'value': 'B6F560'},

	# Description: GameHiliteMemory
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004B0950200895808C700F48B02
	# Original Bytes: C6400CB2C6400DD088580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00048546', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004B0950200895808C700F48B0200'},
	{'name': 'gamehilitememory', 'apply': '1', 'address': '0x0004856B', 'patch_type': '2', 'flip': '0', 'value': 'FFB2D000'},

	# Description: white
	# Colour type: RGB
	# Original Bytes: C6400CFFC6400DFFC6400EFF
	{'name': 'white', 'apply': '1', 'address': '0x000485ED', 'patch_type': '1', 'flip': '0', 'value': 'FFFFFF'},

	# Description: solid green 1
	# Colour type: RGB
	# Original Bytes: C6400C11C6400DFFC6400E22
	{'name': 'solid_green_1', 'apply': '1', 'address': '0x00048634', 'patch_type': '1', 'flip': '0', 'value': '11FF22'},

	# Description: solid green 2
	# Colour type: RGB
	# Original Bytes: C6400C12C6400DC1C6400E0A
	{'name': 'solid_green_2', 'apply': '1', 'address': '0x00048676', 'patch_type': '1', 'flip': '0', 'value': '12C10A'},

	# Description: solid green 3
	# Colour type: RGB
	# Original Bytes: C6400C0AC6400D75C6400E1C
	{'name': 'solid_green_3', 'apply': '1', 'address': '0x000486B8', 'patch_type': '1', 'flip': '0', 'value': '0A751C'},

	# Description: solid green 4
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C740042C950200897008C700F48B02
	# Original Bytes: C6400C12C6400D3788580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x000486D4', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C740042C950200897008C700F48B0200'},
	{'name': 'solid_green_4', 'apply': '1', 'address': '0x000486F9', 'patch_type': '2', 'flip': '0', 'value': 'FF123700'},

	# Description: dark green panels
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C7400408950200897008C700F48B02
	# Original Bytes: C6400C09C6400D2988580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00048715', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C7400408950200897008C700F48B0200'},
	{'name': 'dark_green_panels', 'apply': '1', 'address': '0x0004873A', 'patch_type': '2', 'flip': '0', 'value': 'FF092900'},

	# Description: FlatSurfaces (Panel highlights)
	# Colour type: ARGB
	# Original Bytes: 6BFFF3C0
	{'name': 'flatsurfaces_highlights', 'apply': '1', 'address': '0x0004875A', 'patch_type': '0', 'flip': '1', 'value': 'C0F3FF6B'},
	# Description: FlatSurfaces (Menu Button Backs)
	# Colour type: RGB
	# Original Bytes: 00C014
	{'name': 'flatsurfaces_back', 'apply': '1', 'address': '0x0004875F', 'patch_type': '0', 'flip': '1', 'value': '14C000'},

	# Description: FlatSurfacesSelected (Menu header text colour)
	# Colour type: ARGB
	# Original Bytes: 00FF1E80
	{'name': 'flatsurfacesselected', 'apply': '1', 'address': '0x000487DA', 'patch_type': '0', 'flip': '1', 'value': '801EFF00'},

	# Description: FlatSurfacesMemory (Memory menu save back panel)
	# Colour type: ARGB
	# Original Bytes: 00A01E80
	{'name': 'flatsurfacesmemory', 'apply': '1', 'address': '0x0004882D', 'patch_type': '0', 'flip': '1', 'value': '801EA000'},

	# Description: DarkSurfaces
	# Colour type: ARGB
	# Original Bytes: 55CDCB5A
	{'name': 'darksurfaces', 'apply': '1', 'address': '0x00048859', 'patch_type': '0', 'flip': '1', 'value': '5ACBCD55'},

	# Description: dark green panels falloff
	# Colour type: ARGB
	# Original Bytes: 052305FF
	{'name': 'dark_green_panels_falloff', 'apply': '1', 'address': '0x0004905D', 'patch_type': '0', 'flip': '1', 'value': 'FF052305'},

	# Description: grey
	# Colour type: RGB
	# Original Bytes: 455347
	{'name': 'grey_1', 'apply': '1', 'address': '0x000490AD', 'patch_type': '0', 'flip': '1', 'value': '475345'},
	# Description: grey
	# Colour type: ARGB
	# Original Bytes: 526456FF
	{'name': 'grey_2', 'apply': '1', 'address': '0x000490B4', 'patch_type': '0', 'flip': '1', 'value': 'FF566452'},

	# Description: grill grey
	# Colour type: RGB
	# Original Bytes: 202020
	{'name': 'grill_grey_1', 'apply': '1', 'address': '0x00049104', 'patch_type': '0', 'flip': '1', 'value': '202020'},
	# Description: grill grey
	# Colour type: ARGB
	# Original Bytes: 404040FF
	{'name': 'grill_grey_2', 'apply': '1', 'address': '0x0004910B', 'patch_type': '0', 'flip': '1', 'value': 'FF404040'},

	# Description: Wireframe
	# Colour type: ARGB
	# Original Bytes: 03000000
	{'name': 'wireframe_1', 'apply': '1', 'address': '0x0004915C', 'patch_type': '0', 'flip': '1', 'value': '00000003'},
	# Description: Wireframe
	# Colour type: ARGB
	# Original Bytes: 22C67D64
	{'name': 'wireframe_2', 'apply': '1', 'address': '0x00049163', 'patch_type': '0', 'flip': '1', 'value': '647DC622'},
	# Description: Wireframe
	# Colour type: RGB
	# Original Bytes: 22C67D
	{'name': 'wireframe_3', 'apply': '1', 'address': '0x0004916A', 'patch_type': '0', 'flip': '1', 'value': '7DC622'},

	# Description: Tubes (behind button tubes)
	# Colour type: ARGB
	# Original Bytes: 99FAF2D7
	{'name': 'tubes_1', 'apply': '1', 'address': '0x0004917E', 'patch_type': '0', 'flip': '1', 'value': 'D7F2FA99'},
	# Description: Tubes (lower tubes)
	# Colour type: ARGB
	# Original Bytes: 00680725
	{'name': 'tubes_2', 'apply': '1', 'address': '0x00049183', 'patch_type': '0', 'flip': '1', 'value': '25076800'},

	# Description: MemoryHeader (Memory menu save back panel header outline)
	# Colour type: ARGB
	# Original Bytes: 43C63C7A
	{'name': 'memoryheader', 'apply': '1', 'address': '0x00049392', 'patch_type': '0', 'flip': '1', 'value': '7A3CC643'},

	# Description: MemoryHeaderHilite (Memory menu save panel header hightlight)
	# Colour type: ARGB
	# Original Bytes: 00E8C7F0
	{'name': 'memoryheaderhilite_1', 'apply': '1', 'address': '0x000493E9', 'patch_type': '0', 'flip': '1', 'value': 'F0C7E800'},
	# Description: MemoryHeaderHilite (Memory menu save back panel header no focus)
	# Colour type: ARGB
	# Original Bytes: 00726182
	{'name': 'memoryheaderhilite_2', 'apply': '1', 'address': '0x000493F0', 'patch_type': '0', 'flip': '1', 'value': '82617200'},

	# Description: EggGlow
	# Colour type: ARGB
	# Original Bytes: BCFFFEE4
	{'name': 'eggglow_1', 'apply': '1', 'address': '0x0004941E', 'patch_type': '0', 'flip': '1', 'value': 'E4FEFFBC'},
	# Description: EggGlow
	# Colour type: RGB
	# Original Bytes: 00FFFC
	{'name': 'eggglow_2', 'apply': '1', 'address': '0x00049423', 'patch_type': '0', 'flip': '1', 'value': 'FCFF00'},

	# Description: Gradient
	# Colour type: ARGB
	# Original Bytes: 6BFFBF33
	{'name': 'gradient_1', 'apply': '1', 'address': '0x000494D2', 'patch_type': '0', 'flip': '1', 'value': '33BFFF6B'},
	# Description: Gradient
	# Colour type: RGB
	# Original Bytes: 12FF00
	{'name': 'gradient_2', 'apply': '1', 'address': '0x000494D7', 'patch_type': '0', 'flip': '1', 'value': '00FF12'},
	
	# Description: CellEgg/Parts
	# Colour type: ARGB
	# Original Bytes: 6BFFF3B2
	{'name': 'cellegg_parts_1', 'apply': '1', 'address': '0x0004966C', 'patch_type': '0', 'flip': '1', 'value': 'B2F3FF6B'},
	# Description: CellEgg/Parts
	# Colour type: RGB
	# Original Bytes: 00FF1E
	{'name': 'cellegg_parts_2', 'apply': '1', 'address': '0x00049671', 'patch_type': '0', 'flip': '1', 'value': '1EFF00'},

	# Description: FlatSurfaces2sided3
	# Colour type: ARGB
	# Original Bytes: 001EFDFF
	{'name': 'flatsurfaces2sided3_1', 'apply': '1', 'address': '0x0004973C', 'patch_type': '0', 'flip': '1', 'value': 'FFFD1E00'},
	# Description: FlatSurfaces2sided3
	# Colour type: RGB
	# Original Bytes: 001CF2
	{'name': 'flatsurfaces2sided3_2', 'apply': '1', 'address': '0x00049743', 'patch_type': '0', 'flip': '1', 'value': 'F21C00'},

	# Description: console_hilite
	# Colour type: ARGB
	# Original Bytes: 6BADFFFF
	{'name': 'console_hilite_1', 'apply': '1', 'address': '0x00049793', 'patch_type': '0', 'flip': '1', 'value': 'FFFFAD6B'},
	# Description: console_hilite
	# Colour type: RGB
	# Original Bytes: 00FFF6
	{'name': 'console_hilite_2', 'apply': '1', 'address': '0x0004979A', 'patch_type': '0', 'flip': '1', 'value': 'F6FF00'},

	# Description: Metal_Chrome (highlight shading)
	# Colour type: RGB
	# Original Bytes: E5E5E5
	{'name': 'metal_chrome_1', 'apply': '1', 'address': '0x00049AAF', 'patch_type': '0', 'flip': '1', 'value': 'E5E5E5'},
	# Description: Metal_Chrome (hightlight)
	# Colour type: ARGB
	# Original Bytes: E5E5E5FF
	{'name': 'metal_chrome_2', 'apply': '1', 'address': '0x00049AB4', 'patch_type': '0', 'flip': '1', 'value': 'FFE5E5E5'},

	# Description: PanelBacking_01
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004FC8D0200895808C700F88B02
	# Original Bytes: C6400C04C6400D1488580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00049B36', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004FC8D0200895808C700F88B0200'},
	{'name': 'panelbacking_01', 'apply': '1', 'address': '0x00049B5B', 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: PanelBacking_03
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004BC8D0200895808C700F88B0200
	# Original Bytes: C6400C04C6400D1488580EC6400FF0
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00049BB6', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004BC8D0200895808C700F88B0200'},
	{'name': 'panelbacking_03', 'apply': '1', 'address': '0x00049BDB', 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: PanelBacking_04 (Music Menu)
	# Colour type: RGB
	# Original Bytes: C6400C0EC6400D2EC6400E07
	{'name': 'panelbacking_04', 'apply': '1', 'address': '0x00049C1D', 'patch_type': '1', 'flip': '0', 'value': '0E2E07'},

	# Description: DarkenBacking
	# Original Bytes: 74378B0D54E7170089048DB8C6170041890D54E71700C74004808D0200C740080A000000C700F88B02
	# Original Bytes: C6400C04C6400D3288580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00049C39', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004808D0200C740080A000000C700F88B0200'},
	{'name': 'darkenbacking', 'apply': '1', 'address': '0x00049C62', 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: button
	# Colour type: RGB
	# Original Bytes: C6400C9DC6400D6DC6400EC2
	{'name': 'button', 'apply': '1', 'address': '0x00049CEC', 'patch_type': '1', 'flip': '0', 'value': '9D6DC2'},

	# Description: image
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C740044C8D0200897008C700EC8B02
	# Original Bytes: C6400C04C6400D1488580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00049D08', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C740044C8D0200897008C700EC8B0200'},
	{'name': 'image', 'apply': '1', 'address': '0x00049D2D', 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: live header
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C74004348D0200897008C700EC8B02
	# Original Bytes: C6400C04C6400D1488580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00049D49', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C74004348D0200897008C700EC8B0200'},
	{'name': 'live_header', 'apply': '1', 'address': '0x00049D6E', 'patch_type': '2', 'flip': '0', 'value': 'FF041400'},

	# Description: highlight
	# Colour type: RGB
	# Original Bytes: C6400C9DC6400D6DC6400EC2
	{'name': 'highlight', 'apply': '1', 'address': '0x00049DB0', 'patch_type': '1', 'flip': '0', 'value': '9D6DC2'},

	# Description: footer
	# Colour type: RGB
	# Original Bytes: 9D6DC2
	{'name': 'footer', 'apply': '1', 'address': '0x00049DF2', 'patch_type': '1', 'flip': '0', 'value': '9D6DC2'},

	# Description: LiveChrome
	# Colour type: RGB
	# Original Bytes: 9D6DC2
	{'name': 'livechrome', 'apply': '1', 'address': '0x00049E34', 'patch_type': '1', 'flip': '0', 'value': '9D6DC2'},

	# Description: GameHilite
	# Colour type: RGB
	# Original Bytes: FFFFFF
	{'name': 'gamehilite', 'apply': '1', 'address': '0x00049EAA', 'patch_type': '1', 'flip': '0', 'value': 'FFFFFF'},

	# Description: PanelBacking
	# Original Bytes: 74338B0D54E7170089048DB8C6170041890D54E71700C740045C8C0200897008C700EC8B02
	# Original Bytes: C6400C04C6400D1488580EC6400FFF
	# Colour type: ARGB
	{'name': '', 'apply': '1', 'address': '0x00049F6B', 'patch_type': '0', 'flip': '0', 'value': '908B0D54E7170089048DB8C6170041890D54E71700C740045C8C0200897008C700EC8B0200'},
	{'name': 'panelbacking', 'apply': '1', 'address': '0x00049F90', 'patch_type': '2', 'flip': '0', 'value': 'FF041400'}
]

class ColourizerUI:
	def __init__(self, root, args=None):
		
		self.process_script = False
		self.root = root
		self.root.title("")
		self.export_button = None

		# Load config
		self.config = configparser.ConfigParser()
		self.config.read("settings.ini")

		# Check if FTP is enabled
		ftp_enabled = self.config.getboolean("FTP", "enable", fallback=False)
		self.ftp_enabled = tk.BooleanVar(value=ftp_enabled)
		self.ftp_ip = tk.StringVar(value=self.config.get("FTP", "server", fallback=""))

		# Target colour and brightness default vars
		self.target_colour = args.target_colour if args and args.target_colour else "FFFFFF"
		self.brightness_factor = args.brightness_factor if args and args.brightness_factor else 1.0
		self.colour_file = args.colour_file if args and args.colour_file else ""

		# Main layout frame
		self.main_frame = tk.Frame(root)
		self.main_frame.pack(anchor="w", padx=20, pady=20)
		
		# Title
		tk.Label(self.main_frame, text="Xboxdash 5960 Colourizer", font=("Arial", 14, "bold")).pack(anchor="center", pady=5)
		
		transfer_button_initial_color = '#90EE90' if ftp_enabled else '#ADD8E6'

		# Create a frame to hold both sections side by side
		self.info_ftp_frame = tk.Frame(self.main_frame)
		self.info_ftp_frame.pack(fill="x", pady=5)

		# Skin Info Panel (Left side)
		self.info_frame = tk.LabelFrame(self.info_ftp_frame, text="Skin Info", padx=5, pady=5)
		self.info_frame.pack(side="left", fill="both", expand=True, padx=5)

		self.loaded_skin_label = tk.Label(self.info_frame, text="Loaded:\nNone", font=("Arial", 10), justify="left", anchor="w")
		self.loaded_skin_label.pack(fill="x", pady=2)
		self.base_colour_skin_label = tk.Label(self.info_frame, text="Base Colour:\nNone", font=("Arial", 10), justify="left", anchor="w")
		self.base_colour_skin_label.pack(fill="x", pady=2)

		# FTP Setting Panel (Right side)
		self.ftp_frame = tk.LabelFrame(self.info_ftp_frame, text="FTP Settings", padx=5, pady=5)
		self.ftp_frame.pack(side="left", fill="both", expand=True, padx=5)

		# Radio buttons and IP entry
		tk.Radiobutton(self.ftp_frame, text="Enabled", variable=self.ftp_enabled, value=True, command=self.with_refresh_gui(self.update_ftp_status)).pack(anchor="w")
		tk.Radiobutton(self.ftp_frame, text="Disabled", variable=self.ftp_enabled, value=False, command=self.with_refresh_gui(self.update_ftp_status)).pack(anchor="w")

		ftp_entry_frame = tk.Frame(self.ftp_frame)
		ftp_entry_frame.pack(fill="x", pady=2)
		self.ftp_ip_entry = tk.Entry(ftp_entry_frame, textvariable=self.ftp_ip, width=20)
		self.ftp_ip_entry.pack(side="left", padx=5)

		# Save button (hidden unless IP is changed)
		self.save_ip_button = tk.Button(ftp_entry_frame, text="Save", command=self.with_refresh_gui(self.save_ftp_ip), relief="ridge")
		self.save_ip_button.pack(side="right", padx=5)
		self.save_ip_button.pack_forget()
		self.ftp_ip_entry.bind("<KeyRelease>", self.show_save_button)

		# Hide IP box if FTP is disabled on startup
		if not ftp_enabled:
			ftp_entry_frame.pack_forget()

		# Options Buttons Panel
		self.menu_frame = tk.LabelFrame(self.main_frame, text="Options", padx=5, pady=5)
		self.menu_frame.pack(fill="x", pady=5)

		button_container = tk.Frame(self.menu_frame)
		button_container.pack(anchor="center", pady=5)

		# Buttons start
		tk.Button(button_container, text="Load Skin ini", command=self.with_refresh_gui(self.load_ini), width=25, relief="ridge").pack(pady=2)
		self.base_colour_button = tk.Button(button_container, text="Custom Base Colour", command=self.with_refresh_gui(self.set_base_colour), width=25, relief="ridge").pack(pady=2)
		self.transfer_button = tk.Button(button_container, text="Transfer to Xbox" if self.ftp_enabled.get() else "Save", command=self.send_to_xbox, width=25, bg=transfer_button_initial_color, relief="ridge")
		self.transfer_button.pack(pady=2)
		tk.Button(button_container, text="Restore Stock Files", command=self.restore_stock_theme, width=25, fg="white", bg="#B73B3D", relief="ridge").pack(pady=5)

	def with_refresh_gui(self, command):
		return lambda: [command(), self.refresh_gui()]

	def refresh_gui(self):
		# def delay():
		self.root.geometry("")
		self.root.update_idletasks()

		width = self.root.winfo_width()
		height = self.root.winfo_height()
		screen_width = self.root.winfo_screenwidth()
		screen_height = self.root.winfo_screenheight()

		x = (screen_width // 2) - (width // 2)
		y = (screen_height // 2) - (height // 2)

		self.root.geometry(f"{width}x{height}+{x}+{y}")
		# self.root.after(1, delay)

	def export(self):
		exported_name = simpledialog.askstring("", "Enter a name for exported file:")
		if exported_name is None:
			pass
		elif exported_name.strip() == "":
			messagebox.showinfo("ERROR", "The name can't be blank")
			self.export()
		else:
			target_colour = self.target_colour
			brightness_factor = self.brightness_factor
			
			if exported_name.lower() == "stock":
				target_colour = exported_name.lower()
				brightness_factor = 1.0
			
			export_colours = []
			patches = colour_patches + xonline_colour_patches
			for data in patches:
				float_type = data.get('float_type', '')
				name, value = data['name'], data['value']
				if name:
					if len(value) in [2, 4, 6, 8] and exported_name.lower() != 'stock':
						value = adjust_colour_to_target(value, target_colour, float_type, float(brightness_factor))
					export_colours.append('{}={}'.format(name, value))

			with open('{} ({}).ini'.format(exported_name, target_colour), "w") as file:
				template = '[These are mandatory]\ncolour=%s\nbrightness=%s\n[Override colours with your own]'
				file.write(template % (target_colour, brightness_factor) + "\n")
				for line in export_colours:
					file.write(line + "\n")
			messagebox.showinfo("", 'Saved: {} ({}).ini'.format(exported_name, target_colour))

	def load_ini(self):
		path = filedialog.askopenfilename(filetypes=[("INI files", "*.ini")])
		if not path:
			return

		skin_config = configparser.ConfigParser()
		skin_config.read(path)

		for section in skin_config.sections():
			for key, value in skin_config.items(section):
				value = value.strip().upper()
				if key.lower() == "colour":
					self.target_colour = value
				elif key.lower() == "brightness":
					try:
						self.brightness_factor = float(value)
					except ValueError:
						self.brightness_factor = 1.0

		skin_name = os.path.basename(path).replace(".ini", "")
		xip_file = f"{skin_name}.xip"
		xip_path = os.path.join(os.path.dirname(path), xip_file)

		self.xip_file = "Yes" if os.path.exists(xip_path) else "No"
		self.colour_file = skin_name if path else "No file selected"

		self.loaded_skin_label.config(text=f"Skin Loaded: {self.colour_file}\nHas XIP: {self.xip_file}")
		self.base_colour_skin_label.config(text=f"Base Colour: {self.target_colour}\nBrightness: {self.brightness_factor}")

		if self.export_button:
			self.export_button.destroy()
			self.export_button = None

	def restore_stock_theme(self):
		if messagebox.askyesno("Confirm", "This will restore the original look of the MSDash?"):
			self.colour_file = "stock"
			self.target_colour = ""
			self.brightness_factor = 1.0
			self.send_to_xbox()
			
	def show_save_button(self, event=None):
		self.save_ip_button.pack(side="right", padx=5)
		self.refresh_gui()

	def save_ftp_ip(self, event=None):
		self.config.set("FTP", "server", self.ftp_ip.get())
		with open("settings.ini", "w") as configfile:
			self.config.write(configfile)

		messagebox.showinfo("NOTICE", "IP Address Saved.")

		self.save_ip_button.pack_forget()
		self.root.focus_set()

	def send_to_xbox(self):
		if self.colour_file == "":
			messagebox.showinfo("ERROR", f"Nothing to process")
		else:
			self.process_script = True
			self.root.destroy()

	def set_base_colour(self):
		messagebox.showinfo("NOTICE", "This will apply colour to most of the dashboard, but to achieve the complete visual effect, you'll need to create a skin.xip file for your theme.")

		colour_code = colorchooser.askcolor(title="Select Base colour")[1]
		if not colour_code:
			return

		self.target_colour = colour_code.lstrip("#").upper()
		self.brightness_factor = simpledialog.askfloat("", "Set brightness factor (Range: 0.0 to 10.0)", minvalue=0.0, maxvalue=10.0, initialvalue=1.0)

		if self.brightness_factor is None:
			self.brightness_factor = 1.0

		self.colour_file = "UI Custom Colour"
		self.loaded_skin_label.config(text="Using: Stock.xip")
		self.base_colour_skin_label.config(text=f"Base Colour: {self.target_colour}\nBrightness: {self.brightness_factor}")

		if not self.export_button:
			self.export_button = tk.Button(self.menu_frame, text="Export ini File", command=self.export, width=20, relief="ridge")
			self.export_button.pack(pady=5)

	def update_ftp_status(self):
		self.config.set("FTP", "enable", str(self.ftp_enabled.get()).lower())

		with open("settings.ini", "w") as configfile:
			self.config.write(configfile)

		# FTP IP visible
		if self.ftp_enabled.get():
			self.ftp_ip_entry.pack(anchor="w", pady=2)
			self.ftp_ip_entry.master.pack(fill="x", pady=2)
			self.save_ip_button.pack_forget()
			self.transfer_button.config(bg="#90EE90")
		else:
			self.ftp_ip_entry.pack_forget()
			self.ftp_ip_entry.master.pack_forget()
			self.save_ip_button.pack_forget()
			self.transfer_button.config(bg="#ADD8E6")
		
		self.transfer_button.config(text="Transfer to Xbox" if self.ftp_enabled.get() else "Save")

if __name__ == '__main__':
	version = 1.4
	cmd_title = 'Xboxdash Colourizer v{}'.format(version)
	# Initial window size
	if os.name == 'nt':
		set_window_size(cmd_title, '0B', 70, 11)
	else:
		set_window_size(cmd_title)

	target_colour = ''
	brightness_factor = ''
	override = 0
	export = 0
	process_script = True
	
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
	os.makedirs('xbe file', exist_ok=True)
	xboxdash = os.path.join('xbe file','xboxdash.xbe')
	xb0xdash = os.path.join('xbe file','xb0xdash.xbe')
	xonlinedash = os.path.join('xbe file','xonlinedash.xbe')
	x0nlinedash = os.path.join('xbe file','x0nlinedash.xbe')
	if os.path.isfile(xboxdash):
		xboxdash_file_path = xboxdash
	elif os.path.isfile(xb0xdash):
		xboxdash_file_path = xb0xdash
	else:
		show_error("\n Error: Cannot find valid xbe file:\n\n Looking for:\n  - xbe file\\xboxdash.xbe\n  - xbe file\\xb0xdash.xbe\n\n Please ensure the file exists and try again.")
	if os.path.isfile(xonlinedash):
		xonlinedash_file_path = xonlinedash
	elif os.path.isfile(x0nlinedash):
		xonlinedash_file_path = x0nlinedash
	else:
		xonlinedash_file_path = None
	
	parser = argparse.ArgumentParser(description='Patch Xboxdash colours')
	parser.add_argument('--colour_file', type=str, help='Path to the file')
	parser.add_argument('--target_colour', type=str, help='Target colour')
	parser.add_argument('--brightness_factor', type=float, help='Brightness factor')

	args = parser.parse_args()
	
	# UI stuff if no args are present
	if not any(vars(args).values()):
		root = tk.Tk()
		args = ColourizerUI(root, args)
		root.after(10, args.refresh_gui)
		root.mainloop()

		# Vars from GUI
		process_script = args.process_script
		colour_file = args.colour_file
		target_colour = args.target_colour
		brightness_factor = args.brightness_factor
		ftp_enabled = str(args.ftp_enabled.get())
		ftp_server = args.ftp_ip.get()
		
		if not process_script:
			sys.exit()
	
	# Check if xbe is clean.
	if calculate_md5(xboxdash_file_path) in ['08d3a6f99184679aa13008d6397bacce']:
		# Create output folder, copy xbe from xbe file folder so source file is left intact.
		tran_folder = 'transfer to xbox'
		os.makedirs(tran_folder, exist_ok=True)
		shutil.rmtree(tran_folder)
		os.makedirs(tran_folder, exist_ok=True)
		
		# Set new file path
		shutil.copyfile(xboxdash_file_path, os.path.join(tran_folder, os.path.basename(xboxdash_file_path)))
		xboxdash_file_path = os.path.join(tran_folder, os.path.basename(xboxdash_file_path))
		
		# Assign the arguments to variables
		colour_file = args.colour_file if args.colour_file else input('\n     Use a "theme".ini file to override specific colours.\n     (optional enter export to output a "theme.ini" file)\n     Enter filename: ') or ''
		colour_file = os.path.join('skins', colour_file)
	
		# So you can enter filename without extension
		if not colour_file.endswith('.ini'):
			colour_file += '.ini'
		
		print('\n\n  Patching colours:')
		if target_colour.lower() == 'stock':
			print('   - Stock colours restored\n')
		else:
			print('   - INI: {}'.format(os.path.basename(colour_file)))

		if os.path.isfile(colour_file):
			override = 1
			target_colour, brightness_factor, external_patches = read_external_patch_list(colour_file)
		elif not os.path.isfile(colour_file) and not "export.ini":
			print("   - Couldn't fine the file {}".format(colour_file))
		
		if target_colour == '':
			target_colour = args.target_colour if args.target_colour else input('\n     Will restore Stock colours if left blank.\n     Enter RGB Hex colour value: ') or 'stock'
		else:
			print('   - Colour: {}'.format(target_colour))
		
		check_length_of_colour(target_colour)
		
		if brightness_factor == '':
			brightness_factor = args.brightness_factor if args.brightness_factor else float(input('\n     Default to 1.0 if left blank.\n     Enter brightness factor (defaults to 1.0): ') or 1.0)
		else:
			print('   - Brightness factor: {}'.format(brightness_factor))
	
		check_for_float(brightness_factor)

		if target_colour != 'stock':
			target_a, target_r, target_g, target_b = hex_to_argb(target_colour)
			target_hue, target_lightness, target_saturation = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
		
		# Pause so you can see info for external files
		print("  Patching complete.")

		if xboxdash:
		# Patch partitions
			patch_file(xboxdash_file_path, fg_patches, '', '', 1)
		
			# Patch out xip stuff
			patch_file(xboxdash_file_path, other_patches, '', '', 1)
		
			# Patch colours.
			patch_file(xboxdash_file_path, colour_patches, target_colour, brightness_factor, 0)
		
		if xonlinedash_file_path:
			if calculate_md5(xonlinedash_file_path) in ['8149654a030d813bcc02a24f39fd3ce9']:
				# Create folders needed
				xodash_folder = os.path.join(tran_folder, 'xodash')
				media_folder = os.path.join(xodash_folder, 'media')
				xbg_output = os.path.join(media_folder, 'xbg')
				xbx_output = os.path.join(media_folder, 'xbx')
				os.makedirs(xbg_output, exist_ok=True)
					
				xonlinedash_new_file_path = os.path.join(xodash_folder, os.path.basename(xonlinedash_file_path))
				shutil.copyfile(xonlinedash_file_path, xonlinedash_new_file_path)
				
				# Patch out xip stuff and 720p
				patch_file(xonlinedash_new_file_path, xonline_patches, '', '', 1)
				
				# Patch colours.
				patch_file(xonlinedash_new_file_path, xonline_colour_patches, target_colour, brightness_factor, 0)
				
				# If override file is found
				if override:
					xonlinedash_external_patches = update_patches(xonline_colour_patches, external_patches)
					patch_file(xonlinedash_new_file_path, xonlinedash_external_patches, '', '', 1)

				# Extract zip containing xbx/xbg files
				xbx_files = os.path.splitext(colour_file)[0] + '.zip'
				if not os.path.isfile(xbx_files):
					xbx_files = 'skins\\stock.zip'
				with zipfile.ZipFile(xbx_files, 'r') as zip:
					zip.extractall(media_folder)
			else:
				print("\n Error: xonlinedash.xbe mismatch:\n Valid MD5 Hash:\n  - 8149654a030d813bcc02a24f39fd3ce9")
				time.sleep(3)
		
		# If override file is found
		xip_file = 'skins\\stock.xip'
		ini_name = 'stock'
		if override or target_colour == 'stock':
			if target_colour != 'stock':
				external_patches = update_patches(colour_patches, external_patches)
				patch_file(xboxdash_file_path, external_patches, '', '', 1)
				ini_name = os.path.basename(os.path.splitext(colour_file)[0])
				xip_file = os.path.splitext(colour_file)[0] + '.xip'
				if not os.path.isfile(xip_file):
					xip_file = xip_file

		# Copy the xip to the dashdata folder, always copy stock if a theme doesn't have one
		xbdash_folder = os.path.join(tran_folder, 'xboxdashdata.185ead00')
		os.makedirs(xbdash_folder, exist_ok=True)
		shutil.copyfile(xip_file, os.path.join(xbdash_folder, 'skin.xip'))

		if ftp_enabled.lower() in ['true']:
			time.sleep(1)
			clear_screen()
			# FTP window size
			if os.name == 'nt':
				set_window_size(cmd_title, '0B', 70, 21)
			else:
				set_window_size(cmd_title)
			if xboxdash_file_path:
				print('\n  Uploading:')
				upload_file(ftp_server, '/C/', tran_folder, ftp_username, ftp_password)
			ftp.quit()
			print("  Done.")
			shutil.rmtree(tran_folder)

		time.sleep(2)
	else:
		show_error("\n Error: xboxdash.xbe mismatch:\n\n Valid MD5 Hash:\n  - 08d3a6f99184679aa13008d6397bacce\n\n Please ensure this file is from the Microsoft Dashboard 5960.\n")