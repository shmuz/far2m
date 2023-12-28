#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 3,0,0,52 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x1AF0754D;
  aInfo->Version       = Version;
  aInfo->Title         = L"HlfViewer";
  aInfo->Description   = L"Hlf-file Viewer for Far Manager";
  aInfo->Author        = L"Far Group, Shmuel Zeigerman";
}
//---------------------------------------------------------------------------

SHAREDSYMBOL int WINAPI GetMinFarVersionW(void)
{
  return MAKEFARVERSION(2,4);
}
//---------------------------------------------------------------------------
