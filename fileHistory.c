//----------------------------------------------------------------------------

#include "crsUtils.h"

#include <sys/inotify.h> // file system notifications (Linux specific)

bool useCompression=false;
char watchDir[0x100]="";
char historyDir[0x100]="";

double // time stamp (seconds) of last copy or -1.0 if none
lookForLastCopy(char *lastCopy, const char *fileName)
{
  (void)lastCopy; // avoid ``unused parameter'' warning
  (void)fileName; // avoid ``unused parameter'' warning
  double lastTimeStamp=-1.0;
  //char lastCopy[0x100]="";
  int prefixLen=strlen(fileName);
  double suffix;
  
  // ...
// = ...
// 
// if((!strncmp(elementName,fileName,prefixLen))&& // same prefix
// (sscanf(elementName+prefixLen,"_%lg",&suffix)==1)&& // suitable suffix
// (suffix>lastTimeStamp)) // last copy
// {
// lastTimeStamp=suffix;
// strcpy(lastCopy,elementName);
// }
  //
  // ... À COMPLÉTER {E-1} ...
  //
  // Parcourir le contenu de ``historyDir''.
  // Pour chaque élément, s'il est constitué de ``fileName'', du caractère
  // souligné ``_'' et d'un réel, et si ce réel est supérieur à
  // ``lastTimeStamp'', alors il s'agit de la dernière copie de ``fileName''.
  // Il faut alors mettre à jour ``lastTimeStamp'' et ``lastCopy''.
  //
  DIR* histRep = NULL;
  struct dirent* fichierBackup = NULL;
  histRep = opendir(historyDir);
  
  if(!histRep) {
    perror("ouverture repertoire principal");
    exit(1);
  }
  
   while ((fichierBackup = readdir(histRep)) != NULL) {
     const char *elementName = fichierBackup->d_name;
     if((!strncmp(elementName,fileName,prefixLen)) && (sscanf(elementName+prefixLen,"_%lg",&suffix)==1) && (suffix>lastTimeStamp)){
	lastTimeStamp=suffix;
	strcpy(lastCopy,elementName);
     }
//      printf("Notre fichier dans repertoire historique : %s\n",fichierBackup->d_name);
   }
  
  if(closedir(histRep) == -1){
    perror("Il y a eu un probleme lors de la fermeture du repertoire");
    exit(1);
  }

printf("Le timeStamp de sortie après vérification est : %f\n", lastTimeStamp);
return lastTimeStamp;
}

bool
identicalCopy(const char *lastPath,
              const char *path)
{
(void)lastPath; // avoid ``unused parameter'' warning
(void)path; // avoid ``unused parameter'' warning
if(useCompression)
  {
  //
  // ... À COMPLÉTER {F-2} ...
  //
  // Creer un tube de communication.
  // Recouvrir un premier processus enfant par la ligne de commande :
  //   { "gunzip" , NULL }
  // Son entrée standard devra être redirigée depuis ``lastPath''.
  // Sa sortie standard devra être redirigée vers le côté écriture du tube.
  // Recouvrir un second processus enfant par la ligne de commande :
  //   { "cmp" , "-s" , (char *)path , NULL }
  // Son entrée standard devra être redirigée depuis le côté lecture du tube.
  // Fermer les deux extrémités du tube.
  // Attendre la terminaison des deux processus ; si le résultat du second
  // est nul, les fichiers sont identiques, on renvoie ``true''.
  //
  int input = open(lastPath,O_RDONLY);
  int status, r, fd[2];
  pid_t pid, child;
 
  pipe(fd); /* creer le tube */
  if(r==-1) { 
    perror("pipe"); return -1;
    exit(1);
  }  
  
  pid_t result = fork();
  
  switch(result) {
    case -1 : perror("Probleme avec le fork generateBackUp");
      break;
    case 0 :
      if(dup2(input, STDIN_FILENO) == -1){
	perror("dup2 entrée standart error");
	exit(1);
      }
      
      close(input);
      
      char* arg[] = { "gunzip", NULL};
      int retour = execvp(arg[0], arg);
      return retour;
      break;
    default:
      child = fork(); /* creer un processus enfant */
      switch(child)
      {
	case -1: perror("fork"); return -1;
	case 0: /*---- dans le processus enfant ----*/
	  close(fd[1]);
	  writeFully(fd[0], lastPath);
	  close(fd[0]); 
	  char* arg[]={ "cmp" , "-s" , (char *)path , (char *)lastPath, NULL };
	  int retour = execvp(arg[0], arg);
	  perror("execvp");
	  exit(1);
	default: /*---- dans le processus parent ----*/
	  waitpid(child,(int *)0,0); /* attente du processus enfant */
	  if(r==-1) { perror("waitpid"); return -1; }
      }


      do { pid=waitpid(result,&status,0); } while((pid==-1)&&(errno==EINTR));
      if(WIFEXITED(status)) {
	printf("On a bien fini le zippage\n");
      }
      else if(WEXITSTATUS(status)) {
	  printf("WEXITSTATUS retourne 1\n") ;
      }
      else
	fprintf(stderr," Le fichier n'a ?\n");
      }

  }
else
  {
  //
  // ... À COMPLÉTER {E-2} ...
  //
  // Recouvrir un processus enfant par la ligne de commande :
  //   { "cmp" , "-s" , (char *)path , (char *)lastPath, NULL }
  // Attendre sa terminaison ; si le résultat est nul, les fichiers sont
  // identiques, on renvoie ``true''.
  //
  pid_t result = fork();

  if(result == -1){
    perror("probleme avec le fork comparaison");
    exit(1);
  }else if(!result) {
    printf("Nous sommes dans l'enfant !!! hahaha");
    char* arg[]={ "cmp" , "-s" , (char *)path , (char *)lastPath, NULL };
    int retour = execvp(arg[0], arg);
    perror("execvp");
    exit(1);
  }
  
  int status;
  pid_t pid;
  do { pid=waitpid(result,&status,0); } while((pid==-1)&&(errno==EINTR));
  //fprintf(stderr,"After waitpid() : %d\n",pid);
//   printf("WEXITSTATUS(status) : %d\n", WEXITSTATUS(status));
  if(!WIFEXITED(status)) {
    printf("Heuuu quelque chose de bizarre s'est produit");
  }
  else if(WEXITSTATUS(status) == 0) {
      printf("Ils sont identiques !!!!!!!!\n") ;
      return true;
  }
  else
    fprintf(stderr," Le fichier n'a ?\n");
  }
return false;
}

void
generateBackup(const char *destPath,
               const char *srcPath)
{
(void)destPath; // avoid ``unused parameter'' warning
(void)srcPath; // avoid ``unused parameter'' warning
if(useCompression)
  {
  //
  // ... À COMPLÉTER {F-1} ...
  //
  // Recouvrir un processus enfant par la ligne de commande :
  //   { "gzip" , NULL }
  // Son entrée standard devra être redirigée depuis ``srcPath''.
  // Sa sortie standard devra être redirigée vers ``destPath''.
  // Attendre sa terminaison.
  //
  
  int input = open(srcPath,O_RDONLY);
  int output = open(destPath,O_WRONLY|O_TRUNC|O_CREAT,0666);
  int status;
  pid_t pid;
  pid_t result = fork();
  
  switch(result) {
    case -1 : perror("Probleme avec le fork generateBackUp");
      break;
    case 0 :
      if(dup2(input, STDIN_FILENO) == -1){
	perror("dup2 entrée standart error");
	exit(1);
      }
      if(dup2(output, STDOUT_FILENO) == -1){
	perror("dup2 sortie standart error");
	exit(1);
      }
      close(input);
      close(output);
      
      char* arg[] = { "gzip", NULL};
      int retour = execvp(arg[0], arg);
      return retour;
      break;
    default:
      do { pid=waitpid(result,&status,0); } while((pid==-1)&&(errno==EINTR));
      if(WIFEXITED(status)) {
	printf("On a bien fini le zippage\n");
      }
      else if(WEXITSTATUS(status)) {
	  printf("WEXITSTATUS retourne 1\n") ;
      }
      else
	fprintf(stderr," Le fichier n'a ?\n");
      }
  }
else
  {
    //
    // ... À COMPLÉTER {D-3} ...
    //
    // Ouvrir les fichiers ``srcPath'' en lecture et ``destPath'' en écriture.
    // Faire une boucle qui recopie des blocs d'octets de l'un vers l'autre.
    // Fermer les deux fichiers.
    //
    int input=open(srcPath,O_RDONLY);
    int output=open(destPath,O_WRONLY|O_TRUNC|O_CREAT,0666);
    
    if(input == -1){
      perror("Probleme avec le input srcPath");
      return;
    }
    if(output == -1){
      perror("Probleme avec le output destPath");
      exit(1);
    }
    
    char evtBuf[0x400];
    int r=0;

    while((r = read(input, evtBuf, sizeof(evtBuf)))) {
      if(r == -1){
	perror("Probleme avec le read backup");
	exit(1);
      }
      if(write(output, evtBuf, r) == -1){
	perror("Probleme lors de l'ecriture backup");
	exit(1);
      }
    }
    
    close(input);
    close(output);
  }
}

void
updateFile(const char *fileName)
{
//---- join watch directory and file name ----
char path[0x100];
sprintf(path,"%s/%s",watchDir,fileName);
printf("updating %s\n",path);

//---- ensure it is a regular file ----
//
// ... À COMPLÉTER {D-1} ...
//
// Obtenir les propriétés de ``path'' et vérifier qu'il s'agit d'un fichier.
// Quitter la fonction sinon.
//
struct stat spth;
if(stat(path, &spth) == -1){
  perror("probleme lors du stat path");
  exit(1);
}

if(!S_ISREG(spth.st_mode)) {
  printf("ce n'est pas un fichier ordinaire\n");
  return;
}

//---- look for identical last copy ----
char lastCopy[0x100]="";
double lastTimeStamp=lookForLastCopy(lastCopy,fileName);
if(lastTimeStamp>=0.0) // last copy detected
  {
  char lastPath[0x100];
  sprintf(lastPath,"%s/%s",historyDir,lastCopy);
  if(identicalCopy(lastPath,path)) // identical to current file
    {
    printf("%s and %s are identical\n",path,lastPath);
    return; // no new copy necessary --> stop here
    }
  }

//---- generate a time stamp ----
double timeStamp=0.0;
//
// ... À COMPLÉTER {D-2} ...
//
// Obtenir la date courante et initialiser ``timeStamp''
// (secondes avec des décimales).
//
struct timeval tv;
if(gettimeofday(&tv, NULL) == -1){
  perror("probleme lors du gettimeofDay");
  exit(1);
}
printf("TimeStamp : %f\n",timeStamp = (tv.tv_sec + tv.tv_usec * 0.000001) );

//---- copy file content to a time stamped backup ----
char copyPath[0x100];
sprintf(copyPath,"%s/%s_%.6f",historyDir,fileName,timeStamp);
fprintf(stderr,"copying %s to %s\n",path,copyPath);
generateBackup(copyPath,path);
}

void
initialUpdate(void)
{
  //
  // ... À COMPLÉTER {B-1} ...
  //
  // Parcourir le contenu de ``watchDir''.
  // Pour chaque élément, appeler ``updateFile()''.
  //
  DIR* rep = NULL;
  struct dirent* fichierLu = NULL;
  
  rep = opendir(watchDir);
  
  if(!rep) {
    perror("ouverture repertoire principal");
    exit(1);
  }
  
  while ((fichierLu = readdir(rep)) != NULL)
    updateFile(fichierLu->d_name);
  
  if(closedir(rep) == -1){
    perror("Il y a eu un probleme lors de la fermeture du repertoire");
    exit(1);
  }
}

int
main(int argc,
     char **argv)
{
//---- check command line arguments ----
for(int i=1;i<argc;++i)
  {
  if(!strcmp(argv[i],"-c"))
    {
    useCompression=!useCompression;
    }
  else
    {
    strcpy(watchDir,argv[i]);
    int len=strlen(watchDir);
    while(len&&(watchDir[len-1]=='/'))
      {
      watchDir[--len]='\0'; // remove trailing '/'
      }
    }
  }
if(!watchDir[0])
  { fprintf(stderr,"usage: %s directory\n",argv[0]); exit(1); }


//---- check watch directory ----
//
// ... À COMPLÉTER {A-1} ...
//
// Obtenir les propriétés de ``watchDir'' et vérifier qu'il s'agit d'un
// répertoire.
//
struct stat sb;
if(stat(watchDir, &sb) == -1) {
  perror("Il y a eu un probleme lors du stat de watchDir");
  exit(1);
}

if(S_ISDIR(sb.st_mode)){
  printf("Le répertoire a été trouvé ! \n");
}else{
  printf("Etes vous bien sur d'avoir passé un repertoire en paramètre ? \n");
  exit(1);
}
  

//---- check or create history directory ----
sprintf(historyDir,"%s_history",watchDir);
//
// ... À COMPLÉTER {A-2} ...
//
// Obtenir les propriétés de ``historyDir'' et vérifier qu'il s'agit d'un
// répertoire.
// Si ce chemin est inexistant, il suffit de créer ce répertoire.
//

struct stat htD;
if(stat(historyDir, &htD) == -1) {
  if(mkdir(historyDir, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
    perror("mkdir historyDir");
    exit(1);
  }
}else{
  if(S_ISDIR(htD.st_mode)){
    printf("Le répertoire a été trouvé ! \n");
  }else{
    printf("Il y a eu un probleme ! ? \n");
    exit(1);
  }
}


  

//---- initial update ----
printf("maintaining a%s history of %s in %s\n",
       useCompression ? " compressed" : "",watchDir,historyDir);
initialUpdate();

//---- initialise event notification on watchDir ----
int notifyFd=inotify_init();
if(notifyFd==-1)
  { perror("inotify_init()"); exit(1); }
int wd=inotify_add_watch(notifyFd,watchDir,IN_ALL_EVENTS);
if(wd==-1)
  {
  fprintf(stderr,"inotify_add_watch(%s): %s\n",watchDir,strerror(errno));
  exit(1);
  }

for(;;)
  {
  //---- wait for notification events ----
  char evtBuf[0x400];
  int r=0;
  //
  // ... À COMPLÉTER {C-1} ...
  //
  // Lire depuis ``notifyFd'' au maximum ``sizeof(evtBuf)'' octets qui seront
  // placés dans ``evtBuf''.
  // Le nombre d'octets effectivement lus doit être mémorisé dans ``r''.
  //
  if((r = read(notifyFd, evtBuf, sizeof(evtBuf))) == -1) perror("Probleme lors du read notifyFd");

  //---- manage each notification event ----
  const struct inotify_event *evt;
  for(int offset=0;offset<r;offset+=sizeof(struct inotify_event)+evt->len)
    {
    evt=(const struct inotify_event *)(evtBuf+offset);
    if(1)
      {
      printf("EVT: ");
      if(evt->mask&IN_ACCESS)        printf("IN_ACCESS ");
      if(evt->mask&IN_ATTRIB)        printf("IN_ATTRIB ");
      if(evt->mask&IN_CLOSE_WRITE)   printf("IN_CLOSE_WRITE ");
      if(evt->mask&IN_CLOSE_NOWRITE) printf("IN_CLOSE_NOWRITE ");
      if(evt->mask&IN_CREATE)        printf("IN_CREATE ");
      if(evt->mask&IN_DELETE)        printf("IN_DELETE ");
      if(evt->mask&IN_DELETE_SELF)   printf("IN_DELETE_SELF ");
      if(evt->mask&IN_MODIFY){
	updateFile(evt->name);
	printf("IN_MODIFY ");
      }
      if(evt->mask&IN_MOVE_SELF)     printf("IN_MOVE_SELF ");
      if(evt->mask&IN_MOVED_FROM)    printf("IN_MOVED_FROM ");
      if(evt->mask&IN_MOVED_TO){
	updateFile(evt->name);
	printf("IN_MOVED_TO ");
      }
      if(evt->mask&IN_OPEN)          printf("IN_OPEN ");
      printf("%s/%s\n",watchDir,evt->len ? evt->name : "");
      }
    //
    // ... À COMPLÉTER {C-2} ...
    //
    // Si le masque de l'événement contient IN_MODIFY ou IN_MOVED_TO
    // appeler ``updateFile()''.
    //

    }
  }

//---- close event notification ----
if(close(notifyFd)==-1)
  { perror("close(notifyFd)"); exit(1); }

return 0;
}

//----------------------------------------------------------------------------
