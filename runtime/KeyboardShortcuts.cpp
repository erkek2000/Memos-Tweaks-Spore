#include "stdafx.h"
#include "KeyboardShortcuts.h"

#include <Spore\App\IMessageManager.h>
#include <Spore\Palettes\PalettePageUI.h>
#include <Spore\Simulator.h>
#include <Spore\Simulator\cSimulatorSpaceGame.h>
#include <Spore\Simulator\SubSystem\CommManager.h>
#include <Spore\Simulator\SubSystem\GameInputManager.h>
#include <Spore\Simulator\SubSystem\GameModeManager.h>
#include <Spore\UI\SpaceToolPanelUI.h>
#include <Spore\UTFWin\IWinProc.h>
#include <Spore\UTFWin\IWindowManager.h>

namespace
{
	bool IsUsable(UTFWin::IWindow* window)
	{
		for (UTFWin::IWindow* current = window; current != nullptr; current = current->GetParent())
		{
			const UTFWin::WindowFlags flags = current->GetFlags();
			if ((flags & UTFWin::kWinFlagVisible) == 0 ||
				(flags & UTFWin::kWinFlagEnabled) == 0)
			{
				return false;
			}
		}
		return true;
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
		if (window == nullptr || !IsUsable(window))
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
				if (panel != nullptr && panel->HandleUIMessage(window, message))
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

	bool HandleCommunicationShortcut(int virtualKey)
	{
#if ERKEK_CLOSE_WITH_ESCAPE || ERKEK_CLOSE_WITH_TAB
		const bool closeKey =
			(ERKEK_CLOSE_WITH_ESCAPE && virtualKey == VK_ESCAPE) ||
			(ERKEK_CLOSE_WITH_TAB && virtualKey == VK_TAB);
		if (!closeKey)
		{
			return false;
		}

		Simulator::cCommManager* manager = Simulator::cCommManager::Get();
		if (manager == nullptr || !manager->IsCommScreenActive())
		{
			return false;
		}

		UTFWin::IButton* exitButton = manager->GetCommButton(Simulator::kBtnExit);
		UTFWin::IWindow* exitWindow = exitButton == nullptr ? nullptr : exitButton->ToWindow();
		if (!IsUsable(exitWindow))
		{
			// Hidden or disabled Goodbye controls indicate a submenu whose own
			// native cancel/confirmation behavior must remain in charge.
			return false;
		}

		UTFWin::Message click{};
		click.source = exitWindow;
		click.eventType = UTFWin::kMsgButtonClick;
		WindowManager.SendMsg(exitWindow, exitWindow, click, true);
		return true;
#else
		(void)virtualKey;
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

		return ClickPaletteItem(WindowManager.GetMainWindow(), itemID, alternateItemID);
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

		return DispatchToVisibleToolPanel(
			WindowManager.GetMainWindow(), virtualKey, modifiers);
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
			if (HandleCommunicationShortcut(virtualKey))
			{
				return true;
			}
			// The planner gets first refusal on 1-4. Outside it, the same keys
			// are ordinary Space hotbar slots.
			if (HandleBuildingShortcut(virtualKey, modifiers) ||
				HandleSpaceHotbarShortcut(virtualKey, modifiers))
			{
				return true;
			}
			return original_function(this, virtualKey, modifiers);
		}
	};

	eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;
	bool sAttachAttempted = false;
	bool sAttached = false;

	void AttachKeyboardInputDetour()
	{
		if (sAttached || sAttachAttempted)
		{
			return;
		}

		Simulator::cGameInputManager* manager = Simulator::cGameInputManager::Get();
		if (manager == nullptr)
		{
			return;
		}

		static_assert(sizeof(void*) == 4, "The Spore input detour requires the 32-bit game.");
		void** vtable = *reinterpret_cast<void***>(manager);
		constexpr size_t kOnKeyDownVtableSlot = 0x4C / sizeof(void*);
		void* target = vtable[kOnKeyDownVtableSlot];
		if (target == nullptr)
		{
			sAttachAttempted = true;
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
}
