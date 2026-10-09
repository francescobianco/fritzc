# Fritz 2.5 Reborn
## Progetto di reverse engineering e porting nativo in C dello storico software scacchistico DOS

### 1. Visione del progetto

L'obiettivo è realizzare un porting nativo e multipiattaforma di **Fritz 2.5 / Fritz 2.51**, lo storico programma di scacchi per MS-DOS sviluppato negli anni Novanta da Frans Morsch e distribuito da ChessBase.

Il progetto intende recuperare e preservare il funzionamento del programma originale, ricostruendone in linguaggio C tanto il motore scacchistico quanto l'interfaccia utente.

Non si vuole creare un nuovo programma di scacchi semplicemente ispirato a Fritz, né distribuire un emulatore DOS contenente il programma originale.

Si vuole ottenere una nuova implementazione nativa che riproduca, nel modo più fedele possibile, l'esperienza del software storico.

Il risultato dovrà essere utilizzabile sui sistemi operativi moderni:

- Linux
- Windows
- macOS

Senza richiedere DOSBox, macchine virtuali o altri ambienti DOS.

La priorità è la **fedeltà storica e funzionale**, non il miglioramento del programma originale.

### 2. Filosofia di sviluppo

Il progetto adotta alcuni principi fondamentali.

**Fedeltà prima della modernizzazione**

Le caratteristiche originali devono essere comprese, documentate e possibilmente preservate prima di introdurre miglioramenti.

**Reverse engineering guidato dall'osservazione**

Ogni componente deve essere studiato attraverso l'analisi dei file originali, del codice macchina e del comportamento del programma in esecuzione.

**Implementazione comprensibile**

Il codice risultante deve essere scritto in C moderno, organizzato in moduli semplici e mantenibili.

Non è prioritario riprodurre la struttura interna originaria se questa dipende da limitazioni dell'architettura DOS.

È invece importante riprodurre i comportamenti osservabili e, soprattutto per il motore scacchistico, gli algoritmi e le decisioni originali.

**Assenza di emulazione nel prodotto finale**

DOSBox e gli strumenti di emulazione saranno utilizzati esclusivamente come strumenti di analisi, debugging e confronto.

**Separazione tra motore e interfaccia**

La logica scacchistica deve essere indipendente dall'interfaccia grafica.

Il motore dovrà poter essere compilato ed eseguito anche separatamente.

### 3. Ambito del reverse engineering

Il lavoro riguarda due grandi sottosistemi.

#### 3.1 Motore scacchistico

Dovranno essere analizzate e possibilmente ricostruite le seguenti componenti:

- Rappresentazione interna della scacchiera.
- Codifica dei pezzi e delle mosse.
- Generazione delle mosse pseudolegali e legali.
- Applicazione e annullamento delle mosse.
- Gestione di arrocco, en passant e promozione.
- Rilevamento di scacco, scacco matto e stallo.
- Regole di patta.
- Valutazione statica della posizione.
- Ricerca delle mosse.
- Ordinamento delle mosse.
- Eventuali tabelle di trasposizione.
- Gestione del tempo.
- Livelli di difficoltà.
- Gestione delle aperture.
- Eventuali euristiche o ottimizzazioni specifiche.

Non bisogna dare per scontato che Fritz utilizzi le stesse strutture dati o gli stessi algoritmi dei motori moderni.

Gli algoritmi devono essere identificati attraverso prove e analisi del binario.

L'obiettivo ideale è ottenere, a parità di posizione e condizioni di ricerca, lo stesso comportamento decisionale del motore originale.

#### 3.2 Interfaccia grafica

Il progetto comprende anche la ricostruzione dell'intera interfaccia utente.

Saranno oggetto di analisi:

- Schermata principale.
- Scacchiera.
- Set di pezzi.
- Palette cromatiche.
- Tipografia e caratteri.
- Menu.
- Sottomenu.
- Finestre di dialogo.
- Pulsanti e controlli.
- Gestione del mouse.
- Scorciatoie da tastiera.
- Orologi.
- Pannelli informativi.
- Notazione delle mosse.
- Visualizzazione delle analisi.
- Impostazioni e configurazione.
- Gestione delle partite e degli eventuali database.
- Messaggi e finestre di conferma.
- Comportamento delle finestre e delle interazioni.

Questo elenco rappresenta l'ambito di indagine: le funzionalità realmente presenti devono essere accertate nella versione originale.

Non basta riprodurre una schermata simile. È necessario ricostruire anche il comportamento dell'interfaccia.

### 4. Tecnologia proposta

Il linguaggio principale sarà **C11**.

Per l'interfaccia grafica si propone **SDL3**.

SDL3 verrà utilizzata come livello di astrazione multipiattaforma per:

- Creazione e gestione della finestra.
- Rendering grafico.
- Eventi del mouse.
- Eventi della tastiera.
- Gestione del tempo.
- Eventuale riproduzione audio.

L'interfaccia vera e propria sarà implementata in C, senza utilizzare framework di widget moderni.

Questo permette di preservare l'aspetto e il comportamento dell'interfaccia DOS originale, senza dover adattare i suoi controlli a quelli di GTK, Qt, Cocoa o Win32.

#### 4.1 Rendering a risoluzione virtuale

Una possibile strategia consiste nel creare una superficie grafica virtuale corrispondente alla risoluzione originale del programma.

Tutti gli elementi saranno disegnati in coordinate virtuali.

L'immagine finale verrà poi scalata alla dimensione della finestra moderna.

La risoluzione originale, la modalità video e le proporzioni dei pixel dovranno essere ricavate dall'analisi.

Dovranno essere considerate:

- Scala intera per preservare i pixel.
- Eventuale correzione del rapporto d'aspetto.
- Rendering nearest-neighbor.
- Modalità finestra.
- Modalità a schermo intero.
- Conversione delle coordinate del mouse tra finestra e superficie virtuale.

La modalità predefinita dovrà privilegiare la fedeltà visiva.

In futuro sarà possibile introdurre metodi di scaling alternativi senza alterare il rendering logico.

### 5. Reverse engineering del programma DOS

Il lavoro comincerà con un'analisi della distribuzione originale.

#### 5.1 Inventario dei file

Occorrerà identificare tutti i file presenti:

- Eseguibili.
- Librerie.
- File di configurazione.
- Database.
- Libri di apertura.
- Risorse grafiche.
- Font.
- File audio.
- Eventuali dati compressi o proprietari.

Per ogni file saranno registrati nome, dimensione, hash, struttura e possibile funzione.

Non si assume che il motore sia contenuto in un eseguibile separato dall'interfaccia: questa separazione dovrà essere verificata.

#### 5.2 Analisi dell'eseguibile

Si dovranno identificare:

- Formato dell'eseguibile DOS.
- Architettura di esecuzione.
- Codice a 16 o 32 bit.
- Eventuali DOS extender.
- Segmentazione e modello di memoria.
- Importazioni e chiamate di sistema.
- Routine di inizializzazione.
- Eventuali sistemi di compressione o packing.

Gli strumenti principali saranno:

**Ghidra**

Per disassemblaggio, analisi delle funzioni, riferimenti incrociati e ricostruzione delle strutture dati.

**DOSBox con supporto al debugging**

Per osservare l'esecuzione del programma, tracciare il codice e individuare le routine associate alle operazioni dell'utente.

**Python**

Per analizzare formati binari, individuare tabelle, elaborare dump di memoria e sviluppare strumenti di confronto.

**Toolchain C**

Per implementare e verificare progressivamente i componenti recuperati.

L'identificazione corretta dell'ambiente di esecuzione è una fase preliminare indispensabile.

### 6. Reverse engineering dell'interfaccia

Per l'interfaccia si adotta un approccio principalmente comportamentale.

Non sarà necessario ricostruire istruzione per istruzione il codice originale delle routine grafiche.

Si dovranno invece documentare e riprodurre gli stati visivi e le transizioni causate dalle azioni dell'utente.

#### 6.1 Catalogazione delle schermate

Il programma originale sarà avviato in DOSBox.

Ogni schermata verrà documentata attraverso screenshot e descrizioni.

Per ogni elemento si registreranno:

- Posizione.
- Dimensioni.
- Colori.
- Font.
- Aspetto grafico.
- Stato iniziale.
- Interazioni disponibili.
- Eventi generati.
- Effetti sugli altri componenti.

Dovrà essere predisposto un catalogo completo dei menu e delle finestre.

#### 6.2 Ricostruzione dei controlli

Si svilupperà una piccola libreria di widget dedicata.

Questa potrà includere:

- Menu.
- Menu a cascata.
- Pulsanti.
- Finestre di dialogo.
- Caselle di testo.
- Liste.
- Barre di scorrimento.
- Pannelli.
- Etichette.
- Controlli scacchistici specifici.

La libreria non dovrà essere generalizzata oltre quanto necessario.

Lo scopo è riprodurre Fritz, non realizzare un framework GUI universale.

#### 6.3 Asset grafici

Occorrerà stabilire se i pezzi, i font e gli elementi dell'interfaccia siano contenuti in file esterni, incorporati nell'eseguibile oppure generati proceduralmente.

Nel caso di formati proprietari si potranno realizzare decoder dedicati.

Nel caso di disegno procedurale si potranno ricostruire le primitive grafiche equivalenti.

L'uso e la redistribuzione delle risorse originali dovranno essere valutati separatamente dal loro studio tecnico, rispettando i diritti applicabili.

### 7. Reverse engineering del motore

Il motore richiede un processo differente.

In questo caso non basta ottenere un programma capace di giocare a scacchi.

È importante comprendere il funzionamento del motore originale.

#### 7.1 Identificazione delle strutture dati

Il primo obiettivo sarà localizzare e interpretare la rappresentazione della posizione.

Si cercheranno:

- Array della scacchiera.
- Codifica dei pezzi.
- Informazioni sul tratto.
- Diritti di arrocco.
- Stato en passant.
- Stack delle mosse.
- Strutture per la ricerca.

Le informazioni recuperate saranno documentate prima di procedere con una riscrittura.

#### 7.2 Identificazione della ricerca

Occorrerà ricostruire il flusso di esecuzione che conduce dalla posizione corrente alla scelta della mossa.

Verranno studiate le possibili routine di:

- Generazione mosse.
- Valutazione.
- Ricerca ricorsiva.
- Potatura.
- Quiescenza.
- Ordinamento delle mosse.
- Controllo del tempo.
- Interruzione della ricerca.

Non si presuppone alcuna specifica implementazione di alpha-beta, negamax o altre tecniche finché non sarà confermata.

#### 7.3 Riscrittura in C

Le funzioni recuperate saranno progressivamente reimplementate in C.

Una possibile API moderna potrebbe essere:

```c
typedef struct Position Position;
typedef struct Move Move;

void generate_moves(
    const Position *position,
    Move *moves,
    int *count
);

void make_move(
    Position *position,
    Move move
);

void unmake_move(
    Position *position,
    Move move
);

int evaluate(
    const Position *position
);

Move find_best_move(
    Position *position,
    int depth
);
```

Le firme sono indicative e non rappresentano strutture recuperate da Fritz.

L'architettura definitiva dovrà riflettere quanto emerso dall'analisi.

La riscrittura deve privilegiare equivalenza algoritmica e chiarezza.

Eventuali ottimizzazioni saranno introdotte solo dopo aver ottenuto un comportamento verificabile.

### 8. Verifica della fedeltà

Il software DOS originale rappresenterà il riferimento comportamentale.

Il porting nativo dovrà essere sottoposto a test di confronto.

#### 8.1 Test del motore

Le due versioni riceveranno le stesse posizioni.

Quando disponibili, verranno confrontati:

- Mosse generate.
- Mosse selezionate.
- Valutazioni.
- Varianti principali.
- Profondità raggiunta.
- Reazioni a condizioni particolari.
- Risultati in posizioni tattiche.
- Comportamento nei finali.

Per ottenere confronti significativi sarà necessario controllare le condizioni di ricerca.

Il tempo di esecuzione su processori moderni non corrisponde a quello del software originale.

Pertanto sarà preferibile confrontare profondità, nodi o altri criteri deterministici, quando ricostruibili, anziché basarsi unicamente sul tempo.

#### 8.2 Test dell'interfaccia

Ogni schermata ricostruita verrà confrontata visivamente con uno screenshot della versione DOS.

Verranno analizzate:

- Coordinate.
- Proporzioni.
- Palette.
- Disposizione dei componenti.
- Risposte agli input.
- Sequenze di navigazione.
- Transizioni tra stati.

Si potranno sviluppare test automatici che confrontino immagini e sequenze di eventi.

#### 8.3 Test funzionali

Dovranno essere riprodotti scenari completi:

- Avvio del programma.
- Avvio di una nuova partita.
- Inserimento delle mosse.
- Partita contro il computer.
- Navigazione dei menu.
- Modifica delle opzioni.
- Caricamento di una partita.
- Salvataggio.
- Chiusura del programma.

Le funzionalità effettivamente disponibili saranno determinate dall'analisi della versione originale.

### 9. Architettura del progetto

Si propone una struttura modulare, con un nucleo scacchistico indipendente dall'ambiente grafico.

```text
fritz-reborn/
|
+-- CMakeLists.txt
+-- README.md
+-- LICENSE
|
+-- docs/
|   +-- architecture.md
|   +-- reverse-engineering.md
|   +-- engine-analysis.md
|   +-- ui-analysis.md
|   +-- compatibility.md
|
+-- src/
|   |
|   +-- main.c
|   |
|   +-- engine/
|   |   +-- position.c
|   |   +-- movegen.c
|   |   +-- search.c
|   |   +-- evaluate.c
|   |   +-- timecontrol.c
|   |   +-- opening.c
|   |
|   +-- gui/
|   |   +-- screen.c
|   |   +-- renderer.c
|   |   +-- palette.c
|   |   +-- fonts.c
|   |   +-- widgets.c
|   |   +-- menus.c
|   |   +-- dialogs.c
|   |   +-- chessboard.c
|   |   +-- panels.c
|   |
|   +-- input/
|   |   +-- mouse.c
|   |   +-- keyboard.c
|   |
|   +-- game/
|   |   +-- controller.c
|   |   +-- notation.c
|   |   +-- history.c
|   |   +-- persistence.c
|   |
|   +-- platform/
|       +-- sdl.c
|
+-- include/
|
+-- assets/
|
+-- tools/
|   +-- extract/
|   +-- compare/
|   +-- inspect/
|
+-- tests/
|   +-- engine/
|   +-- gui/
|   +-- integration/
|
+-- reference/
|   +-- README.md
|
+-- cmake/
```

Questa struttura è preliminare.

La directory `reference` sarà utilizzata per documentare come reperire ed eseguire la versione originale e per conservare eventuali manifest e risultati dell'analisi. I binari e gli asset originali non dovranno essere inclusi automaticamente nel repository pubblico.

### 10. Interazione tra GUI e motore

L'interfaccia deve comunicare con il motore attraverso un insieme limitato di operazioni.

Il controller dell'applicazione mantiene lo stato della partita e coordina i due sottosistemi.

L'interfaccia produce eventi:

```text
Mouse click
    |
    v
GUI
    |
    v
Game Controller
    |
    v
Chess Engine
    |
    v
Game State
    |
    v
GUI Rendering
```

L'architettura dovrà impedire che il ciclo di ricerca scacchistica blocchi la gestione degli eventi grafici.

La prima implementazione potrà utilizzare una ricerca cooperativa o un worker separato, a seconda delle caratteristiche della logica ricostruita.

In ogni caso, la sincronizzazione tra ricerca e stato della partita deve essere esplicita.

### 11. Strategia di sviluppo

Non si tenterà di convertire automaticamente l'intero eseguibile DOS in C.

Il progetto sarà sviluppato per incrementi verificabili.

#### Fase 0 — Acquisizione e identificazione

- Reperire una distribuzione originale di Fritz 2.5 o 2.51.
- Identificare esattamente la versione.
- Calcolare gli hash dei file.
- Individuare i componenti.
- Verificare il formato dell'eseguibile.
- Stabilire come eseguirlo correttamente.

**Risultato:** inventario tecnico completo della distribuzione.

#### Fase 1 — Ambiente di riferimento

- Configurare DOSBox.
- Verificare il funzionamento del programma.
- Documentare le impostazioni dell'ambiente.
- Registrare schermate e interazioni.
- Preparare posizioni di prova riproducibili.

**Risultato:** ambiente di riferimento per il confronto con il porting.

#### Fase 2 — Analisi dell'interfaccia

- Catalogare le schermate.
- Esplorare tutti i menu.
- Identificare gli elementi grafici.
- Studiare il sistema di coordinate.
- Analizzare gli input.
- Individuare eventuali risorse grafiche.

**Risultato:** specifica comportamentale e visiva della GUI.

#### Fase 3 — Prototipo grafico SDL3

- Creare una finestra nativa.
- Implementare la superficie virtuale.
- Riprodurre la schermata principale.
- Disegnare la scacchiera.
- Riprodurre i pezzi.
- Implementare mouse e tastiera.
- Ricostruire i primi menu.

**Risultato:** prima replica navigabile dell'interfaccia.

#### Fase 4 — Analisi del motore

- Individuare le strutture dati.
- Localizzare la generazione delle mosse.
- Identificare le routine di ricerca.
- Individuare la valutazione.
- Studiare i livelli di gioco.
- Documentare le dipendenze tra funzioni.

**Risultato:** mappa tecnica del motore originale.

#### Fase 5 — Implementazione del motore in C

- Ricostruire la posizione.
- Implementare la generazione delle mosse.
- Implementare applicazione e annullamento.
- Ricostruire la valutazione.
- Ricostruire la ricerca.
- Integrare le eventuali euristiche recuperate.

**Risultato:** motore C indipendente dalla GUI.

#### Fase 6 — Integrazione

- Collegare il motore all'interfaccia.
- Implementare il controller di partita.
- Gestire le mosse dell'utente.
- Gestire le mosse del computer.
- Riprodurre orologi e indicatori.
- Integrare caricamento e salvataggio.

**Risultato:** prima versione nativa utilizzabile.

#### Fase 7 — Verifica e compatibilità

- Confrontare gli algoritmi.
- Confrontare le schermate.
- Eseguire test di regressione.
- Correggere divergenze.
- Verificare la compatibilità delle risorse.
- Testare Linux, Windows e macOS.

**Risultato:** porting validato rispetto al comportamento originale.

### 12. Interfaccia UCI opzionale

Una volta ricostruito il motore, sarà possibile sviluppare un adattatore UCI.

Questo permetterebbe di utilizzare il motore storico con altri programmi di scacchi, tra cui Pragma Chess.

L'interfaccia UCI non è necessaria per la replica originale e non deve influenzarne l'architettura grafica.

Sarà considerata una funzionalità aggiuntiva, implementabile dopo la ricostruzione del motore.

### 13. Evoluzioni possibili

Dopo aver raggiunto una replica soddisfacente potranno essere valutate alcune estensioni:

- Modalità finestra ridimensionabile.
- Scaling grafico configurabile.
- Supporto a schermi HiDPI.
- Esportazione PGN.
- Interfaccia UCI.
- Supporto a formati moderni.
- Integrazione con database esterni.
- Modalità di analisi del motore.
- Strumenti per confrontare il motore originale con motori moderni.

Queste funzionalità dovranno rimanere opzionali e non compromettere l'esperienza originale.

### 14. Questioni legali e distribuzione

Il progetto deve distinguere chiaramente tra reverse engineering, studio degli algoritmi, ricostruzione funzionale e distribuzione di materiali derivati.

Il fatto che una vecchia versione di Fritz sia reperibile gratuitamente non implica che il suo codice, i suoi asset o il software siano di pubblico dominio.

Le possibilità di decompilazione, interoperabilità e utilizzo dei risultati dipendono dalla normativa e dalle condizioni applicabili.

Si dovrà pertanto:

- Verificare lo stato dei diritti della versione originale.
- Evitare la pubblicazione non autorizzata degli eseguibili originali.
- Verificare la possibilità di redistribuire grafica, font e altre risorse.
- Documentare la provenienza delle risorse.
- Distinguere codice originale studiato e nuova implementazione.
- Considerare un'implementazione indipendente del comportamento quando necessario.
- Valutare un'autorizzazione specifica prima della distribuzione di materiali protetti o codice derivato.

La scelta della licenza open source potrà essere effettuata soltanto dopo aver chiarito quali componenti possono essere legittimamente distribuiti.

### 15. Principali rischi tecnici

**Codice DOS complesso**

L'eseguibile potrebbe utilizzare segmentazione, DOS extender, assembler ottimizzato o formati compressi.

**Motore difficile da isolare**

Le funzioni scacchistiche potrebbero condividere strutture e stato globale con l'interfaccia.

**Asset non recuperabili direttamente**

Font e grafica potrebbero essere compressi, incorporati o prodotti da routine proprietarie.

**Comportamento non deterministico**

Il motore potrebbe dipendere dal tempo, dalla macchina, dallo stato di memoria o da euristiche che rendono difficile un confronto esatto.

**Fedeltà visiva**

La riproduzione corretta della modalità video DOS potrebbe richiedere attenzione alle palette, alle proporzioni dei pixel e alle caratteristiche del rendering originale.

**Dimensione del lavoro**

La ricostruzione completa potrebbe essere significativamente più impegnativa della realizzazione di una nuova applicazione scacchistica.

Per questo motivo ogni fase dovrà produrre risultati autonomamente verificabili.

### 16. Primo esperimento concreto

Prima di procedere alla ricostruzione completa, sarà realizzata una prova di fattibilità.

Si partirà dalla distribuzione originale.

Gli obiettivi saranno:

1. Identificare il formato dell'eseguibile e il suo ambiente di esecuzione.
2. Verificare l'avvio in DOSBox.
3. Catturare la schermata principale.
4. Identificare modalità video e risoluzione.
5. Analizzare i file contenenti eventuali risorse grafiche.
6. Riprodurre la schermata principale attraverso SDL3.
7. Individuare nel binario almeno una funzione riconducibile al motore scacchistico.
8. Ricostruire e verificare una prima funzione in C.

Questa prova dovrà dimostrare la fattibilità sia della replica grafica sia del recupero algoritmico.

### 17. Definizione del risultato finale

Il progetto si potrà considerare riuscito quando sarà possibile:

- Avviare Fritz nativamente su un sistema operativo moderno.
- Visualizzare un'interfaccia sostanzialmente fedele all'originale.
- Navigare i menu e utilizzare i controlli.
- Giocare una partita completa.
- Far eseguire al computer mosse attraverso il motore ricostruito.
- Ottenere un comportamento scacchistico coerente con la versione DOS, nei limiti documentati.
- Utilizzare le principali funzionalità originali.
- Compilare il progetto con una toolchain C moderna.
- Eseguire il programma senza emulatori DOS.

### 18. Principio conclusivo

**Fritz 2.5 Reborn non deve essere un clone esteticamente somigliante a Fritz. Deve essere un tentativo documentato di ricostruzione del programma originale.**

Il valore del progetto è duplice.

Da una parte, preservare un'esperienza software storica, rendendola nuovamente accessibile sui sistemi moderni.

Dall'altra, studiare e recuperare le tecniche di programmazione e di intelligenza artificiale scacchistica utilizzate negli anni Novanta.

L'ambizione è recuperare il più possibile del comportamento originale e restituirlo in una codebase C piccola, comprensibile, verificabile e portabile.

La prima attività operativa consiste nell'acquisire i file originali di Fritz 2.5 / 2.51 e analizzarne la struttura, senza assumere preventivamente nulla sulla loro organizzazione interna.