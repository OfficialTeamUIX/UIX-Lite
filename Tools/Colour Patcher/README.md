# Xboxdash 5960 Colourizer

## What is this?

The Xboxdash 5960 Colourizer is a tool (a Python script wrapped in an executable for simplicity) that patches the 5960 `xboxdash.xbe` (or `xb0xdash.xbe`) with custom color values specified via hex codes. You can override existing color values or completely customize all colors using a theme `.ini` file. This package includes six themes, with one replicating the stock dashboard values. The `source` folder contains the script's source files, allowing Linux and Mac users to run it easily. (requires Python 2.7 or 3)

### How to use:

1. Obtain a clean copy of `xboxdash.xbe` and place it inside the `xbe file` folder. It can also be named `xb0xdash.xbe` (this can also be a freshly patched xbe from the patcher).
2. Run the "Xboxdash 5960 Colourizer.exe/.bat/.sh" file, and follow the on-screen instructions.

On the first run, the tool will create a `settings.ini` file, allowing you to enable FTP support for transferring files to the Xbox. Editing this file is/should be straightforward.

#### Examples:

##### To use the sky theme:
1. Open the tool, type **sky**, and hit enter.
2. See **FTP'ing** below.

Yes, it's that simple!

##### Make your dashboard pink:
1. Open the tool and hit enter.
2. Enter **f19cbb**, and hit enter.
3. Enter **0.95** (if left blank it defaults to 1.0 for brightness).
4. See **FTP support** below.

##### If you want to export a theme ini:
1. Open the tool, enter `export` and hit enter.
2. Enter a color value, let's use pink again, **f19cbb**, and hit enter.
3. We will leave this blank for now, just hit enter.
4. See **FTP'ing** below.

You will get a new ini file named `export f19cbb.ini`. This file contains all the color values for theming the dash pink. You can fine-tune this file by changing the values inside. Please note that values are a mix of ARGB and RGB. If something uses only 6 letters, it is RGB, and 8 letters is ARGB. These cannot be muddled up or have missing characters.

#### `FTP'ing`
If `FTP support` is set up, the tool will patch the `xbe` and send it to the Xbox. If not, you will get a new folder called `transfer to xbox`. Inside is the new patched `xbe` and `xboxdashdata.185ead00` folder containing the `skin.xip` if available.

### The tool will patch the following:

1. Xip hash and check bypass
2. Extended partition support (F, G, HHD0 & HHD1)
3. No DVD region patch (thanks to Sylver)
4. All color values that can be patched (over 80 in total)

**Note:**
The `extended partition` patch won't be applied if the xbe is prepatched.
