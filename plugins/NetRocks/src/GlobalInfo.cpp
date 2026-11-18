#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI GetGlobalInfoW(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->MinFarVersion = MAKEFARVERSION2(2,9,0,0);
  aInfo->Version       = MAKEFARVERSION2(1,0,0,0);
  aInfo->SysID         = 0xAE8CE351;
  aInfo->Title         = L"NetRocks";
  aInfo->Description   = L"Adds SFTP/SCP/NFS/SMB/WebDAV connectivity";
  aInfo->Author        = L"elfmz";
  aInfo->UseMenuGuids  = 0;
}
