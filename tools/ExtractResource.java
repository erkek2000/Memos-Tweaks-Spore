import java.nio.file.*;
import sporemodder.MainApp;
import sporemodder.file.ResourceKey;
import sporemodder.file.dbpf.DatabasePackedFile;
import sporemodder.file.filestructures.FileStream;

/** Extract one named resource from a DBPF package for local reference. */
class ExtractResource {
    public static void main(String[] args) throws Exception {
        MainApp.testInit();
        var key = new ResourceKey();
        key.setGroupID(args[2]); key.setInstanceID(args[3]); key.setTypeID(args[4]);
        try (var fs = new FileStream(args[0], "r")) {
            var db = new DatabasePackedFile(); db.read(fs);
            var item = db.getItem(key);
            if (item == null) throw new IllegalStateException("Resource not found: " + key);
            try (var data = item.processFile(fs)) {
                Files.createDirectories(Path.of(args[1]).getParent());
                Files.write(Path.of(args[1]), data.toByteArray());
                System.out.println("Extracted " + key + " (" + data.length() + " bytes)");
            }
        }
    }
}
