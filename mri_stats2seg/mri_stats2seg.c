
/*--------------------------------------------------------------
Example Usage:

set subject = fsr-tst

mri_segstats \
  --in  $SUBJECTS_DIR/$subject/mri/norm.mgz \
  --seg $SUBJECTS_DIR/$subject/mri/aseg.mgz \
  --ctab-default \
  --avgwfvol stats.mgh --avgwf stats.txt \
  --sum sum.txt

./mri_stats2seg --stat stats.mgh \
  --seg $SUBJECTS_DIR/$subject/mri/aseg.mgz \
  --o asegstats.mgh

tkmedit $subject norm.mgz -aux ./asegstats.mgh\
  -segmentation $SUBJECTS_DIR/$subject/mri/aseg.mgz \
      $FREESURFER_HOME/FreeSurferColorLUT.txt
--------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
double round(double x);
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "macros.h"
#include "utils.h"
#include "mrisurf.h"
#include "mrisutils.h"
#include "error.h"
#include "diag.h"
#include "mri.h"
#include "mri2.h"
#include "fio.h"
#include "version.h"
#include "label.h"
#include "matrix.h"
#include "annotation.h"
#include "fmriutils.h"
#include "cmdargs.h"
#include "fsglm.h"
#include "pdf.h"
#include "fsgdf.h"
#include "timer.h"
#include "matfile.h"
#include "volcluster.h"
#include "surfcluster.h"
#include "fsenv.h"

int LoadDavidsTable(char *fname, int **pplutindex, double **pplog10p);

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void dump_options(FILE *fp);
int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_stats2seg.c,v 1.5 2006/08/08 22:33:28 greve Exp $";
char *Progname = NULL;
char *cmdline, cwd[2000];
int debug=0;
int checkoptsonly=0;
struct utsname uts;

char *TempVolFile=NULL;
char *subject, *hemi, *SUBJECTS_DIR;

char *statfile=NULL;
char *segfile=NULL;
MRI *seg;

char *outfile=NULL;
MRI *out;
int nitems=0;
int *lutindex;
double *log10p;


/*---------------------------------------------------------------*/
int main(int argc, char *argv[])
{
  int nargs,r,c,s,n,segid;
  double val;

  nargs = handle_version_option (argc, argv, vcid, "$Name:  $");
  if (nargs && argc - nargs == 1) exit (0);
  argc -= nargs;
  cmdline = argv2cmdline(argc,argv);
  uname(&uts);
  getcwd(cwd,2000);

  Progname = argv[0] ;
  argc --;
  argv++;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;
  if(argc == 0) usage_exit();
  parse_commandline(argc, argv);
  check_options();
  if(checkoptsonly) return(0);
  dump_options(stdout);

  SUBJECTS_DIR = getenv("SUBJECTS_DIR");
  if(SUBJECTS_DIR == NULL){
    printf("ERROR: SUBJECTS_DIR not defined in environment\n");
    exit(1);
  }

  seg = MRIread(segfile);
  if(seg == NULL) exit(1);

  nitems = LoadDavidsTable(statfile, &lutindex, &log10p);
  if(nitems == 0){
    printf("ERROR: could not find any items in %s\n",statfile);
    exit(1);
  }

  out = MRIallocSequence(seg->width,seg->height,seg->depth,MRI_FLOAT,1);
  MRIcopyHeader(seg,out);

  for(c=0; c < seg->width; c++){
    printf("%3d ",c); 
    if(c%20 == 19) printf("\n");
    fflush(stdout);
    for(r=0; r < seg->height; r++){
      for(s=0; s < seg->depth; s++){
	segid = MRIgetVoxVal(seg,c,r,s,0);
	val = 0;
	if(segid != 0){
	  for(n=0; n < nitems; n++){
	    if(lutindex[n] == segid){
	      val = log10p[n];
	      break;
	    }
	  }
	}
	MRIsetVoxVal(out,c,r,s,0,val);
      }
    }
  }
  printf("\n");
  MRIwrite(out,outfile);

  printf("mri_stats2seg done\n");
  return 0;
}
/* --------------------------------------------- */
static int parse_commandline(int argc, char **argv)
{
  int  nargc , nargsused;
  char **pargv, *option ;

  if(argc < 1) usage_exit();

  nargc   = argc;
  pargv = argv;
  while(nargc > 0){

    option = pargv[0];
    if(debug) printf("%d %s\n",nargc,option);
    nargc -= 1;
    pargv += 1;

    nargsused = 0;

    if (!strcasecmp(option, "--help"))  print_help() ;
    else if (!strcasecmp(option, "--version")) print_version() ;
    else if (!strcasecmp(option, "--debug"))   debug = 1;
    else if (!strcasecmp(option, "--checkopts"))   checkoptsonly = 1;
    else if (!strcasecmp(option, "--nocheckopts")) checkoptsonly = 0;

    else if (!strcasecmp(option, "--stat")){
      if(nargc < 1) CMDargNErr(option,1);
      statfile = pargv[0];
      nargsused = 1;
    }
    else if (!strcasecmp(option, "--seg")){
      if(nargc < 1) CMDargNErr(option,1);
      segfile = pargv[0];
      nargsused = 1;
    }
    else if (!strcasecmp(option, "--o")){
      if(nargc < 1) CMDargNErr(option,1);
      outfile = pargv[0];
      nargsused = 1;
    }
    else{
      fprintf(stderr,"ERROR: Option %s unknown\n",option);
      if(CMDsingleDash(option))
	fprintf(stderr,"       Did you really mean -%s ?\n",option);
      exit(-1);
    }
    nargc -= nargsused;
    pargv += nargsused;
  }
  return(0);
}
/* ------------------------------------------------------ */
static void usage_exit(void)
{
  print_usage() ;
  exit(1) ;
}
/* --------------------------------------------- */
static void print_usage(void)
{
  printf("USAGE: %s \n",Progname) ;
  printf("\n");
  printf("   --stat mristat : stat file in an mri format\n");
  //printf("   --stat-txt stat.txt : text stat file \n");
  printf("   --seg segvol \n");
  printf("   --o out\n");
  printf("\n");
  printf("\n");
  printf("   --debug     turn on debugging\n");
  printf("   --checkopts don't run anything, just check options and exit\n");
  printf("   --help      print out information on how to use this program\n");
  printf("   --version   print out version and exit\n");
  printf("\n");
  printf("%s\n", vcid) ;
  printf("\n");
}
/* --------------------------------------------- */
static void print_help(void)
{
  print_usage() ;
  printf("WARNING: this program is not yet tested!\n");
  exit(1) ;
}
/* --------------------------------------------- */
static void print_version(void)
{
  printf("%s\n", vcid) ;
  exit(1) ;
}
/* --------------------------------------------- */
static void check_options(void)
{
  if(statfile == NULL){
    printf("ERROR: need to specify a stat file\n");
    exit(1);
  }
  if(segfile == NULL){
    printf("ERROR: need to specify a seg file\n");
    exit(1);
  }
  if(outfile == NULL){
    printf("ERROR: need to specify an out file\n");
    exit(1);
  }
  return;
}

/* --------------------------------------------- */
static void dump_options(FILE *fp)
{
  fprintf(fp,"\n");
  fprintf(fp,"%s\n",vcid);
  fprintf(fp,"cwd %s\n",cwd);
  fprintf(fp,"cmdline %s\n",cmdline);
  fprintf(fp,"sysname  %s\n",uts.sysname);
  fprintf(fp,"hostname %s\n",uts.nodename);
  fprintf(fp,"machine  %s\n",uts.machine);
  fprintf(fp,"user     %s\n",VERuser());

  return;
}

/*---------------------------------------------------------------------
  Reads in data created by Statview.
  1. Looks for a line where the first string is "Unpaired". 
  2. Gets the last string from this line.
  3. Removes the last 4 chars from this string to get the
     segmentation name (this could be a prob?)
  4. Finds the segmentation name in the color table to get the index
     This is returned in pplutindex.
  5. Scrolls thru the file until it finds a line where the first
     string is "Mean".
  6. Goes one more line
  7. Loads the last value from this line (this is the p-value)
  8. Removes any less-than signs (ie, "<")
  9. Computes -log10 of this value (returns in pplog10p)

  Example:

   Unpaired t-test for Left-Inf-Lat-Vent_vol
   Grouping Variable: Dx
   Hypothesized Difference = 0
   Inclusion criteria: AGE > 60 from wmparc_vals_hypotest_log (imported)
           Mean Diff.      DF      t-Value P-Value
   Dementia, Nondemented   505.029 164     4.136   <.0001

  The segmentation name would be: Left-Inf-Lat-Vent, the p value
  would be .0001 (the -log10 of which would be 4.0).

  ---------------------------------------------------------------------*/

int LoadDavidsTable(char *fname, int **pplutindex, double **pplog10p)
{
  FSENV *fsenv;
  int err,tmpindex[1000];
  double tmpp[1000];
  char tmpstr[2000],tmpstr2[2000],segname[2000];
  FILE *fp;
  double p;
  int n,segindex,nitems;
  char *item;

  fsenv = FSENVgetenv();
  fp = fopen(fname,"r");
  if(fp == NULL) {
    printf("ERROR: could not open%s\n",fname);
    exit(1);
  }

  nitems = 0;
  while(1){
    if(fgets(tmpstr,2000-1,fp) == NULL) break;
    memset(tmpstr2,'\0',2000);
    sscanf(tmpstr,"%s",tmpstr2);
    if(strcmp(tmpstr2,"Unpaired") == 0){
      item = gdfGetNthItemFromString(tmpstr,-1); // get last item
      sscanf(item,"%s",segname);
      free(item);
      // strip off _vol
      for(n=strlen(segname)-4;n<strlen(segname);n++) segname[n]='\0';
      err = CTABfindName(fsenv->ctab, segname, &segindex);
      if(segindex < 0){
	printf("ERROR: reading %s, cannot find %s in color table\n",
	       fname,segname);
	printf("%s",tmpstr);
	printf("item = %s\n",item);
	exit(1);
      }
      n=0;
      while(1){
	fgets(tmpstr,2000-1,fp);
	sscanf(tmpstr,"%s",tmpstr2);
	if(strcmp(tmpstr2,"Mean") == 0) break;
	n++;
	if(n > 1000){
	  printf("There seems to be an error finding key string 'Mean'\n");
	  exit(1);
	}
      }
      fgets(tmpstr,2000-1,fp);
      item = gdfGetNthItemFromString(tmpstr,-1);
      // remove less-than signs
      for(n=0; n < strlen(item); n++) if(item[n] == '<') item[n] = '0';
      sscanf(item,"%lf",&p);
      printf("%2d %2d %s %lf  %lf\n",nitems,segindex,segname,p,-log10(p));
      tmpindex[nitems] = segindex;
      tmpp[nitems] = p;
      nitems++;
      free(item);
    }
  }

  *pplutindex =    (int *) calloc(nitems,sizeof(int));
  *pplog10p   = (double *) calloc(nitems,sizeof(double));

  for(n=0; n < nitems; n++){
    (*pplutindex)[n] = tmpindex[n];
    (*pplog10p)[n]   = -log10(tmpp[n]);
  }


  FSENVfree(&fsenv);
  return(nitems);
}
