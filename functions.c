#include <math.h>
#include "utils.h"
#define FOUR 4
#define BITS_PER_BYTE 8

typedef unsigned char byte;

// returneaza diferenta pana la primul multiplu de 4 dupa `n'
unsigned short difToMultipleOfFour(unsigned long n) {
  return (FOUR - (unsigned short)(n % FOUR)) % FOUR;
}

void errMsg() {
  printf("== Encountered error while allocating memory. Try again ==\n");
}

// incarca imaginea de la `path' in memorie la locatia punctata de `loadbmp'
// returneaza un cod de eroare in cazul in care formatul imaginii este invalid
// cod 0 inseamna executie fara erori, -1 inseamna eroare
short edit(bmp* loadbmp, char* path) {
  // daca o imagine a fost deja incarcata, o dealocam
  if (loadbmp->bitmap) dealloc(loadbmp);
  FILE* file = fopen(path, "rb");
  if (!file) { printf("== File \"%s\" doesn't exist ==\n", path); return -1; }

  // buffer temporar pentru bytes din file header
  byte* fheaderbin = (byte*)malloc(sizeof(bmp_fileheader) * sizeof(byte));
  if (!fheaderbin) { errMsg(); fclose(file); return -1; }
  fread(fheaderbin, 1, sizeof(bmp_fileheader), file);
  if (fheaderbin[0] != 'B' || fheaderbin[1] != 'M') {
    printf("== The object at \"%s\" is not a BMP file ==\n", path);
    free(fheaderbin); fclose(file); return -1;
  }
  // copiem continutul bufferului in campul pentru file header
  loadbmp->fileheader = *((bmp_fileheader*)fheaderbin);
  free(fheaderbin);

  // buffer temporar pentru bytes din info header
  byte* iheaderbin = (byte*)malloc(sizeof(bmp_infoheader) * sizeof(byte));
  if (!iheaderbin) { errMsg(); fclose(file); return -1; }
  fread(iheaderbin, 1, sizeof(bmp_infoheader), file);
  // copiem continutul bufferului in campul pentru info header
  loadbmp->infoheader = *(bmp_infoheader*)iheaderbin;
  if (loadbmp->infoheader.biSize != sizeof(bmp_infoheader)) {
    printf("== Unsupported file format: "
    "InfoHeader must be 40 bytes long ==\n");
    free(iheaderbin); fclose(file); return -1;
  }
  if (loadbmp->infoheader.bitPix != sizeof(bgr) * BITS_PER_BYTE) {
    printf("== Unsupported file format: Colors must be in bgr format ==\n");
    free(iheaderbin); fclose(file); return -1;
  }
  free(iheaderbin);
  // sarim peste padding-ul pana la matricea de pixeli
  fseek(file, loadbmp->fileheader.imageDataOffset, SEEK_SET);

  int lw = loadbmp->infoheader.width, lh = loadbmp->infoheader.height;
  loadbmp->bitmap = (bgr**)malloc(lh * sizeof(bgr*));
  if (!loadbmp->bitmap) { errMsg(); fclose(file); return -1; }
  // calculam paddingul de pe fiecare rand al matricei
  unsigned short paddingBytes = difToMultipleOfFour(lw * sizeof(bgr));
  // salvam matricea in ordinea din fiser (imaginea rasturnata)
  for (int i = 0; i < lh; ++i) {
    loadbmp->bitmap[i] = (bgr*)malloc(lw * sizeof(bgr));
    if (!loadbmp->bitmap[i]) {
      errMsg(); dealloc(loadbmp);
      fclose(file); return -1;
    }
    fread(loadbmp->bitmap[i], sizeof(bgr), lw, file);
    // ignoram paddingul
    fseek(file, (signed)paddingBytes, SEEK_CUR);
  }

  fclose(file);
  return 0;
}

// salveaza imaginea din memorie indicata de `loadbmp' la calea `path'
void save(bmp* loadbmp, char* path) {
  FILE* file = fopen(path, "wb");
  if (!file) { printf("== Invalid path \"%s\" ==\n", path); return; }

  // scriem headerele
  fwrite((void*)&(loadbmp->fileheader), 1, sizeof(bmp_fileheader), file);
  fwrite((void*)&(loadbmp->infoheader), 1, sizeof(bmp_infoheader), file);

  // calculam paddingul dintre info header si matricea de pixeli
  size_t bytesToOffset = loadbmp->fileheader.imageDataOffset -
  sizeof(bmp_fileheader) - sizeof(bmp_infoheader);
  // alocam bytes 0 pe care ii scriem ca padding in fisier
  byte* paddingToOffset = (byte*)calloc(bytesToOffset, 1);
  if (paddingToOffset) {
    fwrite((void*)paddingToOffset, 1, bytesToOffset, file);
    free(paddingToOffset);
  } else {
    // fallback mai putin performant
    byte zero = 0;
    for (int i = 0; i < bytesToOffset; ++i) fwrite(&zero, 1, 1, file);
  }

  int lw = loadbmp->infoheader.width, lh = loadbmp->infoheader.height;
  // calculam paddingul de pe fiecare rand al matricei
  size_t paddingBytes = difToMultipleOfFour(lw * sizeof(bgr));
  // alocam un buffer de bytes 0 pentru a scrie paddingul
  byte* padding = (byte*)calloc(paddingBytes, 1);
  for (int i = 0; i < lh; ++i) {
    fwrite((void*)(loadbmp->bitmap[i]), lw, sizeof(bgr), file);
    // scriem paddingul
    if (padding) {
      fwrite((void*)padding, 1, paddingBytes, file);
    } else {
      byte zero = 0;
      for (int i = 0; i < paddingBytes; ++i) fwrite(&zero, 1, 1, file);
    }
  }
  if (padding) free(padding);

  // calculam paddingul de final, alocam un buffer cu 0 si il scriem
  size_t endBytes = loadbmp->fileheader.bfSize - ftell(file);
  byte* end = (byte*)calloc(endBytes, 1);
  if (end) {
    fwrite(end, endBytes, 1, file);
    free(end);
  } else {
    byte zero = 0;
    for (int i = 0; i < endBytes; ++i) fwrite(&zero, 1, 1, file);
  }
}

// in functiile urmatoare, coordonatele x si y sunt considerate astfel:
// x - axa orizontala (coloane), y - axa verticala (linii)
// liniile sunt numerotate incepand din partea de *jos* a imaginii

// insereaza imaginea de la calea `path' peste cea din memorie de la `loadbmp'
// la pozitia (x, y).
// returneaza un cod de eroare ce respecta aceeasi conventie ca la `edit'
short insert(bmp* loadbmp, char* path, unsigned x, unsigned y) {
  // incarcam in memorie imaginea de la calea `path' in `new_bmp`
  bmp new_bmp = {};
  short err = edit(&new_bmp, path);
  if (err) { return err; }

  int lh = loadbmp->infoheader.height, nh = new_bmp.infoheader.height;
  int lw = loadbmp->infoheader.width, nw = new_bmp.infoheader.width;
  for (int i = (int)y; i < lh && i - y < nh; ++i) {
    for (int j = (int)x; j < lw && j - x < nw; ++j) {
      loadbmp->bitmap[i][j] = new_bmp.bitmap[i - y][j - x];
    }
  }

  dealloc(&new_bmp);
  return 0;
}

// seteaza o celula (pixelul central si bordura lui)
// a unui segment de desenat in memorie la `loadbmp',
// in matricea de pixeli la pozitia (x, y). `x' - coloana, `y' - linia
// `width' - grosimea bordurii. `color' - culoarea celulii
void draw_cell(bmp* loadbmp, int x, int y, int width, bgr color) {
  if (x < 0 || x >= loadbmp->infoheader.width) return;
  if (y < 0 || y >= loadbmp->infoheader.height) return;
  // pixelul central
  loadbmp->bitmap[y][x] = color;
  for (int i = 1; i <= width / 2; ++i) {
    // bordura de sus si de jos
    int left = x - i; left = left >= 0 ? left : 0;
    for (int j = left; j <= x + i && j < loadbmp->infoheader.width; ++j) {
      if (y - i >= 0) loadbmp->bitmap[y - i][j] = color;
      if (y + i < loadbmp->infoheader.height)
        loadbmp->bitmap[y + i][j] = color;
    }
    int top = y - i; top = top >= 0 ? top : 0;
    // bordura de la stanga si de la dreapta
    for (int j = top; j <= y + i && j < loadbmp->infoheader.height; ++j) {
      if (x - i >= 0) loadbmp->bitmap[j][x - i] = color;
      if (x + i < loadbmp->infoheader.width)
        loadbmp->bitmap[j][x + i] = color;
    }
  }
}

// in functiile urmatoare, parametrii `draw_color' si `line_width' sunt comuni:
// `draw_color' - culoarea creionului
// `line_width' - grosimea creionului

// deseneaza un segment in imaginea din memorie de la `loadbmp'
// `x1', `y1', `x2', `y2' - coordonatele capetelor segmentului
void draw_line(bmp* loadbmp, bgr draw_color, byte line_width,
int x1, int y1, int x2, int y2) {
  // abscisa minima/maxima si ordonatele respective
  int abscmin = 0, abscmax = 0, ordm = 0, ordM = 0;
  // graficul este pozitionat normal? absica se afla pe axa orizontala?
  byte x_principal = 1;
  // identificam pozitionarea graficului/abscisele/ordonatele
  if (abs(x1 - x2) > abs(y1 - y2)) {
    if (x1 < x2) {
      abscmin = x1; abscmax = x2;
      ordm = y1; ordM = y2;
    } else {
      abscmin = x2; abscmax = x1;
      ordm = y2; ordM = y1;
    }
  } else {
    x_principal = 0;
    if (y1 < y2) {
      abscmin = y1; abscmax = y2;
      ordm = x1; ordM = x2;
    } else {
      abscmin = y2; abscmax = y1;
      ordm = x2; ordM = x1;
    }
  }
  // desenam celulele din capete
  draw_cell(loadbmp, x1, y1, line_width, draw_color);
  draw_cell(loadbmp, x2, y2, line_width, draw_color);
  for (int absc = abscmin; absc < abscmax; ++absc) {
    int ord = ordm + (int)floor(1.0 * (absc - abscmin) *
    (ordM - ordm) / (abscmax - abscmin));
    int x = x_principal ? absc : ord, y = x_principal ? ord : absc;
    draw_cell(loadbmp, x, y, line_width, draw_color);
  }
}

// deseneaza un dreptunghi in imaginea din memorie de la `loadbmp'
// `x', `y' - coordonatele coltului stanga-jos al dreptunghiului
// `width', `height' - dimensiunile dreptunghiului
void draw_rect(bmp* loadbmp, bgr draw_color, byte line_width,
int x, int y, int width, int height) {
  draw_line(loadbmp, draw_color, line_width,
  x, y, x + width, y);
  draw_line(loadbmp, draw_color, line_width,
  x, y, x, y + height);
  draw_line(loadbmp, draw_color, line_width,
  x, y + height, x + width, y + height);
  draw_line(loadbmp, draw_color, line_width,
  x + width, y, x + width, y + height);
}

// deseneaza un triunghi in imaginea din memorie de la `loadbmp'
// `x1', `y1', `x2', `y2', `x3', `y3' - coordonatele varfurilor
void draw_tri(bmp* loadbmp, bgr draw_color, byte line_width,
int x1, int y1, int x2, int y2, int x3, int y3) {
  draw_line(loadbmp, draw_color, line_width, x1, y1, x2, y2);
  draw_line(loadbmp, draw_color, line_width, x2, y2, x3, y3);
  draw_line(loadbmp, draw_color, line_width, x3, y3, x1, y1);
}

// verifica daca doua culori sunt egale
int colors_eq(bgr c1, bgr c2) {
  return (c1.r == c2.r) && (c1.g == c2.g) && (c1.b == c2.b);
}

// structura interna pentru reprezentarea parametrului functiei `__fill_util'
#pragma pack(1)
typedef struct {
  bmp* bmpp;  // imaginea in memorie unde se face fill
  int x, y;  // coordonatele pixelului de unde se face fill
  bgr color;  // culoarea de umplere
  bgr backcolor;  // culoarea peste care se face fill
} __fill_param;
#pragma pack()

// functie interna pentru umplerea imaginii. Dimensiunea parametrilor
// a fost redusa pentru a permite recursivitatea adanca.
void __fill_util(__fill_param* p) {
  if ((p->x < 0) || (p->x >= p->bmpp->infoheader.width)) return;
  if ((p->y < 0) || (p->y >= p->bmpp->infoheader.height)) return;
  if (!colors_eq(p->bmpp->bitmap[p->y][p->x], p->backcolor)) return;
  p->bmpp->bitmap[p->y][p->x] = p->color;
  p->y--; __fill_util(p); p->y++;  // pixelul de deasupra
  p->y++; __fill_util(p); p->y--;  // pixelul de sub
  p->x--; __fill_util(p); p->x++;  // pixelul de la stanga
  p->x++; __fill_util(p); p->x--;  // pixelul de la dreapta
}

// functia de umplere in format 'user-friendly'.
// umple imaginea de la `loadbmp' pornind de la pixelul (x, y)
void fill(bmp* loadbmp, bgr draw_color, int x, int y) {
  bgr backcolor = loadbmp->bitmap[y][x];  // culoarea peste care se umple
  if (colors_eq(draw_color, backcolor)) return;
  __fill_param p = {loadbmp, x, y, draw_color, backcolor};
  __fill_util(&p);
}

// dealoca memoria retinuta pentru matricea de pixeli a
// imaginii de la `loadbmp'
void dealloc(bmp* loadbmp) {
  if (!loadbmp->bitmap) return;  // nimic de dealocat
  for (int i = 0; i < loadbmp->infoheader.height; ++i)
    if (loadbmp->bitmap[i]) free(loadbmp->bitmap[i]);
  free(loadbmp->bitmap);
  bmp empty = {};
  *loadbmp = empty;
}

// Sterea Stefan Octavian
// 314CB

