//ADAMAKIS CHRISTOS 2148
//CHRISTODOULOU DIMITRIS 2113
//OMADA 2
// dhmiourgei n thread (dinetai san parametros apo to pliktrologio) pou ilopoioun
// tin mandel_Calc gia ka8e tmhma kai apo8ikeuoun to apotelesma
// gia na to sxediasei i main
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "mandelCore.h"
#include "binary_semaphores.h"

#define WinW 300
#define WinH 300
#define ZoomStepFactor 0.5
#define ZoomIterationFactor 2

static Display *dsp = NULL;
static unsigned long curC;
static Window win;
static GC gc;

// orismos struct gia na pername sta thread tis parametrous tis mandel_Calc
struct assignments{
	mandel_Pars *slices;
	int *res;
	int maxIterations;
	int num_of_worker;
	mybsem_t *mybsem;
	mybsem_t *mybsempaint;
};

volatile int number_run_work;//metablhth gia na kseroume posa thread trexoun akoma

/* basic win management rountines */

static void openDisplay() {
  if (dsp == NULL) {
    dsp = XOpenDisplay(NULL);
  }
}

static void closeDisplay() {
  if (dsp != NULL) {
    XCloseDisplay(dsp);
    dsp=NULL;
  }
}

void openWin(const char *title, int width, int height) {
  unsigned long blackC,whiteC;
  XSizeHints sh;
  XEvent evt;
  long evtmsk;

  whiteC = WhitePixel(dsp, DefaultScreen(dsp));
  blackC = BlackPixel(dsp, DefaultScreen(dsp));
  curC = blackC;

  win = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0, WinW, WinH, 0, blackC, whiteC);

  sh.flags=PSize|PMinSize|PMaxSize;
  sh.width=sh.min_width=sh.max_width=WinW;
  sh.height=sh.min_height=sh.max_height=WinH;
  XSetStandardProperties(dsp, win, title, title, None, NULL, 0, &sh);

  XSelectInput(dsp, win, StructureNotifyMask|KeyPressMask);
  XMapWindow(dsp, win);
  do {
    XWindowEvent(dsp, win, StructureNotifyMask, &evt);
  } while (evt.type != MapNotify);

  gc = XCreateGC(dsp, win, 0, NULL);

}

void closeWin() {
  XFreeGC(dsp, gc);
  XUnmapWindow(dsp, win);
  XDestroyWindow(dsp, win);
}

void flushDrawOps() {
  XFlush(dsp);
}

void clearWin() {
  XSetForeground(dsp, gc, WhitePixel(dsp, DefaultScreen(dsp)));
  XFillRectangle(dsp, win, gc, 0, 0, WinW, WinH);
  flushDrawOps();
  XSetForeground(dsp, gc, curC);
}

void drawPoint(int x, int y) {
  XDrawPoint(dsp, win, gc, x, WinH-y);
  flushDrawOps();
}

void getMouseCoords(int *x, int *y) {
  XEvent evt;

  XSelectInput(dsp, win, ButtonPressMask);
  do {
    XNextEvent(dsp, &evt);
  } while (evt.type != ButtonPress);
  *x=evt.xbutton.x; *y=evt.xbutton.y;
}

/* color stuff */

void setColor(char *name) {
  XColor clr1,clr2;

  if (!XAllocNamedColor(dsp, DefaultColormap(dsp, DefaultScreen(dsp)), name, &clr1, &clr2)) {
    printf("failed\n"); return;
  }
  XSetForeground(dsp, gc, clr1.pixel);
  curC = clr1.pixel;
}

char *pickColor(int v, int maxIterations) {
  static char cname[128];

  if (v == maxIterations) {
    return("black");
  }
  else {
    sprintf(cname,"rgb:%x/%x/%x",v%64,v%128,v%256);
    return(cname);
  }
}

// sinartisi worker pou kalei to ka8e thread
void *worker(void * arg){

	struct assignments *args;
	int num_of_worker;

	args = (struct assignments *) arg;
	num_of_worker=args->num_of_worker;

// 	elegxoume pote 8a anate8oun ta dedomena sto worker gia na upologisei tin mandel_Calc
	while(1){
		mybsem_down(args->mybsem);
		printf("starting slice nr. %d\n",num_of_worker+1);
		mandel_Calc(&args->slices[num_of_worker],args->maxIterations,&args->res[num_of_worker*args->slices[num_of_worker].imSteps*args->slices[num_of_worker].reSteps]);
		printf("done\n");
		number_run_work--;
 		mybsem_up(args->mybsempaint);
	}
}

int main(int argc, char *argv[]) {
  mandel_Pars pars,*slices;
	struct assignments *assignment;
	pthread_t *pthread;
	mybsem_t *mybsem, *mybsempaint;
  int i,j,x,y,nofslices,maxIterations,level,*res, check;
  int xoff,yoff;
  long double reEnd,imEnd,reCenter,imCenter;

  printf("\n");
  printf("This program starts by drawing the default Mandelbrot region\n");
  printf("When done, you can click with the mouse on an area of interest\n");
  printf("and the program will automatically zoom around this point\n");
  printf("\n");
  printf("Press enter to continue\n");
  getchar();

  pars.reSteps = WinW; /* never changes */
  pars.imSteps = WinH; /* never changes */

  /* default mandelbrot region */

  pars.reBeg = (long double) -2.0;
  reEnd = (long double) 1.0;
  pars.imBeg = (long double) -1.5;
  imEnd = (long double) 1.5;
  pars.reInc = (reEnd - pars.reBeg) / pars.reSteps;
  pars.imInc = (imEnd - pars.imBeg) / pars.imSteps;

  printf("enter max iterations (50): ");
  scanf("%d",&maxIterations);
  printf("enter no of slices: ");
  scanf("%d",&nofslices);


  /* adjust slices to divide win height */

  while (WinH % nofslices != 0) { nofslices++;}

  /* allocate slice parameter and result arrays */

	slices = (mandel_Pars *) malloc(sizeof(mandel_Pars)*nofslices);
	res = (int *) malloc(sizeof(int)*pars.reSteps*pars.imSteps);
	pthread= (pthread_t *) malloc(sizeof(pthread_t)*nofslices);
	assignment = (struct assignments*)malloc(sizeof(struct assignments)*nofslices);
	mybsem = (mybsem_t*)malloc(sizeof(mybsem_t)*nofslices);
	mybsempaint = (mybsem_t*)malloc(sizeof(mybsem_t)*nofslices);

//   dhmiourgeia twn thread mesw tis pthread_create kai arxikopoihsh tou check_worker se 2
//   gia na gnwrizoume oti den ektelei kapoia ergasia(upologismou 'i sxediashs )
//   se ka8e thread episis pername mesw twn parametrwn kai ton aukson ari8mo tou
	for(i=0; i<nofslices; i++){
		mybsem_init(&mybsem[i], 0);
		mybsem_init(&mybsempaint[i], 0);
		assignment[i].num_of_worker = i;
		assignment[i].mybsem = &mybsem[i];
		assignment[i].mybsempaint = &mybsempaint[i];
		check = pthread_create(&pthread[i],NULL,worker, &assignment[i]);
		if (check){
			printf("error pthread %d\n", i);
		}
	}

  /* open window for drawing results */

  openDisplay();
  openWin(argv[0], WinW, WinH);

  level = 1;

  while (1) {
// 	  se ka8e epanalipsi arxikopoioume to number_run_work gia na gnwrizoume pote ola ta thread teleiwsan
// 	  gia na sinexisei i main
	number_run_work=nofslices;

    clearWin();

    mandel_Slice(&pars,nofslices,slices);

    y=0;
// 	ana8esi se ka8e epanalipsi timwn stis parametrous tou i thread
    for (i=0; i<nofslices; i++) {
		//i = counter_worker
		assignment[i].slices= slices;
		assignment[i].maxIterations = maxIterations;
		assignment[i].res = res;
	}
// 	afou ana8esoume se ola ta thread times tote allazoume to check_worker se 1 gia na ksekinisoun oi worker

	for (i=0; i<nofslices; i++){
		mybsem_up(&mybsem[i]);
	}

// 	oso iparxoun worker pou den exoun sxediastei ta apotelesmata tous
	for (i=0; i<nofslices; i++){
 		mybsem_down(&mybsempaint[i]);
		for (j=0; j<slices[i].imSteps; j++) {
			for (x=0; x<slices[i].reSteps; x++) {
				setColor(pickColor(res[y*slices[i].reSteps+x],maxIterations));
				drawPoint(x,y);
			}
			y++;
		}
	}


    /* get next focus/zoom point */

    getMouseCoords(&x,&y);
    xoff = x;
    yoff = WinH-y;

    /* adjust region and zoom factor  */

    reCenter = pars.reBeg + xoff*pars.reInc;
    imCenter = pars.imBeg + yoff*pars.imInc;
    pars.reInc = pars.reInc*ZoomStepFactor;
    pars.imInc = pars.imInc*ZoomStepFactor;
    pars.reBeg = reCenter - (WinW/2)*pars.reInc;
    pars.imBeg = imCenter - (WinH/2)*pars.imInc;

    maxIterations = maxIterations*ZoomIterationFactor;
    level++;

  }

  /* never reach this point; for cosmetic reasons */

  for(i=0; i<nofslices; i++){
		mybsem_destroy(&mybsem[i]);
		mybsem_destroy(&mybsempaint[i]);
  }

  free(slices);
  free(res);
  free(assignment);
  free(pthread);

  closeWin();
  closeDisplay();

}
