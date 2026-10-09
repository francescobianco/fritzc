# Reverse engineering — Fase 0: acquisizione e identificazione

Stato dell'analisi della distribuzione presente in `dos/fritz251/`
(non inclusa nel repository, vedi `.gitignore` e IDEA.md §14).
Gli hash SHA-256 di tutti i file sono in `reference/fritz251.sha256`.

Legenda: **[verificato]** = confermato da prove dirette;
**[ipotesi]** = da confermare con analisi successive.

## 1. Identificazione della versione

La distribuzione è **ChessBase Fritz 2.51** per MS-DOS. **[verificato]**

Prove:

1. **Finestra "About Fritz" a runtime.** Avviando `FRITZ2.EXE` in DOSBox,
   il menu `Fritz=F1 → About Fritz` mostra `ChessBase Fritz 2.51` e i crediti
   *Mathias Feist, Frans Morsch and Matthias Wüllenweber*.
2. **Stringa nel binario.** Nell'eseguibile decompresso, all'offset immagine
   `0x3e0e9`, c'è la stringa con prefisso di lunghezza (formato Pascal, `0x14`
   = 20 byte) `ChessBase Fritz 2.51`, seguita dai nomi degli autori. È la
   stringa letta dalla finestra About, non un'etichetta aggiunta in seguito.
3. **Coerenza della distribuzione.** Tutti i file del programma hanno lo stesso
   timestamp `1993-08-01 05:34:56`, tipico di una distribuzione masterizzata
   in un'unica volta. Fa eccezione `FRITZ2.DAT` (`1993-08-01 11:45:06`, stesso
   giorno). I libri in `BOOKS/` sono del 23-25/11/1992, quindi precedenti e
   probabilmente condivisi con versioni o prodotti ChessBase anteriori.
4. **Nome dei file.** Il prefisso `FRITZ2.*` indica il prodotto "Fritz 2";
   2.51 è il livello di revisione. Il nome della cartella `fritz251` è quindi
   coerente con il contenuto. Il programma non dipende dal nome della
   directory: legge i file dalla directory corrente.

Unico file modificato dopo la distribuzione: **`FRITZ2.PAR`**
(timestamp `2001-02-17`). È il file delle impostazioni utente, che Fritz
riscrive. Contiene percorsi come `C:\TEMP` e `C:FRITZ2.CBF`, quindi la copia
proviene da un'installazione usata, non dai dischetti originali. Con questo
file presente, il programma salta la scelta della lingua e della scheda
grafica. Per riprodurre il primo avvio "pulito" bisognerà rimuoverlo
(nella copia di lavoro, mai nell'originale).

Non è stato trovato un elenco pubblico di hash di riferimento per confrontare
la distribuzione con una copia nota. Gli hash locali sono comunque registrati.

## 2. Inventario dei file

| File | Dimensione | Funzione |
|---|---:|---|
| `FRITZ2.EXE` | 244 858 | Eseguibile principale (MZ, compresso con EXEPACK) **[verificato]** |
| `FRITZ2.BIN` | 35 877 | Modulo binario caricato a runtime. Inizia con `JMP` e ha uno stack proprio. Potrebbe essere il motore scacchistico separato dall'interfaccia **[ipotesi]** |
| `FRITZ2.DAT` | 888 406 | Campioni audio a 8 bit non segnati (intorno a `0x80`), blocchi con firma `SOL` **[ipotesi: suoni/voce]** |
| `FRITZ2.IMG` | 1 512 | Bitmap compressa, probabilmente il logo "fritz 2" **[ipotesi]** |
| `FRITZ2.PAR` | 759 | Impostazioni utente (lingua, percorsi, database) **[verificato]** |
| `FENGLISH.LAN`, `FDEUTSCH.LAN` | 10 445 / 11 334 | Testi dell'interfaccia (menu, messaggi) in inglese e tedesco **[verificato]** |
| `FENGLISH.HLP`, `FDEUTSCH.HLP` | 8 871 / 9 006 | Testi dell'aiuto in linea **[verificato]** |
| `*.CBF`, `*.CBI`, `*.CKO`, `*.CPO`, `*.CPA` | — | Database in formato ChessBase (partite, indice, chiavi, posizioni). Set: `FRITZ2`, `C45_C47`, `TAKTIK` **[ipotesi sul ruolo di ciascuna estensione]** |
| `FRITZ2.FBK`, `BOOKS/*.FBK` | — | Libri di apertura Fritz. `FRITZ2.FBK` è identico a `BOOKS/A00-E99.FBK` **[verificato: stesso hash]** |
| `BOOKS/A00-E99.CKB`, `.CPB` | — | Libro di apertura in formato ChessBase **[ipotesi]** |

Altri nomi di file citati nell'eseguibile: `MAINBASE.CBF`, `EXPORT.CBF`,
`EXPORT.DOC`, `IMPORT.CKO`, `DIAGRAMM.DOC`. Ci sono anche i pattern
`*.CFG`, `*.VFG`, `*.EFG`, `*.HFG`, probabilmente configurazioni colori per
VGA, EGA ed Hercules.

## 3. Formato dell'eseguibile

- **Formato:** MZ (DOS real mode, 16 bit), compresso con **Microsoft EXEPACK**.
  Il file ha zero rilocazioni nell'header e lo stub di decompressione è in
  fondo, con il messaggio `Packed file is corrupt`. **[verificato]**
- **Decompressione:** `tools/inspect/unexepack.py`. Produce un MZ standard di
  254 880 byte con 2 468 rilocazioni, tutte con destinazione valida.
  Entry point originale `0000:8C0C`, stack `3F7D:4000`. **[verificato]**
- **Compilatore:** Microsoft C (`MS Run-Time Library - Copyright (c) 1988,
  Microsoft Corp`, messaggi `R6xxx` del runtime MSC). Nessun DOS extender.
  **[verificato]**
- **Segmenti referenziati dalle rilocazioni:** `0000`, `0A1F`, `19E7`, `293B`,
  `344D`, `3464`. Modello di memoria con più segmenti di codice (medium o
  large). **[verificato per i segmenti; modello da confermare]**
- **Grafica:** usa la libreria grafica di Microsoft C (`_setvideomode`, con
  supporto Hercules a basso livello sulle porte `3B4h/3B8h/3BFh`). Lo
  switch da riga di comando include `/X`, `/EMS`, `/VGA`, … **[verificato]**

## 4. Ambiente di esecuzione e modalità video

- Funziona in DOSBox 0.74-3 (`machine=svga_s3`, `cycles=20000`).
  Configurazione: `reference/dosbox.conf`. **[verificato]**
- Scelta della scheda grafica supportata: VGA, EGA, Hercules, CGA.
- Con la scheda VGA la modalità è **640×480, 16 colori** (palette EGA/VGA
  standard, VGA mode 12h). La schermata principale usa solo 4 colori:
  nero, bianco, blu scuro e azzurro. I pixel sono quadrati, quindi non serve
  correggere il rapporto d'aspetto. **[verificato con screenshot nativo DOSBox]**

## 5. Metodo

Il porting si ottiene **traducendo in C il codice e i dati del binario**,
non imitando gli screenshot. Ogni funzione C cita l'indirizzo `SEG:OFF`
della routine originale (segmenti dell'immagine decompressa; DGROUP = `3464`).

DOSBox serve a due scopi:

1. **Esperimenti con patch.** Si modifica `FRITZ2.UNP.EXE` con
   `tools/compare/patch.py`, lo si esegue con `FRITZ_EXE=… dosbox-run.sh`
   e si verifica se l'effetto osservato conferma l'ipotesi. Ogni
   esperimento va annotato nel registro (§7).
2. **Riferimento per i test.** Gli screenshot nativi si confrontano con il
   frame prodotto dal porting (`fritz --dump`).

L'EXE decompresso è equivalente all'originale: la schermata iniziale
coincide, salvo l'indicatore di memoria libera ("105 kB" contro "101 kB"),
che dipende dalla dimensione del file.

## 6. Libreria grafica (segmento `293b`)

Libreria propria, non quella di Microsoft C. Ha funzioni far in convenzione
Pascal (`retf N`) e driver per scheda selezionati tramite le tabelle
`DS:6FA1…6FDD`, indicizzate da `[DS:6F58]`. **[verificato]**

- **Coordinate logiche 720×348.** L'interfaccia ragiona nella geometria
  Hercules e converte con `293b:aa5f` (x) e `293b:aa75` (y):
  `x' = (x·[6F48] + [6F4C]) / [6F4A]` e `y' = (y·[6F50] + [6F54]) / [6F52]`.
  La somma avviene senza propagare il riporto. I parametri sono impostati
  da `293b:aa8b`, chiamata da `0000:814b`. **[verificato]**
- **Parametri per scheda.** `0000:7eaf` copia i dati da `DS:7478`.
  Per VGA (`DS:7478`) sono `16/18 +9` e `48/35 +17`, quindi
  720×348 → 640×477: le righe 477–479 dello schermo restano nere. Per
  EGA (`DS:74AE`) la scala è `16/18 +9` e `1/1`, quindi 640×348. **[verificato]**
- **`293b:9e44` FillPattern(x, y, w, h, colore, retino, rop).** Coordinate
  dispositivo. La maschera di riga è `retino[y & 7]` e il bit 7 è il pixel
  con `x & 7 == 0`. I bit a 0 lasciano il pixel invariato. `rop` è il
  valore del registro VGA Function Select (`0` copia, `08h` AND, `10h` OR,
  `18h` XOR). Il driver VGA è `293b:9ec4` → `293b:9b2c` (write mode 0 con
  set/reset). Tradotta in `src/gui/gfx.c`. **[verificato]**
- **Retini** `DS:7023` → 8 maschere 8×8 a `DS:7033…706A`; il n. 1
  (`88 22…`) è lo sfondo del desktop. **[verificato con patch]**

## 7. Registro degli esperimenti

| # | Ipotesi | Patch | Risultato |
|---|---|---|---|
| 1 | L'EXE decompresso è equivalente all'originale | nessuna (`FRITZ_EXE=FRITZ2.UNP.EXE`) | Confermata: schermata identica salvo l'indicatore di memoria |
| 2 | `DS:703B` è il retino del desktop | `DS:703b=ff00ff00ff00ff00` | Confermata: lo sfondo diventa a righe orizzontali blu e bianche |

## 8. Funzioni ricostruite

| Originale | C | Stato |
|---|---|---|
| `293b:aa5f`, `293b:aa75`, `293b:aa8b` | `gfx_scale_x/y`, `gfx_set_scale` | completo |
| `293b:9e44` (solo driver VGA) | `gfx_fill_pattern` | completo per VGA |
| `0000:2512` fino a `0000:259d` | `desktop_draw` | parziale: colori `[AD6E]`, `[0F4E]`, `[B0C6]` presi dall'osservazione, origine da ricostruire |

Verifica: `fritz --dump` coincide pixel per pixel con lo screenshot DOSBox
in tutte le zone dove il desktop è visibile.

## 9. Strumenti

| Strumento | Uso |
|---|---|
| `tools/inspect/unexepack.py IN OUT` | Decomprime un EXE EXEPACK |
| `tools/inspect/xkeys.py TASTI…` | Invia tasti via XTest alla finestra attiva (DOSBox) |
| `tools/inspect/dis.py EXE SEG:OFF [N]` | Disassembla N istruzioni a un indirizzo |
| `tools/compare/patch.py IN OUT SEG:OFF=HEX…` | Applica patch all'EXE decompresso (per esperimenti) |
| `tools/compare/dosbox-run.sh OUTDIR SEC TASTI…` | Avvia Fritz su una copia temporanea e raccoglie gli screenshot (`ctrl+F5`); `FRITZ_EXE` sostituisce l'eseguibile |

Esempio, cattura della finestra About:

```sh
tools/compare/dosbox-run.sh build/captures 15 F1 Up Return sleep:1.5 ctrl+F5
```

Nota: sotto Wayland DOSBox gira tramite XWayland. XTest funziona finché
la finestra di DOSBox ha il focus.

## 10. Prossimi passi

1. Continuare `0000:2512` da `call 0000:05f3`: cornici, pannelli, barra dei menu.
2. Trovare l'origine dei colori `[AD6E]`, `[0F4E]`, `[B0C6]`
   (probabilmente da `FRITZ2.PAR` o da una tabella di default).
3. Importare `build/re/FRITZ2.UNP.EXE` in Ghidra (scaricato in
   `build/deps/`), con nomi e commenti delle funzioni già identificate.
4. Verificare se `FRITZ2.BIN` è il motore: tracciarne il caricamento da
   `FRITZ2.EXE` e il punto d'ingresso (`JMP 0x2604`).
5. Decodificare `FRITZ2.IMG` (logo) e localizzare nell'EXE le bitmap dei pezzi
   e i font 8×14 (sono visibili nei dati non compressi).
6. Catalogare tutti i menu (F1–F6) con `dosbox-run.sh` (IDEA.md §6.1).
