# Preambul

Pentru aceasta tema am ales sa fac o implementare putin mai generala a cli-ului
decat ne era cerut in enuntul problemei. In acest sens, am ales sa tin cont
de toate caile valide din sistemul de fisiere, adica inclusiv cai ce contin
portiuni cuprinse intre ghilimele, cu spatii, cu escape sequences si chiar si
cu expandari de tipul `$()`

Pentru aceasta am utilizat functia `wordexp` din <wordexp.h>, ce face parte
din biblioteca standard C (libc.a) si este linkata in mod implicit.

De asemenea, argumentele unei comenzi pentru cli pot fi separate de comanda
si intre ele atat prin unul **sau mai multe** spatii, cat si prin **taburi**.

Asadar, setul de comezi valide pentru implementarea mea este mai larg decat
cel din cerinta problemei, fiind o supramultime a acestuia. Spre exemplu,
urmatoarele comenzi sunt toate valide presupunand ca lucram in folderul in
care a fost extras arhiva si in el a fost creat un fisier `input.txt` avand
 continutul `images/christmas.bmp`

```plintext
edit	$(cat input.txt)
set      draw_color 	0 0		 255
insert images/"tree.bmp" 20 20
fill 10 10
save output/Task5/"output 3".bmp
quit
```

Inca un lucru in plus, care a fost mai degraba dictat de nevoia de a-mi
verifica programul in lipsa unui checker pana la data publicarii acestuia,
programul meu afisaza si mesaje de eroare pentru comenzile invalide
(am descarcat niste imagini bmp de pe net pentru a testa si aceasta masura
m-a ajutat sa indentific rapid care imagini erau valide conform cerintei -
infoheader de marimea corecta, culori in format bgr etc.). Exemplu:

```plaintext
edit images 
== The object at "images" is not a BMP file ==
edit images/chritsmas.bmp
== File "images/chritsmas.bmp" doesn't exist ==
edit images/christmas.bmp
set drawcolor 255 0 0
=== Unknown attribute drawcolor. `draw_color' or `line_width' expected ===
set draw_color 255 0 0
drow line 0 0 100 100
== Unknown command 'drow' ==
draw line 0 0 100 100
draw line
=== 4 arguments after `line' expected ===
save /
== Invalid path "/" ==
save output/Task5/test.bmp
quit
```

[Chiar va invit sa incercati aceste exemple daca aveti timp :))]

# Implementarea CLI-ului

Fisierul principal al temei este `bmpcli.c`. Acesta primeste continuu input de
la consola pana la primirea comenzii `quit`. Asa cum am explicat in preambul,
parsingul argumentelor comenzilor este facut de `wordexp`. Am implementat
fiecare comanda intr-un subprogram separat pentru a nu face functia `main`
prea mare.

# Structuri
In afara de structurile date in scheletul temei pentru headerele fisierelor,
am mai definit o structura pentru pixeli in format bgr si una pentru toata
imaginea formata din headere si o matrice de pixeli.
Directivele `#pragma pack(1)` au fost de folos in citirea fisierelor.

# Comenzi
## Edit
Aceasta operatie incarca o imagine in memorie, la o adresa specificata.
Citirea imaginilor se face pe rand, citind mai intai headerele in buffere
temporare si copierea lor in structurile specializate (direct prin atribuirea
prin copiere a continutului bufferelor interpretate ca obiecte de tipul
structurilor - posibila fara probleme datorita directivelor `#pragma pack(1)`)
si apoi matricea de pixeli. Padding-urile sunt calculate si ignorate pe masura
ce parcurgem fisierul. Am retinut matricea de pixeli asa cum se afla in fisier,
adica rasturnata, pentru ca am considerat ca ar fi si mai simplu si posibil si
mai eficient sa lucrez cu ea asa.

## Save
Salvarea imaginilor presupune exact operatiile inverse fata de Edit: scrierea
headerelor din structuri in fisier, adaugarea paddingului, parcurgerea matricei
de pixeli si scrierea fiecarui rand urmat de padding.

## Quit
Pentru operatia `quit` am implementat o functie de curatare a memoriei folosita
pentru o imagine.

## Insert
Pentru inserarea unei alte imagini m-am folosit de operatiile deja implementate,
incarcand in memorie, la o alta locatie fata de imaginea din modul edit,
imaginea de inserat. Astfel am putut lucra simplu cu liniile si coloanele ei
fara sa mai trebuiasca sa tin cont de padding. Inserarea se face prin
suprascrierea pixelilor respectivi din matricea imaginii din modul edit
cu pixelii imaginii de inserat. Calculul indicilor l-am facut tot
'de jos in sus'. In final, imaginea de inserat este curatata din memorie.

## Set
Operatiile de `set draw_color` si `set line_width` reprezinta simple atribuiri
(realizate aici prin pointeri primiti ca parametru la handlerele operatiilor)
ale unor variabile ce vor fi pasate ca parametru tuturor functiilor ce au
nevoie de culoarea si grosimea creionului.

## Draw
### Line
Desenarea de linii se face alfland mai intai axa 'principala', pe care va fi
plasat graficul functiei dreptei, aflarea absciselor si ordonatelor in raport
cu aceasta plasare a graficului pentru capete, parcurgerea axei principale si
calcularea ordonatelor in parte intreaga dupa ecuatia unei drepte si desenarea
dreptei de grosimea si culoarea data. Pentru a da grosimea liniei, am
considerat ca in loc sa deseman linia pixel cu pixel, o desenam 'celula' cu
'celula' si am scris o functie ce deseneaza o celula de grosimea si culoarea
precizata la o pozitie data.

## Rectangle si Triangle
Desenarea dreptunghiurilor reprezinta desenarea a 4 linii, iar desenarea
triughiurilor, a 3 linii. Aceste operatii se bazeaza exclusiv pe operatia
anterioara.

## Fill
Aceasta a fost o operatie foarte interesant de realizat pentru care am gasit o
implementare foarte puternica. Umplerea recursiva a imaginii presupunea
recursivitate adanca si de la o anumita limita producea Stack Overflow.
Pentru a trece de aceasta problema, a trebuit sa micsorez dimensiunea
parametrilor functiei si am incercat sa o micsorez pe cat de mult posibil.
Astfel, am impartit functia in doua: una 'interna' cu un singur parametru ce
contine functionalitatea operatiei, si una 'user-friendly', ce este doar o
decoratie a primeia, ce accepta parametri mai intuitivi. Am creat o structura
interna (a se intelege de uz unic sau restrans) pentru a impacheta informatia
necesara functiei de fill, si a permite astfel scrierea ei cu un singur
parametru de tip pointer la aceasta structura. Pentru a nu crea si alte
variabile pe stiva, am facut apelurile recursive cu acelasi pointer la aceeasi
structura, dar inainte de fiecare apel am modificat datele punctate de acesta
si dupa apel am inversat aceste modificari.

Mai jos aveti efectul aplicarii unei astfel de operatii de fill pe o imagine
descarcata de pe net ce cred eu ca ilustreaza bine cat de puternica este
aceasta implementare (si da, poate si sa umple toata imaginea blank.bmp)

![before-image](https://imgur.com/a/VNI97t5)
![after-image](https://imgur.com/a/wODLZFo)
