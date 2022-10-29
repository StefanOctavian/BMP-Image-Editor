#include <string.h>
#include <wordexp.h>
#include "utils.h"
#define DRAW_LINE_ARGS 5
#define DRAW_RECT_ARGS 5
#define DRAW_TRI_ARGS 7
#define FILL_ARGS 2

// functii handler pentru comenzile cli-ului
// parametrii sunt comuni:
// argc - numarul de argumente al comenzii
// argv - vectorul de argumente primite
// internalbmp - pointer la reprezentarea din memorie a imaginii din modul edit
// line_width - pointer la locatia de memorie unde memoram grosimea liniilor
// draw_color - pointer la reprezentarea din memorie a culorii de desenare

void handleSave(int argc, char** argv, bmp* internalbmp) {
  if (argc >= 1) save(internalbmp, argv[0]);
  else
    printf("== Path to file expected ==\n");
}

void handleEdit(int argc, char** argv, bmp* internalbmp) {
  if (argc >= 1) edit(internalbmp, argv[0]);
  else
    printf("== Path to file expected ==\n");
}

void handleInsert(int argc, char** argv, bmp* internalbmp) {
  if (argc >= 3) {
    char* path = argv[0];
    unsigned x = (unsigned)atoi(argv[1]);
    unsigned y = (unsigned)atoi(argv[2]);
    insert(internalbmp, path, x, y);
  } else {
    printf("== 3 arguments expected: path y x ==\n");
  }
}

void handleSet(int argc, char** argv, unsigned char* line_width,
bgr* draw_color) {
  if (argc < 1) {
    printf("=== Attribute (draw_color|line_width) expected ===\n");
    return;
  }
  char* attr = argv[0];
  if (strcmp(attr, "draw_color") == 0) {
    int argi = 1;
    if (argc >= argi + 1) draw_color->r = (unsigned char)atoi(argv[argi++]);
    if (argc >= argi + 1) draw_color->g = (unsigned char)atoi(argv[argi++]);
    if (argc >= argi + 1) draw_color->b = (unsigned char)atoi(argv[argi]);
  } else if (strcmp(attr, "line_width") == 0) {
    int argi = 1;
    if (argc >= argi + 1) {
      unsigned char width = (unsigned char)atoi(argv[argi]);
      if (width % 2 == 0) printf("=== line_width must be odd ===\n");
      else
        *line_width = width;
    }
  } else {
    printf("=== Unknown attribute %s. "
    "`draw_color' or `line_width' expected ===\n", attr);
  }
}

void handleDraw(int argc, char** argv, unsigned char line_width,
bgr draw_color, bmp* internalbmp) {
  if (argc < 1) {
    printf("=== Shape (line|rectangle|triangle) expected ===\n");
    return;
  }
  char* shape = argv[0];
  if (strcmp(shape, "line") == 0) {
    if (argc >= DRAW_LINE_ARGS) {
      int argi = 1;
      // x pentru coloane, y pentru linii conform reperului cartezian
      // x si y din cerinta problemei sunt invers.
      int x1 = atoi(argv[argi++]), y1 = atoi(argv[argi++]);
      int x2 = atoi(argv[argi++]), y2 = atoi(argv[argi]);
      draw_line(internalbmp, draw_color, line_width, x1, y1, x2, y2);
    } else {
      printf("=== 4 arguments after `line' expected ===\n");
    }
  } else if (strcmp(shape, "rectangle") == 0) {
    if (argc >= DRAW_RECT_ARGS) {
      int argi = 1;
      int x1 = atoi(argv[argi++]), y1 = atoi(argv[argi++]);
      int width = atoi(argv[argi++]), height = atoi(argv[argi]);
      draw_rect(internalbmp, draw_color, line_width, x1, y1,
      width, height);
    } else {
      printf("=== 4 arguments expected after `rectangle' ===\n");
    }
  } else if (strcmp(shape, "triangle") == 0) {
    if (argc >= DRAW_TRI_ARGS) {
      int argi = 1;
      int x1 = atoi(argv[argi++]), y1 = atoi(argv[argi++]);
      int x2 = atoi(argv[argi++]), y2 = atoi(argv[argi++]);
      int x3 = atoi(argv[argi++]), y3 = atoi(argv[argi]);
      draw_tri(internalbmp, draw_color, line_width,
      x1, y1, x2, y2, x3, y3);
    } else {
      printf("=== 6 arguments expected after `triangle' ===\n");
    }
  }
}

void handleFill(int argc, char** argv, bgr draw_color, bmp* internalbmp) {
  if (argc >= FILL_ARGS) {
    int argi = 0;
    int x = atoi(argv[argi++]), y = atoi(argv[argi]);
    fill(internalbmp, draw_color, x, y);
  } else {
    printf("=== 2 arguments expected ===\n");
  }
}

int main() {
  unsigned char quit = 0;
  bmp internalbmp = {};  // reprezentarea din memorie a imaginii din modul edit
  bgr draw_color = {};  // reprezentarea din memorie a culorii de desenat
  unsigned char line_width = 1;  // grosimea liniilor
  while (!quit) {
    char* command = NULL; size_t size = 0;
    ssize_t read = getline(&command, &size, stdin);
    // eliminam caracterul \n
    command[read - 1] = command[read];

    unsigned char hasArgs = 1;
    char* firstSpace = strchr(command, ' ');
    char* firstTab = strchr(command, '\t');
    char* commandEnd = NULL;

    // tratam si cazul in care primul argument este separat de comanda prin tab
    if (firstSpace && firstTab) {
      if (firstTab - command > firstSpace - command)
        commandEnd = firstSpace;
      else
        commandEnd = firstTab;
    } else if (firstTab) {
      commandEnd = firstTab;
    } else if (firstSpace) {
      commandEnd = firstSpace;
    }

    if (!commandEnd) {
      commandEnd = &command[read];
      hasArgs = 0;
    }

    // structura ce contine numarul si vectorul de argumente
    wordexp_t args = {};
    if (hasArgs) {
      // izolam doar comanda din `command'
      *commandEnd = '\0';
      // retinem si toate argumentele
      char* argstr = commandEnd + 1;
      // functie ce parseaza un sir de argumente in linia de comanda
      // al treilea argument al ei (0) inseamna modul normal
      wordexp(argstr, &args, 0);
    }
    // numarul de argumente primite gasite in argstr
    int argc = (int)args.we_wordc;
    // vectorul de argumente parsate din argstr
    char** argv = args.we_wordv;

    if (strcmp(command, "save") == 0) {
      handleSave(argc, argv, &internalbmp);
    } else if (strcmp(command, "edit") == 0) {
      handleEdit(argc, argv, &internalbmp);
    } else if (strcmp(command, "insert") == 0) {
      handleInsert(argc, argv, &internalbmp);
    } else if (strcmp(command, "set") == 0) {
      handleSet(argc, argv, &line_width, &draw_color);
    } else if (strcmp(command, "draw") == 0) {
      handleDraw(argc, argv, line_width, draw_color, &internalbmp);
    } else if (strcmp(command, "fill") == 0) {
      handleFill(argc, argv, draw_color, &internalbmp);
    } else if (strcmp(command, "quit") == 0 || feof(stdin)) {
      quit = 1; dealloc(&internalbmp);
    } else {
      printf("== Unknown command '%s' ==\n", command);
    }

    // wordexp aloca memorie dinamic si trebuie eliberata cu wordfree
    wordfree(&args);
    free(command);
  }
  return 0;
}

// Sterea Stefan Octavian
// 314CB

