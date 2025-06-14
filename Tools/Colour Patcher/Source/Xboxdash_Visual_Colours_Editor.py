import tkinter as tk
from tkinter import colorchooser, filedialog, messagebox, simpledialog
from tkinter import ttk
import colorsys
import configparser
import os

DEFAULTS = {
	"pulse_highlight": "B2D000",
	"pulsing_selection": "FFFF80",
	"egg_button_press": "FEFFBC",
	"kb_button_focus": "B2D000",
	"kb_button_focus_highlight": "FFB2D000",
	"kb_button_large_no_focus_highlight": "FFF3FF6B",
	"kb_button_large_no_focus": "19E100",
	"kb_button_no_focus_highlight": "DEF3FF6B",
	"kb_button_no_focus": "14C000",
	"kb_letters_no_focus": "BEFA5E",
	"innerwall_01_transition": "FFF3FF6B",
	"innerwall_01": "1428D414",
	"innerwall_02": "1414C000",
	"material_#1334": "FF0B2000",
	"xboxgreendark": "FF062100",
	"egg_xboxgreen_sub": "8CC919",
	"xboxgreen2": "8BC818",
	"gamehilite33": "DDD078",
	"nothing": "808080",
	"navtype": "BEFA5E",
	"orangenavtype": "F99819",
	"egg_xboxgreen": "8BC818",
	"type": "64C819",
	"typesdsafsda": "FFFFFF",
	"material_#1335": "FF4CA200",
	"material_#133511": "FF295700",
	"hilightedtype": "FF032C00",
	"xboxgreenq": "8BC818",
	"cellegg_partsw": "4DE039",
	"material_#108": "FFA0FC00",
	"itemstype": "B6F560",
	"gamehilitememory": "FFB2D000",
	"white": "FFFFFF",
	"solid_green_1": "11FF22",
	"solid_green_2": "12C10A",
	"solid_green_3": "0A751C",
	"solid_green_4": "FF123700",
	"dark_green_panels": "FF092900",
	"flatsurfaces_highlights": "C0F3FF6B",
	"flatsurfaces_back": "14C000",
	"flatsurfacesselected": "801EFF00",
	"flatsurfacesmemory": "801EA000",
	"darksurfaces": "5ACBCD55",
	"dark_green_panels_falloff": "FF052305",
	"grey_1": "475345",
	"grey_2": "FF566452",
	"grill_grey_1": "202020",
	"grill_grey_2": "FF404040",
	"wireframe_1": "00000003",
	"wireframe_2": "647DC622",
	"wireframe_3": "7DC622",
	"tubes_1": "D7F2FA99",
	"tubes_2": "25076800",
	"memoryheader": "7A3CC643",
	"memoryheaderhilite_1": "F0C7E800",
	"memoryheaderhilite_2": "82617200",
	"eggglow_1": "E4FEFFBC",
	"eggglow_2": "FCFF00",
	"gradient_1": "33BFFF6B",
	"gradient_2": "00FF12",
	"cellegg_parts_1": "B2F3FF6B",
	"cellegg_parts_2": "1EFF00",
	"flatsurfaces2sided3_1": "FFFD1E00",
	"flatsurfaces2sided3_2": "F21C00",
	"console_hilite_1": "FFFFAD6B",
	"console_hilite_2": "F6FF00",
	"metal_chrome_1": "E5E5E5",
	"metal_chrome_2": "FFE5E5E5",
	"panelbacking_01": "FF041400",
	"panelbacking_03": "FF041400",
	"panelbacking_04": "0E2E07",
	"darkenbacking": "FF041400",
	"button": "9D6DC2",
	"image": "FF041400",
	"live_header": "FF041400",
	"highlight": "9D6DC2",
	"footer": "9D6DC2",
	"livechrome": "9D6DC2",
	"gamehilite": "FFFFFF",
	"panelbacking": "FF041400",
}

PATCH_KEYS = [
	{'name': 'pulse_highlight', 'patch_type': 0, 'argb': False},
	{'name': 'pulsing_selection', 'patch_type': 0, 'argb': False},
	{'name': 'egg_button_press', 'patch_type': 0, 'argb': False},
	{'name': 'kb_button_focus', 'patch_type': 0, 'argb': False},
	{'name': 'kb_button_focus_highlight', 'patch_type': 0, 'argb': True},
	{'name': 'kb_button_large_no_focus_highlight', 'patch_type': 0, 'argb': True},
	{'name': 'kb_button_large_no_focus', 'patch_type': 0, 'argb': False},
	{'name': 'kb_button_no_focus_highlight', 'patch_type': 0, 'argb': True},
	{'name': 'kb_button_no_focus', 'patch_type': 0, 'argb': False},
	{'name': 'kb_letters_no_focus', 'patch_type': 0, 'argb': False},
	{'name': 'innerwall_01_transition', 'patch_type': 0, 'argb': True},
	{'name': 'innerwall_01', 'patch_type': 0, 'argb': True},
	{'name': 'innerwall_02', 'patch_type': 0, 'argb': True},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'material_#1334', 'patch_type': 2, 'argb': True},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'xboxgreendark', 'patch_type': 2, 'argb': True},
	{'name': 'egg_xboxgreen_sub', 'patch_type': 1, 'argb': False},
	{'name': 'xboxgreen2', 'patch_type': 1, 'argb': False},
	{'name': 'gamehilite33', 'patch_type': 1, 'argb': False},
	{'name': 'nothing', 'patch_type': 1, 'argb': False},
	{'name': 'navtype', 'patch_type': 1, 'argb': False},
	{'name': 'orangenavtype', 'patch_type': 1, 'argb': False},
	{'name': 'egg_xboxgreen', 'patch_type': 1, 'argb': False},
	{'name': 'type', 'patch_type': 1, 'argb': False},
	{'name': 'typesdsafsda', 'patch_type': 1, 'argb': False},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'material_#1335', 'patch_type': 2, 'argb': True},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'material_#133511', 'patch_type': 2, 'argb': True},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'hilightedtype', 'patch_type': 2, 'argb': True},
	{'name': 'xboxgreenq', 'patch_type': 1, 'argb': False},
	{'name': 'cellegg_partsw', 'patch_type': 1, 'argb': False},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'material_#108', 'patch_type': 2, 'argb': True},
	{'name': 'itemstype', 'patch_type': 1, 'argb': False},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'gamehilitememory', 'patch_type': 2, 'argb': True},
	{'name': 'white', 'patch_type': 1, 'argb': False},
	{'name': 'solid_green_1', 'patch_type': 1, 'argb': False},
	{'name': 'solid_green_2', 'patch_type': 1, 'argb': False},
	{'name': 'solid_green_3', 'patch_type': 1, 'argb': False},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'solid_green_4', 'patch_type': 2, 'argb': True},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'dark_green_panels', 'patch_type': 2, 'argb': True},
	{'name': 'flatsurfaces_highlights', 'patch_type': 0, 'argb': True},
	{'name': 'flatsurfaces_back', 'patch_type': 0, 'argb': False},
	{'name': 'flatsurfacesselected', 'patch_type': 0, 'argb': True},
	{'name': 'flatsurfacesmemory', 'patch_type': 0, 'argb': True},
	{'name': 'darksurfaces', 'patch_type': 0, 'argb': True},
	{'name': 'dark_green_panels_falloff', 'patch_type': 0, 'argb': True},
	{'name': 'grey_1', 'patch_type': 0, 'argb': False},
	{'name': 'grey_2', 'patch_type': 0, 'argb': True},
	{'name': 'grill_grey_1', 'patch_type': 0, 'argb': False},
	{'name': 'grill_grey_2', 'patch_type': 0, 'argb': True},
	{'name': 'wireframe_1', 'patch_type': 0, 'argb': True},
	{'name': 'wireframe_2', 'patch_type': 0, 'argb': True},
	{'name': 'wireframe_3', 'patch_type': 0, 'argb': False},
	{'name': 'tubes_1', 'patch_type': 0, 'argb': True},
	{'name': 'tubes_2', 'patch_type': 0, 'argb': True},
	{'name': 'memoryheader', 'patch_type': 0, 'argb': True},
	{'name': 'memoryheaderhilite_1', 'patch_type': 0, 'argb': True},
	{'name': 'memoryheaderhilite_2', 'patch_type': 0, 'argb': True},
	{'name': 'eggglow_1', 'patch_type': 0, 'argb': True},
	{'name': 'eggglow_2', 'patch_type': 0, 'argb': False},
	{'name': 'gradient_1', 'patch_type': 0, 'argb': True},
	{'name': 'gradient_2', 'patch_type': 0, 'argb': False},
	{'name': 'cellegg_parts_1', 'patch_type': 0, 'argb': True},
	{'name': 'cellegg_parts_2', 'patch_type': 0, 'argb': False},
	{'name': 'flatsurfaces2sided3_1', 'patch_type': 0, 'argb': True},
	{'name': 'flatsurfaces2sided3_2', 'patch_type': 0, 'argb': False},
	{'name': 'console_hilite_1', 'patch_type': 0, 'argb': True},
	{'name': 'console_hilite_2', 'patch_type': 0, 'argb': False},
	{'name': 'metal_chrome_1', 'patch_type': 0, 'argb': False},
	{'name': 'metal_chrome_2', 'patch_type': 0, 'argb': True},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'panelbacking_01', 'patch_type': 2, 'argb': True},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'panelbacking_03', 'patch_type': 2, 'argb': True},
	{'name': 'panelbacking_04', 'patch_type': 1, 'argb': False},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'darkenbacking', 'patch_type': 2, 'argb': True},
	{'name': 'button', 'patch_type': 1, 'argb': False},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'image', 'patch_type': 2, 'argb': True},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'live_header', 'patch_type': 2, 'argb': True},
	{'name': 'highlight', 'patch_type': 1, 'argb': False},
	{'name': 'footer', 'patch_type': 1, 'argb': False},
	{'name': 'livechrome', 'patch_type': 1, 'argb': False},
	{'name': 'gamehilite', 'patch_type': 1, 'argb': False},
	{'name': '', 'patch_type': 0, 'argb': False},
	{'name': 'panelbacking', 'patch_type': 2, 'argb': True},
]

def set_window_size(title, color='00', width=100, height=100):
	if os.name == 'nt':
		os.system('mode con: cols={} lines={}'.format(width, height))
		os.system('title {}'.format(title))
		os.system('color {}'.format(color))
	else:
		os.system('echo "\033]0;{}\007"'.format(title))

class SkinEditorApp:
	def __init__(self, root, title):
		self.root = root
		self.root.title(title)

		style = ttk.Style(self.root)
		style.configure("TLabel", font=("Segoe UI", 10))
		style.configure("TButton", padding=4)
		style.configure("TEntry", relief="flat")
		style.map("TButton",
				  background=[("active", "#e0e0e0")],
				  foreground=[("active", "black")])

		self.config = configparser.ConfigParser()
		self.config.optionxform = str
		self.entries = {}
		self.alpha_sliders = {}
		self.swatches = {}
		self.labels = {}
		
		self.center_window()

		main_frame = ttk.Frame(root, padding=(10, 10))
		main_frame.grid(row=0, column=0, sticky="nsew")

		root.columnconfigure(0, weight=1)
		root.rowconfigure(0, weight=1)

		self.canvas = tk.Canvas(main_frame, borderwidth=0, highlightthickness=0)
		self.scrollbar = ttk.Scrollbar(main_frame, orient="vertical", command=self.canvas.yview)
		self.scrollable_frame = ttk.Frame(self.canvas)

		self.canvas.create_window((0, 0), window=self.scrollable_frame, anchor="nw")
		self.canvas.configure(yscrollcommand=self.scrollbar.set)

		self.scrollable_frame.bind(
			"<Configure>",
			lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all"))
		)

		self.canvas.grid(row=0, column=0, sticky="nsew")
		self.scrollbar.grid(row=0, column=1, sticky="ns")

		main_frame.columnconfigure(0, weight=1)
		main_frame.rowconfigure(0, weight=1)

		self.scrollable_frame.columnconfigure(1, weight=1)

		button_frame = ttk.Frame(main_frame)
		button_frame.grid(row=1, column=0, columnspan=2, pady=(8, 0))

		btn_colour = ttk.Button(button_frame, text="Set Base Colour", command=self.set_base_colour)
		btn_load = ttk.Button(button_frame, text="Load INI", command=self.load_ini)
		btn_save = ttk.Button(button_frame, text="Save INI", command=self.save_ini)
		btn_quit = ttk.Button(button_frame, text="Quit", command=self.root.quit)

		btn_colour.pack(side="left", padx=10)
		btn_load.pack(side="left", padx=10)
		btn_save.pack(side="left", padx=10)
		btn_quit.pack(side="left", padx=10)

		self.create_widgets()

	def create_widgets(self):
		row = 0
		for patch in PATCH_KEYS:
			name = patch["name"]
			argb = patch["argb"]

			if not name:
				# Insert a Separator instead of a blank row
				sep = ttk.Separator(self.scrollable_frame, orient="horizontal")
				sep.grid(row=row, column=0, columnspan=6, sticky="ew", pady=6)
				row += 1
				continue

			# Column 0: Label
			lbl = ttk.Label(self.scrollable_frame, text=name)
			lbl.grid(row=row, column=0, sticky="w", padx=(2, 10), pady=2)
			self.labels[name] = lbl

			# Column 1: Entry
			entry = ttk.Entry(self.scrollable_frame, width=10)
			entry.grid(row=row, column=1, padx=(0, 10), sticky="ew")
			self.entries[name] = entry

			default_val = DEFAULTS.get(name, "").upper()
			if default_val:
				entry.insert(0, default_val)

			# Column 2: Color swatch (tk.Label for direct bg control)
			swatch = tk.Label(self.scrollable_frame, bg="#000000", width=4, relief="ridge", borderwidth=1)
			swatch.grid(row=row, column=2, padx=(0, 10))
			self.swatches[name] = swatch
			self.update_swatch(name, default_val)

			# Column 3: “Pick” button
			btn_pick = ttk.Button(
				self.scrollable_frame,
				text="Pick",
				command=lambda n=name, a=argb: self.pick_color(n, a)
			)
			btn_pick.grid(row=row, column=3, padx=(0, 10))

			# Column 4: Alpha slider (if argb=True)
			if argb:
				slider = ttk.Scale(
					self.scrollable_frame,
					from_=0,
					to=255,
					orient="horizontal",
					length=100,
					command=lambda val, n=name: self.update_alpha(n, val)
				)
				init_alpha = int(default_val[:2], 16) if len(default_val) == 8 else 255
				slider.set(init_alpha)
				slider.grid(row=row, column=4, padx=(0, 10))
				self.alpha_sliders[name] = slider

			# Column 5: “Reset” button
			btn_reset = ttk.Button(
				self.scrollable_frame,
				text="Reset",
				command=lambda n=name, d=default_val: self.reset_to_default(n, d)
			)
			btn_reset.grid(row=row, column=5, padx=(0, 10))

			row += 1

	def center_window(self, width=660, height=640):
		screen_width = self.root.winfo_screenwidth()
		screen_height = self.root.winfo_screenheight()

		x = (screen_width // 2) - (width // 2)
		y = (screen_height // 2) - (height // 2)

		self.root.geometry(f"{width}x{height}+{x}+{y}")

	def menu_hex_to_rgb(self, hexval: str) -> tuple:
		hex_digits = hexval[-6:]
		return tuple(int(hex_digits[i : i + 2], 16) for i in (0, 2, 4))

	def update_swatch(self, key, hexval):
		if not hexval:
			return
		try:
			rgb = self.menu_hex_to_rgb(hexval)
			self.swatches[key].config(bg=f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}")
		except ValueError:
			print(f"Invalid hex color: {hexval}")

	def update_alpha(self, key, alpha_val):
		current = self.entries[key].get().strip().upper()
		if not current:
			return
		alpha_hex = f"{int(float(alpha_val)):02X}"
		if len(current) == 6:
			newval = alpha_hex + current
		elif len(current) == 8:
			newval = alpha_hex + current[2:]
		else:
			return
		self.entries[key].delete(0, tk.END)
		self.entries[key].insert(0, newval)
		self.update_swatch(key, newval)
		self.highlight_if_modified(key)

	def pick_color(self, key, argb):
		color_code = colorchooser.askcolor(title=f"Pick color for {key}")
		if not color_code or not color_code[1]:
			return
		hexval = color_code[1].lstrip("#").upper()
		current = self.entries[key].get().strip().upper()
		alpha = current[:2] if (argb and len(current) == 8) else "FF"
		newval = alpha + hexval if argb else hexval
		self.entries[key].delete(0, tk.END)
		self.entries[key].insert(0, newval)
		self.update_swatch(key, newval)
		self.highlight_if_modified(key)

	def reset_to_default(self, key, default_val):
		self.entries[key].delete(0, tk.END)
		self.entries[key].insert(0, default_val)
		self.update_swatch(key, default_val)
		if key in self.alpha_sliders and len(default_val) == 8:
			self.alpha_sliders[key].set(int(default_val[:2], 16))
		self.highlight_if_modified(key)

	def highlight_if_modified(self, key):
		entry_val = self.entries[key].get().strip().upper()
		default_val = DEFAULTS.get(key, "").strip().upper()
		
		if entry_val != default_val:
			self.labels[key].config(foreground="red")
		else:
			self.labels[key].config(foreground="black")

	# Taken from the Xboxdash_5960_Colourizer.py script
	def hex_to_argb(self, hex_color):
		if len(hex_color) == 6:
			hex_color = 'FF' + hex_color
		return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4, 6))

	def hex_to_rgb(self, hex_color):
		return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))

	def argb_to_hex(self, argb_color):
		return '{:02X}{:02X}{:02X}{:02X}'.format(*argb_color)

	def rgb_to_hex(self, rgb_color):
		return '{:02X}{:02X}{:02X}'.format(*rgb_color)

	def adjust_color_to_target(self, color, target_color, brightness_factor):
		if len(color) == 8: # ARGB
			a, r, g, b = self.hex_to_argb(color)
			h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
			# print('ARGB - Original Lightness: {}'.format(l))
			target_r, target_g, target_b = self.hex_to_rgb(target_color)
			target_h, _, target_s = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
			l = min(float(l) * brightness_factor, 1.0)
			# print('ARGB - Adjusted Lightness: {}'.format(l))
			new_r, new_g, new_b = colorsys.hls_to_rgb(target_h, l, target_s)
			adjusted_color = self.argb_to_hex((a, int(new_r * 255), int(new_g * 255), int(new_b * 255)))
		elif len(color) == 6: # RGB
			r, g, b = self.hex_to_rgb(color)
			h, l, s = colorsys.rgb_to_hls(r / 255.0, g / 255.0, b / 255.0)
			# print('RGB - Original Lightness: {}'.format(l))
			target_r, target_g, target_b = self.hex_to_rgb(target_color.lstrip("#"))
			target_h, _, target_s = colorsys.rgb_to_hls(target_r / 255.0, target_g / 255.0, target_b / 255.0)
			l = min(float(l) * brightness_factor, 1.0)
			# print('RGB - Adjusted Lightness: {}'.format(l))
			new_r, new_g, new_b = colorsys.hls_to_rgb(target_h, l, target_s)
			adjusted_color = self.rgb_to_hex((int(new_r * 255), int(new_g * 255), int(new_b * 255)))
		else:
			# print('Skipping color: {}'.format(color))
			return color
		# print('Original: {}\tAdjusted: {}'.format(color, adjusted_color))
		return adjusted_color

	def adjust_colors_to_target(self, colors, target_color, brightness_factor=1.0):
		adjusted_colors = []
		for color in colors:
			adjusted_color = self.adjust_color_to_target(color, target_color, brightness_factor)
			adjusted_colors.append(adjusted_color)
		return adjusted_colors
	# #################################################

	# Load the ini and read the colour value and update all colours that aren't overriden in the ini file.
	def load_ini(self):
		path = filedialog.askopenfilename(filetypes=[("INI files", "*.ini")])
		if not path:
			return

		base_color = "FFFFFF"
		base_brightness = 1.0
		
		self.config.clear()
		self.config.read(path)

		for section in self.config.sections():
			for key, value in self.config.items(section):
				value = value.strip().upper()
				if key.lower() == "colour":
					base_color = value
				elif key.lower() == "brightness":
					try:
						base_brightness = float(value)
					except ValueError:
						base_brightness = 1.0

		DEFAULTSNEW = {}
		DEFAULTSNEW.update(DEFAULTS.copy())

		for key, original_color in DEFAULTSNEW.items():
			if not any(key.lower() in self.config.options(section) for section in self.config.sections()):
				DEFAULTSNEW[key] = self.adjust_color_to_target(original_color, base_color, brightness_factor=base_brightness)
			else:
				for section in self.config.sections():
					if key.lower() in self.config.options(section):
						DEFAULTSNEW[key] = self.config.get(section, key).strip().upper()

		for key in self.entries:
			self.entries[key].delete(0, tk.END)

		for key in DEFAULTSNEW:
			if key in self.entries:
				self.entries[key].insert(0, DEFAULTSNEW[key])
				self.update_swatch(key, DEFAULTSNEW[key])

		messagebox.showinfo("Loaded", f"Loaded {os.path.basename(path)}")

	# Set the base colour and brightness then update the ui colours.
	def set_base_colour(self):
		color_code = colorchooser.askcolor(title="Select Base Color")[1]
		if not color_code:
			return

		base_color = color_code.lstrip("#").upper()
		brightness_value = simpledialog.askfloat("", "Set brightness factor (Range: 0.0 to 10.0)", minvalue=0.0, maxvalue=10.0, initialvalue=1.0)
		if brightness_value is None:
			brightness_value = 1.0

		DEFAULTSNEW = {}
		DEFAULTSNEW.update(DEFAULTS.copy())

		for key, original_color in DEFAULTSNEW.items():
			DEFAULTSNEW[key] = self.adjust_color_to_target(original_color, base_color, brightness_factor=brightness_value)

		for key in self.entries:
			self.entries[key].delete(0, tk.END)

		for key in DEFAULTSNEW:
			if key in self.entries:
				self.entries[key].delete(0, tk.END)
				self.entries[key].insert(0, DEFAULTSNEW[key])
				self.update_swatch(key, DEFAULTSNEW[key])

	# Save the ini with all the new colours, really it should be the colour value & only colours you adjusted but
	# it simpler to just export them all and leave the base colour as FFFFFF (stock) my other script will take care of it all.
	def save_ini(self):
		save_path = filedialog.asksaveasfilename(defaultextension=".ini", filetypes=[("INI files", "*.ini")])
		if not save_path:
			return

		with open(save_path, "w") as configfile:
			configfile.write("[These are mandatory]\n")
			base_color = "FFFFFF"
			base_brightness = self.config.get("These are mandatory", "brightness", fallback="1.0").strip()
			
			configfile.write(f"colour={base_color}\n")
			configfile.write(f"brightness={base_brightness}\n")

			configfile.write("\n[Override colours with your own]\n")
			for key, entry in self.entries.items():
				value = entry.get().strip().upper()
				if value:
					configfile.write(f"{key}={value}\n")

		messagebox.showinfo("Saved", f"Skin saved to {save_path}")

def launch_skin_editor(title):
	root = tk.Tk()
	root.geometry("660x640")
	root.minsize(660, 640)
	root.maxsize(660, 1280)
	app = SkinEditorApp(root, title)
	root.mainloop()

if __name__ == "__main__":
	version = 1.0
	cmd_title = 'Xboxdash Visual Colours Editor v{}'.format(version)
	if os.name == 'nt':
		set_window_size(cmd_title, '0B', 70, 11)
	else:
		set_window_size(cmd_title)
	launch_skin_editor(cmd_title)