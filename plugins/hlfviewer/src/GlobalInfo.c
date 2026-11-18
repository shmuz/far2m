#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(3,0,0,52);
  aInfo->SysID         = 0x1AF0754D;
  aInfo->Title         = L"HlfViewer";
  aInfo->Description   = L"Hlf-file Viewer for Far Manager";
  aInfo->Author        = L"Far Group, Shmuel Zeigerman";
  aInfo->UseMenuGuids  = 1;
}
