import java.nio.file.*;
import java.util.*;
import sporemodder.MainApp;
import sporemodder.HashManager;
import sporemodder.file.ResourceKey;
import sporemodder.file.dbpf.*;
import sporemodder.file.filestructures.*;
import sporemodder.file.prop.PropertyList;
import sporemodder.file.spui.SporeUserInterface;
import sporemodder.file.spui.components.WindowBase;
import sporemodder.util.Vector2;

/** Verify the compiled package, not just its editable source. */
class VerifyPackage {
    static Map<ResourceKey, byte[]> read(String path) throws Exception {
        var result = new HashMap<ResourceKey, byte[]>();
        try (var fs = new FileStream(path, "r")) {
            var db = new DatabasePackedFile();
            db.read(fs);
            for (var item : db.index.items) {
                try (var data = item.processFile(fs)) {
                    if (result.put(item.name, data.toByteArray()) != null)
                        throw new IllegalStateException("Duplicate resource " + item.name);
                }
            }
        }
        return result;
    }
    static PropertyList props(byte[] data) throws Exception {
        if (data == null) throw new IllegalStateException("Missing property resource");
        var result = new PropertyList();
        try (var stream = new MemoryStream(data)) { result.read(stream); }
        return result;
    }
    static ResourceKey key(String group, String name) {
        var key = new ResourceKey();
        key.setGroupID(group); key.setInstanceID(name); key.setTypeID("prop");
        return key;
    }
    static void require(boolean value, String message) {
        if (!value) throw new IllegalStateException(message);
    }
    static void verifyInstantCommunication(byte[] data) throws Exception {
        SporeUserInterface.getDesigner().load();
        var ui = new SporeUserInterface();
        try (var stream = new MemoryStream(data)) { ui.read(stream); }
        int panels = 0, instantGlides = 0;
        for (var element : ui.getElements()) {
            if (!(element instanceof WindowBase window)) continue;
            String id = window.getControlID();
            if (!"0x05E4E5F0".equalsIgnoreCase(id) && !"0x05E4E5F8".equalsIgnoreCase(id)) continue;
            panels++;
            for (var proc : window.getWinProcs()) {
                if (!"Glide".equals(proc.getDesignerClass().getName())) continue;
                var type = proc.getDesignerClass();
                var time = (Float) proc.getProperty(type.getProperty("Time").getProxyID());
                var offset = (Vector2) proc.getProperty(type.getProperty("Offset").getProxyID());
                if (time != null && time == 0.0f && offset != null
                        && offset.getX() == 0.0f && offset.getY() == 0.0f) instantGlides++;
            }
        }
        require(panels == 2 && instantGlides == 2,
                "Communication UI does not contain the two expected instant panel glides");
    }
    public static void main(String[] args) throws Exception {
        MainApp.testInit();
        var hm = HashManager.get();
        var original = read(args[0]);
        var built = read(args[1]);
        var enabled = new HashSet<String>(Arrays.asList(args[4].split(",")));
        enabled.remove("none");
        var expected = new HashMap<ResourceKey, byte[]>(original);
        var commKey = key("layouts_atlas~", "CommScreen-3");
        commKey.setTypeID("spui");
        byte[] commSource = null;
        if (enabled.contains("InstantPlanetaryDialogue")) {
            commSource = Files.readAllBytes(Path.of(args[3]));
            expected.put(commKey, commSource);
        }
        int removed = 0;
        for (var k : original.keySet()) {
            if (k.getGroupID() == hm.getFileHash("space_badges~") &&
                    !k.equals(key("space_badges~", "captain"))) {
                if (enabled.contains("RestoreVanillaBadgeRequirements")) {
                    require(!built.containsKey(k), "Tiered badge override remains: " + k);
                    expected.remove(k); removed++;
                }
            }
        }
        require(removed == (enabled.contains("RestoreVanillaBadgeRequirements") ? 110 : 0),
                "Unexpected tiered badge count: " + removed);
        // Exact intended deltas: all values are copied from decoded vanilla data.
        Map<ResourceKey, String[]> changes = new LinkedHashMap<>();
        if (enabled.contains("RestoreColonyBuildingCosts")) changes.put(
                key("animations~", "CityGameBuildingTuning"), new String[]{
                    "DefenseCostSpace", "EntertainmentCostSpace", "IndustryCostSpace", "SleepCostSpace"});
        var economyChanges = new ArrayList<String>();
        if (enabled.contains("RestoreGeneralSpiceProduction"))
            economyChanges.add("spaceEconomySpiceProductionMultiplier");
        if (enabled.contains("RestoreVanillaMaximumTradeRoutes"))
            economyChanges.add("tradeRouteMaxNumber");
        if (enabled.contains("RestoreVanillaMaximumSpiceBought"))
            economyChanges.add("spaceEconomyTradeMaxSpiceBought");
        if (!economyChanges.isEmpty()) changes.put(key("gametuning~", "SpaceEconomy"),
                economyChanges.toArray(new String[0]));
        if (enabled.contains("RestoreVanillaColonySpiceStorage")) changes.put(
                key("gametuning~", "SpaceColonization"),
                new String[]{"colonyMaxSpiceStoredPerColony"});
        if (enabled.contains("RestoreCoreTravelRestriction")) changes.put(
                key("gametuning~", "SpaceGalacticConstants"), new String[]{"galacticCoreTravelRadii"});
        var solarChanges = new ArrayList<String>();
        if (enabled.contains("RestoreGroxEmpireSize")) solarChanges.add("grobEmpireSize");
        if (enabled.contains("RestoreTerrestrialMoonChance")) solarChanges.add("chanceTerrestrialHasMoon");
        if (enabled.contains("RestoreTerrestrialRingChance")) solarChanges.add("chanceTerrestrialHasRings");
        if (enabled.contains("RestoreGroxExclusiveRadius")) solarChanges.add("grobOnlyRadius");
        if (enabled.contains("RestoreGroxSpreadRadius")) solarChanges.add("grobStarsSpreadRadius");
        if (!solarChanges.isEmpty()) changes.put(key("gametuning~", "SpaceSolarSystem"),
                solarChanges.toArray(new String[0]));
        if (enabled.contains("RestoreShieldCooldown")) changes.put(
                key("spacetools~", "shield"), new String[]{"spaceToolRechargeRate"});
        // Compare against the reference properties decoded from vanilla PatchData
        // by InspectPackages.java, using the same installed SMFX property parser.
        Path configuredProject = Path.of(args[5]);
        for (var change : changes.entrySet()) {
            var k = change.getKey();
            String group = hm.getFileName(k.getGroupID());
            String name = hm.getFileName(k.getInstanceID());
            var configured = new PropertyList();
            var script = configured.generateStream();
            script.process(Files.readString(configuredProject.resolve(group).resolve(name + ".prop.prop_t")));
            require(script.getErrors().isEmpty(), "Invalid configured source: " + k);
            var want = props(original.get(k));
            for (var property : change.getValue()) {
                require(configured.get(property) != null, "Missing configured property " + property);
                want.add(property, configured.get(property));
            }
            require(want.toArgScript().toString().equals(props(built.get(k)).toArgScript().toString()),
                    "Unexpected changes or wrong restored values: " + k);
            System.out.println("PASS configured values: " + k + " / " + String.join(", ", change.getValue()));
        }
        require(expected.keySet().equals(built.keySet()), "Resource set differs from configured Kisu baseline");
        if (enabled.contains("InstantPlanetaryDialogue")) {
            require(Arrays.equals(commSource, built.get(commKey)), "Packed communication UI differs from generated source");
            verifyInstantCommunication(built.get(commKey));
            System.out.println("PASS communication UI contains two instant main-panel glides");
        } else {
            require(!built.containsKey(commKey), "Communication override present while feature is disabled");
            System.out.println("PASS communication UI override omitted");
        }
        int exact = 0, equivalent = 0;
        var namesKey = new ResourceKey();
        namesKey.setGroupID("sporemaster"); namesKey.setInstanceID("names"); namesKey.setTypeID("txt");
        for (var entry : expected.entrySet()) {
            var k = entry.getKey();
            if (changes.containsKey(k)) continue;
            if (k.equals(commKey)) continue;
            // SMFX regenerates this editor-only name dictionary when packing.
            // It is not gameplay data; duplicate IDs are still rejected above.
            if (k.equals(namesKey)) continue;
            if (Arrays.equals(entry.getValue(), built.get(k))) { exact++; continue; }
            require(k.getTypeID() == hm.getTypeHash("prop"), "Inherited binary asset changed: " + k);
            require(props(entry.getValue()).toArgScript().toString().equals(props(built.get(k)).toArgScript().toString()),
                    "Inherited properties changed: " + k);
            equivalent++;
        }
        System.out.println("PASS no duplicate resource IDs or unexpected resources");
        if (enabled.contains("RestoreVanillaBadgeRequirements"))
            System.out.println("PASS " + removed + " tiered badge overrides omitted; vanilla fallback enabled");
        else System.out.println("PASS Kisu badge overrides retained");
        System.out.println("PASS " + exact + " inherited resources byte-identical; " + equivalent + " canonically equivalent");
        System.out.println("PASS all retained cargo, white spice, models, icons, locale and GA signature resources preserved");
        System.out.println("INFO sporemaster/names.txt regenerated by SMFX (editor metadata only)");
        System.out.println("TOTAL " + original.size() + " original resources -> " + built.size() + " built resources");
        System.out.println("Static package verification only; in-game behavior has not been tested.");
    }
}
