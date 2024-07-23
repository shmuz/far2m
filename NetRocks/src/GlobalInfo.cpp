#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 1,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xAE8CE351;
  aInfo->Version       = Version;
  aInfo->Title         = L"NetRocks";
  aInfo->Description   = L"Adds SFTP/SCP/NFS/SMB/WebDAV connectivity";
  aInfo->Author        = L"elfmz";
}
