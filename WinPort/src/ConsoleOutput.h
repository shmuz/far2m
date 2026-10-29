#pragma once
#include <mutex>
#include <vector>
#include <string>
#include <condition_variable>
#include "WinCompat.h"
#include "ConsoleBuffer.h"
#include "Backend.h"

class ConsoleOutput : public IConsoleOutput
{
	HANDLE _con_handle{NULL};
	std::mutex _mutex;
	ConsoleBuffer _buf;
	std::vector<CHAR_INFO> _temp_chars;
	std::wstring _title;
	IConsoleOutputBackend *_backend;
	DWORD _mode;
	DWORD64 _attributes;
	COORD _prev_pos{-1, -1};
	unsigned short _repaint_defer{0}; // unlikely it will be more than 10..
	struct DeferredRepaints : std::vector<SMALL_RECT>
	{
		void Add(const SMALL_RECT &area);
		void Add(const SMALL_RECT *areas, size_t cnt);
	} _deferred_repaints;
	unsigned int _change_id{1};
	std::condition_variable _change_id_cond;

	struct {
		COORD pos;
		UCHAR height;
		bool visible;
	} _cursor;

	struct {
		PCONSOLE_SCROLL_CALLBACK pfn;
		PVOID context;
	} _scroll_callback;

	struct {
		USHORT top;
		USHORT bottom;
	} _scroll_region;

	struct SequenceModifier
	{
		enum {
			SM_WRITE_STR,
			SM_FILL_CHAR,
			SM_FILL_ATTR
		} kind;
		size_t count;
		union {
			const WCHAR *str;
			TCHAR chr;
			DWORD64 attr;
		};
	};

	void LockedChangeIdUpdate();

	SHORT ModifySequenceEntityAt(SequenceModifier &sm, COORD pos, SMALL_RECT &area);
	size_t ModifySequenceAt(SequenceModifier &sm, COORD &pos);
	void ScrollOutputOnOverflow(SMALL_RECT &area);

	const WCHAR *LockedGetTitle() override;
	CHAR_INFO *LockedDirectLineAccess(size_t line_index, unsigned int &width) override;
	void Unlock() override;
	void SetUpdateCellArea(SMALL_RECT &area, COORD pos);
	void CopyFrom(const ConsoleOutput &co);

public:
	ConsoleOutput();
	void SetBackend(IConsoleOutputBackend *listener) override;

	void SetAttributes(DWORD64 attributes) override;
	DWORD64 GetAttributes() override;

	virtual	void SetCursorBlinkTime(DWORD interval) override;
	void SetCursor(COORD pos) override;
	void SetCursor(UCHAR height, bool visible) override;
	COORD GetCursor() override;
	COORD GetCursor(UCHAR &height, bool &visible) override;

	void SetSize(unsigned int width, unsigned int height) override;
	void GetSize(unsigned int &width, unsigned int &height) override;

	COORD GetLargestConsoleWindowSize() override;
	void SetWindowMaximized(bool maximized) override;

	void SetWindowInfo(bool absolute, const SMALL_RECT &rect) override;
	void SetTitle(const WCHAR *title) override;

	DWORD GetMode() override;
	void SetMode(DWORD mode) override;

	void Read(CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect) override;
	void Write(const CHAR_INFO *data, COORD data_size, COORD data_pos, SMALL_RECT &screen_rect) override;
	bool Read(CHAR_INFO &data, COORD screen_pos) override;
	bool Write(const CHAR_INFO &data, COORD screen_pos) override;

	size_t WriteString(const WCHAR *data, size_t count) override;
	size_t WriteStringAt(const WCHAR *data, size_t count, COORD &pos) override;
	size_t FillCharacterAt(WCHAR cCharacter, size_t count, COORD &pos) override;
	size_t FillAttributeAt(DWORD64 qAttribute, size_t count, COORD &pos) override;

	bool Scroll(const SMALL_RECT *lpScrollRectangle, const SMALL_RECT *lpClipRectangle,
				COORD dwDestinationOrigin, const CHAR_INFO *lpFill) override;

	void SetScrollRegion(SHORT top, SHORT bottom) override;
	void GetScrollRegion(SHORT &top, SHORT &bottom) override;
	void SetScrollCallback(PCONSOLE_SCROLL_CALLBACK pCallback, PVOID pContext) override;

	void AdhocQuickEdit() override;
	DWORD64 SetConsoleTweaks(DWORD64 tweaks) override;
	void ConsoleChangeFont() override;
	void ConsoleSaveWindowState() override;
	bool IsActive() override;
	void ConsoleDisplayNotification(const WCHAR *title, const WCHAR *text) override;
	bool ConsoleBackgroundMode(bool TryEnterBackgroundMode) override;
	bool SetFKeyTitles(const CHAR **titles) override;
	BYTE GetColorPalette() override;
	void GetBasePalette(void *p) override;
	bool SetBasePalette(void *p) override;
	void OverrideColor(DWORD Index, DWORD *ColorFG, DWORD *ColorBK) override;
	void RepaintsDeferStart() override;
	void RepaintsDeferFinish(bool force) override;

	IConsoleOutput *ForkConsoleOutput(HANDLE con_handle) override;
	void JoinConsoleOutput(IConsoleOutput *con_out) override;

	unsigned int WaitForChange(unsigned int prev_change_id, unsigned int timeout_msec = -1) override;
	const char *BackendInfo(int entity) override;
	void *GetNativeWindowHandle() override;

	void OnGetConsoleImageCaps(WinportGraphicsInfo *wgi) override;
	bool OnSetConsoleImage(const char *id, DWORD64 flags, COORD pos, DWORD width, DWORD height, const void *buffer) override;
	bool OnDeleteConsoleImage(const char *id) override;
};
