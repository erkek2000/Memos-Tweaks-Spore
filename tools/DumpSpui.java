import java.lang.reflect.*;
import java.util.*;
import sporemodder.MainApp;
import sporemodder.file.filestructures.FileStream;
import sporemodder.file.spui.*;
import sporemodder.file.spui.components.*;
import sporemodder.file.spui.uidesigner.*;

/** Diagnostic SPUI structure dump for comparing known UI layouts. */
class DumpSpui {
    @SuppressWarnings("unchecked")
    public static void main(String[] args) throws Exception {
        MainApp.testInit();
        SporeUserInterface.getDesigner().load();
        var ui = new SporeUserInterface();
        try (var fs = new FileStream(args[0], "r")) { ui.read(fs); }
        Field valuesField = SpuiElement.class.getDeclaredField("properties");
        valuesField.setAccessible(true);
        var indexes = new IdentityHashMap<Object,Integer>();
        for (int i = 0; i < ui.getElements().size(); i++) indexes.put(ui.getElements().get(i), i);
        for (int i = 0; i < ui.getElements().size(); i++) {
            var e = ui.getElements().get(i);
            String window = e instanceof WindowBase w
                ? " control=" + w.getControlID() + " command=" + w.getCommandID() + " parent=" + indexes.get(w.getParent())
                    + " procs=" + w.getWinProcs().stream().map(indexes::get).toList() + " area=" + w.getArea()
                : "";
            System.out.println(i + " " + e.getDesignerClass().getName() + window);
            var map = (Map<Integer,Object>) valuesField.get(e);
            for (var v : map.entrySet()) {
                var property = e.getDesignerClass().getProperty(v.getKey());
                System.out.println("  " + (property == null ? String.format("0x%08X", v.getKey()) : property.getName())
                    + " = " + String.valueOf(v.getValue()));
            }
        }
    }
}
