import java.nio.file.*;
import java.util.*;
import sporemodder.MainApp;
import sporemodder.file.filestructures.FileStream;
import sporemodder.file.spui.*;
import sporemodder.file.spui.components.*;
import sporemodder.util.Vector2;

/**
 * Makes the two main Space communication panels appear in place immediately.
 * The input is kept as an external vanilla reference; only the output is changed.
 */
class PatchCommScreen {
    static SporeUserInterface read(Path path) throws Exception {
        var ui = new SporeUserInterface();
        try (var stream = new FileStream(path.toFile(), "r")) { ui.read(stream); }
        return ui;
    }

    static boolean isMainCommunicationPanel(WindowBase window) {
        return "0x05E4E5F0".equalsIgnoreCase(window.getControlID())
            || "0x05E4E5F8".equalsIgnoreCase(window.getControlID());
    }

    static List<SpuiElement> verticalMainPanelGlides(SporeUserInterface ui) {
        var result = new ArrayList<SpuiElement>();
        for (var element : ui.getElements()) {
            if (!(element instanceof WindowBase window) || !isMainCommunicationPanel(window)) continue;
            for (var proc : window.getWinProcs()) {
                if (!"Glide".equals(proc.getDesignerClass().getName())) continue;
                var offsetProperty = proc.getDesignerClass().getProperty("Offset");
                var offset = (Vector2) proc.getProperty(offsetProperty.getProxyID());
                if (offset != null && offset.getY() != 0.0f) result.add(proc);
            }
        }
        return result;
    }

    static void validateInstant(SporeUserInterface ui) {
        int mainPanelCount = 0;
        int instantGlideCount = 0;
        for (var element : ui.getElements()) {
            if (!(element instanceof WindowBase window) || !isMainCommunicationPanel(window)) continue;
            mainPanelCount++;
            for (var proc : window.getWinProcs()) {
                if (!"Glide".equals(proc.getDesignerClass().getName())) continue;
                var time = (Float) proc.getProperty(proc.getDesignerClass().getProperty("Time").getProxyID());
                var offset = (Vector2) proc.getProperty(proc.getDesignerClass().getProperty("Offset").getProxyID());
                if (time != null && time == 0.0f && offset != null
                        && offset.getX() == 0.0f && offset.getY() == 0.0f) instantGlideCount++;
            }
        }
        if (mainPanelCount != 2 || instantGlideCount != 2)
            throw new IllegalStateException("Expected two instant main communication panels; found panels="
                    + mainPanelCount + ", instant glides=" + instantGlideCount);
    }

    public static void main(String[] args) throws Exception {
        MainApp.testInit();
        SporeUserInterface.getDesigner().load();
        Path input = Path.of(args[0]);
        Path output = Path.of(args[1]);
        var ui = read(input);
        var glides = verticalMainPanelGlides(ui);
        if (glides.size() != 2) throw new IllegalStateException("Expected exactly two vertical main-panel glides; found " + glides.size());
        for (var glide : glides) {
            var type = glide.getDesignerClass();
            var oldTime = (Float) glide.getProperty(type.getProperty("Time").getProxyID());
            var oldOffset = (Vector2) glide.getProperty(type.getProperty("Offset").getProxyID());
            System.out.println("Neutralizing Glide: time=" + oldTime + ", offset=" + oldOffset);
            glide.setProperty(type.getProperty("Time").getProxyID(), 0.0f);
            glide.setProperty(type.getProperty("Offset").getProxyID(), new Vector2(0.0f, 0.0f));
        }
        Files.createDirectories(output.getParent());
        try (var stream = new FileStream(output.toFile(), "rw")) {
            stream.setLength(0);
            new SpuiWriter(ui.getRootWindows()).write(stream);
        }
        validateInstant(read(output));
        System.out.println("PASS patched SPUI reopened with two instantaneous main-panel glides");
    }
}
