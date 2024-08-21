# Build UIX Lite Skins

## What is this?

This simple tool is for creating `skin.xip` files for UIX Lite (or THC). An example skin called **stockish** is included to help you get up and running quickly. This skin contains all the images that can be used to theme UIX Lite.

### How to use:

1. Drag the **stockish** folder onto the **Make Skin.xip.bat** file.
2. See **FTP support** below.

On the first run, the tool will create a `settings.ini` file, allowing you to enable FTP support for transferring files to the Xbox. Editing this file is/should be straightforward.

#### `FTP'ing`
If `FTP support` is set up, the tool will create and send the `skin.xip` file to the Xbox. If not, you will get a file named **stockish.xip** or whatever the name of your skin folder is.

### To create a skin
You can use the PSD file included in the `stockish\PSDs` folder. You can use Photoshop or another alternative that can open PSD files. You don't have to resize images as they will be resized when the `skin.xip` is created.

There are a few files that support transparency via TGA images. They are the following:
1. cellwall
2. dvd_button
3. DVD_paneltex
4. xbox4

Note: They can also be BMP files if you don't want to mess with TGA files. I have also included a few different cellwall types in the `stockish\Cellwalls` folder.

Special thanks to **Voltaic** for pixit.
