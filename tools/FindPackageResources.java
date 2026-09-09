import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;
import sporemodder.HashManager;
import sporemodder.MainApp;
import sporemodder.file.dbpf.DatabasePackedFile;
import sporemodder.file.filestructures.FileStream;
import sporemodder.file.prop.PropertyList;

/** Read-only DBPF index lookup for named resource instances. */
class FindPackageResources {
    public static void main(String[] args) throws Exception {
        if (args.length < 2) throw new IllegalArgumentException("package resource-name...");
        MainApp.testInit();
        var hashes = HashManager.get();
        Set<Integer> wanted = new HashSet<>();
        for (int i = 1; i < args.length; i++) wanted.add(hashes.getFileHash(args[i]));

        try (var stream = new FileStream(Path.of(args[0]).toFile(), "r")) {
            var dbpf = new DatabasePackedFile();
            dbpf.read(stream);
            for (var item : dbpf.index.items) {
                if (!wanted.contains(item.name.getInstanceID())) continue;
                System.out.printf("%s  instance=%s group=%s type=%s mem=%d disk=%d%n",
                    hashes.getFileName(item.name.getInstanceID()),
                    hashes.hexToString(item.name.getInstanceID()),
                    hashes.getFileName(item.name.getGroupID()),
                    hashes.getFileName(item.name.getTypeID()),
                    item.memSize, item.compressedSize);
                if (item.name.getTypeID() == hashes.getTypeHash("prop")) {
                    try (var data = item.processFile(stream)) {
                        var properties = new PropertyList();
                        properties.read(data);
                        System.out.println(properties.toArgScript());
                    }
                }
            }
        }
    }
}
