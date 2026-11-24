#pragma once

#include "frame.hpp"
#include "edit.hpp"
#include "FARString.hpp"

class Panel;

class FastFind final : public Frame
{
private:
	Panel *OwnerPanel;
	FarKey FirstKey;
	Edit FindEdit;
	FarKey KeyToProcess;
	bool Closed;

	void InitPositionAndSize();
	void ProcessName(const wchar_t *Src);
	void ShowBorder();
	void Close(int ExitCode);

protected:
	void DisplayObject() override;

public:
	FastFind(Panel *Owner, FarKey FirstKey);
	~FastFind() override = default;

	int ProcessKey(FarKey Key) override;
	int ProcessMouse(MOUSE_EVENT_RECORD *MouseEvent) override;
	void ResizeConsole() override;
	int FastHide() override { return TRUE; }
	int GetType() const override { return MODALTYPE_SEARCH; }
	int GetTypeAndName(FARString &strType, FARString &strName) override;
	const wchar_t *GetTypeName() const override { return L"[FastFind]"; }

	void Process();
	FarKey GetKeyToProcess() const { return KeyToProcess; }
};

