#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 3,0,0,49 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x1AF0754D;
  aInfo->Version       = Version;
  aInfo->Title         = L"HlfViewer";
  aInfo->Description   = L"Hlf-file Viewer for Far Manager";
  aInfo->Author        = L"Far Group, Shmuel Zeigerman";
}
