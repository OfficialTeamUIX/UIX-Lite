# F & G Patch
Implementation of F & G Partition Access in Xbox Dashboard #5960

## What It Is
On retail machines, and in retail software the "F & G" Partitions mounted at Partition6 and Partition7 don't exist. These are "scene" creations and needed to be implemented by adding the additional partition paths to the XBE via a preload.

Fortunately for us, much like the original patches used by tHc and xboxdash.net by fuck_db this code could be injected into the existing xbe. But it's 2024, so it was done a bit cleaner than it was previously by accomplishing the following;

Patch 1: Modify the xbe header to mark one of the unused sections as executable and preload
Patch 2: Add a MountDrive function that takes a drive letter and system path as an argument
Patch 3: Add a function that calls MountDrive twice, once for F and once for G:
Patch 4: Extend main() such that it calls our MountDrive function before calling the msdash main routine.

The above patches, are not currently supplied within the source as they involved manual modification to the retail XBE. The source to the actual stub that was injected however, is.

On a retail machine, "F&G" are being mounted under "N&O". Keep this in mind if you make any alterations to XIP's that rely on drive letters.