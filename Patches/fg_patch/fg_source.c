int MountDrive(char a1, PCSZ SourceString)
{
  STRING v3; // [esp+Ch] [ebp-20h] BYREF
  char Buffer[7]; // [esp+15h] [ebp-17h] BYREF
  STRING DestinationString; // [esp+1Ch] [ebp-10h] BYREF

  sprintf(Buffer, "\\??\\%c:", a1);
  RtlInitAnsiString(&DestinationString, Buffer);
  RtlInitAnsiString(&v3, SourceString);
  return IoCreateSymbolicLink(&DestinationString, &v3);
}

int MountDrives()
{
  MountDrive('N', aDeviceHarddisk_11);
  MountDrive('O', aDeviceHarddisk_12);
  return 0;
}