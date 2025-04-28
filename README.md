Task 1 - Procesul de dirijare:
    Am creat initial, cu acces global, cate 2 variabile pentru
fiecare tabela: una cu tabela propriu-zisa, iar una pentru lungimea sa.
    De asemenea, la inceputul main-ului, am alocat si parsat
tabelele si lungimea acestora in variabile prin functiile puse la dispozitie in 'lib.h'. Header-ul pentru ethernet fiind creat, am realizat asemenea unul pentru IP, luand-ul din cadrul buffer-ului, din continuarea celui de ethernet. Inaintea implementarii pasilor din documentatia pentru IPv4, verific daca tipul eth_hdr este, intr-adevar, IPv4. In continuare, urmaresc pasii oferiti:
    -> Verifica daca el este destinatia: Am implementat acest pas
in cadrul Task-ului 3, pentru a putea trimite inapoi un pachet ICMP.
    -> Verifica checksum: Daca checksum-ul obinut este diferit de
0, arunc pachetul (trec la urmatorul). (Functie implementata in laboratorul 4).
    -> Verificare si actualizare TTL: Daca ttl este 1 sau 0 (<=1),
arunc pachetul. Altfel, decrementez ttl.
    -> Cautare in tabela de rutare: Initial, pentru acest task am
implementat o functie (cu ajutorul laboratului 4), 'get_best_route', ce cauta liniar in rtable si gaseste adresa destinatie cu masca cea mai mare. Ulterior, am modificat in cautare binara la Task 2. Aceasta functie e apelata iar adresa obinuta salvata in variabila 'best_route'. Daca este NULL, arunc pachetul.
    -> Actualizare checksum: Modific in header-ul IP, campul
check, cu valoarea returnata de functia 'checksum' oferita in lib.h, insa, trecuta in Network order.
    -> Rescriere adrese L2: Folosesc destinatia gasita in
'best_route': Am implementat o functie de cautare liniara in tablea de ARP (implementata anterior in laboratorul 4), pentru a obtine adresa MAC a destinatiei. Daca este NULL, arunc pachetul. Daca nu, rescriu destinatia in header-ul de ethernet, precum si sursa prin functia oferita 'get_interface_mac'.
    ->Trimiterea noului pachet: In final, trimit pachetul prin
functia oferita 'send_to_link', pe interfata corespunzatoare urmatorului hop.

Task 2 - Longest Prefix Match eficient:
    Pentru a face mai eficienta functia 'get_best_route', am
decis sa realizez o cautare binara. Pentru asta, a trebuit, mai intai, sa sortez vectorul.
    Am construit o functie de comparare pentru qsort, 'cmpfunc'
ce ordoneaza crescator intrarile in rtable dupa masca, ce are prioritate maxima deoarece o masca mai mare reprezinta un match mai exact, iar, daca mastile sunt egale, dupa prefix.
    In functia 'get_best_route' am pornit de la implementarea
clasica a unei cautari binare. Cat timp extremitatea stanga e mai mica decat cea dreapta, daca nu gasesc o potrivire, compar prefixul curent cu cel pe care il caut (dest & masca) si modific una dintre margini. Totusi, atunci cand gasesc un match (prefix = dest & masca), in loc de a il returna, il salvez intr-o variabila daca indeplineste 2 conditii: Fie variabila e NULL (nu am gasit vreun match anterior), fie masca noului match este mai mare. In final, returnez ultimul match salvat in variabila.
    In plus, am trecut prefixul, masca si destinatia in Host
Order, pentru a obtine un rezultat corect in urma operatiilor (&, <).
    In final, am apelat 'qsort' cu functia de comparare in main,
dupa care am obinut destinatia prin noul 'get_best_route'.

Task 4 - Protocolul ICMP:
    Am obtinut header-ul ICMP din buffer, din continuarea celui
IP, dupa care am tratat cele 3 cazuri:
    -ICMP "Echo reply" in cazul in care router-ul este chiar
destinatia
    -ICMP "Time exceeded" in cazul in care a ttl-ul a ajuns la 0
sau 1.
    -ICMP "Destination unreachable" in cazul in care
'get_best_route' returneaza NULL.
    Pentru fiecare, am realizat urmatoarele: Am schimbat tipul si
codul header-ului ICMP dupa caz, cum am fost indrumat in documentatie, dupa care am calculat checksum-ul acestuia. Am facut swap la destinatie si sursa in header-ul de IP (deoarece pachetul e trimis inapoi in locul din care provine), am setat 'protocol' pe 1, pentru a anunta encapsularea unui protocol ICMP, am setat 'tot_len' ca fiind dimensiunea acestuia + dimensiunea header-ului ICMP pe care il include si am modificat checsum-ul corespunzator.
    In final, am trimis pachetul prin functia 'send_to_link', dar 
cu o dimensiunea corecta a acestuia, anume suma dimensiunilor header-urilor Ethernet, IP, ICMP. 
