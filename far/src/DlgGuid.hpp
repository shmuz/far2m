#pragma once

/*
DlgGuid.hpp

GUID'ы диалогов.
*/
/*
Copyright (c) 2010 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <WinCompat.h>

// {8C9EAD29-910F-4b24-A669-EDAFBA6ED964}
DEFINE_GUID(FindFileId,0x8c9ead29,0x910f,0x4b24,0xa6,0x69,0xed,0xaf,0xba,0x6e,0xd9,0x64);
// {9FBCB7E1-ACA2-475d-B40D-0F7365B632FF}
DEFINE_GUID(CopyOverwriteId,0x9fbcb7e1,0xaca2,0x475d,0xb4,0xd,0xf,0x73,0x65,0xb6,0x32,0xff);
// {1D07CEE2-8F4F-480a-BE93-069B4FF59A2B}
DEFINE_GUID(FileOpenCreateId,0x1d07cee2,0x8f4f,0x480a,0xbe,0x93,0x6,0x9b,0x4f,0xf5,0x9a,0x2b);
// {9162F965-78B8-4476-98AC-D699E5B6AFE7}
DEFINE_GUID(FileSaveAsId,0x9162f965,0x78b8,0x4476,0x98,0xac,0xd6,0x99,0xe5,0xb6,0xaf,0xe7);
// {FAD00DBE-3FFF-4095-9232-E1CC70C67737}
DEFINE_GUID(MakeFolderId,0xfad00dbe,0x3fff,0x4095,0x92,0x32,0xe1,0xcc,0x70,0xc6,0x77,0x37);
// {80695D20-1085-44d6-8061-F3C41AB5569C}
DEFINE_GUID(FileAttrDlgId,0x80695d20,0x1085,0x44d6,0x80,0x61,0xf3,0xc4,0x1a,0xb5,0x56,0x9c);
// {879A8DE6-3108-4beb-80DE-6F264991CE98}
DEFINE_GUID(CopyReadOnlyId,0x879a8de6,0x3108,0x4beb,0x80,0xde,0x6f,0x26,0x49,0x91,0xce,0x98);
// {FCEF11C4-5490-451d-8B4A-62FA03F52759}
DEFINE_GUID(CopyFilesId,0xfcef11c4,0x5490,0x451d,0x8b,0x4a,0x62,0xfa,0x3,0xf5,0x27,0x59);
// {431A2F37-AC01-4ecd-BB6F-8CDE584E5A03}
DEFINE_GUID(MoveFilesId,0x431a2f37,0xac01,0x4ecd,0xbb,0x6f,0x8c,0xde,0x58,0x4e,0x5a,0x3);
// {5EB266F4-980D-46af-B3D2-2C50E64BCA81}
DEFINE_GUID(HardSymLinkId,0x5eb266f4,0x980d,0x46af,0xb3,0xd2,0x2c,0x50,0xe6,0x4b,0xca,0x81);
// {536754EB-C2D1-4626-933F-A25D1E1D110A}
DEFINE_GUID(FindFileResultId,0x536754eb,0xc2d1,0x4626,0x93,0x3f,0xa2,0x5d,0x1e,0x1d,0x11,0x0a);
// {3F9311F5-3CA3-4169-A41C-89C76B3A8C1D}
DEFINE_GUID(EditorSavedROId,0x3f9311f5,0x3ca3,0x4169,0xa4,0x1c,0x89,0xc7,0x6b,0x3a,0x8c,0x1d);
// {85532BD5-1583-456D-A810-41AB345995A9}
DEFINE_GUID(EditorSaveF6DeletedId,0x85532bd5,0x1583,0x456d,0xa8,0x10,0x41,0xab,0x34,0x59,0x95,0xa9);
// {2D71DCCE-F0B8-4E29-A3A9-1F6D8C1128C2}
DEFINE_GUID(EditorSaveExitDeletedId,0x2d71dcce,0xf0b8,0x4e29,0xa3,0xa9,0x1f,0x6d,0x8c,0x11,0x28,0xc2);
// {4109C8B3-760D-4011-B1D5-14C36763B23E}
DEFINE_GUID(EditorAskOverwriteId,0x4109c8b3,0x760d,0x4011,0xb1,0xd5,0x14,0xc3,0x67,0x63,0xb2,0x3e);
// {D8AA706F-DA7E-4BBF-AB78-6B7BDB49E006}
DEFINE_GUID(EditorOpenRSHId,0xd8aa706f,0xda7e,0x4bbf,0xab,0x78,0x6b,0x7b,0xdb,0x49,0xe0,0x06);
// {40A699F1-BBDD-4E21-A137-97FFF798B0C8}
DEFINE_GUID(EditAskSaveExtId,0x40a699f1,0xbbdd,0x4e21,0xa1,0x37,0x97,0xff,0xf7,0x98,0xb0,0xc8);
// {F776FEC0-50F7-4E7E-BDA6-2A63F84A957B}
DEFINE_GUID(EditAskSaveId,0xf776fec0,0x50f7,0x4e7e,0xbd,0xa6,0x2a,0x63,0xf8,0x4a,0x95,0x7b);
// {502D00DF-EE31-41CF-9028-442D2E352990}
DEFINE_GUID(CopyCurrentOnlyFileId,0x502d00df,0xee31,0x41cf,0x90,0x28,0x44,0x2d,0x2e,0x35,0x29,0x90);
// {89664EF4-BB8C-4932-A8C0-59CAFD937ABA}
DEFINE_GUID(MoveCurrentOnlyFileId,0x89664ef4,0xbb8c,0x4932,0xa8,0xc0,0x59,0xca,0xfd,0x93,0x7a,0xba);
// {937F0B1C-7690-4F85-8469-AA935517F202}
DEFINE_GUID(PluginsMenuId,0x937f0b1c,0x7690,0x4f85,0x84,0x69,0xaa,0x93,0x55,0x17,0xf2,0x02);
// {AFDAD388-494C-41E8-BAC6-BBE9115E1CC0}
DEFINE_GUID(EditorReloadId,0xafdad388,0x494c,0x41e8,0xba,0xc6,0xbb,0xe9,0x11,0x5e,0x1c,0xc0);
// {72E6E6D8-0BC6-4265-B9C4-C8DB712136AF}
DEFINE_GUID(FarAskQuitId,0x72e6e6d8,0x0bc6,0x4265,0xb9,0xc4,0xc8,0xdb,0x71,0x21,0x36,0xaf);
// {A204FF09-07FA-478C-98C9-E56F61377BDE}
DEFINE_GUID(AdvancedConfigId,0xa204ff09,0x07fa,0x478c,0x98,0xc9,0xe5,0x6f,0x61,0x37,0x7b,0xde);
// {4CD742BC-295F-4AFA-A158-7AA05A16BEA1}
DEFINE_GUID(FolderShortcutsId,0x4cd742bc,0x295f,0x4afa,0xa1,0x58,0x7a,0xa0,0x5a,0x16,0xbe,0xa1);
// {DC8D98AC-475C-4F37-AB1D-45765EF06269}
DEFINE_GUID(FolderShortcutsDlgId,0xdc8d98ac,0x475c,0x4f37,0xab,0x1d,0x45,0x76,0x5e,0xf0,0x62,0x69);
// {601DD149-92FA-4601-B489-74C981BC8E38}
DEFINE_GUID(FolderShortcutsMoreId,0x601dd149,0x92fa,0x4601,0xb4,0x89,0x74,0xc9,0x81,0xbc,0x8e,0x38);
// {72EB948A-5F1D-4481-9A91-A4BFD869D127}
DEFINE_GUID(ScreensSwitchId,0x72eb948a,0x5f1d,0x4481,0x9a,0x91,0xa4,0xbf,0xd8,0x69,0xd1,0x27);
// {B8B6E1DA-4221-47D2-AB2E-9EC67D0DC1E3}
DEFINE_GUID(SelectSortModeId,0xb8b6e1da,0x4221,0x47d2,0xab,0x2e,0x9e,0xc6,0x7d,0x0d,0xc1,0xe3);
// {880968A6-6258-43E0-9BDC-F2B8678EC278}
DEFINE_GUID(HistoryCmdId,0x880968a6,0x6258,0x43e0,0x9b,0xdc,0xf2,0xb8,0x67,0x8e,0xc2,0x78);
// {FC3384A8-6608-4C9B-8D6B-EE105F4C5A54}
DEFINE_GUID(HistoryFolderId,0xfc3384a8,0x6608,0x4c9b,0x8d,0x6b,0xee,0x10,0x5f,0x4c,0x5a,0x54);
// {E770E044-23A8-4F4D-B268-0E602B98CCF9}
DEFINE_GUID(HistoryEditViewId,0xe770e044,0x23a8,0x4f4d,0xb2,0x68,0x0e,0x60,0x2b,0x98,0xcc,0xf9);
// {B56D5C08-0336-418B-A2A7-CF0C80F93ACC}
DEFINE_GUID(PanelViewModesId,0xb56d5c08,0x0336,0x418b,0xa2,0xa7,0xcf,0x0c,0x80,0xf9,0x3a,0xcc);
// {98B75500-4A97-4299-BFAD-C3E349BF3674}
DEFINE_GUID(PanelViewModesEditId,0x98b75500,0x4a97,0x4299,0xbf,0xad,0xc3,0xe3,0x49,0xbf,0x36,0x74);
// {78A4A4E3-C2F0-40BD-9AA7-EAAC11836631}
DEFINE_GUID(CodePagesMenuId,0x78a4a4e3,0xc2f0,0x40bd,0x9a,0xa7,0xea,0xac,0x11,0x83,0x66,0x31);
// {8BCCDFFD-3B34-49F8-87CD-F4D885B75873}
DEFINE_GUID(EditorReplaceId,0x8bccdffd,0x3b34,0x49f8,0x87,0xcd,0xf4,0xd8,0x85,0xb7,0x58,0x73);
// {5D3CBA90-F32D-433C-B016-9BB4AF96FACC}
DEFINE_GUID(EditorSearchId,0x5d3cba90,0xf32d,0x433c,0xb0,0x16,0x9b,0xb4,0xaf,0x96,0xfa,0xcc);
// {F63B558F-9185-46BA-8701-D143B8F62658}
DEFINE_GUID(HelpSearchId,0xf63b558f,0x9185,0x46ba,0x87,0x01,0xd1,0x43,0xb8,0xf6,0x26,0x58);
// {5B87B32E-494A-4982-AF55-DAFFCD251383}
DEFINE_GUID(FiltersMenuId,0x5b87b32e,0x494a,0x4982,0xaf,0x55,0xda,0xff,0xcd,0x25,0x13,0x83);
// {EDDB9286-3B08-4593-8F7F-E5925A3A0FF8}
DEFINE_GUID(FiltersConfigId,0xeddb9286,0x3b08,0x4593,0x8f,0x7f,0xe5,0x92,0x5a,0x3a,0x0f,0xf8);
// {D0422DF0-AAF5-46E0-B98B-1776B427E70D}
DEFINE_GUID(HighlightMenuId,0xd0422df0,0xaaf5,0x46e0,0xb9,0x8b,0x17,0x76,0xb4,0x27,0xe7,0x0d);
// {51B6E342-B499-464D-978C-029F18ECCE59}
DEFINE_GUID(HighlightConfigId,0x51b6e342,0xb499,0x464d,0x97,0x8c,0x02,0x9f,0x18,0xec,0xce,0x59);
// {B4C242E7-AA8E-4449-B0C3-BD8D9FA11AED}
DEFINE_GUID(PluginsConfigMenuId,0xb4c242e7,0xaa8e,0x4449,0xb0,0xc3,0xbd,0x8d,0x9f,0xa1,0x1a,0xed);
// {252CE4A3-C415-4B19-956B-83E2FDD85960}
DEFINE_GUID(ChangeDiskMenuId,0x252ce4a3,0xc415,0x4b19,0x95,0x6b,0x83,0xe2,0xfd,0xd8,0x59,0x60);
// {F6D2437C-FEDC-4075-AA56-275666FC8979}
DEFINE_GUID(FileAssocMenuId,0xf6d2437c,0xfedc,0x4075,0xaa,0x56,0x27,0x56,0x66,0xfc,0x89,0x79);
// {D2BCB5A5-6B82-4EB5-B321-1AE7607A6236}
DEFINE_GUID(SelectAssocMenuId,0xd2bcb5a5,0x6b82,0x4eb5,0xb3,0x21,0x1a,0xe7,0x60,0x7a,0x62,0x36);
// {6F245B1A-47D9-41A6-AF3F-FA2C8DBEEBD0}
DEFINE_GUID(FileAssocModifyId,0x6f245b1a,0x47d9,0x41a6,0xaf,0x3f,0xfa,0x2c,0x8d,0xbe,0xeb,0xd0);
// {15568DC5-4D6B-4C60-B43D-2040EE39871A}
DEFINE_GUID(EditorSwitchUnicodeCPDisabledId,0x15568dc5,0x4d6b,0x4c60,0xb4,0x3d,0x20,0x40,0xee,0x39,0x87,0x1a);
// {CD2AC546-9E4F-4445-A258-AB5F7A7800E0}
DEFINE_GUID(GetNameAndPasswordId,0xcd2ac546,0x9e4f,0x4445,0xa2,0x58,0xab,0x5f,0x7a,0x78,0x00,0xe0);
// {4406C688-209F-4378-8B7B-465BF16205FF}
DEFINE_GUID(SelectFromEditHistoryId,0x4406c688,0x209f,0x4378,0x8b,0x7b,0x46,0x5b,0xf1,0x62,0x05,0xff);
// {D6F557E8-7E89-4895-BD75-4D3F2C30E382}
DEFINE_GUID(EditorReloadModalId,0xd6f557e8,0x7e89,0x4895,0xbd,0x75,0x4d,0x3f,0x2c,0x30,0xe3,0x82);
// {CCA2C4D0-8705-4FA1-9B10-C9E3C8F37A65}
DEFINE_GUID(EditorCanNotEditDirectoryId,0xcca2c4d0,0x8705,0x4fa1,0x9b,0x10,0xc9,0xe3,0xc8,0xf3,0x7a,0x65);
// {E3AFCD2D-BDE5-4E92-82B6-87C6A7B78FB6}
DEFINE_GUID(EditorFileLongId,0xe3afcd2d,0xbde5,0x4e92,0x82,0xb6,0x87,0xc6,0xa7,0xb7,0x8f,0xb6);
// {6AD4B317-C1ED-44C8-A76A-9146CA8AF984}
DEFINE_GUID(EditorFileGetSizeErrorId,0x6ad4b317,0xc1ed,0x44c8,0xa7,0x6a,0x91,0x46,0xca,0x8a,0xf9,0x84);
// {A1BDBEB1-2911-41FF-BC08-EEBC44040B50}
DEFINE_GUID(DisconnectDriveId,0xa1bdbeb1,0x2911,0x41ff,0xbc,0x08,0xee,0xbc,0x44,0x04,0x0b,0x50);
// {F87F9351-6A80-4872-BEEE-96EF80C809FB}
DEFINE_GUID(ChangeDriveModeId,0xf87f9351,0x6a80,0x4872,0xbe,0xee,0x96,0xef,0x80,0xc8,0x09,0xfb);
// {75554EEB-A3A7-45FD-9795-4A85887A75A0}
DEFINE_GUID(SUBSTDisconnectDriveId,0x75554eeb,0xa3a7,0x45fd,0x97,0x95,0x4a,0x85,0x88,0x7a,0x75,0xa0);
// {629A8CA6-25C6-498C-B3DD-0E18D1CC0BCD}
DEFINE_GUID(VHDDisconnectDriveId,0x629a8ca6,0x25c6,0x498c,0xb3,0xdd,0x0e,0x18,0xd1,0xcc,0x0b,0xcd);
// {9BD3E306-EFB8-4113-8405-E7BADE8F0A59}
DEFINE_GUID(EditorFindAllListId,0x9bd3e306,0xefb8,0x4113,0x84,0x05,0xe7,0xba,0xde,0x8f,0x0a,0x59);
// {4811039D-03A3-4F15-8D7A-8EBC4BCC97F9}
DEFINE_GUID(BadEditorCodePageId,0x4811039d,0x03a3,0x4f15,0x8d,0x7a,0x8e,0xbc,0x4b,0xcc,0x97,0xf9);
// {D2750B57-D3E6-42F4-8137-231C50DDC6E4}
DEFINE_GUID(UserMenuUserInputId,0xd2750b57,0xd3e6,0x42f4,0x81,0x37,0x23,0x1c,0x50,0xdd,0xc6,0xe4);
// {D8AF7A38-8357-44A5-A44B-A595CF707549}
DEFINE_GUID(DescribeFileId,0xd8af7a38,0x8357,0x44a5,0xa4,0x4b,0xa5,0x95,0xcf,0x70,0x75,0x49);
// {29C03C36-9C50-4F78-AB99-F5DC1A9C67CD}
DEFINE_GUID(SelectDialogId,0x29c03c36,0x9c50,0x4f78,0xab,0x99,0xf5,0xdc,0x1a,0x9c,0x67,0xcd);
// {34614DDB-2A22-4EA9-BD4A-2DC075643F1B}
DEFINE_GUID(UnSelectDialogId,0x34614ddb,0x2a22,0x4ea9,0xbd,0x4a,0x2d,0xc0,0x75,0x64,0x3f,0x1b);
// {FF18299E-1881-42FA-AF7E-AC05D99F269C}
DEFINE_GUID(SUBSTDisconnectDriveError1Id,0xff18299e,0x1881,0x42fa,0xaf,0x7e,0xac,0x05,0xd9,0x9f,0x26,0x9c);
// {43B0FFC2-70BE-4289-91E6-FE9A3D54311B}
DEFINE_GUID(SUBSTDisconnectDriveError2Id,0x43b0ffc2,0x70be,0x4289,0x91,0xe6,0xfe,0x9a,0x3d,0x54,0x31,0x1b);
// {D6DC3621-877E-4BE2-80CC-BDB2864CE038}
DEFINE_GUID(EjectHotPlugMediaErrorId,0xd6dc3621,0x877e,0x4be2,0x80,0xcc,0xbd,0xb2,0x86,0x4c,0xe0,0x38);
// {F06953B8-25AA-4FC0-9899-422FC1D49F7A}
DEFINE_GUID(RemoteDisconnectDriveError2Id,0xf06953b8,0x25aa,0x4fc0,0x98,0x99,0x42,0x2f,0xc1,0xd4,0x9f,0x7a);
// {C9439386-9544-49BF-954B-6BEEDE7F1BD0}
DEFINE_GUID(RemoteDisconnectDriveError1Id,0xc9439386,0x9544,0x49bf,0x95,0x4b,0x6b,0xee,0xde,0x7f,0x1b,0xd0);
// {B890E6B0-05A9-4ED8-A4C3-BBC4D29DA3BE}
DEFINE_GUID(VHDDisconnectDriveErrorId,0xb890e6b0,0x05a9,0x4ed8,0xa4,0xc3,0xbb,0xc4,0xd2,0x9d,0xa3,0xbe);
// {F3D46DC3-380B-4264-8BF8-10B05B897A5E}
DEFINE_GUID(ChangeDriveCannotReadDiskErrorId,0xf3d46dc3,0x380b,0x4264,0x8b,0xf8,0x10,0xb0,0x5b,0x89,0x7a,0x5e);
// {044EF83E-8146-41B2-97F0-404C2F4C7B69}
DEFINE_GUID(ApplyCommandId,0x044ef83e,0x8146,0x41b2,0x97,0xf0,0x40,0x4c,0x2f,0x4c,0x7b,0x69);
// {6EF09401-6FE1-495A-8539-61B0F761408E}
DEFINE_GUID(DeleteFileFolderId,0x6ef09401,0x6fe1,0x495a,0x85,0x39,0x61,0xb0,0xf7,0x61,0x40,0x8e);
// {85A5F779-A881-4B0B-ACEE-6D05653AE0EB}
DEFINE_GUID(DeleteRecycleId,0x85a5f779,0xa881,0x4b0b,0xac,0xee,0x6d,0x05,0x65,0x3a,0xe0,0xeb);
// {9C054039-5C7E-4B04-96CD-3585228C916F}
DEFINE_GUID(DeleteWipeId,0x9c054039,0x5c7e,0x4b04,0x96,0xcd,0x35,0x85,0x22,0x8c,0x91,0x6f);
// {B1099BC3-14BD-4B22-87AC-44770D4189A3}
DEFINE_GUID(DeleteLinkId,0xb1099bc3,0x14bd,0x4b22,0x87,0xac,0x44,0x77,0x0d,0x41,0x89,0xa3);
// {4E714029-11BF-476F-9B17-9E47AA0DA8EA}
DEFINE_GUID(DeleteFolderId,0x4e714029,0x11bf,0x476f,0x9b,0x17,0x9e,0x47,0xaa,0x0d,0xa8,0xea);
// {E23BB390-036E-4A30-A9E6-DC621617C7F5}
DEFINE_GUID(WipeFolderId,0xe23bb390,0x036e,0x4a30,0xa9,0xe6,0xdc,0x62,0x16,0x17,0xc7,0xf5);
// {A318CBDC-DBA9-49E9-A248-E6A9FF8EC849}
DEFINE_GUID(DeleteFolderRecycleId,0xa318cbdc,0xdba9,0x49e9,0xa2,0x48,0xe6,0xa9,0xff,0x8e,0xc8,0x49);
// {6792A975-57C5-4110-8129-2D8045120964}
DEFINE_GUID(DeleteAskWipeROId,0x6792a975,0x57c5,0x4110,0x81,0x29,0x2d,0x80,0x45,0x12,0x09,0x64);
// {8D4E84B3-08F6-47DF-8C40-7130CD31D0E6}
DEFINE_GUID(DeleteAskDeleteROId,0x8d4e84b3,0x08f6,0x47df,0x8c,0x40,0x71,0x30,0xcd,0x31,0xd0,0xe6);
// {5297DDFE-0A37-4465-85EF-CBF9006D65C6}
DEFINE_GUID(WipeHardLinkId,0x5297ddfe,0x0a37,0x4465,0x85,0xef,0xcb,0xf9,0x00,0x6d,0x65,0xc6);
// {26A7AB9F-51F5-40F7-9061-1AE6E2FBD00A}
DEFINE_GUID(RecycleFolderConfirmDeleteLinkId,0x26a7ab9f,0x51f5,0x40f7,0x90,0x61,0x1a,0xe6,0xe2,0xfb,0xd0,0x0a);
// {52CEB5A5-06FA-43DD-B37C-239C02652C99}
DEFINE_GUID(CannotRecycleFileId,0x52ceb5a5,0x06fa,0x43dd,0xb3,0x7c,0x23,0x9c,0x02,0x65,0x2c,0x99);
// {BBD9B7AE-9F6B-4444-89BF-C6124A5A83A4}
DEFINE_GUID(CannotRecycleFolderId,0xbbd9b7ae,0x9f6b,0x4444,0x89,0xbf,0xc6,0x12,0x4a,0x5a,0x83,0xa4);
// {57209AD5-51F6-4257-BAB6-837462BBCE74}
DEFINE_GUID(AskInsertMenuOrCommandId,0x57209ad5,0x51f6,0x4257,0xba,0xb6,0x83,0x74,0x62,0xbb,0xce,0x74);
// {73BC6E3E-4CC3-4FE3-8709-545FF72B49B4}
DEFINE_GUID(EditUserMenuId,0x73bc6e3e,0x4cc3,0x4fe3,0x87,0x09,0x54,0x5f,0xf7,0x2b,0x49,0xb4);
// {FC4FD19A-43D2-4987-AC31-0F7A94901692}
DEFINE_GUID(PluginInformationId,0xfc4fd19a,0x43d2,0x4987,0xac,0x31,0x0f,0x7a,0x94,0x90,0x16,0x92);
// {03B6C098-A3D6-4DFB-AED4-EB32D711D9AA}
DEFINE_GUID(ViewerSearchId,0x03b6c098,0xa3d6,0x4dfb,0xae,0xd4,0xeb,0x32,0xd7,0x11,0xd9,0xaa);
// {CCE538E9-5B53-4AD5-B8CF-C2302110B1F2}
DEFINE_GUID(EditorConfirmReplaceId,0xcce538e9,0x5b53,0x4ad5,0xb8,0xcf,0xc2,0x30,0x21,0x10,0xb1,0xf2);
// {F8CE8646-BC51-4EEF-8162-ED5BA4C913E0}
DEFINE_GUID(MaskGroupsMenuId,0xF8CE8646,0xBC51,0x4EEF,0x81,0x62,0xED,0x5B,0xA4,0xC9,0x13,0xE0);
// {C57682CA-8DC9-4D62-B3F5-9ED37CD207B9}
DEFINE_GUID(EditMaskGroupId,0xC57682CA,0x8DC9,0x4D62,0xB3,0xF5,0x9E,0xD3,0x7C,0xD2,0x07,0xB9);
