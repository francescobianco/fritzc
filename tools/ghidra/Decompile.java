// Decompiles the functions at (or containing) the given addresses (SEG:OFF relative to the
// FRITZ2.UNP.EXE image, as used in docs/reverse-engineering.md) and prints C.
// Headless usage: -postScript Decompile.java 293b:9e44 0000:2512 ...
//@category Fritz

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class Decompile extends GhidraScript {
    @Override
    protected void run() throws Exception {
        // The MZ loader places the image at a base segment (1000 by default).
        long base = currentProgram.getMemory().getMinAddress().getOffset() >> 4;
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        for (String arg : getScriptArgs()) {
            // "@SEG:OFF" is an absolute Ghidra address (raw FRITZ2.BIN import).
            boolean absolute = arg.startsWith("@");
            String[] p = (absolute ? arg.substring(1) : arg).split(":");
            long seg = Long.parseLong(p[0], 16) + (absolute ? 0 : base);
            long off = Long.parseLong(p[1], 16);
            Address addr = toAddr(String.format("%04x:%04x", seg, off));
            Function f = getFunctionContaining(addr);
            if (f == null) {
                disassemble(addr);
                f = createFunction(addr, null);
            }
            println("==== " + arg + " (ghidra " + addr + ")"
                    + (f != null ? " in " + f.getEntryPoint() : ""));
            if (f == null) {
                println("no function");
                continue;
            }
            DecompileResults r = decomp.decompileFunction(f, 120, monitor);
            println(r.decompileCompleted() ? r.getDecompiledFunction().getC()
                                           : r.getErrorMessage());
        }
    }
}
