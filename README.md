# UIX-Ultra-Lite
Modern/Re-Implemented Patches and Scripts for the Xbox Dashboard #5960

## How-To

By following [this link](https://github.com/MrMilenko/UIX-Ultra-Lite/tree/main/xboxdashdata.185ead00#what-are-these-files) you can follow a tutorial on how to manually patch out XIP signature checks, and modify XIP files to add an HDD Loader to dash 5960.

## Family Tree

## tHc/tHc Lite
* Non-Source Based 4920 Modification.
* Additional, "unavailable-in-source" (F and G Support, for example. The assembly patches were never public, installer only.) binary patches.
* XIP and XAP Modifications.
* Live Support Stripped from XIPS. 

## UIX Ultra Lite
* Non-Source Based 5960 Modification
* Re-Implementation and open sourcing of Binary Patches from 4920.
* Re-Implementation of XIP Modifications, adapting to changes between 4920 and 5960.


# Why?

In the early 2000's we didn't have access to source code, thus projects like UIX and UIX Lite didn't exist. What we did have was a sub-scene, within the Xbox scene that focused on modifications to the retail dashboards files. The work of these people, was the foundation for what UIX eventually became, as well as the original TeamUIX. So, we're going into the archives and taking a look at patches and mods from our friends Gcue, fuckdb, Vulgasprofanum, the original tHc, and xboxdash[.]net community and implementing them in a modern, live enabled, dashboard.

The end goal, will be to provide a legal, open source, distributable patch set and tutorials for ease of use.

A modified console will be required to run this, but we will be utilizing 20 years of modification techniques to make this as easy as possible for the end user.

# To-Do

* Run Unsigned Xips W/O Modifying the XBE Source Code, will still require a patch and a modded system.
* Launch Games From the 5960 Dashboard.
* Launch Games From the F and G Partitions.

# Non-Features
One of the caveats of turning back the clock here, is we won't be touching any source code. It's leaked, old, outdated and will not allow us to have a proper setup for systems intending to connect to network services.

* No FTP Server.
* No Web Server.
* No Game Icons In HDD Loader (Maybe..)

# POC

We successfully got the HDD Loader from tHc Lite (Which is why the POC Video here is called tHc Ultra-Lite) to launch XBE's from within 5960.

[![tHc Ultra Lite POC](http://img.youtube.com/vi/IlFVf--V0Ac/0.jpg)](https://www.youtube.com/watch?v=IlFVf--V0Ac)
