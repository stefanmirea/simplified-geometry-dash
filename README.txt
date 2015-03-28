This was my first homework for a computer graphics course in the 3rd year.

Task: implementing the Geometry Dash game in C++ using a given framework (which
uses freeglut for window and input control).

Making a game similar to the original one was not too easy, as the framework
didn't have support for images, transparency or even color gradient.

Full statement:
http://cs.curs.pub.ro/2014/pluginfile.php/8721/mod_resource/content/1/Enunt%20Tema%201%20EGC.pdf
(You might not have access to it as a guest.)

Check out the repo wiki for some screenshots.

Original README file:
================================================================================
EGC - Tema 1
    Simplified Geometry Dash
    Stefan-Gabriel Mirea, 331CC

Cuprins

1. Cerinta
2. Utilizare
3. Implementare
4. Testare
5. Probleme Aparute
6. Continutul Arhivei
7. Functionalitati

1. Cerinta
--------------------------------------------------------------------------------
Tema cere implementarea unui joc similar cu Geometry Dash[1], utilizand
framework-ul folosit in primele laboratoare. Jucatorul trebuie sa poata sari,
trebuie sa existe obstacole reprezentate de obiecte ascutite, elemente
ajutatoare pentru ocolirea obstacolelor si elemente ce ofera puncte si permit
continuarea din acel loc in caz de pierdere in limita a 3 incercari, scopul
jocului fiind acumularea a 5 astfel de puncte.

2. Utilizare
--------------------------------------------------------------------------------
2.1 Fisiere

La pornire, jocul va citi harta din fisierul map.txt din directorul curent.
Acesta trebuie sa contina cel putin o linie si toate liniile trebuie sa aiba
aceeasi lungime, nefiind mai scurte de 20 de caractere.
Semnificatia caracterelor:
   'S' = pozitia de start a jucatorului
   '#' = patrat component al unei platforme (aceste patrate se coloreaza cu un
         pattern ce va reda un efect de continuitate)
   '_' = dreptunghi ajutator situat in partea de jos a spatiului patratic
   '-' = dreptunghi ajutator situat in partea de sus a spatiului patratic
   'n' = patrat
   '.' = cerc ce aplica automat o saritura; va fi plasat in partea de jos a
         spatiului patratic
   'A', 'a' = triunghiuri (mare respectiv mic) cu varful in sus
   'V', 'v' = triunghiuri (mare respectiv mic) cu varful in jos
   '>', ';' = triunghiuri (mare respectiv mic) cu varful spre dreapta
   '<', ':' = triunghiuri (mare respectiv mic) cu varful spre stanga
   '^' = zona pe care nu se poate sta (spatiu dintre platforme)
   'P' = stegulet
   orice altceva = spatiu liber
Obiectele simbolizate pe ultima linie a fisierului vor fi plasate direct pe un
pamant solid, iar cele de pe prima linie se vor afla direct sub un tavan solid.
Spatiul va fi nelimitat in stanga si in dreapta, creandu-se impresia repetarii
la infinit a coloanelor in ambele sensuri. De exemplu, la stanga primei coloane
va fi unita ultima.

2.2 Consola

Programul poate primi in linia de comanda un parametru optional proportional cu
viteza jocului. Acesta reprezinta mai exact numarul de patrate parcurse pe
orizontala intre doua apeluri onIdle() succesive, valoarea lui implicita fiind
0.15. Exemplu de apel:
>egc_tema1.exe 0.05
El va afecta toate fenomenele, mai putin efectele de afisare a traiectoriei dupa
lovirea unui element de saritura automata, de afisare a unor cercuri la
atingerea anumitor elemente, de scalare periodica a elementelor de sarire si de
explozie.
Tot la consola, programul va afisa anumite mesaje de eroare daca sunt probleme
cu fisierul de intrare (in cazul in care programul pare sa nu porneasca, e
foarte probabil ca a aparut o astfel de problema, asa ca neaparat trebuie vazut
ce afiseaza la consola).

2.3 Input Tastatura

Esc: pauza / revenire
Spatiu: sarire
Orice tasta in afara de Esc: inceperea unui joc nou dupa terminarea altuia

3. Implementare
--------------------------------------------------------------------------------
Platforma:
   IDE: Microsoft Visual Studio Professional 2013
   Compilator: Microsoft C/C++ Optimizing Compiler Version 18.00.21005.1 for x86
   Sistem de operare: Microsoft Windows 7 Ultimate SP1 Version 6.1.7601

Generalitati:
Datorita repetarii pe orizontala a intregului spatiu, jocul poate continua la
nesfarsit cat timp nu au fost adunate 5 stegulete.

Vectorul DrawingWindow::objects2D contine elementele grupate pe categorii,
pentru suprapunerea corecta a acestora. In linii mari, acesta va contine, de la
stanga la dreapta:
   - jucatorul
   - liniutele ce alcatuiesc efectul de explozie
   - cercurile ce alcatuiesc efectul de a atinge anumite elemente
   - liniutele ce compun traiectoria jucatorului la o sarire automata
   - pamantul si tavanul
   - elementele din joc (triunghiuri etc.)
   - fundalul
Pentru insertia ulterioara a altor elemente la categoria corespunzatoare,
folosesc variabilele first_explosion_pos, last_explosion_pos, first_circle_pos,
last_circle_pos, first_traj_pos, last_traj_pos, first_elem_pos, last_elem_pos.

Exista un numar de 5 contexte vizuale:
   - script: afiseaza textele din partea superioara
   - script_backgr: afiseaza culoarea din spatele acestor texte (nu puteam
folosi un singur context pentru text si culoare, pentru ca dreptunghiurile apar
implicit in fata textului)
   - game: spatiul de joc propriu-zis
   - backgr1, backgr2: spatiile negre pentru umplerea ferestrei
In urma redimensionarii, se garanteaza ca spatiul propriu-zis de jos isi va
pastra aspectul 16:9. Banda cu textele din partea superioara nu se va mai afisa
daca nu exista spatiu suficient pentru acestea, caz in care spatiul de joc se
poate extinde pe toata inaltimea ferestrei.

Pamantul si tavanul sunt simulate prin afisarea a 20 de dreptunghiuri ce se
translateaza concomitent cu modificarea ferestrei contextului vizual game.
Fundalul e simulat printr-un numar de dreptunghiuri grupate in 4 panouri care
de asemenea se muta pentru a nu lasa niciodata un gol vizibil.

In functia DrawingWindow::init() are loc citirea fisierului de intrare si
initializarea majoritatii variabilelor globale. Cele mai importante dintre
acestea sunt lines, vectorul de linii din fisier ce va fi folosit pe parcursul
executiei pentru a accesa elementele hartii si objects, o matrice 3D prin care
fiecarui element din lines ii corespunde un vector de Object2D *, continand
obiectele din reprezentarea grafica a elementului. Tot aici sunt create si
contextele vizuale.

In functia DrawingWindow::onIdle() au loc toate actualizarile care fac posibile
efecte precum aparitia / disparitia elementelor prin scalare sau adaugarea /
eliminarea lor din DrawingWindow::objects2D cand acestea intra / ies din cadru.
Aici se verifica si interactiunile dintre jucator si celelalte elemente si, in
functie de starea sa, i se actualizeaza pozitia, pentru ca apoi fereastra
contextului vizual sa se actualizeze si ea in functie de pozitia jucatorului,
si, ulterior, pamantul, fundalul, si obiectele vizibile sa se actualizeze in
functie de miscarea camerei.

Salturile personajului sunt implementate prin retinerea coordonatelor de start
pe verticala si in timp (jump_start_y, jump_start_t) si calculul la fiecare pas
a pozitiei pe verticala in functie de timpul trecut de la jump_start_t si de
inaltimea initiala, pe baza unei functii de gradul 2. Coeficientii au fost
determinati experimental din jocul original si difera in functie de tipul
sariturii (saritura manuala, saritura automata generata de un cerc asezat pe un
alt element sustinator sau de un cerc aflat in aer).

Miscarea "camerei", simulata de modificarea ferestrei contextului vizual game,
se face cu o viteza ce depinde de pozitia pe verticala a jucatorului fata de ea.
Exista un interval admis, care nu determina miscarea camerei pe verticala. Daca
personajul depaseste acel interval, camera se va indrepta spre el. Daca se
constata ca jucatorul ar fi pe cale sa iasa din cadru, camera se va misca exact
cu aceeasi viteza ca a lui.

Intrucat nu am implementat lucru cu mouse-ul, am sters functiile mouseFunction()
si onMouse() din framework.

4. Testare
--------------------------------------------------------------------------------
Tema a fost testata doar pe calculatorul personal (platforma mentionata la 3.).
In folderul Teste, se gasesc mai multe exemple de fisiere pe care le-am folosit
pentru testare:
   map_jumpers.txt: un exemplu de multiplicare masiva a cercurilor care apar la
                    trecerea prin elemente de sarire
   map_numeric.txt: demonstreaza cum, datorita lipsei de precizie a metodei
                    numerice, jucatorul poate pierde uneori, cu toate ca
                    matematic nu ar trebui; astfel de situatii apar adesea la
                    folosirea elementului '_'. Lansand programul cu
                    "egc_tema1.exe 0.05", se observa ca problema dispare.
   map_real.txt: imita partea de inceput a jocului original
   map_stairs.txt: test pentru urcarea unei scari, ceea ce se poate face foarte
                   simplu prin tinerea apasata a tastei Space, evident, daca se
                   sare initial la momentul potrivit. Cu toate ca am ales
                   parametrii de asa natura incat jucatorul sa pice pe fiecare
                   treapta in aceeasi pozitie, in practica jucatorul "intrece"
                   treptele tot datorita preciziei, efect ce se diminueaza la
                   viteze mai mici.

5. Probleme aparute
--------------------------------------------------------------------------------
In legatura cu enuntul, nu a fost foarte clar cum trebuie actualizat numarul de
incercari, de exemplu daca acesta ar trebui sa fie 3 la inceputul jocului si pe
parcurs doar sa scada eventual, jocul reincepand de la ultimul checkpoint cu
numarul de vieti ramas, sau ar fi trebuit ca fiecare stegulet sa reinitializeze
numarul de vieti la 3. Eu am ales cea de-a doua varianta, cu mentiunea ca una
din vieti / incercari este chiar cea curenta. De asemenea, incercarile nu sunt
contorizate inainte de colectarea primului stegulet, pentru ca s-ar ajunge la
paradoxala situatie de a fi mai favorabila inceperea unui joc nou decat
continuarea celui curent.
Dupa cum demonstreaza si testele map_numeric.txt si map_stairs.txt, functia
DrawingWindow::onIdle() se apeleaza destul de rar, incat, pentru ca jocul sa
functioneze la o viteza comparabila cu cea a jocului original, numarul mic de
actualizari din unitatea de timp duce la erori de suprapuneri cu efecte vizibile
uneori.

6. Continutul Arhivei
--------------------------------------------------------------------------------
lab2/map.txt - fisierul de intrare ce contine scena finala
lab2/include/cell.hpp - declararea clasei Cell
lab2/src/* - sursele *.cpp
Teste/* - explicat la 4.
Toate celelalte fisiere fac parte din framework-ul din laborator.

7. Functionalitati
--------------------------------------------------------------------------------
7.1 Functionalitati standard

    - jucatorul sare si se roteste
    - exista obstacole, elemente ajutatoare, checkpoint-uri ce functioneaza
      corespunzator

7.2 Functionalitati Bonus / Suplimentare

    - animatie de scalare a obiectelor cand intra si ies din cadru, insotita de
      adaugarea si stergerea lor din DrawingWindow::objects2D
    - fundal care se translateaza pe ambele directii mai lent decat elementele
      din prim plan
    - setarea vitezei jocului printr-un argument in linia de comanda (vezi 2.2)
    - posibilitatea de a pune pauza in orice moment cu Esc
    - mentinerea unei pozitii constante a textelor la redimensionarea ferestrei,
      relativ la marginile ei sau la axa centrala si disparitia benzii ce le
      afiseaza atunci cand se constata ca nu ar mai avea loc (fereastra fiind
      foarte mica)

Referinte
--------------------------------------------------------------------------------
[1] https://play.google.com/store/apps/details?id=com.robtopx.geometryjumplite&hl=en