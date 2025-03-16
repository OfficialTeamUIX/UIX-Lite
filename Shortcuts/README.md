# UIX-Ultra-Lite
Modern/Re-Implemented Patches and Scripts for the Xbox Dashboard #5960

**Warning: The information below is obsolete as of Release v0.3 and is no longer applicable to UIX Lite**

## How-To: Make shortcuts to titles on the F: and/or G: drives for UIX Ultra Lite.

- Download and extract the Shortcut.zip which contains 2 files: default.xbe and default.cfg
- Create the following file structure on the E: drive or on your computer and copy to the E: drive later.
```
Shortcuts
|-- Apps
|-- Dashboards
|-- Games
|-- Emus
```
* Note: The folder names in Shortcuts folder shown above are the default values. If you have customized these then use your custom folder names.

The following is an example to create a shortcut to the game "Fable" located at F:\\Games\\Fable. 
 
- Browse to Shortcuts\\Games\\ and create the folder named Fable.
- Then copy the shortcut file, default.xbe that you extracted from the Shortcut.zip, to Shortcuts\\Games\\Fable\\
- Next edit the default.cfg in your favorite text editor and put the path to the target .xbe file on the F: drive. i.e. F:\\Games\\Fable\\default.xbe
- Save the changes and copy the default.cfg to Shortcuts\\Games\\Fable\\
- Repeat the above for all of your F: drive titles.
- Once completed (if this was done on your computer upload the Shortcuts folder to the E: drive) restart UIX Ultra Lite and your title will show up in the Launcher Menu and can be launched.

* Note: Because these shortcuts are in the Shortcuts folder, other dashboards won't load them as dupicate titles.