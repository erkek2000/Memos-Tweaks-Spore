#include "stdafx.h"
#include "ColonyPattern.h"

#include <Spore\Simulator\cBuildingCityHall.h>
#include <Spore\Simulator\cBuildingEntertainment.h>
#include <Spore\Simulator\cBuildingHouse.h>
#include <Spore\Simulator\cBuildingIndustry.h>
#include <Spore\Simulator\cCity.h>
#include <Spore\Simulator\cEmpire.h>
#include <Spore\Simulator\cOrnament.h>
#include <Spore\Simulator\cSimulatorSpaceGame.h>
#include <Spore\Simulator\cTurret.h>
#include <Spore\Simulator\SubSystem\GameNounManager.h>
#include <Spore\Simulator\SubSystem\GameModeManager.h>
#include <Spore\Simulator\SubSystem\PlanetModel.h>
#include <Spore\Simulator\SubSystem\SpacePlayerData.h>
#include <Spore\App\ICheatManager.h>
#include <Spore\App\IMessageManager.h>
#include <Spore\Palettes\PaletteItem.h>
#include <Spore\Palettes\PalettePageUI.h>
#include <Spore\UTFWin\IButton.h>
#include <Spore\UTFWin\IWinProc.h>
#include <Spore\UTFWin\IWindowManager.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// cTurret inherits several ref-counted SDK interfaces without exposing a
// single unambiguous AddRef/Release pair. Tell EASTL to use its cGameData
// implementation when a turret is stored in the city's intrusive vector.
namespace eastl
{
    template <> inline void intrusive_ptr_add_ref<Simulator::cTurret>(Simulator::cTurret* object)
    {
        object->Simulator::cGameData::AddRef();
    }

    template <> inline void intrusive_ptr_release<Simulator::cTurret>(Simulator::cTurret* object)
    {
        object->Simulator::cGameData::Release();
    }
}

namespace
{
    constexpr uint32_t kCopyButtonID = 0xE2A10001;
    constexpr uint32_t kApplyButtonID = 0xE2A10002;
    constexpr uint32_t kApplyAllButtonID = 0xE2A10003;
    constexpr int kCivicObjectCost = 25;
    constexpr uint32_t kPatternMagic = 0x504B5245; // "ERKP"
    // Version 2: slot orientations are recorded relative to the planet
    // surface frame at the slot position instead of the layout quaternion,
    // so a pattern pasted into a colony elsewhere on the planet keeps its
    // orientation. Version 1 files are rejected; copy the pattern again.
    constexpr uint32_t kPatternVersion = 2;
    constexpr uint32_t kMaximumSlotsPerLayout = 64;

    enum class PatternKind : uint8_t
    {
        Empty,
        Building,
        Turret,
        Ornament,
        // A slot that must never be copied out of the source city and never be
        // created or destroyed in the target city. The city hall is the prime
        // example: it derives from cBuilding, so it looks like an ordinary
        // building, but destroying it corrupts the whole city.
        Protected
    };

    struct PatternSlot
    {
        PatternKind kind = PatternKind::Empty;
        uint32_t nounID = 0;
        uint32_t definitionID = 0;
        ResourceKey modelKey{};
        Math::Quaternion relativeOrientation{};
        float scale = 1.0f;
        int cost = 0;
    };

    struct ColonyPattern
    {
        std::vector<PatternSlot> buildings;
        std::vector<PatternSlot> decorations;
        std::vector<PatternSlot> turrets;
        bool valid = false;
    };

    struct PatternFileHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t buildingCount;
        uint32_t decorationCount;
        uint32_t turretCount;
        uint32_t checksum;
    };

    struct PatternFileSlot
    {
        uint8_t kind;
        uint8_t reserved[3];
        uint32_t nounID;
        uint32_t definitionID;
        uint32_t modelInstanceID;
        uint32_t modelTypeID;
        uint32_t modelGroupID;
        float orientationX;
        float orientationY;
        float orientationZ;
        float orientationW;
        float scale;
        int32_t cost;
    };
    static_assert(sizeof(PatternFileSlot) == 48, "Persistent colony slot format changed.");

    ColonyPattern sPattern;
    IWindowPtr sMainWindow;
    IButtonPtr sCopyButton;
    IButtonPtr sApplyButton;
    IButtonPtr sApplyAllButton;
    IButtonPtr sHoverText;
    IButtonPtr sStatusText;
    IWinProcPtr sController;
    eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;
    uint32_t sHoveredButton = 0;
    ULONGLONG sStatusUntil = 0;

    std::wstring GetPatternDirectory()
    {
        wchar_t appData[MAX_PATH]{};
        const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, _countof(appData));
        if (length == 0 || length >= _countof(appData)) return {};
        return std::wstring(appData) + L"\\Spore\\ERKEK2000_QoL";
    }

    std::wstring GetPatternPath()
    {
        const std::wstring directory = GetPatternDirectory();
        return directory.empty() ? std::wstring{} : directory + L"\\colony-pattern.bin";
    }

    bool EnsurePatternDirectory()
    {
        const std::wstring directory = GetPatternDirectory();
        if (directory.empty()) return false;
        const std::wstring sporeDirectory = directory.substr(0, directory.find_last_of(L'\\'));
        CreateDirectoryW(sporeDirectory.c_str(), nullptr);
        if (!CreateDirectoryW(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
            return false;
        return true;
    }

    // Appends one ASCII line to a diagnostic file next to the pattern file.
    // The file is opened, appended, flushed and closed on every call, so a
    // crash immediately after a line still leaves that line on disk. This is
    // the primary evidence source when an apply crashes mid-way.
    void WriteDiagnosticLine(const wchar_t* fileName, const char* text)
    {
        if (!EnsurePatternDirectory()) return;
        const std::wstring path = GetPatternDirectory() + L"\\" + fileName;
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

    uint32_t UpdateChecksum(uint32_t checksum, const void* bytes, size_t byteCount)
    {
        const uint8_t* data = static_cast<const uint8_t*>(bytes);
        for (size_t index = 0; index < byteCount; ++index)
        {
            checksum ^= data[index];
            checksum *= 16777619u;
        }
        return checksum;
    }

    PatternFileSlot ToFileSlot(const PatternSlot& source)
    {
        PatternFileSlot result{};
        result.kind = static_cast<uint8_t>(source.kind);
        result.nounID = source.nounID;
        result.definitionID = source.definitionID;
        result.modelInstanceID = source.modelKey.instanceID;
        result.modelTypeID = source.modelKey.typeID;
        result.modelGroupID = source.modelKey.groupID;
        result.orientationX = source.relativeOrientation.x;
        result.orientationY = source.relativeOrientation.y;
        result.orientationZ = source.relativeOrientation.z;
        result.orientationW = source.relativeOrientation.w;
        result.scale = source.scale;
        result.cost = source.cost;
        return result;
    }

    bool IsSupportedNoun(PatternKind kind, uint32_t nounID)
    {
        switch (kind)
        {
        case PatternKind::Empty: return nounID == 0;
        case PatternKind::Protected: return nounID == 0;
        case PatternKind::Building:
            return nounID == Simulator::cBuildingHouse::NOUN_ID ||
                nounID == Simulator::cBuildingEntertainment::NOUN_ID ||
                nounID == Simulator::cBuildingIndustry::NOUN_ID;
        case PatternKind::Turret: return nounID == Simulator::cTurret::NOUN_ID;
        case PatternKind::Ornament: return nounID == Simulator::cOrnament::NOUN_ID;
        default: return false;
        }
    }

    bool FromFileSlot(const PatternFileSlot& source, PatternSlot& result)
    {
        const PatternKind kind = static_cast<PatternKind>(source.kind);
        const bool finite = std::isfinite(source.orientationX) &&
            std::isfinite(source.orientationY) && std::isfinite(source.orientationZ) &&
            std::isfinite(source.orientationW) && std::isfinite(source.scale);
        if (!IsSupportedNoun(kind, source.nounID) || !finite || source.scale <= 0.0f ||
            source.scale > 100.0f || source.cost < 0 || source.cost > 100000000)
            return false;

        result.kind = kind;
        result.nounID = source.nounID;
        result.definitionID = source.definitionID;
        result.modelKey = ResourceKey(source.modelInstanceID, source.modelTypeID, source.modelGroupID);
        result.relativeOrientation = Math::Quaternion(source.orientationX, source.orientationY,
            source.orientationZ, source.orientationW);
        result.scale = source.scale;
        result.cost = source.cost;
        return true;
    }

    std::vector<PatternFileSlot> ToFileSlots(const std::vector<PatternSlot>& slots)
    {
        std::vector<PatternFileSlot> result;
        result.reserve(slots.size());
        for (const PatternSlot& slot : slots) result.push_back(ToFileSlot(slot));
        return result;
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

    bool SavePersistentPattern()
    {
        const std::wstring directory = GetPatternDirectory();
        const std::wstring path = GetPatternPath();
        if (directory.empty() || path.empty()) return false;

        if (!EnsurePatternDirectory()) return false;

        const std::vector<PatternFileSlot> buildings = ToFileSlots(sPattern.buildings);
        const std::vector<PatternFileSlot> decorations = ToFileSlots(sPattern.decorations);
        const std::vector<PatternFileSlot> turrets = ToFileSlots(sPattern.turrets);
        PatternFileHeader header{ kPatternMagic, kPatternVersion,
            static_cast<uint32_t>(buildings.size()), static_cast<uint32_t>(decorations.size()),
            static_cast<uint32_t>(turrets.size()), 2166136261u };
        header.checksum = UpdateChecksum(header.checksum, buildings.data(), buildings.size() * sizeof(PatternFileSlot));
        header.checksum = UpdateChecksum(header.checksum, decorations.data(), decorations.size() * sizeof(PatternFileSlot));
        header.checksum = UpdateChecksum(header.checksum, turrets.data(), turrets.size() * sizeof(PatternFileSlot));

        const std::wstring temporaryPath = path + L".tmp";
        HANDLE file = CreateFileW(temporaryPath.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;
        bool success = WriteAll(file, &header, sizeof(header));
        if (success && !buildings.empty()) success = WriteAll(file, buildings.data(), static_cast<DWORD>(buildings.size() * sizeof(PatternFileSlot)));
        if (success && !decorations.empty()) success = WriteAll(file, decorations.data(), static_cast<DWORD>(decorations.size() * sizeof(PatternFileSlot)));
        if (success && !turrets.empty()) success = WriteAll(file, turrets.data(), static_cast<DWORD>(turrets.size() * sizeof(PatternFileSlot)));
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

    bool LoadPersistentPattern()
    {
        const std::wstring path = GetPatternPath();
        if (path.empty()) return false;
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;

        PatternFileHeader header{};
        bool success = ReadAll(file, &header, sizeof(header));
        success = success && header.magic == kPatternMagic && header.version == kPatternVersion &&
            header.buildingCount <= kMaximumSlotsPerLayout &&
            header.decorationCount <= kMaximumSlotsPerLayout &&
            header.turretCount <= kMaximumSlotsPerLayout;
        std::vector<PatternFileSlot> buildings(success ? header.buildingCount : 0);
        std::vector<PatternFileSlot> decorations(success ? header.decorationCount : 0);
        std::vector<PatternFileSlot> turrets(success ? header.turretCount : 0);
        if (success && !buildings.empty()) success = ReadAll(file, buildings.data(), static_cast<DWORD>(buildings.size() * sizeof(PatternFileSlot)));
        if (success && !decorations.empty()) success = ReadAll(file, decorations.data(), static_cast<DWORD>(decorations.size() * sizeof(PatternFileSlot)));
        if (success && !turrets.empty()) success = ReadAll(file, turrets.data(), static_cast<DWORD>(turrets.size() * sizeof(PatternFileSlot)));
        LARGE_INTEGER fileSize{};
        const uint64_t expectedSize = sizeof(header) +
            (buildings.size() + decorations.size() + turrets.size()) * sizeof(PatternFileSlot);
        success = success && GetFileSizeEx(file, &fileSize) && static_cast<uint64_t>(fileSize.QuadPart) == expectedSize;
        CloseHandle(file);
        if (!success) return false;

        uint32_t checksum = 2166136261u;
        checksum = UpdateChecksum(checksum, buildings.data(), buildings.size() * sizeof(PatternFileSlot));
        checksum = UpdateChecksum(checksum, decorations.data(), decorations.size() * sizeof(PatternFileSlot));
        checksum = UpdateChecksum(checksum, turrets.data(), turrets.size() * sizeof(PatternFileSlot));
        if (checksum != header.checksum) return false;

        ColonyPattern loaded;
        const auto convert = [](const std::vector<PatternFileSlot>& source, std::vector<PatternSlot>& target)
        {
            target.reserve(source.size());
            for (const PatternFileSlot& fileSlot : source)
            {
                PatternSlot slot;
                if (!FromFileSlot(fileSlot, slot)) return false;
                target.push_back(slot);
            }
            return true;
        };
        if (!convert(buildings, loaded.buildings) || !convert(decorations, loaded.decorations) ||
            !convert(turrets, loaded.turrets)) return false;
        loaded.valid = !loaded.buildings.empty() || !loaded.decorations.empty() || !loaded.turrets.empty();
        if (!loaded.valid) return false;
        sPattern = loaded;
        return true;
    }

    int GetObjectCost(Simulator::cGameData* object)
    {
        if (Simulator::cBuilding* building = object_cast<Simulator::cBuilding>(object))
        {
            return std::max(0, static_cast<int>(building->GetCost() + 0.5f));
        }
        if (object_cast<Simulator::cTurret>(object) != nullptr)
        {
            return ERKEK_TURRET_COST;
        }
        if (object_cast<Simulator::cOrnament>(object) != nullptr)
        {
            return kCivicObjectCost;
        }
        return 0;
    }

    PatternKind GetKind(Simulator::cGameData* object)
    {
        if (object_cast<Simulator::cBuilding>(object) != nullptr) return PatternKind::Building;
        if (object_cast<Simulator::cTurret>(object) != nullptr) return PatternKind::Turret;
        if (object_cast<Simulator::cOrnament>(object) != nullptr) return PatternKind::Ornament;
        return PatternKind::Empty;
    }

    // The city hall is load-bearing city infrastructure. It must never be
    // removed or recreated by this feature, in the source city or the target.
    bool IsCityHallObject(Simulator::cGameData* object)
    {
        return object != nullptr && object->GetNounID() == Simulator::cBuildingCityHall::NOUN_ID;
    }

    // The apply step may only update these supported nouns in place. Anything
    // else occupying a target slot (city halls, scenario buildings, special
    // colony structures) is left untouched.
    bool IsSafelyReplaceable(Simulator::cGameData* object)
    {
        if (object == nullptr || IsCityHallObject(object)) return false;
        const uint32_t nounID = object->GetNounID();
        return IsSupportedNoun(PatternKind::Building, nounID) ||
            IsSupportedNoun(PatternKind::Turret, nounID) ||
            IsSupportedNoun(PatternKind::Ornament, nounID);
    }

    // The frame in which a slot object's orientation is expressed: the planet
    // surface orientation at the slot position, facing the layout direction.
    // The game places colony objects in per-slot surface frames, so capturing
    // and restoring against this frame keeps pasted structures upright and
    // correctly rotated on colonies elsewhere on the planet. Falls back to
    // the layout quaternion when no planet model is available.
    Math::Quaternion LayoutSlotFrame(const Simulator::cCommunityLayout& layout,
        const Math::Vector3& position)
    {
        Simulator::cPlanetModel* planetModel = Simulator::cPlanetModel::Get();
        if (planetModel != nullptr)
            return planetModel->GetOrientation(position, layout.mDirection);
        return layout.field_2C;
    }

    std::vector<PatternSlot> CaptureLayout(const Simulator::cCommunityLayout& layout)
    {
        std::vector<PatternSlot> result;
        result.reserve(layout.mSlots.size());

        for (const Simulator::cLayoutSlot& layoutSlot : layout.mSlots)
        {
            PatternSlot slot;
            Simulator::cGameData* object = layoutSlot.mpObject.get();
            Simulator::cSpatialObject* spatial = object_cast<Simulator::cSpatialObject>(object);
            slot.kind = GetKind(object);
            // Only the supported house/entertainment/industry nouns, turrets,
            // and ornaments may be copied. Anything else that can occupy a
            // layout slot - most importantly the city hall, which derives from
            // cBuilding and would otherwise be captured as a normal building -
            // becomes Protected so neither the saved file nor the apply step
            // will ever create or destroy it.
            if (slot.kind != PatternKind::Empty)
            {
                if (object == nullptr || spatial == nullptr ||
                    !IsSupportedNoun(slot.kind, object->GetNounID()) || IsCityHallObject(object))
                {
                    slot = PatternSlot{};
                    slot.kind = PatternKind::Protected;
                }
                else
                {
                    slot.nounID = object->GetNounID();
                    slot.definitionID = object->mDefinitionID;
                    slot.modelKey = spatial->GetModelKey();
                    slot.relativeOrientation =
                        LayoutSlotFrame(layout, layoutSlot.mPosition).Inverse() * spatial->GetOrientation();
                    slot.scale = spatial->GetScale();
                    slot.cost = GetObjectCost(object);
                }
            }
            result.push_back(slot);
        }
        return result;
    }

    bool IsActivelyEditedPlayerCity(const Simulator::cCity* city)
    {
        if (city == nullptr || !city->mbIsPlayerCity) return false;
        if (city->mbIsBeingEdited) return true;
        for (const cBuildingPtr& object : city->mBuildings)
            if (object != nullptr && object->mbIsBeingEdited) return true;
        for (const cTurretPtr& object : city->mTurrets)
            if (object != nullptr && object->mbIsBeingEdited) return true;
        for (const cOrnamentPtr& object : city->mCivicObjects)
            if (object != nullptr && object->mbIsBeingEdited) return true;
        return false;
    }

    Simulator::cCity* FindActivelyEditedPlayerCity()
    {
        auto& cities = Simulator::GetData<Simulator::cCity>();
        for (const cCityPtr& cityPtr : cities.data)
        {
            Simulator::cCity* city = cityPtr.get();
            if (IsActivelyEditedPlayerCity(city)) return city;
        }
        return nullptr;
    }

    Simulator::cCity* FindEditedCity()
    {
        if (Simulator::cCity* edited = FindActivelyEditedPlayerCity()) return edited;
        Simulator::cSimulatorSpaceGame* game = Simulator::cSimulatorSpaceGame::Get();
        if (game != nullptr && game->mpPlayerUFO != nullptr && game->mpPlayerUFO->GetUFO() != nullptr &&
            Simulator::cPlanetModel::Get() != nullptr)
        {
            Simulator::cCity* nearest = PlanetModel.GetNearestCity(
                game->mpPlayerUFO->GetUFO()->GetPosition());
            if (nearest != nullptr && nearest->mbIsPlayerCity) return nearest;
        }
        return nullptr;
    }

    // True when the city sits on the same planet as the anchor city. The
    // anchor is the colony the planner has open, which is guaranteed to be on
    // the current planet. GetData<cCity> is believed to be current-planet
    // scoped in planet view, but that assumption is unverified in-game; this
    // distance guard keeps the all-colonies apply from ever mutating a city
    // record that belongs to a planet that is not loaded. Distances are
    // compared against the observed planet radius (~505 units, from the copy
    // diagnostics), with a generous margin.
    bool IsOnAnchorPlanet(Simulator::cCity* city, Simulator::cCity* anchor)
    {
        if (city == nullptr || anchor == nullptr || city == anchor) return true;
        const Math::Vector3 offset = city->GetPosition() - anchor->GetPosition();
        return offset.SquaredLength() < 2000.0f * 2000.0f;
    }

    std::vector<Simulator::cCity*> GetPlayerColonies()
    {
        std::vector<Simulator::cCity*> result;
        Simulator::cCity* anchor = FindActivelyEditedPlayerCity();
        auto& cities = Simulator::GetData<Simulator::cCity>();
        for (const cCityPtr& city : cities.data)
        {
            if (city != nullptr && city->mbIsPlayerCity && IsOnAnchorPlanet(city.get(), anchor))
                result.push_back(city.get());
        }
        return result;
    }

    bool HasMatchingTopology(const Simulator::cCity* city);

    int64_t PatternLayoutCost(const Simulator::cCommunityLayout& layout,
        const std::vector<PatternSlot>& pattern)
    {
        int64_t total = 0;
        if (layout.mSlots.size() != pattern.size()) return total;

        for (size_t index = 0; index < pattern.size(); ++index)
        {
            const PatternSlot& source = pattern[index];
            if (source.kind == PatternKind::Protected || source.kind == PatternKind::Empty)
                continue;

            Simulator::cGameData* existing = layout.mSlots[index].mpObject.get();
            if (existing == nullptr || (!IsCityHallObject(existing) &&
                IsSafelyReplaceable(existing) && existing->GetNounID() == source.nounID))
            {
                total += source.cost;
            }
        }
        return total;
    }

    int64_t PatternCostForCity(const Simulator::cCity* city)
    {
        if (!HasMatchingTopology(city)) return 0;
        return PatternLayoutCost(city->mBuildingsLayout, sPattern.buildings) +
            PatternLayoutCost(city->mDecorationsLayout, sPattern.decorations) +
            PatternLayoutCost(city->mTurretsLayout, sPattern.turrets);
    }

    bool HasMatchingTopology(const Simulator::cCity* city)
    {
        return city != nullptr &&
            city->mBuildingsLayout.mSlots.size() == sPattern.buildings.size() &&
            city->mDecorationsLayout.mSlots.size() == sPattern.decorations.size() &&
            city->mTurretsLayout.mSlots.size() == sPattern.turrets.size();
    }

    // Updates an existing slot object in place when its noun already matches
    // the saved pattern. Objects are never removed or replaced in a live city.
    void UpdateExistingObject(Simulator::cGameData* object, Simulator::cLayoutSlot& slot,
        Simulator::cCommunityLayout& layout, const PatternSlot& pattern)
    {
        Simulator::cSpatialObject* spatial = object_cast<Simulator::cSpatialObject>(object);
        if (spatial == nullptr) return;
        object->mDefinitionID = pattern.definitionID;
        spatial->SetModelKey(pattern.modelKey);
        spatial->SetScale(pattern.scale);
        spatial->SetPosition(slot.mPosition);
        spatial->SetOrientation(LayoutSlotFrame(layout, slot.mPosition) * pattern.relativeOrientation);
        if (Simulator::cOrnament* ornament = object_cast<Simulator::cOrnament>(object))
            ornament->mModelID = pattern.modelKey.instanceID;
        spatial->SetHasModelChanged(true);
    }

    Simulator::cGameData* CreatePatternObject(
        Simulator::cCity* city,
        const PatternSlot& pattern)
    {
        Simulator::cGameData* object = GameNounManager.CreateInstance(pattern.nounID);
        Simulator::cSpatialObject* spatial = object_cast<Simulator::cSpatialObject>(object);
        if (object == nullptr || spatial == nullptr)
        {
            if (object != nullptr) GameNounManager.DestroyInstance(object);
            return nullptr;
        }

        object->mDefinitionID = pattern.definitionID;
        object->SetGameDataOwner(city);
        spatial->SetModelKey(pattern.modelKey);
        spatial->SetScale(pattern.scale);
        // Position and orientation are applied by ApplyLayout after the game's
        // own add/set calls, which may reposition or reorient the object.

        if (Simulator::cBuilding* building = object_cast<Simulator::cBuilding>(object))
        {
            building->SetOwnerCity1(city);
            building->SetOwnerCity2(city);
        }
        else if (Simulator::cTurret* turret = object_cast<Simulator::cTurret>(object))
        {
            turret->mpCity = city;
        }
        else if (Simulator::cOrnament* ornament = object_cast<Simulator::cOrnament>(object))
        {
            ornament->mModelID = pattern.modelKey.instanceID;
        }
        else
        {
            GameNounManager.DestroyInstance(object);
            return nullptr;
        }
        return object;
    }

    // Returns the number of actual building/turret creations. Updating an
    // already occupied slot is deliberately excluded: it changes a model but
    // is not a new colony improvement and must not award badge progress.
    bool ApplyLayout(Simulator::cCity* city, Simulator::cCommunityLayout& layout,
        const std::vector<PatternSlot>& pattern, int& money, bool& ranOut,
        int& createdColonyImprovements)
    {
        if (layout.mSlots.size() != pattern.size()) return false;

        for (size_t index = 0; index < pattern.size(); ++index)
        {
            // The layout vector must not be trusted across mutations: the
            // game's add/remove helpers may resize it. Re-check and re-fetch
            // the slot every time it is touched.
            if (index >= layout.mSlots.size()) break;
            const PatternSlot& source = pattern[index];

            // Breadcrumb for the in-game regression test: the last line in
            // spore_log.txt before a crash identifies the exact slot and kind.
            App::ConsolePrintF("ERKEK2000 QoL Runtime: colony pattern slot %u kind %d noun %u cost %d.",
                static_cast<unsigned>(index), static_cast<int>(source.kind), source.nounID, source.cost);

            // Leave protected slots untouched.
            if (source.kind == PatternKind::Protected) continue;

            Simulator::cLayoutSlot& target = layout.mSlots[index];
            Simulator::cGameData* existing = target.mpObject.get();

            // Never touch the target city's hall, and never destroy an object
            // whose noun this feature cannot recreate safely. Destroying
            // special nouns corrupts the city in the same way the hall did.
            if (IsCityHallObject(existing)) continue;
            if (existing != nullptr && !IsSafelyReplaceable(existing)) continue;

            if (source.kind == PatternKind::Empty)
            {
                // Never demolish a live colony object from this editor tool.
                // The editor may still hold selection/placement references to
                // an occupied slot, and the RemoveObject/RemoveBuilding/
                // DestroyInstance sequence crashed on a partially built city.
                continue;
            }

            // Matching nouns are restyled in place. Existing objects with a
            // different noun are preserved below rather than removed from the
            // live editor.
            if (existing != nullptr && existing->GetNounID() == source.nounID)
            {
                if (money < source.cost)
                {
                    ranOut = true;
                    continue;
                }
                UpdateExistingObject(existing, target, layout, source);
                {
                    char line[192];
                    _snprintf_s(line, _countof(line), _TRUNCATE,
                        "update slot %u noun %08X money %d",
                        static_cast<unsigned>(index), source.nounID, money);
                    WriteDiagnosticLine(L"colony-apply.log", line);
                }
                money -= source.cost;
                continue;
            }

            // Replacing an occupied slot requires removing the live object
            // from its layout, city container, and noun manager. The editor
            // can still own a reference to that building/turret/ornament, so
            // this path is unsafe while pasting into a colony. Preserve it
            // unchanged and continue applying the pattern to empty slots.
            if (existing != nullptr)
            {
                char line[160];
                _snprintf_s(line, _countof(line), _TRUNCATE,
                    "keep occupied slot %u noun %08X (pattern noun %08X)",
                    static_cast<unsigned>(index), existing->GetNounID(), source.nounID);
                WriteDiagnosticLine(L"colony-apply.log", line);
                continue;
            }

            if (money < source.cost)
            {
                ranOut = true;
                continue;
            }

            Simulator::cGameData* replacement = CreatePatternObject(city, source);
            if (replacement == nullptr)
            {
                char line[128];
                _snprintf_s(line, _countof(line), _TRUNCATE,
                    "create failed slot %u noun %08X",
                    static_cast<unsigned>(index), source.nounID);
                WriteDiagnosticLine(L"colony-apply.log", line);
                continue;
            }
            if (index >= layout.mSlots.size())
            {
                // The layout shrank underneath this loop; do not attach to a
                // slot that no longer exists.
                char line[128];
                _snprintf_s(line, _countof(line), _TRUNCATE,
                    "layout shrank at slot %u noun %08X",
                    static_cast<unsigned>(index), source.nounID);
                WriteDiagnosticLine(L"colony-apply.log", line);
                GameNounManager.DestroyInstance(replacement);
                break;
            }

            Simulator::cLayoutSlot& freshTarget = layout.mSlots[index];
            Simulator::cSpatialObject* spatial = object_cast<Simulator::cSpatialObject>(replacement);
            if (Simulator::cBuilding* building = object_cast<Simulator::cBuilding>(replacement))
                city->AddBuilding(building);
            else if (Simulator::cTurret* turret = object_cast<Simulator::cTurret>(replacement))
                city->mTurrets.push_back(turret);
            else if (Simulator::cOrnament* ornament = object_cast<Simulator::cOrnament>(replacement))
                city->mCivicObjects.push_back(ornament);
            freshTarget.SetObject(replacement);

            // The game's own add/set helpers may reposition or reorient the
            // object. Assert the pattern's placement last so it survives.
            const Math::Quaternion frame = LayoutSlotFrame(layout, freshTarget.mPosition);
            spatial->SetPosition(freshTarget.mPosition);
            spatial->SetScale(source.scale);
            spatial->SetOrientation(frame * source.relativeOrientation);

            {
                char line[192];
                _snprintf_s(line, _countof(line), _TRUNCATE,
                    "create slot %u noun %08X money %d frame %.4f %.4f %.4f %.4f",
                    static_cast<unsigned>(index), source.nounID, money,
                    frame.x, frame.y, frame.z, frame.w);
                WriteDiagnosticLine(L"colony-apply.log", line);
            }
            money -= source.cost;
            if (source.kind == PatternKind::Building || source.kind == PatternKind::Turret)
                ++createdColonyImprovements;
        }
        return true;
    }

    bool ApplyToCity(Simulator::cCity* city, int& money, bool& ranOut,
        int& createdColonyImprovements)
    {
        if (!HasMatchingTopology(city)) return false;
        const bool buildings = ApplyLayout(city, city->mBuildingsLayout, sPattern.buildings, money, ranOut,
            createdColonyImprovements);
        const bool decorations = ApplyLayout(city, city->mDecorationsLayout, sPattern.decorations, money, ranOut,
            createdColonyImprovements);
        const bool turrets = ApplyLayout(city, city->mTurretsLayout, sPattern.turrets, money, ranOut,
            createdColonyImprovements);
        // ProcessBuildingUpdate drives the open community-editor UI. Calling
        // it for an off-screen city is unsafe and was the remaining all-city
        // crash path; those cities are reconciled by the normal model update.
        if (IsActivelyEditedPlayerCity(city))
            Simulator::cCity::ProcessBuildingUpdate(city);
        return buildings && decorations && turrets;
    }

    bool HasSelectedCityBuilding()
    {
        Simulator::cSimulatorSpaceGame* game = Simulator::cSimulatorSpaceGame::Get();
        return game != nullptr && game->mpCommunityEditor != 0 && game->mpSelectedBuilding != 0;
    }

    bool IsUsablePaletteWindow(UTFWin::IWindow* window)
    {
        for (UTFWin::IWindow* current = window; current != nullptr; current = current->GetParent())
        {
            const UTFWin::WindowFlags flags = current->GetFlags();
            if ((flags & UTFWin::kWinFlagVisible) == 0 ||
                (flags & UTFWin::kWinFlagEnabled) == 0)
                return false;
        }
        return true;
    }

    void FindPaletteItems(UTFWin::IWindow* window, Palettes::PaletteItem::ItemType itemType,
        std::vector<Palettes::StandardItemUI*>& matches)
    {
        if (window == nullptr) return;
        for (UTFWin::IWinProc* procedure : window->procedures())
        {
            Palettes::PalettePageUI* page = object_cast<Palettes::PalettePageUI>(procedure);
            if (page == nullptr) continue;
            for (const StandardItemUIPtr& itemUI : page->mStandardItems)
            {
                if (itemUI != nullptr && itemUI->mpItem != nullptr &&
                    itemUI->mpItem->mTypeID == itemType &&
                    IsUsablePaletteWindow(itemUI->mpWindow.get()))
                    matches.push_back(itemUI.get());
            }
        }
        for (UTFWin::IWindow* child : window->children())
            FindPaletteItems(child, itemType, matches);
    }

    // A manual palette click normally establishes mpSelectedBuilding. Pasting
    // cannot rely on the player having made that click, so select one random
    // eligible Sporepedia entry from the open planner palette instead. The
    // item type preserves the slot category (building versus turret).
    bool SelectRandomPaletteItem(Palettes::PaletteItem::ItemType itemType)
    {
        std::vector<Palettes::StandardItemUI*> matches;
        FindPaletteItems(WindowManager.GetMainWindow(), itemType, matches);
        if (matches.empty()) return false;

        const size_t index = static_cast<size_t>(GetTickCount64()) % matches.size();
        UTFWin::IWindow* itemWindow = matches[index]->mpWindow.get();
        UTFWin::Message click{};
        click.source = itemWindow;
        click.eventType = UTFWin::kMsgButtonClick;
        WindowManager.SendMsg(itemWindow, itemWindow, click, true);
        return HasSelectedCityBuilding();
    }

    bool EnsureSelectedCityItem()
    {
        if (HasSelectedCityBuilding()) return true;
        for (const PatternSlot& slot : sPattern.buildings)
        {
            if (slot.kind == PatternKind::Building)
                return SelectRandomPaletteItem(Palettes::PaletteItem::kItemCityBuilding);
        }
        for (const PatternSlot& slot : sPattern.turrets)
        {
            if (slot.kind == PatternKind::Turret)
                return SelectRandomPaletteItem(Palettes::PaletteItem::kItemCityTurret);
        }
        return false;
    }

    void AwardColonyImprovementProgress(int improvements)
    {
        if (improvements <= 0) return;
        Simulator::cSimulatorSpaceGame* game = Simulator::cSimulatorSpaceGame::Get();
        if (game == nullptr || game->mpBadgeManager == nullptr) return;

        // Count each newly pasted building or turret toward the requested
        // Colonist badge progress. Existing-object updates and decorations do
        // not create a new colony improvement.
        game->mpBadgeManager->AddToBadgeProgress(
            Simulator::BadgeManagerEvent::ReqPlanetsColonized, improvements);
    }

    void SetAllCaptionColors(UTFWin::IButton* button, Math::Color color)
    {
        if (button == nullptr) return;
        for (uint32_t index = 0; index < 8; ++index)
            button->SetCaptionColor(static_cast<UTFWin::StateIndices>(index), color);
    }

    void ShowStatus(const char16_t* text, Math::Color color, uint32_t milliseconds = 5000)
    {
        if (sStatusText == nullptr) return;
        sStatusText->ToWindow()->SetCaption(text);
        SetAllCaptionColors(sStatusText.get(), color);
        sStatusText->ToWindow()->SetVisible(true);
        sStatusUntil = GetTickCount64() + milliseconds;
    }

    void DumpCopyDiagnostics(Simulator::cCity* city)
    {
        {
            char line[512];
            _snprintf_s(line, _countof(line), _TRUNCATE,
                "--- copy city %08X player=%d edited=%d buildings=%u/%u decorations=%u/%u turrets=%u/%u ---",
                static_cast<unsigned>(reinterpret_cast<uintptr_t>(city)),
                city->mbIsPlayerCity ? 1 : 0, IsActivelyEditedPlayerCity(city) ? 1 : 0,
                static_cast<unsigned>(city->mBuildings.size()),
                static_cast<unsigned>(city->mBuildingsLayout.mSlots.size()),
                static_cast<unsigned>(city->mCivicObjects.size()),
                static_cast<unsigned>(city->mDecorationsLayout.mSlots.size()),
                static_cast<unsigned>(city->mTurrets.size()),
                static_cast<unsigned>(city->mTurretsLayout.mSlots.size()));
            WriteDiagnosticLine(L"colony-copy.log", line);
        }

        const auto dumpLayout = [](const char* name, const Simulator::cCommunityLayout& layout)
        {
            char line[512];
            _snprintf_s(line, _countof(line), _TRUNCATE,
                "%s slots=%u dir=(%.3f %.3f %.3f) origin=(%.3f %.3f %.3f) field2C=(%.4f %.4f %.4f %.4f)",
                name, static_cast<unsigned>(layout.mSlots.size()),
                layout.mDirection.x, layout.mDirection.y, layout.mDirection.z,
                layout.mOrigin.x, layout.mOrigin.y, layout.mOrigin.z,
                layout.field_2C.x, layout.field_2C.y, layout.field_2C.z, layout.field_2C.w);
            WriteDiagnosticLine(L"colony-copy.log", line);
        };
        dumpLayout("buildings", city->mBuildingsLayout);
        dumpLayout("decorations", city->mDecorationsLayout);
        dumpLayout("turrets", city->mTurretsLayout);

        const auto dumpSlots = [](const char* name, const Simulator::cCommunityLayout& layout)
        {
            for (size_t index = 0; index < layout.mSlots.size(); ++index)
            {
                const Simulator::cLayoutSlot& slot = layout.mSlots[index];
                Simulator::cGameData* object = slot.mpObject.get();
                if (object == nullptr) continue;
                Simulator::cSpatialObject* spatial = object_cast<Simulator::cSpatialObject>(object);
                if (spatial == nullptr) continue;
                const Math::Quaternion& world = spatial->GetOrientation();
                const Math::Quaternion frame = LayoutSlotFrame(layout, slot.mPosition);
                char line[512];
                _snprintf_s(line, _countof(line), _TRUNCATE,
                    "%s slot %u noun %08X pos=(%.2f %.2f %.2f) world=(%.4f %.4f %.4f %.4f) frame=(%.4f %.4f %.4f %.4f) scale=%.3f",
                    name, static_cast<unsigned>(index), object->GetNounID(),
                    slot.mPosition.x, slot.mPosition.y, slot.mPosition.z,
                    world.x, world.y, world.z, world.w,
                    frame.x, frame.y, frame.z, frame.w, spatial->GetScale());
                WriteDiagnosticLine(L"colony-copy.log", line);
            }
        };
        dumpSlots("buildings", city->mBuildingsLayout);
        dumpSlots("decorations", city->mDecorationsLayout);
        dumpSlots("turrets", city->mTurretsLayout);
    }

    void CopyCurrentPattern()
    {
        Simulator::cCity* city = FindEditedCity();
        if (city == nullptr)
        {
            ShowStatus(u"No player colony found to copy.", Math::Color(235, 80, 80, 255));
            return;
        }
        sPattern.buildings = CaptureLayout(city->mBuildingsLayout);
        sPattern.decorations = CaptureLayout(city->mDecorationsLayout);
        sPattern.turrets = CaptureLayout(city->mTurretsLayout);
        sPattern.valid = true;
        DumpCopyDiagnostics(city);

        const auto countKind = [](const std::vector<PatternSlot>& slots, PatternKind kind)
        {
            size_t count = 0;
            for (const PatternSlot& slot : slots)
                if (slot.kind == kind) ++count;
            return count;
        };
        const auto countOccupied = [](const Simulator::cCommunityLayout& layout)
        {
            size_t count = 0;
            for (const Simulator::cLayoutSlot& slot : layout.mSlots)
                if (slot.mpObject != nullptr) ++count;
            return count;
        };

        // The city containers can hold objects that are not present in any
        // layout slot. Those objects are invisible to the capture step, and
        // this is the most likely explanation for a copy that misses
        // buildings. Warn instead of silently copying a partial city.
        const bool containersMatchLayouts =
            city->mBuildings.size() == countOccupied(city->mBuildingsLayout) &&
            city->mCivicObjects.size() == countOccupied(city->mDecorationsLayout) &&
            city->mTurrets.size() == countOccupied(city->mTurretsLayout);

        wchar_t buffer[160];
        _snwprintf_s(buffer, _countof(buffer), _TRUNCATE,
            L"Copied %u buildings, %u decorations, %u turrets.",
            static_cast<unsigned>(countKind(sPattern.buildings, PatternKind::Building)),
            static_cast<unsigned>(countKind(sPattern.decorations, PatternKind::Ornament)),
            static_cast<unsigned>(countKind(sPattern.turrets, PatternKind::Turret)));
        wchar_t warning[160]{};
        if (!containersMatchLayouts)
        {
            _snwprintf_s(warning, _countof(warning), _TRUNCATE,
                L" Warning: some objects are not in layout slots (%u/%u, %u/%u, %u/%u).",
                static_cast<unsigned>(city->mBuildings.size()),
                static_cast<unsigned>(countOccupied(city->mBuildingsLayout)),
                static_cast<unsigned>(city->mCivicObjects.size()),
                static_cast<unsigned>(countOccupied(city->mDecorationsLayout)),
                static_cast<unsigned>(city->mTurrets.size()),
                static_cast<unsigned>(countOccupied(city->mTurretsLayout)));
        }
        wcscat_s(buffer, warning);
        if (SavePersistentPattern())
            ShowStatus(reinterpret_cast<const char16_t*>(buffer), Math::Color(80, 230, 100, 255),
                containersMatchLayouts ? 5000 : 12000);
        else
            ShowStatus(u"Pattern copied, but its file could not be saved.", Math::Color(240, 150, 60, 255), 8000);
    }

    void ApplyPattern(bool allColonies)
    {
        if (!sPattern.valid) return;
        App::ConsolePrintF("ERKEK2000 QoL Runtime: applying colony pattern (buildings %u, decorations %u, turrets %u).",
            static_cast<unsigned>(sPattern.buildings.size()),
            static_cast<unsigned>(sPattern.decorations.size()),
            static_cast<unsigned>(sPattern.turrets.size()));
        // Homeworld city templates have layout slots that are not compatible
        // with ordinary colony templates, even when their slot counts happen
        // to agree. Replacing objects through that mismatch can corrupt the
        // community editor, so fail closed instead of attempting a partial
        // positional mapping.
        Simulator::cPlanetRecord* activePlanet = Simulator::GetActivePlanetRecord();
        if (activePlanet != nullptr && activePlanet->mbHomeWorld)
        {
            ShowStatus(u"Saved patterns cannot be applied to a homeworld.",
                Math::Color(240, 150, 60, 255), 8000);
            return;
        }
        // The game factory dereferences the active palette selection. If the
        // player has not selected one, make a matching random selection from
        // the visible, unlocked Sporepedia palette before any city is changed.
        if (!EnsureSelectedCityItem())
        {
            ShowStatus(u"No compatible unlocked Sporepedia building is available for this pattern.",
                Math::Color(240, 150, 60, 255), 8000);
            return;
        }
        Simulator::cEmpire* empire = Simulator::GetPlayerEmpire();
        if (empire == nullptr)
        {
            ShowStatus(u"Player funds are unavailable.", Math::Color(235, 80, 80, 255));
            return;
        }

        std::vector<Simulator::cCity*> targets;
        if (allColonies) targets = GetPlayerColonies();
        else
        {
            Simulator::cCity* city = FindEditedCity();
            if (city != nullptr) targets.push_back(city);
        }
        if (targets.empty())
        {
            ShowStatus(u"No player colony found.", Math::Color(235, 80, 80, 255));
            return;
        }

        if (allColonies)
        {
            // Mutating the city that the community editor has open is the
            // riskiest operation in this path. Process it last so that a
            // failure leaves the other colonies built and the diagnostic log
            // shows how far the apply got.
            Simulator::cCity* editedCity = FindActivelyEditedPlayerCity();
            if (editedCity != nullptr)
            {
                std::vector<Simulator::cCity*> reordered;
                reordered.reserve(targets.size());
                for (Simulator::cCity* city : targets)
                    if (city != editedCity) reordered.push_back(city);
                reordered.push_back(editedCity);
                targets = reordered;
            }
        }

        int money = std::max(0, empire->mEmpireMoney);
        bool ranOut = false;
        bool topologyFailure = false;
        int createdColonyImprovements = 0;
        {
            char line[192];
            _snprintf_s(line, _countof(line), _TRUNCATE,
                "apply start all=%d targets=%u money=%d buildings=%u decorations=%u turrets=%u",
                allColonies ? 1 : 0, static_cast<unsigned>(targets.size()), money,
                static_cast<unsigned>(sPattern.buildings.size()),
                static_cast<unsigned>(sPattern.decorations.size()),
                static_cast<unsigned>(sPattern.turrets.size()));
            WriteDiagnosticLine(L"colony-apply.log", line);
        }
        for (Simulator::cCity* city : targets)
        {
            char line[192];
            _snprintf_s(line, _countof(line), _TRUNCATE,
                "apply city %08X player=%d edited=%d slots=%u/%u/%u money=%d",
                static_cast<unsigned>(reinterpret_cast<uintptr_t>(city)),
                city->mbIsPlayerCity ? 1 : 0, IsActivelyEditedPlayerCity(city) ? 1 : 0,
                static_cast<unsigned>(city->mBuildingsLayout.mSlots.size()),
                static_cast<unsigned>(city->mDecorationsLayout.mSlots.size()),
                static_cast<unsigned>(city->mTurretsLayout.mSlots.size()), money);
            WriteDiagnosticLine(L"colony-apply.log", line);
            if (!ApplyToCity(city, money, ranOut, createdColonyImprovements)) topologyFailure = true;
            char done[128];
            _snprintf_s(done, _countof(done), _TRUNCATE,
                "apply city %08X done money=%d",
                static_cast<unsigned>(reinterpret_cast<uintptr_t>(city)), money);
            WriteDiagnosticLine(L"colony-apply.log", done);
        }
        empire->mEmpireMoney = money;
        GameNounManager.UpdateModels();
        AwardColonyImprovementProgress(createdColonyImprovements);

        if (ranOut)
            ShowStatus(u"Build incomplete: funds ran out.", Math::Color(240, 75, 75, 255), 8000);
        else if (topologyFailure)
            ShowStatus(u"Build incomplete: a colony layout did not match.", Math::Color(240, 150, 60, 255), 8000);
        else
            ShowStatus(u"Saved pattern applied.", Math::Color(80, 230, 100, 255));
    }

    void UpdateHoverText()
    {
        if (sHoverText == nullptr || sHoveredButton == 0 || !sPattern.valid)
        {
            if (sHoverText != nullptr) sHoverText->ToWindow()->SetVisible(false);
            return;
        }

        int64_t cost = 0;
        if (sHoveredButton == kApplyAllButtonID)
        {
            for (Simulator::cCity* city : GetPlayerColonies())
                cost += PatternCostForCity(city);
        }
        else
        {
            cost = PatternCostForCity(FindEditedCity());
        }
        Simulator::cEmpire* empire = Simulator::GetPlayerEmpire();
        const bool affordable = empire != nullptr && cost <= empire->mEmpireMoney;

        wchar_t buffer[256]{};
        _snwprintf_s(buffer, _countof(buffer), _TRUNCATE,
            L"Required: %lld Sporebucks. Warning: builds only until funds run out.", cost);
        sHoverText->ToWindow()->SetCaption(reinterpret_cast<const char16_t*>(buffer));
        SetAllCaptionColors(sHoverText.get(), affordable
            ? Math::Color(70, 235, 95, 255)
            : Math::Color(240, 70, 70, 255));
        sHoverText->ToWindow()->SetVisible(true);
    }

    IButtonPtr CreateButton(UTFWin::IWindow* parent, uint32_t id, const char16_t* caption,
        const Math::Rectangle& area)
    {
        IButtonPtr button = UTFWin::IButton::Create();
        UTFWin::IWindow* window = button->ToWindow();
        window->SetControlID(id);
        window->SetCaption(caption);
        window->SetArea(area);
        window->SetFillColor(Math::Color(35, 55, 75, 225));
        window->SetFlag(UTFWin::kWinFlagAlwaysInFront, true);
        SetAllCaptionColors(button.get(), Math::Color(245, 245, 245, 255));
        parent->AddWindow(window);
        return button;
    }

    void DestroyPlannerButtons()
    {
        sHoveredButton = 0;
        sStatusUntil = 0;
        IButtonPtr buttons[] = { sCopyButton, sApplyButton, sApplyAllButton, sHoverText, sStatusText };
        for (IButtonPtr& button : buttons)
        {
            if (button != nullptr && sMainWindow != nullptr)
                sMainWindow->DisposeWindowFamily(button->ToWindow());
        }
        sCopyButton = nullptr;
        sApplyButton = nullptr;
        sApplyAllButton = nullptr;
        sHoverText = nullptr;
        sStatusText = nullptr;
    }

    void CreatePlannerButtons()
    {
        if (sMainWindow == nullptr || sController == nullptr || sCopyButton != nullptr) return;
        UTFWin::IWindow* mainWindow = sMainWindow.get();
        sCopyButton = CreateButton(mainWindow, kCopyButtonID, u"Copy colony pattern",
            Math::Rectangle(555.0f, 90.0f, 790.0f, 122.0f));
        sApplyButton = CreateButton(mainWindow, kApplyButtonID, u"Apply saved pattern",
            Math::Rectangle(555.0f, 126.0f, 790.0f, 158.0f));
        sApplyAllButton = CreateButton(mainWindow, kApplyAllButtonID, u"Apply to all colonies",
            Math::Rectangle(555.0f, 162.0f, 790.0f, 194.0f));
        sHoverText = CreateButton(mainWindow, 0xE2A10004, u"",
            Math::Rectangle(390.0f, 202.0f, 790.0f, 250.0f));
        sStatusText = CreateButton(mainWindow, 0xE2A10005, u"",
            Math::Rectangle(390.0f, 258.0f, 790.0f, 294.0f));

        sHoverText->ToWindow()->SetEnabled(false);
        sStatusText->ToWindow()->SetEnabled(false);
        sHoverText->ToWindow()->SetVisible(false);
        sStatusText->ToWindow()->SetVisible(false);
        sApplyButton->ToWindow()->SetEnabled(sPattern.valid);
        sApplyAllButton->ToWindow()->SetEnabled(sPattern.valid);
        sCopyButton->ToWindow()->AddWinProc(sController.get());
        sApplyButton->ToWindow()->AddWinProc(sController.get());
        sApplyAllButton->ToWindow()->AddWinProc(sController.get());
    }

    class ColonyPatternController final : public UTFWin::DefaultWinProc<
        UTFWin::kEventFlagBasicInput | UTFWin::kEventRefresh>
    {
    public:
        bool HandleUIMessage(UTFWin::IWindow* window, const UTFWin::Message& message) override
        {
            const uint32_t id = window != nullptr ? window->GetControlID() : 0;
            if (message.IsType(UTFWin::kMsgMouseEnter) &&
                (id == kApplyButtonID || id == kApplyAllButtonID))
            {
                sHoveredButton = id;
                UpdateHoverText();
            }
            else if (message.IsType(UTFWin::kMsgMouseLeave) && id == sHoveredButton)
            {
                sHoveredButton = 0;
                sHoverText->ToWindow()->SetVisible(false);
            }
            else if (message.IsType(UTFWin::kMsgButtonClick))
            {
                if (id == kCopyButtonID) CopyCurrentPattern();
                else if (id == kApplyButtonID) ApplyPattern(false);
                else if (id == kApplyAllButtonID) ApplyPattern(true);
            }
            return false;
        }
    };

    void DetachColonyPatternUI()
    {
        DestroyPlannerButtons();
        sController = nullptr;
        sMainWindow = nullptr;
    }

    void UpdateColonyPatternUI()
    {
        // The planner UI may only exist while a fully entered Space game has
        // the colony planner open. Never create or dispose windows during a
        // stage load or outside Space; that is the transition window in which
        // 0.5.1 through 0.5.3 crashed.
        if (!Simulator::IsSpaceGame() || Simulator::IsLoadingGameMode())
        {
            return;
        }

        Simulator::cSimulatorSpaceGame* game = Simulator::cSimulatorSpaceGame::Get();
        // mpCommunityEditor remains non-null after the planner closes. Couple
        // it with the city's live edit flag so these controls cannot leak into
        // normal planet, solar-system, or galaxy-map UI.
        const bool plannerActive = game != nullptr && game->mpCommunityEditor != 0 &&
            FindActivelyEditedPlayerCity() != nullptr;
        if (!plannerActive)
        {
            if (sCopyButton != nullptr)
            {
                DestroyPlannerButtons();
            }
            // Do not keep a reference to the main window while the planner is
            // closed; it may be replaced by the next UI transition.
            sMainWindow = nullptr;
            return;
        }

        UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
        if (mainWindow == nullptr)
        {
            return;
        }

        if (sMainWindow.get() != mainWindow)
        {
            // The planner was opened in a fresh UI context. Drop any stale
            // window reference and button handles from the previous one.
            if (sCopyButton != nullptr)
            {
                DestroyPlannerButtons();
            }
            sMainWindow = mainWindow;
        }

        if (sController == nullptr)
        {
            sController = new ColonyPatternController();
        }

        CreatePlannerButtons();
        sApplyButton->ToWindow()->SetEnabled(sPattern.valid);
        sApplyAllButton->ToWindow()->SetEnabled(sPattern.valid);
        UpdateHoverText();
        if (sStatusUntil != 0 && GetTickCount64() >= sStatusUntil)
        {
            sStatusText->ToWindow()->SetVisible(false);
            sStatusUntil = 0;
        }
    }
}

void ERKEK2000QoL::InstallColonyPatternButtons()
{
    if (sUpdateListener != nullptr) return;
    LoadPersistentPattern();
    sUpdateListener = App::AddUpdateFunction(UpdateColonyPatternUI);
}

void ERKEK2000QoL::RemoveColonyPatternButtons()
{
    if (sUpdateListener != nullptr)
    {
        App::RemoveUpdateFunction(sUpdateListener);
        sUpdateListener = nullptr;
    }
    DetachColonyPatternUI();
    sPattern = ColonyPattern{};
}
