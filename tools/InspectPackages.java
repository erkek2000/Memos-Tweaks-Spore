import java.nio.file.*;
import java.util.*;
import sporemodder.MainApp;
import sporemodder.HashManager;
import sporemodder.file.dbpf.*;
import sporemodder.file.filestructures.*;
import sporemodder.file.prop.PropertyList;

/** Read-only access to game packages using the installed SporeModder FX library.
 * Writes decoded reference material only to the supplied workspace directory. */
class InspectPackages {
    public static void main(String[] args) throws Exception {
        if (args.length < 3) throw new IllegalArgumentException("output-dir Kisu.package game-package...");
        MainApp.testInit();
        var hm = HashManager.get();
        Path output = Path.of(args[0]);
        Set<sporemodder.file.ResourceKey> kisuKeys = new HashSet<>();
        try (var fs = new FileStream(args[1], "r")) {
            var db = new DatabasePackedFile();
            db.read(fs);
            for (var item : db.index.items) kisuKeys.add(item.name);
        }
        for (int i = 2; i < args.length; i++) {
            var packagePath = Path.of(args[i]);
            Path destination = output.resolve(packagePath.getFileName().toString());
            int count = 0;
            try (var fs = new FileStream(packagePath.toFile(), "r")) {
                var db = new DatabasePackedFile();
                db.read(fs);
                for (var item : db.index.items) {
                    if (item.name.getTypeID() != hm.getTypeHash("prop")) continue;
                    String group = hm.getFileName(item.name.getGroupID());
                    if (!kisuKeys.contains(item.name) && !group.equals("gametuning~")
                            && !group.equals("spaceevents~") && !group.equals("spacetools~")) continue;
                    String instance = hm.getFileName(item.name.getInstanceID());
                    Path dir = destination.resolve(group);
                    Files.createDirectories(dir);
                    try (var data = item.processFile(fs)) {
                        var props = new PropertyList();
                        props.read(data);
                        Files.writeString(dir.resolve(instance + ".prop.prop_t"), props.toArgScript().toString());
                    }
                    count++;
                }
                System.out.println(packagePath.getFileName() + ": " + db.index.items.size()
                        + " indexed resources; " + count + " reference properties decoded");
            }
        }
    }
}
