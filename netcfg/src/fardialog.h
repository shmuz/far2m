#ifndef __FARDIALOG_H__
#define __FARDIALOG_H__

#include <farplug-wide.h>
#include <set>
#include <string>
#include <memory>

typedef struct {
	int type;
	bool incY;
	int X1,X2;
	DWORD flags;
	union {
		const wchar_t * ptrData;
		FarLangMsgID lngIdstring;
	} data;
} DlgConstructorItem;

struct ItemChange {
	unsigned int itemNum;
	int type;
	union {
		int Selected;
		const wchar_t * ptrData;
	} oldVal;
	union {
		int Selected;
		const wchar_t * ptrData;
	} newVal;
	std::wstring ptrData;
	bool empty;
	explicit ItemChange(unsigned int itemNum, int type);
	ItemChange(ItemChange && old);
};

enum {
	WinSuffixSeparatorIndex,
	WinSuffixOkIndex,
	WinSuffixCancelIndex
};

#define DEFAUL_Y_START 1
#define DEFAUL_X_START 3

#define DI_ENDDIALOG (DI_USERCONTROL+1)

class FarDlgConstructor {
	private:
		std::vector<FarDialogItem> dlg;
		std::set<unsigned int> free_items;
		std::wstring helpTopic;
	public:	
		int xStart, yStart, xEnd, yEnd;

		typedef std::vector<FarDialogItem>::const_iterator const_iterator;
		const_iterator begin() const { return dlg.begin(); };
		const_iterator end() const { return dlg.end(); };

		unsigned int Append(const DlgConstructorItem * item);
		unsigned int AppendItems(const DlgConstructorItem * items);
		unsigned int AppendOkCancel(void);

		const wchar_t * GetHelpTopic(void);
		void SetHelpTopic(const wchar_t * help);
		unsigned int GetNumberOfItems(void);
		FarDialogItem * GetDialogItem(unsigned int itemNum);
		const wchar_t * GetConstText(unsigned int itemNum) const;

		void SetSelected(unsigned int itemNum, bool selected);
		void SetFlags(unsigned int itemNum, DWORD flags);
		void OrFlags(unsigned int itemNum, DWORD flags);
		void AndFlags(unsigned int itemNum, DWORD flags);

		void HideItems(unsigned int itemFirst, unsigned int itemLast);
		void UnhideItems(unsigned int itemFirst, unsigned int itemLast);

		DWORD GetFlags(unsigned int itemNum);
		void SetFocus(unsigned int itemNum);
		void SetDefaultButton(unsigned int itemNum);
		void SetMaxTextLen(unsigned int itemNum, size_t maxLen);
		void SetText(unsigned int itemNum, const wchar_t * text, bool dublicate=false);
		void SetX(unsigned int itemNum, int x);
		void SetCountText(unsigned int itemNum, int64_t value);
		void SetCountHex32Text(unsigned int itemNum, uint32_t value, bool prefix = true);
		void SetFileSizeText(unsigned int itemNum, uint64_t value);


		FarDlgConstructor(const DlgConstructorItem * items, int yStart = DEFAUL_Y_START);
		FarDlgConstructor(const wchar_t * title,
						int width,  // xEnd
						int xStart = DEFAUL_X_START,
						int yStart = DEFAUL_Y_START);
		~FarDlgConstructor();
};

void ChangeDialogItemsView(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast, bool hide, bool disable);
bool ShowHideElements(HANDLE hDlg, uint32_t chk, uint32_t chkStore, uint32_t begin, uint32_t end);
void HideDialogItems(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast);
void UnhideDialogItems(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast);
void DisableDialogItems(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast);
void EnableDialogItems(HANDLE hDlg, unsigned int itemFirst, unsigned int itemLast);
uint64_t GetNumberItem(HANDLE hDlg, unsigned int itemNum);
std::wstring GetText(HANDLE hDlg, unsigned int itemNum);

class FarDialog {
	private:
		FarDlgConstructor * fdc;
		HANDLE hDlg;
	public:
		explicit FarDialog(FarDlgConstructor * data, FARWINDOWPROC dlgProc = 0, LONG_PTR param = 0);
		~FarDialog();
		int Run(void);
		bool CreateChangeList(std::vector<ItemChange> & chlst);

		std::wstring GetText(unsigned int itemNum);
		const wchar_t * GetConstText(unsigned int itemNum);
		int32_t GetInt32(unsigned int itemNum);
		int64_t GetInt64(unsigned int itemNum);
		int32_t GetInt32FromHex(unsigned int itemNum, bool prefix = true);
		int64_t GetInt64FromHex(unsigned int itemNum, bool prefix = true);
		bool GetCheck(unsigned int itemNum);
		HANDLE GetDlg(void);
};
#endif
