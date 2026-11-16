# P02 firmware update: möjliga tillägg

Bilaga till [P02 firmware update-flöde](flow_sv.md). Status **Option**: möjligt tillägg, om behovet finns. Inte planerade. Tas med om stakeholders ser ett behov.

English: [possible_additions_en.md](possible_additions_en.md)

## RAM load-varianten: firmware som inte får plats i den interna flashen (Option)

Utöka modulen med RAM load, för produkter vars firmware inte får plats i chipets interna flash. Slotarna ligger
då i extern flash (QSPI), och MCUboot kopierar imagen till RAM och kör den där. Så är det på Daisy (STM32H750:
128 KB intern flash, P02 ~133 KB).

* Kräver att vi först bestämmer vad test, confirm och revert betyder i RAM load. I dag har Daisyns RAM load
  ingen rollback (öppen punkt i AGENTS.md).
* Kräver att USB-felet efter mjuk reset på Daisy är löst, eftersom varje update slutar med en mjuk reset.
* Kan bli aktuellt för den andra produkten, om dess firmware är stor.

## Förbjudna versioner (Option)

En lista med gamla versioner som har kända fel och som det inte är tillåtet att göra en update till. P02
jämför versionen i headern med listan redan i första chunken (upload check-hooken) och nekar uppladdningen med
en felkod till klienten.

* Listan byggs in i firmwaren. Den gäller alltså bara från den version som har den, och en äldre version vet
  inte om senare tillägg.
* Skiljer sig från kontrollen "högre version än den som körs" (`upgrade`), som klienten väljer. Listan gäller
  alltid.
* Skyddar bara uppladdning genom P02. Flashning och en revert av MCUboot går förbi den.

## Kontroll av signaturen vid mottagning (Option)

P02 kontrollerar signaturen när hela imagen har kommit (status-hooken, `DFU_PENDING`), med samma publika nyckel
som MCUboot. En image med fel signatur upptäcks då innan omstarten, inte först när MCUboot vägrar starta den.
Reset-hooken kan avböja omstarten, och resultatet kan visas i `image state-read`.

* Förutsätter iterationen *Signed images* (i dag `BOOT_SIGNATURE_TYPE_NONE`).
* Samma kontroll görs två gånger. MCUboots kontroll vid start är fortfarande den som gäller.
* Kräver kryptokod i P02 (ECDSA P-256), som kostar flash. Särskilt viktigt på Nucleo med 200 KB slot.

## Rätt produkt, inte bara rätt kort (Option)

Kortkontrollen jämför bara board target. Två produkter på likadana kort, till exempel en vindrutetorkare och en
kabinvärmare, har samma board target, så en image för vindrutetorkaren tas emot och startas på kabinvärmaren.
Vill vi också kontrollera att det är rätt produkt?

* Ett sätt: ett produkt-ID i en egen TLV, på samma sätt som board target, jämfört med produkt-ID:t i firmwaren
  som körs. ID:t sätts av varje produkt (till exempel ett Kconfig-val i modulen,
  `CONFIG_FW_UPDATE_PRODUCT_ID`), och kontrollen görs i samma hook som kortkontrollen, med samma radering och
  rad på serieporten.
* Ett annat sätt: en signeringsnyckel per produkt, när *Signed images* finns. MCUboot vägrar då starta en image
  för en annan produkt, men först vid omstarten, inte när den tas emot.
* Att bestämma: om produkt och kort ska vara en kontroll (en TLV med båda) eller två, och hur ett kort
  avsiktligt byter produkt (flashning, som går förbi kontrollen).

## Övervakning av överföringen (Option)

Modulen mäter svarstiden per chunk: tiden från `smp_rx_req()` (main) till att mcumgr anropar `output()` med
svaret (mcumgrs tråd). Eftersom bara en chunk är på väg åt gången räcker en tidsstämpel.

* Modulen sparar relevant information så att den kan hämtas genom kommandot `status`.
* Ingen loggning under uppladdningen: då har `smpmgr` porten, så en rad skulle hamna hos klienten och inte hos
  en människa.
* Ett tillägg: döp om `restartRequested()` till ett uttryckligt `poll()`, som modulen kan använda för
  periodiskt arbete. Då kan modulen också upptäcka en förfrågan som aldrig får svar, och till exempel städa upp
  en halvfärdig uppladdning. Klienten får en timeout i det fallet ändå.
* Ger också mätningen till verifieringen på Nano i fas 2 i [flödet](flow_sv.md): hur mycket flashskrivningen stoppar trådarna.

## C- eller C++-gränssnitt mot modulen (Option)

Modulen har i dag ett C-API (se Bakgrund och beslutet *Zephyr-modul* i [flödet](flow_sv.md)). Ett C++-gränssnitt är ett möjligt
alternativ, om den andra produkten och P02 båda är skrivna i C++.

* För C: går att använda från både C och C++, vilket passar en produkt vars språk inte är känt än. mcumgrs
  callbacks och transport-API är C-funktionspekare, så det behövs ingen översättning. Modulen kan inte råka ta
  in P02:s klasser. I P02 anropas C-funktionerna bara inuti `boot::Zephyr` och `serial::Zephyr`, så `Logic`
  ser fortfarande bara `boot::Interface`.
* För C++: samma stil som P02 (klasser, `noexcept`, inga funktionspekare), och callbacken `tx` kan vara ett
  interface i stället för en funktionspekare med `void* user`.
* Kostnad med C++: varje produkt som använder modulen behöver C++ (`CONFIG_CPP`), som kostar flash.
* Ett mellanläge: C-API:t i modulen, och en tunn C++-header ovanpå för produkter i C++.
