Úloha MRO: Minimální hranový řez hranově ohodnoceného grafu
Vstupní data:
n = přirozené číslo představující počet uzlů grafu G, 150 > n >= 10
k = přirozené číslo představující průměrný stupeň uzlu grafu G, 3n/4 > k >= 5
G(V,E) = jednoduchý souvislý neorientovaný hranově ohodnocený graf o n uzlech a průměrném stupni k, váhy hran jsou z intervalu <80,120>
a = přirozené číslo, 5 =< a =< n/2

Úkol:
Nalezněte rozdělení množiny uzlů V do dvou disjunktních podmnožin X, Y tak, že množina X obsahuje a uzlů, množina Y obsahuje n-a uzlů a součet ohodnocení všech hran {u,v} takových, že u je z X a v je z Y (čili velikost hranového řezu mezi X a Y), je minimální.

Výstup algoritmu:
Výpis disjuktních podmnožin uzlů X a Y a ohodnocených hran minimálního řezu mezi nimi a celková váha tohoto minimálního řezu.

Sekvenční algoritmus:
Řešení existuje vždy, vždy lze sestrojit hranový řez grafu mezi disjunktními podmnožinami vrcholů X a Y.
Sekvenční algoritmus je typu BB-DFS s hloubkou prohledávaného prostoru a.
Přípustný mezistav je definovaný rozdělením množiny uzlů na dvě disjunktní podmnožiny X a Y.
Přípustná koncová řešení jsou všechna zkonstruovaná rozdělení množiny uzlů grafu G na podmnožiny X a Y o velikosti a a n-a.
Cena, kterou minimalizujeme, je součet ohodnocení hran spojujících X a Y.
Doporučení pro sekvenční algoritmus:
Při implementaci postupujeme tak, že postupně přiřazujeme uzlům grafu příznak 0 nebo 1. Příznak 0 znamená, že uzel patří do X a příznak 1, že uzel patří do Y.
Průběžně počítáme váhu řezu pro již přiřazené uzly a pokud je tato váha větší než dosud nalezené minimum, tuto větev ukončíme.
Stejně tak ukončíme větev, pokud velikost menší podmnožiny uzlů překročí a.
Větev ukončíme, i pokud v daném mezistavu pomocí dolního odhadu váhy zbývajícího řezu zjistíme, že z něj nelze žádným způsobem vytvořit přípustný koncový stav s menší vahou řezu, než je nejlepší dosud nalezené řešení (čili toto prořezávání lze dělat až po té, kdy vypočteme váhu prvního řezu).
Dolní odhad váhy zbývajícího řezu lze spočítat tak, že pro každý zatím nepřiřazený uzel grafu v daném mezistavu s částečným řezem vypočteme, o kolik by se váha řezu zvýšila, pokud by tento uzel patřil do X, o kolik by se zvýšila, pokud by tento uzel patřil do Y a vezmeme menší z těchto dvou hodnot a tato minima posčítáme pro všechny nepřiřazené uzly.
Paralelní algoritmus:
Paralelní algoritmus je typu Master-Slave.

Testovací grafy graf_n_k.txt s referenčními sekvenčními časy a vahami minimálních řezů:
Soubor	Hodnota a	Sekvenční čas [s]	# volání rek. fce	Min. váha řezu
graf_10_5.txt	5	0,0	218	974
graf_10_6b.txt	5	0,0	305	1300
graf_20_7.txt	7	0,02	34K	2110
graf_20_7.txt	10	0,02	43K	2378
graf_20_12.txt	10	0,02	142 K	5060
graf_30_10.txt	10	1,9	9 M	4636
graf_30_10.txt	15	3.9	17 M	5333
graf_30_20.txt	15	13	59 M	13159
graf_40_8.txt	15	180	828 M	4256
graf_40_8.txt	20	275	1 G	4690
graf_40_15.txt	20	2312	11 G	11361
graf_40_25.txt	20	5052	27 G	21697
Pokud chcete testovat váš pogram na více datech či vám uvedené testovací grafy (DOWNLOAD) nevyhovují pro práci na závěrečné zprávě (tj. dosažené sekvenční časy nejsou v řádu jednotek minut), buď použijte jinou hodnotu parametru a nebo použijte generátor grafů s volbou typu grafu "-t AD -n PU -k PS -w80,120 -c", který vygeneruje souvislý neorientovaný neohodnocený graf o PU uzlech a průměrném stupni PS a následně hrany ohodnotí číselnými vahami z intervalu <80,120>.
