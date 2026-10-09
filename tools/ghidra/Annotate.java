// Applies names and signatures from tools/ghidra/symbols.txt.
// Headless usage: -postScript Annotate.java /path/to/symbols.txt
//@category Fritz

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.ByteDataType;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.WordDataType;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Function.FunctionUpdateType;
import ghidra.program.model.listing.ParameterImpl;
import ghidra.program.model.listing.Variable;
import ghidra.program.model.symbol.SourceType;

public class Annotate extends GhidraScript {
    @Override
    protected void run() throws Exception {
        long base = currentProgram.getMemory().getMinAddress().getOffset() >> 4;
        int count = 0;
        for (String line : Files.readAllLines(Path.of(getScriptArgs()[0]))) {
            line = line.strip();
            if (line.isEmpty() || line.startsWith("#"))
                continue;
            String[] t = line.split("\\s+");
            String[] p = t[0].split(":");
            Address addr = toAddr(String.format("%04x:%s",
                    Long.parseLong(p[0], 16) + base, p[1]));
            Function f = getFunctionAt(addr);
            if (f == null) {
                disassemble(addr);
                f = createFunction(addr, t[1]);
            }
            if (f == null) {
                printerr("cannot create function at " + t[0]);
                continue;
            }
            f.setName(t[1], SourceType.USER_DEFINED);
            String conv = t[2];
            if (!conv.equals("keep")) {
                List<Variable> params = new ArrayList<>();
                for (int i = 3; i < t.length; i++) {
                    String[] np = t[i].split(":");
                    DataType dt = np.length > 1 && np[1].equals("byte")
                            ? ByteDataType.dataType : WordDataType.dataType;
                    params.add(new ParameterImpl(np[0], dt, currentProgram));
                }
                if (conv.equals("pascal"))
                    Collections.reverse(params);
                f.updateFunction(conv.equals("pascal") ? "__stdcall16far" : "__cdecl16far",
                        null, params, FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
                        true, SourceType.USER_DEFINED);
            }
            count++;
        }
        println("annotated " + count + " functions");
    }
}
