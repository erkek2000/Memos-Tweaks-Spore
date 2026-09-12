#include "stdafx.h"
#include "KeyboardShortcuts.h"

#include <Spore\App\IMessageManager.h>
#include <Spore\Palettes\PalettePageUI.h>
#include <Spore\Simulator.h>
#include <Spore\Simulator\cSimulatorSpaceGame.h>
#include <Spore\Simulator\SubSystem\CommManager.h>
#include <Spore\Simulator\SubSystem\GameInputManager.h>
#include <Spore\Simulator\SubSystem\GameModeManager.h>
#include <Spore\Simulator\SubSystem\GameTimeManager.h>
#include <Spore\UI\SpaceToolPanelUI.h>
#include <Spore\UTFWin\IWinProc.h>
#include <Spore\UTFWin\IWindowManager.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace
{
	constexpr uint32_t kCommScreenRootID = 0x01C3BB0C;
	constexpr size_t kNativeCurrentCommEventOffset = 0x20;
	constexpr ULONGLONG kCommRecoveryRetryMilliseconds = 250;

	void AppendKeyboardLog(const char* line)
	{
		wchar_t appData[MAX_PATH]{};
		if (GetEnvironmentVariableW(L"APPDATA", appData, _countof(appData)) == 0) return;
		const std::wstring directory = std::wstring(appData) + L"\\Spore\\ERKEK2000_QoL";
		CreateDirectoryW(directory.c_str(), nullptr);
		const std::wstring path = directory + L"\\keyboard-input.log";
		HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return;
		DWORD written = 0;
		WriteFile(file, line, static_cast<DWORD>(strlen(line)), &written, nullptr);
		WriteFile(file, "\r\n", 2, &written, nullptr);
		FlushFileBuffers(file);
		CloseHandle(file);
	}

	bool IsKeyboardDiagnosticKey(int virtualKey)
	{
		return virtualKey == VK_ESCAPE || virtualKey == VK_TAB || virtualKey == VK_SPACE ||
			virtualKey == VK_END ||
			(virtualKey >= '0' && virtualKey <= '9');
	}

	bool IsVisibleInHierarchy(UTFWin::IWindow* window)
	{
		if (window == nullptr) return false;
		for (UTFWin::IWindow* current = window; current != nullptr; current = current->GetParent())
		{
			const UTFWin::WindowFlags flags = current->GetFlags();
			if ((flags & UTFWin::kWinFlagVisible) == 0)
			{
				return false;
			}
		}
		return true;
	}

	bool IsUsable(UTFWin::IWindow* window)
	{
		return IsVisibleInHierarchy(window) &&
			(window->GetFlags() & UTFWin::kWinFlagEnabled) != 0;
	}

	cCommEventPtr GetCurrentCommEvent(Simulator::cCommManager* manager)
	{
		// The SDK's cStrategy size places this C++ field at 0x1C, while its
		// cCommManager layout documents the game's native event slot at 0x20.
		// Copying manager->mCurrentCommEvent reads the preceding native field and
		// attempts AddRef on an unrelated pointer.
		const auto* eventSlot = reinterpret_cast<const cCommEventPtr*>(
			reinterpret_cast<const char*>(manager) + kNativeCurrentCommEventOffset);
		return *eventSlot;
	}

	cCommEventPtr sPendingCommExitEvent;
	int sPendingCommExitKey = 0;
	cCommEventPtr sPendingCommRecoveryEvent;
	ULONGLONG sCommRecoveryDeadline = 0;

	bool IsCommScreenRootVisible()
	{
		UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
		if (mainWindow == nullptr)
		{
			return false;
		}

		UTFWin::IWindow* screen = mainWindow->FindWindowByID(kCommScreenRootID);
		return IsVisibleInHierarchy(screen);
	}

	bool ClickPaletteItem(UTFWin::IWindow* window, uint32_t itemID, uint32_t alternateItemID = 0)
	{
		if (window == nullptr)
		{
			return false;
		}

		for (UTFWin::IWinProc* procedure : window->procedures())
		{
			Palettes::PalettePageUI* page = object_cast<Palettes::PalettePageUI>(procedure);
			if (page == nullptr)
			{
				continue;
			}

			for (const StandardItemUIPtr& itemUI : page->mStandardItems)
			{
				if (itemUI == nullptr || itemUI->mpItem == nullptr)
				{
					continue;
				}

				const uint32_t candidateID = itemUI->mpItem->mName.instanceID;
				if (candidateID != itemID &&
					(alternateItemID == 0 || candidateID != alternateItemID))
				{
					continue;
				}

				UTFWin::IWindow* itemWindow = itemUI->mpWindow.get();
				if (!IsUsable(itemWindow))
				{
					continue;
				}

				UTFWin::Message click{};
				click.source = itemWindow;
				click.eventType = UTFWin::kMsgButtonClick;
				WindowManager.SendMsg(itemWindow, itemWindow, click, true);
				return true;
			}
		}

		for (UTFWin::IWindow* child : window->children())
		{
			if (ClickPaletteItem(child, itemID, alternateItemID))
			{
				return true;
			}
		}
		return false;
	}

	bool DispatchToVisibleToolPanel(UTFWin::IWindow* window, int virtualKey, KeyModifiers modifiers)
	{
		// Container windows often are not themselves interactive. Requiring the
		// Enabled flag on every ancestor hides usable buttons/panels nested below
		// those containers; only visibility is inherited through the tree.
		if (!IsVisibleInHierarchy(window))
		{
			return false;
		}

		UTFWin::Message message{};
		message.source = window;
		message.Key.vkey = virtualKey;
		message.Key.modifiers = static_cast<int>(modifiers.value);

		// KeyDown and KeyDown2 are the two native UTFWin keyboard events. Send
		// each only until the native panel handles the key, so a tool is selected
		// at most once.
		for (int eventType : { UTFWin::kMsgKeyDown, UTFWin::kMsgKeyDown2 })
		{
			message.eventType = eventType;
			for (UTFWin::IWinProc* procedure : window->procedures())
			{
				UI::SpaceToolPanelUI* panel = object_cast<UI::SpaceToolPanelUI>(procedure);
				if (panel != nullptr && IsUsable(window) && panel->HandleUIMessage(window, message))
				{
					return true;
				}
			}
		}

		for (UTFWin::IWindow* child : window->children())
		{
			if (DispatchToVisibleToolPanel(child, virtualKey, modifiers))
			{
				return true;
			}
		}
		return false;
	}

	bool HandleCommunicationShortcut(int virtualKey, KeyModifiers modifiers)
	{
#if ERKEK_CLOSE_WITH_ESCAPE || ERKEK_CLOSE_WITH_TAB || ERKEK_CLOSE_WITH_SPACEBAR || ERKEK_CLOSE_WITH_END
		const bool closeKey =
			(ERKEK_CLOSE_WITH_ESCAPE && virtualKey == VK_ESCAPE) ||
			(ERKEK_CLOSE_WITH_TAB && virtualKey == VK_TAB) ||
			(ERKEK_CLOSE_WITH_SPACEBAR && virtualKey == VK_SPACE && modifiers.value == 0) ||
			(ERKEK_CLOSE_WITH_END && virtualKey == VK_END);
		if (!closeKey)
		{
			return false;
		}

		Simulator::cCommManager* manager = Simulator::cCommManager::Get();
		if (manager == nullptr || !manager->IsCommScreenActive())
		{
			char line[96];
			_snprintf_s(line, _countof(line), _TRUNCATE,
				"comm key %02X passed: no active comm screen", virtualKey);
			AppendKeyboardLog(line);
			return false;
		}

		UTFWin::IButton* exitButton = manager->GetCommButton(Simulator::kBtnExit);
		UTFWin::IWindow* exitWindow = exitButton == nullptr ? nullptr : exitButton->ToWindow();
		if (!IsUsable(exitWindow))
		{
			char line[160];
			_snprintf_s(line, _countof(line), _TRUNCATE,
				"comm key %02X ignored: Goodbye hidden/disabled (flags=%X)",
				virtualKey, exitWindow == nullptr ? 0 : static_cast<unsigned>(exitWindow->GetFlags()));
			AppendKeyboardLog(line);
			// Hidden or disabled Goodbye controls indicate a submenu whose own
			// native cancel/confirmation behavior must remain in charge.
			return false;
		}

		// Keep the current event alive until the update callback can safely invoke
		// the native exit action after this input callback has returned.
		cCommEventPtr event = GetCurrentCommEvent(manager);
		if (event == nullptr)
		{
			AppendKeyboardLog("comm exit action skipped: active screen has no current event");
			return false;
		}

		sPendingCommExitEvent = event;
		sPendingCommExitKey = virtualKey;
		char line[128];
		_snprintf_s(line, _countof(line), _TRUNCATE,
			"comm key %02X queued deferred exit event=%d source=%08X",
			virtualKey, static_cast<int>(event->mEventType),
			static_cast<unsigned>(event->mSource));
		AppendKeyboardLog(line);
		return true;
#else
		(void)virtualKey;
		(void)modifiers;
		return false;
#endif
	}

	bool HandleBuildingShortcut(int virtualKey, KeyModifiers modifiers)
	{
#if ERKEK_BUILDING_SHORTCUTS
		if (modifiers.value != 0 ||
			!Simulator::IsSpaceGame() || Simulator::IsLoadingGameMode() ||
			Simulator::cSimulatorSpaceGame::Get() == nullptr ||
			SimulatorSpaceGame.mpCommunityEditor == 0)
		{
			return false;
		}

		uint32_t itemID = 0;
		uint32_t alternateItemID = 0;
		switch (virtualKey)
		{
		case '1': itemID = id("house"); break;
		case '2': itemID = id("entertainment"); break;
		case '3': itemID = id("factory"); break;
		case '4':
			itemID = id("Space_turret");
			alternateItemID = id("turret");
			break;
		default:
			return false;
		}

		Simulator::cSimulatorSpaceGame* game = Simulator::cSimulatorSpaceGame::Get();
		const int previousSelection = game == nullptr ? 0 : game->mpSelectedBuilding;
		const bool clicked = ClickPaletteItem(WindowManager.GetMainWindow(), itemID, alternateItemID);
		const int currentSelection = game == nullptr ? 0 : game->mpSelectedBuilding;
		char line[160];
		_snprintf_s(line, _countof(line), _TRUNCATE,
			"planner key %02X item=%08X click=%d selected=%08X->%08X",
			virtualKey, itemID, clicked ? 1 : 0,
			static_cast<unsigned>(previousSelection), static_cast<unsigned>(currentSelection));
		AppendKeyboardLog(line);
		return clicked;
#else
		(void)virtualKey;
		(void)modifiers;
		return false;
#endif
	}

	bool HandleSpaceHotbarShortcut(int virtualKey, KeyModifiers modifiers)
	{
#if ERKEK_SPACE_HOTBAR_SHORTCUTS
		if (modifiers.value != 0 || virtualKey < '0' || virtualKey > '9' ||
			!Simulator::IsSpaceGame() || Simulator::IsLoadingGameMode())
		{
			return false;
		}

		Simulator::cSimulatorSpaceGame* game = Simulator::cSimulatorSpaceGame::Get();
		if (game == nullptr || game->GetUI() == nullptr || game->mpCommunityEditor != 0)
		{
			return false;
		}

		const bool selected = DispatchToVisibleToolPanel(
			WindowManager.GetMainWindow(), virtualKey, modifiers);
		char line[96];
		_snprintf_s(line, _countof(line), _TRUNCATE,
			"space hotbar key %02X dispatched=%d", virtualKey, selected ? 1 : 0);
		AppendKeyboardLog(line);
		return selected;
#else
		(void)virtualKey;
		(void)modifiers;
		return false;
#endif
	}

		member_detour(KeyboardShortcutInput, Simulator::cGameInputManager,
		bool(int, KeyModifiers))
	{
		bool detoured(int virtualKey, KeyModifiers modifiers)
		{
			const bool diagnosticKey = IsKeyboardDiagnosticKey(virtualKey);
			if (HandleCommunicationShortcut(virtualKey, modifiers))
			{
				// The shortcut owns this key. Native OnKeyDown returned false in the
				// colony dialogue test and did not release the stale comm state.
				char line[112];
				_snprintf_s(line, _countof(line), _TRUNCATE,
					"comm key %02X consumed for deferred exit",
					virtualKey);
				AppendKeyboardLog(line);
				return true;
			}
			// The planner gets first refusal on 1-4. Outside it, the same keys
			// are ordinary Space hotbar slots.
			if (HandleBuildingShortcut(virtualKey, modifiers) ||
				HandleSpaceHotbarShortcut(virtualKey, modifiers))
			{
				return true;
			}
			if (diagnosticKey)
			{
				Simulator::cSimulatorSpaceGame* game = Simulator::cSimulatorSpaceGame::Get();
				const bool inSpace = Simulator::IsSpaceGame();
				const bool planner = inSpace && game != nullptr && game->mpCommunityEditor != 0;
				Simulator::cCommManager* comm = Simulator::cCommManager::Get();
				const bool inCommunication = comm != nullptr && comm->IsCommScreenActive();
				char line[160];
				_snprintf_s(line, _countof(line), _TRUNCATE,
					"key %02X mods=%X comm=%d space=%d planner=%d passed native",
					virtualKey, modifiers.value, inCommunication ? 1 : 0,
					inSpace ? 1 : 0, planner ? 1 : 0);
				AppendKeyboardLog(line);
			}
			return original_function(this, virtualKey, modifiers);
		}
	};

	eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;
	bool sAttachAttempted = false;
	bool sAttached = false;
	bool sLoggedMissingManager = false;

	void ProcessPendingCommunicationExit()
	{
		if (sPendingCommExitEvent == nullptr)
		{
			return;
		}

		// Run only after the central input callback has returned. The native exit
		// action may close its own UI and is unsafe to invoke from OnKeyDown.
		cCommEventPtr event = sPendingCommExitEvent;
		const int virtualKey = sPendingCommExitKey;
		sPendingCommExitEvent = nullptr;
		sPendingCommExitKey = 0;

		Simulator::cCommManager* manager = Simulator::cCommManager::Get();
		if (manager == nullptr || !manager->IsCommScreenActive() ||
			GetCurrentCommEvent(manager).get() != event.get())
		{
			AppendKeyboardLog("deferred comm exit skipped: communication event changed");
			return;
		}

		UTFWin::IButton* exitButton = manager->GetCommButton(Simulator::kBtnExit);
		UTFWin::IWindow* exitWindow = exitButton == nullptr ? nullptr : exitButton->ToWindow();
		if (!IsUsable(exitWindow))
		{
			AppendKeyboardLog("deferred comm exit skipped: Goodbye hidden/disabled");
			return;
		}

		Simulator::CnvAction exitAction{};
		exitAction.actionID = Simulator::kCnvCommExit;
		char line[144];
		_snprintf_s(line, _countof(line), _TRUNCATE,
			"comm key %02X executing deferred exit event=%d source=%08X",
			virtualKey, static_cast<int>(event->mEventType),
			static_cast<unsigned>(event->mSource));
		AppendKeyboardLog(line);

		switch (event->mEventType)
		{
		case Simulator::cCommEventType::Space:
			manager->HandleSpaceCommAction(exitAction, event->mSource, event->mPlanetKey,
				event->mpMission.get());
			break;
		case Simulator::cCommEventType::Civ:
			manager->HandleCivCommAction(exitAction, event->mpSourceCivilization.get(),
				event->mpSourceCity.get(), event->mpTargetCity.get());
			break;
		default:
			AppendKeyboardLog("deferred comm exit skipped: unsupported event type");
			return;
		}
		AppendKeyboardLog("deferred comm exit action returned");

		// A Space-stage colony Speak screen can disappear while leaving its
		// current event active. Retry briefly if the close transition spans frames.
		if (event->mEventType == Simulator::cCommEventType::Space)
		{
			sPendingCommRecoveryEvent = event;
			sCommRecoveryDeadline = GetTickCount64() + kCommRecoveryRetryMilliseconds;
			AppendKeyboardLog("queued Space communication state check after close");
		}
	}

	void ProcessPendingCommunicationRecovery()
	{
		if (sPendingCommRecoveryEvent == nullptr)
		{
			return;
		}

		const ULONGLONG now = GetTickCount64();
		cCommEventPtr event = sPendingCommRecoveryEvent;
		Simulator::cCommManager* manager = Simulator::cCommManager::Get();
		if (manager == nullptr || GetCurrentCommEvent(manager).get() != event.get())
		{
			sPendingCommRecoveryEvent = nullptr;
			sCommRecoveryDeadline = 0;
			AppendKeyboardLog("Space communication state check skipped: current event changed");
			return;
		}

		if (IsCommScreenRootVisible())
		{
			if (now < sCommRecoveryDeadline)
			{
				return;
			}

			sPendingCommRecoveryEvent = nullptr;
			sCommRecoveryDeadline = 0;
			AppendKeyboardLog("Space communication state check skipped: CommScreen still visible");
			return;
		}

		sPendingCommRecoveryEvent = nullptr;
		sCommRecoveryDeadline = 0;

		// Recover on the first update where the CommScreen root is hidden. The
		// short retry window above only allows for a close transition spanning
		// frames. Release the stale event and one matching pause, but leave queued
		// communication events untouched.
		auto* currentEventSlot = reinterpret_cast<cCommEventPtr*>(
			reinterpret_cast<char*>(manager) + kNativeCurrentCommEventOffset);
		Simulator::cGameTimeManager* timeManager = Simulator::cGameTimeManager::Get();
		const int pauseBefore = timeManager == nullptr ? -1 :
			timeManager->GetPauseCount(Simulator::TimeManagerPause::CommScreen);
		currentEventSlot->reset();

		int pauseAfter = -1;
		if (timeManager != nullptr)
		{
			if (pauseBefore > 0)
			{
				timeManager->Resume(Simulator::TimeManagerPause::CommScreen);
			}
			pauseAfter = timeManager->GetPauseCount(Simulator::TimeManagerPause::CommScreen);
		}

		char line[160];
		_snprintf_s(line, _countof(line), _TRUNCATE,
			"Space communication state recovered: pause=%d->%d active=%d",
			pauseBefore, pauseAfter, manager->IsCommScreenActive() ? 1 : 0);
		AppendKeyboardLog(line);
	}

	void AttachKeyboardInputDetour()
	{
		ProcessPendingCommunicationExit();
		ProcessPendingCommunicationRecovery();
		if (sAttached || sAttachAttempted)
		{
			return;
		}

		Simulator::cGameInputManager* manager = Simulator::cGameInputManager::Get();
		if (manager == nullptr)
		{
			if (!sLoggedMissingManager)
			{
				AppendKeyboardLog("central input manager not ready; retrying");
				sLoggedMissingManager = true;
			}
			return;
		}
		sLoggedMissingManager = false;

		static_assert(sizeof(void*) == 4, "The Spore input detour requires the 32-bit game.");
		void** vtable = *reinterpret_cast<void***>(manager);
		constexpr size_t kOnKeyDownVtableSlot = 0x4C / sizeof(void*);
		void* target = vtable[kOnKeyDownVtableSlot];
		if (target == nullptr)
		{
			sAttachAttempted = true;
			AppendKeyboardLog("central input hook failed: null vtable target");
			App::ConsolePrintF("ERKEK2000 QoL Runtime: keyboard hook not attached (input callback missing).");
			return;
		}

		LONG status = DetourTransactionBegin();
		if (status == NO_ERROR)
		{
			status = DetourUpdateThread(GetCurrentThread());
		}
		if (status == NO_ERROR)
		{
			status = KeyboardShortcutInput::attach(static_cast<UINT>(
				reinterpret_cast<UINT_PTR>(target)));
		}
		if (status == NO_ERROR)
		{
			status = DetourTransactionCommit();
		}
		else
		{
			DetourTransactionAbort();
		}

		sAttachAttempted = true;
		sAttached = status == NO_ERROR;
		char line[160];
		_snprintf_s(line, _countof(line), _TRUNCATE,
			"central input hook %s status=%ld manager=%p target=%p",
			sAttached ? "attached" : "failed", status, manager, target);
		AppendKeyboardLog(line);
		App::ConsolePrintF("ERKEK2000 QoL Runtime: central keyboard hook %s (status %ld).",
			sAttached ? "attached" : "failed", status);
	}
}

void ERKEK2000QoL::InstallKeyboardShortcuts()
{
	if (sUpdateListener == nullptr)
	{
		sUpdateListener = App::AddUpdateFunction(AttachKeyboardInputDetour);
	}
}

void ERKEK2000QoL::RemoveKeyboardShortcuts()
{
	sPendingCommExitEvent = nullptr;
	sPendingCommExitKey = 0;
	sPendingCommRecoveryEvent = nullptr;
	sCommRecoveryDeadline = 0;

	if (sUpdateListener != nullptr)
	{
		App::RemoveUpdateFunction(sUpdateListener);
		sUpdateListener = nullptr;
	}

	if (sAttached)
	{
		LONG status = DetourTransactionBegin();
		if (status == NO_ERROR)
		{
			status = DetourUpdateThread(GetCurrentThread());
		}
		if (status == NO_ERROR)
		{
			status = KeyboardShortcutInput::detach();
		}
		if (status == NO_ERROR)
		{
			status = DetourTransactionCommit();
		}
		else
		{
			DetourTransactionAbort();
		}
		App::ConsolePrintF("ERKEK2000 QoL Runtime: central keyboard hook removed (status %ld).", status);
	}
	sAttached = false;
	sAttachAttempted = false;
	sLoggedMissingManager = false;
}
