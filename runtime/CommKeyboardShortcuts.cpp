#include "stdafx.h"
#include "CommKeyboardShortcuts.h"

#include <Spore\Simulator\SubSystem\CommManager.h>
#include <Spore\Simulator\SubSystem\GameModeManager.h>
#include <Spore\Simulator.h>
#include <Spore\UTFWin\GlideEffect.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace
{
    eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;

#if ERKEK_FAST_DIALOGUE_OPENING
    constexpr uint32_t kCommScreenRootID = 0x01C3BB0C;
    constexpr uint32_t kCommUpperPanelID = 0x05E4E5F8;
    // Keep retrying for a short window after a screen appears. The glitch this
    // replaces faulted while the comm screen was still being built, so a later
    // attempt can succeed after the panel's effects are fully attached.
    constexpr ULONGLONG kCommOpeningRetryMilliseconds = 3000;

    ULONGLONG sCommOpeningDeadline = 0;
    bool sCommOpeningApplied = false;
    bool sCommOpeningAttempted = false;

    void AppendCommLog(const char* text)
    {
        wchar_t appData[MAX_PATH]{};
        if (GetEnvironmentVariableW(L"APPDATA", appData, _countof(appData)) == 0) return;
        const std::wstring directory = std::wstring(appData) + L"\\Spore\\ERKEK2000_QoL";
        CreateDirectoryW(directory.c_str(), nullptr);
        const std::wstring path = directory + L"\\comm-open.log";
        HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;
        SetFilePointer(file, 0, nullptr, FILE_END);
        DWORD written = 0;
        WriteFile(file, text, static_cast<DWORD>(strlen(text)), &written, nullptr);
        WriteFile(file, "\r\n", 2, &written, nullptr);
        FlushFileBuffers(file);
        CloseHandle(file);
    }

    // The March2017 Ghidra type database gives the real game vtables:
    //   IBiStateEffect: 0x10 ToWinProc, 0x14 GetTime, 0x18 SetTime, ...
    //   IGlideEffect:   0x10 ToWinProc, 0x14 GetOffset, 0x18 SetOffset
    // The SDK models IGlideEffect as deriving from IBiStateEffect, so the
    // compiler places SetTime/SetOffset on an IGlideEffect* at slots 0x18/0x40
    // instead of the game's 0x18 - calling the wrong function, which is what
    // crashed the dialogue (SetTime(0.0f) reached SetOffset and the float was
    // read as a null Point*). SetTime goes through IBiStateEffect, whose SDK
    // layout matches the game exactly; SetOffset goes through this mirror of
    // the game's IGlideEffect vtable.
    class IGlideEffectGameLayout
    {
    public:
        virtual void* objectSlot0() = 0;   // 0x00 AddRef
        virtual void* objectSlot1() = 0;   // 0x04 Release
        virtual void* objectSlot2() = 0;   // 0x08 virtual destructor
        virtual void* objectSlot3() = 0;   // 0x0C Cast
        virtual UTFWin::IWinProc* ToWinProc() = 0;              // 0x10
        virtual const Math::Point& GetOffset() const = 0;       // 0x14
        virtual void SetOffset(const Math::Point& offset) = 0;  // 0x18
    };

    // Every game call in this walk is individually guarded: the panel's
    // game-owned procedure list can hold effects that are not safely readable
    // in every state. A fault skips just that call; the rest of the panel
    // still benefits.
    void MakePanelInstant(UTFWin::IWindow* panel, int& instant, int& faults)
    {
        if (panel == nullptr) return;
        bool changed = false;
        UTFWin::IWinProc* procedure = nullptr;
        for (;;)
        {
            __try
            {
                procedure = panel->GetNextWinProc(procedure);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ++faults;
                break;
            }
            if (procedure == nullptr) break;
            __try
            {
                UTFWin::IBiStateEffect* state = object_cast<UTFWin::IBiStateEffect>(procedure);
                if (state != nullptr)
                {
                    __try
                    {
                        state->SetTime(0.0f);
                        changed = true;
                        ++instant;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        ++faults;
                    }
                }
                UTFWin::IGlideEffect* glide = object_cast<UTFWin::IGlideEffect>(procedure);
                if (glide != nullptr)
                {
                    __try
                    {
                        auto* raw = reinterpret_cast<IGlideEffectGameLayout*>(glide);
                        raw->SetOffset(Math::Point(0.0f, 0.0f));
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        ++faults;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ++faults;
            }
        }
        if (changed)
        {
            __try
            {
                panel->Revalidate();
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                ++faults;
            }
        }
    }

    int MakeCommOpeningInstant()
    {
        Simulator::cCommManager* manager = Simulator::cCommManager::Get();
        if (manager == nullptr || !manager->IsCommScreenActive())
        {
            sCommOpeningApplied = false;
            sCommOpeningDeadline = 0;
            sCommOpeningAttempted = false;
            return 0;
        }
        if (sCommOpeningApplied) return 0;

        const ULONGLONG now = GetTickCount64();
        if (sCommOpeningDeadline == 0) sCommOpeningDeadline = now + kCommOpeningRetryMilliseconds;
        else if (now > sCommOpeningDeadline) return 0;

        UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
        if (mainWindow == nullptr) return 0;

        UTFWin::IWindow* screen = nullptr;
        __try
        {
            screen = mainWindow->FindWindowByID(kCommScreenRootID);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }
        if (screen == nullptr) return 0;

        int instant = 0;
        int faults = 0;
        UTFWin::IWindow* panel = nullptr;
        __try
        {
            panel = screen->FindWindowByID(Simulator::kWindowPanelLower);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            panel = nullptr;
        }
        MakePanelInstant(panel, instant, faults);

        panel = nullptr;
        __try
        {
            panel = screen->FindWindowByID(kCommUpperPanelID);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            panel = nullptr;
        }
        MakePanelInstant(panel, instant, faults);

        if (instant > 0)
        {
            sCommOpeningApplied = true;
            char line[128];
            _snprintf_s(line, _countof(line), _TRUNCATE,
                "comm opening: %d effect(s) made instant, %d fault(s)", instant, faults);
            AppendCommLog(line);
            App::ConsolePrintF("ERKEK2000 QoL Runtime: comm opening instant (%d effects, %d faults).",
                instant, faults);
        }
        else if (faults > 0 && !sCommOpeningAttempted)
        {
            char line[128];
            _snprintf_s(line, _countof(line), _TRUNCATE,
                "comm opening: no effect applied, %d fault(s)", faults);
            AppendCommLog(line);
        }
        sCommOpeningAttempted = true;
        return instant;
    }
#endif

    void UpdateCommOpening()
    {
        // Same lifecycle rule as the other Space-only procedures: never touch
        // the UI tree while a stage is loading or outside Space.
        if (!Simulator::IsSpaceGame() || Simulator::IsLoadingGameMode())
        {
            return;
        }

#if ERKEK_FAST_DIALOGUE_OPENING
        // Native CommScreen windows and their Glide effects already exist at
        // this point. Altering only their live effect values avoids shipping a
        // replacement SPUI resource, which was the source of the 0.5.5 crash.
        MakeCommOpeningInstant();
#endif
    }
}

#if ERKEK_FAST_DIALOGUE_OPENING
member_detour(InstantCommOpening, Simulator::cCommManager, void(Simulator::cCommEvent*))
{
    void detoured(Simulator::cCommEvent* event)
    {
        original_function(this, event);
        MakeCommOpeningInstant();
    }
};
#endif

void ERKEK2000QoL::InstallFastDialogueOpening()
{
    if (sUpdateListener == nullptr)
        sUpdateListener = App::AddUpdateFunction(UpdateCommOpening);
}

void ERKEK2000QoL::RemoveFastDialogueOpening()
{
    if (sUpdateListener != nullptr)
    {
        App::RemoveUpdateFunction(sUpdateListener);
        sUpdateListener = nullptr;
    }
}

void ERKEK2000QoL::AttachDialogueSpeedDetour()
{
#if ERKEK_FAST_DIALOGUE_OPENING
    InstantCommOpening::attach(GetAddress(Simulator::cCommManager, ShowCommEvent));
#endif
}
