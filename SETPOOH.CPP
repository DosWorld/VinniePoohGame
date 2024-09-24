#include  <conio.h>
#include  <stdlib.h>
#include  <bios.h>
#include  <alloc.h>
#include  <string.h>
#include  <time.h>
#include  <dos.h> // PATCH: missing header file for delay()
#include  <io.h> // PATCH: access()

#include "famegraf.h"
//#include "mouse.h"
#include "joy.h"
#include "keys.h"

void far mz(unsigned char butt);

//Joy J;
//Mouse m;
//Keys k = Keys(mz);

long tic;
int end, left, right, up, down, jump, fire;

int dev = 255, mix = 8000, vol = 255, stat;
int pos = 0;

#pragma pack(push, 1)
struct {
  short dev, mix, vol;
  short j;  //исп джойстик или нет
  short f;  //скорость вывода на экран
  short cheat;
} setup;
#pragma pack(pop)

static char *tmenu[] = {
  " Компьютер",
  " Звуковое устройство",
  " Детализация",
  " Управление",
  " Выход + запись"
};

static char cmenu[] = {0, 6, 0, 0};

static char mmenu[] = {5, 7, 4, 2};

static char *typ_cpu[] = {
  "PC AT 12 Mhz ",
  "386 SX 33 Mhz ",
  "386 DX 40 Mhz ",
  "486 DX2 50 Mhz ",
  "Pentium 100 Mhz "
};

static char *typ_speed[] = {
  "Нет ",
  "Плохая ",
  "Хорошая ",
  "Отличная "
};

static char *typ_control[] = {
  "Клавиатура ",
  "1й джойстик ",
  "Клавиатура (Нет джойстика) ",
};

static char *typ_sound[] = {
  "Внутренний динамик ",
  "COVOX в LPT1 ",
  "COVOX в LPT2 ",
  "COVOX в LPT3 ",
  "Sound Blaster (220h) ",
  "Sound Blaster (240h) ",
  "Без звука "
};

static char cod_sound[] = {0, 1, 2, 3, 7, 8, 255};

screen bckg; //задний фон
screen hidscr; //скрытая страничка

void fatalerror(char *t) {
  // выход по фатальной ошибке
  SetLib("");
  CloseGraph();
  kb_off();
  puts(t);
  exit(1);
}

void far mz(unsigned char butt) {
  // обработка прерывания по клавиатуре
  if (butt == 1) //Esc
    end = 1;
  if (butt == 77)
    right=1;
  if (butt == 75)
    left = 1;
  if (butt == 72)
    up = 1;
  if ( butt == 80)
    down = 1;
  if ((butt == 56) || (butt == 57) || (butt == 28)) //alt spc entr
    fire = 1;
  if (butt == 29) //ctrl
    jump = 1;
}

#define nmenx 18
#define nmeny 5 //80
void showem(void) {
int i;
  // меню
  PutImg(0, 0, 320, 200, (block) bckg);
  WPut(0/*nmenx-5*/, nmeny - 5, 320/*294*/, 8 * 5 + 16);
  for (i = 0; i < 5; ++i) {
    char_fgd = (i == pos) ? 17 : 31;
    char_bkgd = 0;
    MoveXY(nmenx, nmeny + 1 + (9 * i));
    vputs(tmenu[i]);
    if (i < 4) {
      vputs(" : ");
    }
    switch (i) {
    case 0:
      vputs(typ_cpu[cmenu[i]]);
      break;

    case 1:
      vputs(typ_sound[cmenu[i]]);
      break;

    case 2:
      vputs(typ_speed[cmenu[i]]);
      break;

    case 3:
      vputs(typ_control[cmenu[i]]);
      break;
    }
  }
  CopyToScreen(hidscr);
}

void main(int argc, char *argv[]) {
unsigned long sz;
//char t[10] = "y";
  // PATCH: check for library files
  if ( access("GRAPH.FML", 6) ) {
    fatalerror("! Missing one or more resource files !");
  }
  kb_set(mz);
  kb_on();
  js_init();

  SetLib("graph");

  InitGraph();
  GetPalette(palette);
  PaletteOff(palette);
  Vga256();
  GetLib("normal.fit", fonts);
  PutFont(fonts);
  GetLib("normal.col", palette);
  _fmemset(palette1, 0, 768);
  PutPalette(palette1);

  sz = SizeLib("reclama.pic");
  bckg = (block) famemalloc(sz);
  GetLib("reclama.pic", (block) bckg);
  hidscr = (block) famemalloc(sz);

  SetScreen(hidscr);
  Clip(0, 0, 319, 199);

  pos = end = left = right = up = down = jump = fire = 0;

  // PATCH: read settings
  if (GetLib("pooh.cfg", (block) &setup) == 0) {
    setup.dev = dev;
    setup.mix = mix;
  } else {
    // load settings
    switch (setup.mix) {
      case 5000:
        cmenu[0] = 1;
        break;
      case 10000:
        cmenu[0] = 2;
        break;
      case 16000:
        cmenu[0] = 3;
        break;
      case 22000:
        cmenu[0] = 4;
        break;
      default:
        cmenu[0] = 0;
        break;
    }
    switch (setup.dev) {
      case 0:
      case 1:
      case 2:
      case 3:
        cmenu[1] = setup.dev;
        break;
      case 7:
        cmenu[1] = 4;
        break;
      case 8:
        cmenu[1] = 5;
        break;
      default:
        cmenu[1] = 6;
        break;
    }
    cmenu[2] = setup.f & 3;
    cmenu[3] = setup.j ? 1 : 0;
  }
  setup.cheat = 0; // PATCH: disable by default
  setup.vol = vol;

  showem();
  PaletteOn(palette);

  main_menu: showem();
  jump = fire = 0;
  while ((fire == 0) && (jump == 0)) {
    tic = gettic() + 2;
    while (gettic() < tic) {
      delay(100);
    }

    if (up || left) {
      up = left = 0;
      if (--pos < 0) {
        pos = 4;
      }
      showem();
    }
    if (down || right) {
      down = right = 0;
      if (++pos > 4) {
        pos = 0;
      }
      showem();
    }
    if (end == 1) { //выход по еску
      fire = pos = 4;
      showem();
    }
  }
  switch (pos) {
  case 0:
    cmenu[pos]++;
    if (cmenu[pos] >= mmenu[pos]) {
      cmenu[pos] = 0;
    }
    switch (cmenu[pos]) {
    case 0:
      if (cmenu[2] > 1) {
        cmenu[2] = 1;
      }
      setup.mix = 0;
      cmenu[1] = 6;
      break;
    case 1:
      if (cmenu[2] > 2) {
        cmenu[2] = 2;
      }
      setup.mix = 5000;
      break;
    case 2:
      if (cmenu[2] > 2) {
        cmenu[2] = 2;
      }
      setup.mix = 10000;
      break;
    case 3:
      cmenu[2] = 3;
      setup.mix = 16000;
      break;
    case 4:
      cmenu[2] = 3;
      setup.mix = 22000;
      break;
    }
    break;
  case 1:
    cmenu[pos]++;
    if (cmenu[pos] >= mmenu[pos]) {
      cmenu[pos] = 0;
    }
    break;
  case 2:
    cmenu[pos]++;
    if (cmenu[pos] >= mmenu[pos]) {
      cmenu[pos] = 0;
    }
    break;

  case 3: //джойстик
    if (/*J.present*/js_state()) {
      cmenu[pos]++;
      if (cmenu[pos] >= mmenu[pos]) {
        cmenu[pos] = 0;
      }
    } else {
      cmenu[pos] = 2;
    }
    break;

  case 4:
    if (end) {
      fatalerror("Изменений нет");
    }

    //выход в дос с записью
    if ((argc >= 2) && (stricmp(argv[1], "cheat") == 0)) {
      setup.cheat = 1;
      //puts("(c) 1995 FaMe by BMV");
    }

    setup.dev = cod_sound[cmenu[1]];
    // исп джойстик или нет
    setup.j = (cmenu[3] == 1) ? 1 : 0;
    // скорость вывода на экран
    setup.f = cmenu[2];

    PutLib("pooh.cfg", (block) &setup, sizeof(setup));

    fatalerror("Установка закончена. Спасибо за покупку!\n"
        "Для запуска игры наберите POOH и нажмите ENTER\n");

  }

  goto main_menu;
}
