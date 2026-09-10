#include "stdafx.h"
#include "CropCircleUplift.h"

#include <Spore\Simulator\cCropCirclesToolStrategy.h>
#include <Spore\Simulator\cPlanetRecord.h>
#include <Spore\Simulator\SubSystem\StarManager.h>
#include <Spore\App\IMessageManager.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
    // A monolith's native mean evolution interval is 120 seconds. Crop
    // Circles deliberately take ten times that long: 90% slower progress.
    constexpr uint64_t kFileTimeTicksPerSecond = 10000000ull;
    constexpr uint64_t kUpliftInterval =
        static_cast<uint64_t>(ERKEK_CROP_CIRCLE_UPLIFT_INTERVAL_SECONDS) * kFileTimeTicksPerSecond;
    constexpr uint32_t kUpliftMagic = 0x55435245; // "ERC U"
    constexpr uint32_t kUpliftVersion = 1;
    constexpr uint32_t kMaximumTasks = 256;

    struct UpliftFileHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t count;
        uint32_t checksum;
    };

    struct UpliftTask
    {
        uint32_t planetID;
        uint32_t reserved;
        uint64_t dueTime;
    };
    static_assert(sizeof(UpliftTask) == 16, "Persistent Crop Circle format changed.");

    std::vector<UpliftTask> sTasks;
    eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;

    uint64_t GetCurrentFileTime()
    {
        FILETIME time{};
        GetSystemTimeAsFileTime(&time);
        ULARGE_INTEGER value{};
        value.LowPart = time.dwLowDateTime;
        value.HighPart = time.dwHighDateTime;
        return value.QuadPart;
    }

    std::wstring GetUpliftDirectory()
    {
        wchar_t appData[MAX_PATH]{};
        const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, _countof(appData));
        if (length == 0 || length >= _countof(appData)) return {};
        return std::wstring(appData) + L"\\Spore\\ERKEK2000_QoL";
    }

    std::wstring GetUpliftPath()
    {
        const std::wstring directory = GetUpliftDirectory();
        return directory.empty() ? std::wstring{} : directory + L"\\crop-circle-uplift.bin";
    }

    uint32_t UpdateChecksum(uint32_t checksum, const void* bytes, size_t byteCount)
    {
        const auto* data = static_cast<const uint8_t*>(bytes);
        for (size_t index = 0; index < byteCount; ++index)
        {
            checksum ^= data[index];
            checksum *= 16777619u;
        }
        return checksum;
    }

    bool WriteAll(HANDLE file, const void* data, DWORD byteCount)
    {
        DWORD written = 0;
        return WriteFile(file, data, byteCount, &written, nullptr) != FALSE && written == byteCount;
    }

    bool ReadAll(HANDLE file, void* data, DWORD byteCount)
    {
        DWORD read = 0;
        return ReadFile(file, data, byteCount, &read, nullptr) != FALSE && read == byteCount;
    }

    bool SaveTasks()
    {
        const std::wstring directory = GetUpliftDirectory();
        const std::wstring path = GetUpliftPath();
        if (directory.empty() || path.empty()) return false;

        const std::wstring sporeDirectory = directory.substr(0, directory.find_last_of(L'\\'));
        CreateDirectoryW(sporeDirectory.c_str(), nullptr);
        if (!CreateDirectoryW(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
            return false;

        UpliftFileHeader header{ kUpliftMagic, kUpliftVersion,
            static_cast<uint32_t>(sTasks.size()), 2166136261u };
        header.checksum = UpdateChecksum(header.checksum, sTasks.data(), sTasks.size() * sizeof(UpliftTask));

        const std::wstring temporaryPath = path + L".tmp";
        HANDLE file = CreateFileW(temporaryPath.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        bool success = WriteAll(file, &header, sizeof(header));
        if (success && !sTasks.empty())
            success = WriteAll(file, sTasks.data(), static_cast<DWORD>(sTasks.size() * sizeof(UpliftTask)));
        if (!FlushFileBuffers(file)) success = false;
        CloseHandle(file);
        if (!success)
        {
            DeleteFileW(temporaryPath.c_str());
            return false;
        }
        if (!MoveFileExW(temporaryPath.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            DeleteFileW(temporaryPath.c_str());
            return false;
        }
        return true;
    }

    void LoadTasks()
    {
        const std::wstring path = GetUpliftPath();
        if (path.empty()) return;
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return;

        UpliftFileHeader header{};
        bool success = ReadAll(file, &header, sizeof(header));
        success = success && header.magic == kUpliftMagic && header.version == kUpliftVersion &&
            header.count <= kMaximumTasks;
        std::vector<UpliftTask> loaded(success ? header.count : 0);
        if (success && !loaded.empty())
            success = ReadAll(file, loaded.data(), static_cast<DWORD>(loaded.size() * sizeof(UpliftTask)));
        LARGE_INTEGER fileSize{};
        const uint64_t expectedSize = sizeof(header) + loaded.size() * sizeof(UpliftTask);
        success = success && GetFileSizeEx(file, &fileSize) && static_cast<uint64_t>(fileSize.QuadPart) == expectedSize;
        CloseHandle(file);
        if (!success) return;

        const uint32_t checksum = UpdateChecksum(2166136261u, loaded.data(), loaded.size() * sizeof(UpliftTask));
        if (checksum != header.checksum) return;
        sTasks = std::move(loaded);
    }

    bool IsEligible(Simulator::cPlanetRecord* planet)
    {
        if (planet == nullptr || planet->mbHomeWorld) return false;
        const Simulator::TechLevel level = planet->GetTechLevel();
        return level == Simulator::TechLevel::Creature || level == Simulator::TechLevel::Tribe ||
            level == Simulator::TechLevel::Civilization;
    }

    void QueueActivePlanet()
    {
        Simulator::cPlanetRecord* planet = Simulator::GetActivePlanetRecord();
        if (!IsEligible(planet)) return;

        const uint32_t planetID = planet->GetID().internalValue;
        const auto existing = std::find_if(sTasks.begin(), sTasks.end(),
            [planetID](const UpliftTask& task) { return task.planetID == planetID; });
        if (existing != sTasks.end()) return; // Repeat uses never stack.
        if (sTasks.size() >= kMaximumTasks) return;

        sTasks.push_back({ planetID, 0, GetCurrentFileTime() + kUpliftInterval });
        SaveTasks();
        App::ConsolePrintF("ERKEK2000 QoL Runtime: Crop Circles started a slow uplift.");
    }

    void UpdateUplifts()
    {
        Simulator::cStarManager* starManager = Simulator::cStarManager::Get();
        if (starManager == nullptr || sTasks.empty()) return;

        const uint64_t now = GetCurrentFileTime();
        bool changed = false;
        for (auto iterator = sTasks.begin(); iterator != sTasks.end();)
        {
            if (iterator->dueTime > now)
            {
                ++iterator;
                continue;
            }

            Simulator::PlanetID planetID(iterator->planetID);
            Simulator::cPlanetRecord* planet = starManager->GetPlanetRecord(planetID);
            if (!IsEligible(planet))
            {
                iterator = sTasks.erase(iterator);
                changed = true;
                continue;
            }

            const Simulator::TechLevel currentLevel = planet->GetTechLevel();
            const Simulator::TechLevel nextLevel = currentLevel == Simulator::TechLevel::Creature
                ? Simulator::TechLevel::Tribe
                : currentLevel == Simulator::TechLevel::Tribe
                    ? Simulator::TechLevel::Civilization
                    : Simulator::TechLevel::Empire;

            // Empire-level planet data requires an owning empire. This is the
            // native StarManager path for obtaining one; it avoids constructing
            // a partial empire record by hand and leaves Crop Circle visuals
            // and hit handling entirely with the original Crop Circle tool.
            if (nextLevel == Simulator::TechLevel::Empire)
            {
                Simulator::cStarRecord* star = planet->GetStarRecord();
                if (star == nullptr || starManager->GetEmpireForStar(star) == nullptr)
                {
                    iterator->dueTime = now + kUpliftInterval;
                    ++iterator;
                    continue;
                }
            }
            Simulator::cPlanetRecord::FillPlanetDataForTechLevel(planet, nextLevel);
            planet->mTechLevel = nextLevel;
            changed = true;
            if (nextLevel == Simulator::TechLevel::Empire)
            {
                App::ConsolePrintF("ERKEK2000 QoL Runtime: Crop Circle uplift reached Empire.");
                iterator = sTasks.erase(iterator);
            }
            else
            {
                iterator->dueTime = now + kUpliftInterval;
                ++iterator;
            }
        }
        if (changed) SaveTasks();
    }
}

member_detour(CropCircleSlowUplift, Simulator::cCropCirclesToolStrategy,
    bool(Simulator::cSpaceToolData*, const Math::Vector3&, Simulator::SpaceToolHit, int))
{
    bool detoured(Simulator::cSpaceToolData* tool, const Math::Vector3& position,
        Simulator::SpaceToolHit hitType, int value)
    {
        const bool result = original_function(this, tool, position, hitType, value);
        if (result && (hitType == Simulator::kHitGround || hitType == Simulator::kHitWater))
            QueueActivePlanet();
        return result;
    }
};

void ERKEK2000QoL::InstallCropCircleUplift()
{
    if (sUpdateListener != nullptr) return;
    LoadTasks();
    sUpdateListener = App::AddUpdateFunction(UpdateUplifts);
}

void ERKEK2000QoL::RemoveCropCircleUplift()
{
    if (sUpdateListener != nullptr)
    {
        App::RemoveUpdateFunction(sUpdateListener);
        sUpdateListener = nullptr;
    }
    sTasks.clear();
}

void ERKEK2000QoL::AttachCropCircleUpliftDetour()
{
    CropCircleSlowUplift::attach(GetAddress(Simulator::cCropCirclesToolStrategy, OnHit));
}
