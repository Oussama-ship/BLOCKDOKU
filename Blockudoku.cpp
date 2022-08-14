#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"

// Dimensions de la grille de jeu
#define NB_LIGNES   12
#define NB_COLONNES 19

// Nombre de cases maximum par piece
#define NB_CASES    4

// Macros utilisees dans le tableau tab
#define VIDE        0
#define BRIQUE      1
#define DIAMANT     2

int tab[NB_LIGNES][NB_COLONNES]
={ {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};


// ---------------- Création des structures -------------------
typedef struct
{
  int ligne;
  int colonne;
} CASE;

typedef struct
{
  CASE cases[NB_CASES];
  int  nbCases;
  int  couleur;
} PIECE;


//-------------------Variables globales--------------------------


  //Tableaux de pieces
  PIECE pieces[12] = { 0,0,0,1,1,0,1,1,4,0,       // carre 4
                      0,0,1,0,2,0,2,1,4,0,       // L 4
                      0,1,1,1,2,0,2,1,4,0,       // J 4
                      0,0,0,1,1,1,1,2,4,0,       // Z 4
                      0,1,0,2,1,0,1,1,4,0,       // S 4
                      0,0,0,1,0,2,1,1,4,0,       // T 4
                      0,0,0,1,0,2,0,3,4,0,       // I 4
                      0,0,0,1,0,2,0,0,3,0,       // I 3
                      0,1,1,0,1,1,0,0,3,0,       // J 3
                      0,0,1,0,1,1,0,0,3,0,       // L 3
                      0,0,0,1,0,0,0,0,2,0,       // I 2
                      0,0,0,0,0,0,0,0,1,0 };     // carre 1
  
  //Tableau de carrés
    CASE carres[9];
  //Variable du thread event
    bool sortie = false;
    EVENT_GRILLE_SDL event;
  //Variable pour threadDefileMessage
    char* message;                  // pointeur vers le message à faire défiler
    int tailleMessage;              // longueur du message
    int indiceCourant;              // indice du premier caractère à afficher dans la zone graphique
  //Variable pour le threadScore
    int score = 0;                  // le score du jeu
    bool MAJScore;                  // maj score
    int combo = 0;
    bool MAJCombos;
  //Variable pour le threadPiece
    CASE casesInserees[NB_CASES];   // cases insérées par le joueur
    int nbCasesInserees;            // nombre de cases actuellement insérées par le joueur
    PIECE pieceEnCours;
  //Variable pour le threadNettoyeur
    int lignesCompletes[NB_CASES];
    int nbLignesCompletes = 0;
    int colonnesCompletes[NB_CASES];
    int nbColonnesCompletes = 0;
    int carresComplets[NB_CASES];
    int nbCarresComplets = 0;
    int nbAnalyses = 0;
    bool traitementEnCours = false;
  //Thread Handle
    pthread_t threadHandleMessage;
    pthread_t threadHandlePiece;
    pthread_t threadHandleEvent;
    pthread_t threadHandleScore;
    pthread_t threadHandleNettoyeur;
    pthread_t tabThreadCase[9][9];
  //Mutex
    pthread_mutex_t mutexMessage;
    pthread_mutex_t mutexCasesInserees;
    pthread_mutex_t mutexScore;
    pthread_mutex_t mutexAnalyse;
    pthread_mutex_t mutextraitementEnCours;
    pthread_mutex_t mutexSortie;
  //Variable de condition
    pthread_cond_t condCasesInserees;
    pthread_cond_t condScore;
    pthread_cond_t condAnalyse;
    pthread_cond_t condtraitementEnCours;
  //Cle
    pthread_key_t key1;
  //SigAction
    struct sigaction ActionAlarm;
    struct sigaction ActionSIGUSR1;
  //Contrôleur
    pthread_once_t controleur = PTHREAD_ONCE_INIT;

//------------------- Fonctions des threads ------------------------
  void* threadDefileMessage();
  void* threadPiece();
  void* threadEvent();
  void* threadScore();
  void* threadNettoyeur();
  void* threadCases(CASE* ca);

//------------------- Fonctions ------------------------------------
  void setMessage(const char *texte, bool signalOn);
  void RotationPiece(PIECE *p);
  bool CheckGameOver();
  int  CompareCases(CASE case1,CASE case2);
  void TriCases(CASE *vecteur,int indiceDebut,int indiceFin);
  void initcle();
  void destructeur(void* p);
  void fctFinThreadMessage(void* p);

//-------------------- Handler signal ------------------------------
  void HandlerAlarm(int sig);
  void HandlerSIGUSR1(int sig);





//---------------------------------- MAIN ----------------------------------------------
int main(int argc,char* argv[])
{
  int i = 0, j = 0;
 
  srand((unsigned)time(NULL));

  //On initialise les tableaux de lignes et de colonnes et carré complet, à -1 parce qu'on a choisi arbitrairement que 
  //quand ça valait -1 c'était vide

  for(i = 0; i < NB_CASES; i++)
  {
    lignesCompletes[i] = -1;
    colonnesCompletes[i] = -1;
    carresComplets[i] = -1;
  }
  
  //Tableau de cases carres[] qui contient la première case de chaque carré à compléter

  for ( i = 0; i < 9; i++)
  {
    carres[i].ligne = (i/3) * 3;
    carres[i].colonne = (j * 3);
    j++;

    if((i+1)%3 == 0 && i != 0)
      j = 0;
  }


  /* Armement du signal alarm */
  ActionAlarm.sa_handler = HandlerAlarm;
	sigemptyset(&ActionAlarm.sa_mask);
	ActionAlarm.sa_flags = 0;
	sigaction(SIGALRM, &ActionAlarm, NULL);

  /* Armement du signal SIGUSR1 */
  ActionSIGUSR1.sa_handler = HandlerSIGUSR1;
  sigemptyset(&ActionSIGUSR1.sa_mask);
  ActionSIGUSR1.sa_flags = 0;
  sigaction(SIGUSR1, &ActionSIGUSR1, NULL);


  // Ouverture de la fenetre graphique
  printf("(MAIN %d) Ouverture de la fenetre graphique\n",pthread_self()); fflush(stdout);
  if (OuvertureFenetreGraphique() < 0)
  {
    printf("Erreur de OuvrirGrilleSDL\n");
    fflush(stdout);
    exit(1);
  }

  DessineVoyant(8,10,VERT);

  // Initialisation des Mutex
  pthread_mutex_init(&mutexMessage, NULL);
  pthread_mutex_init(&mutexCasesInserees, NULL);
  pthread_mutex_init(&mutexScore, NULL);
  pthread_mutex_init(&mutexAnalyse, NULL);
  pthread_mutex_init(&mutextraitementEnCours, NULL);
  pthread_mutex_init(&mutexSortie, NULL);

  // Initialisation des variables de conditions
  pthread_cond_init(&condCasesInserees, NULL);
  pthread_cond_init(&condScore, NULL);
  pthread_cond_init(&condAnalyse, NULL);
  pthread_cond_init(&condtraitementEnCours, NULL);

  // Lancement du thread message
  setMessage("Bienvenue dans Blockudoku ", true);
  pthread_create(&threadHandleMessage, NULL, (void*(*)(void*)) threadDefileMessage, NULL);
  puts("Thread FileMessage lance !");

  // Lancement du thread piece
  pthread_create(&threadHandlePiece, NULL,(void*(*)(void*)) threadPiece, NULL);
  puts("Thread Piece lance !");

  // Lancement du thread Event
  pthread_create(&threadHandleEvent, NULL,(void*(*)(void*)) threadEvent, NULL);
  puts("Thread Event lance !");

  // Lancement du thread Score
  pthread_create(&threadHandleScore, NULL,(void*(*)(void*)) threadScore, NULL);
  puts("Thread Score lance !");

  // Lancement des thread Case
  for(i = 0; i < 9; i++)
  {
    for(j = 0; j < 9; j++)
    {
      CASE *caseTmp = new CASE();
      caseTmp->ligne = i;
      caseTmp->colonne = j;
      pthread_create(&tabThreadCase[i][j], NULL,(void*(*)(void*)) threadCases, caseTmp);
    }
  }
  puts("Threads case lance !");

  // lancement du thread nettoyeur
  // On attend un peu sinon il se lance avant un autre thread
  struct timespec sec;
  sec.tv_sec = 0;
  sec.tv_nsec = 400000000l;
  nanosleep(&sec, NULL);
  pthread_create(&threadHandleNettoyeur, NULL,(void*(*)(void*)) threadNettoyeur, NULL);
  puts("Thread Nettoyeur lance !");
  

  /* Masque le signal ALARM et SIGUSR1 */
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigaddset(&mask, SIGALRM);
  sigprocmask(SIG_SETMASK, &mask, NULL);

  int* retThread;

  pthread_join(threadHandlePiece, (void**)&retThread); //On attend la fin de thread piece

  pthread_cancel(threadHandleEvent); //On tue les threads qui ne servent plus a rien car où on est en game over, ou on a appuyer sur la croix
  pthread_cancel(threadHandleScore);
  pthread_cancel(threadHandleNettoyeur);
  
  for ( i = 0; i < 9; i++)
  {
    for ( j = 0; j < 9; j++)
    {
      pthread_cancel(tabThreadCase[i][j]);
    }
  }
  
  pthread_mutex_lock(&mutexSortie);
  
  while (!sortie) //On attend que l'utilisateur clique sur la croix
  {
    event = ReadEvent();
    
    if(event.type == CROIX)
       sortie = true;

  }
  pthread_mutex_unlock(&mutexSortie);

  pthread_cancel(threadHandleMessage); //Game over était toujours en train de tourner, on peut le tuer

  printf("(MAIN %d) Fermeture de la fenetre graphique...\n",pthread_self()); 
  fflush(stdout);
  FermetureFenetreGraphique();
  exit(0);
}

// ----------------------------------- FONCTIONS DU PROF ----------------------------------
int CompareCases(CASE case1,CASE case2)
{
  if (case1.ligne < case2.ligne) return -1;
  if (case1.ligne > case2.ligne) return +1;
  if (case1.colonne < case2.colonne) return -1;
  if (case1.colonne > case2.colonne) return +1;
  return 0;
}

void TriCases(CASE *vecteur,int indiceDebut,int indiceFin)
{ // trie les cases de vecteur entre les indices indiceDebut et indiceFin compris
  // selon le critere impose par la fonction CompareCases()
  // Exemple : pour trier un vecteur v de 4 cases, il faut appeler TriCases(v,0,3); 
  int  i,iMin;
  CASE tmp;

  if (indiceDebut >= indiceFin) return;

  // Recherche du minimum
  iMin = indiceDebut;
  for (i=indiceDebut ; i<=indiceFin ; i++)
    if (CompareCases(vecteur[i],vecteur[iMin]) < 0) iMin = i;

  // On place le minimum a l'indiceDebut par permutation
  tmp = vecteur[indiceDebut];
  vecteur[indiceDebut] = vecteur[iMin];
  vecteur[iMin] = tmp;

  // Tri du reste du vecteur par recursivite
  TriCases(vecteur,indiceDebut+1,indiceFin); 
}


// ----------------------------------- FONCTIONS GLOBAL ----------------------------------
void setMessage(const char *texte,bool signalOn)
{
  alarm(0); //S'il y avait un signal d'alarme en cours, ça l'annule
  pthread_mutex_lock(&mutexMessage);
  if(message != NULL) //S'il y a déjà un message on le supprime
    delete message;

  message = new char[strlen(texte)]; //On alloue le nouveau message
  strcpy(message, texte);
  tailleMessage = strlen(message); 
  pthread_mutex_unlock(&mutexMessage);
  if(signalOn) //C'est un message qui va disparâitre dans 10 secondes
  {
    puts("Lancement de l'alarme!");
    alarm(10);
  }
    
}

void RotationPiece(PIECE *p)
{
  int i, j;
  int tmp, nbRotation;
  int lmin, cmin;
  
  //Rotation d'un quart de tour
  for(j = 0; j < p->nbCases; j++)
  {
    tmp = p->cases[j].ligne;
    p->cases[j].ligne = -(p->cases[j].colonne);
    p->cases[j].colonne = tmp;
  }


  //Recherche de la ligne et la colonne minimum
  lmin = p->cases[0].ligne;
  cmin = p->cases[0].colonne;
  for (i = 0; i < p->nbCases; i++)
  {
    if(p->cases[i].ligne < lmin)
      lmin = p->cases[i].ligne;
    if(p->cases[j].colonne < cmin)
      cmin = p->cases[i].colonne;
  }
  
  //Remise de la pièce dans le coin 0,0
  if(lmin < 0) //Si elle est passée en négatif on la remet à 0
  {
    for(i = 0; i < p->nbCases; i++)
    {
      p->cases[i].ligne -= lmin;
    }
  }
  if(cmin < 0) //On recherche la colonne minimum
  {
    for(i = 0; i < p->nbCases; i++)
    {
      p->cases[i].colonne -= cmin;
    }
  }
  TriCases(p->cases, 0, p->nbCases-1); //Ca permet de trier les cases par ordre croissant d'abord sur la ligne, puis sur la colonne. 
  //Pour être sur que les cases d'une pièce soit toujours encodée de la même façon, pour qu'on puisse vérifier les mêmes cases lors de l'encodage.
  
}

bool CheckGameOver()
{
  int i, j, k;
  int lTest, cTest;
  bool ok = false;
  
  for ( i = 0; i < 9; i++) //Parcourir ...
  {
    for(j = 0; j < 9; j++) //... toutes les cases du tableau
    {
      for ( k = 0; k < pieceEnCours.nbCases; k++) //Parcourir la pièce qu'on doit insérer
      {
        lTest = i + pieceEnCours.cases[k].ligne;
        cTest = j + pieceEnCours.cases[k].colonne;

        if (lTest >= 9) //On vérifie que la pièce ne sorte horizontalement pas du tableau
        {
          ok = false;
          break;
        }
        
        else if (cTest >= 9) //On vérifie que la pièce ne sorte verticalement pas du tableau
        {
          ok = false;
          break;
        }

        else if(tab[lTest][cTest] != VIDE) // Si on est pas sorti, on vérifie que la case ne soit pas vide.
        {
          ok = false; //GAME OVER
        }
        else
          ok = true; //Sinon la partie continue
      }
      if(ok == true) //La pièce peut être insérée
        break;
    }
    if(ok == true)
      break;
  }
  if(ok == true) //GAME OVER
    return false;

  return true;
}

void initcle() //Fonction d'inialisation de la clé
{
  printf("Initialisation cle\n");
  pthread_key_create(&key1, destructeur);
}

void destructeur(void* p) //On détruit le pointeur quand on a fini avec la clé, ça détruit la case qu'on a mis en variable spécifique du thread case
{
  printf("destruction variable spec\n");
  free(p);
}

void fctFinThreadMessage(void* p) //Elle delete juste le message pour libérer l'allocation qu'on a faite.
{
    pid_t pid = getpid();
    pthread_t tid = pthread_self();
    delete message;
    printf("\n Thread %d-%u se termine\n", pid, tid);
}


// ----------------------------------- FONCTIONS THREAD ------------------------------------

//----------- MESSAGE -------------
void* threadDefileMessage()
{
  pthread_cleanup_push(fctFinThreadMessage, 0); //Fonction de terminaison

  indiceCourant = 0;
  struct timespec tim;
  tim.tv_sec = 0;
  tim.tv_nsec = 400000000L;
  while(1) //La barre de message tourne en continu
  {
    for (int j = 0; j < tailleMessage+1; j++) //Elle boucle pour faire défiler le message, à chaque fois qu'on rentre dans la boucle, le message se 
    //décale
    {
      pthread_mutex_lock(&mutexMessage); //On verrouille le mutex message au cas ou d'autres threads voudraient y accéder
      for (int i=0 ; i < (tailleMessage-indiceCourant) && i < 17 ; i++) //On écrit le message en commençant par l'indice courant pour le décaler.
        DessineLettre(10,1+i,message[indiceCourant+i]);

      if ((tailleMessage-indiceCourant) < 17) //S'il y a encore de la place, on écrit le deuxième message
      {
        for (int k=(tailleMessage-indiceCourant) ; k < 17 ; k++) //Là on écrit un deuxième message.
          DessineLettre(10, 1+k, message[k - ((tailleMessage-indiceCourant)+1)]);
      }
      indiceCourant++; //Décaler à chaque fois le message
      pthread_mutex_unlock(&mutexMessage); //On a fini, on peut déverrouiller
      nanosleep(&tim, NULL); //nanosleep attend 4 millièmes de secondes entre chaque décalage (voir pour le paramètre NULL)
    }
    pthread_mutex_lock(&mutexMessage);
    indiceCourant = 0;
    pthread_mutex_unlock(&mutexMessage); //Le mutex protège aussi la variable indice courant.
  }
  
  pthread_cleanup_pop(1); //Appeler la fonction de terminaison
  pthread_exit(NULL);  //On sort du thread
}


//----------- PIECE -------------
void* threadPiece()
{
  int i, j;
  int lmin, cmin;
  int ligneGraphique, colGraphique;
  bool validation = true, gameOver = false;
  
  /* Masque le signal ALARM et SIGUSR1*/
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGALRM);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_SETMASK, &mask, NULL); //On veut ignorer les signaux SIGALARM et SIGUSR1

  while(1) //La génération de pièce, il va regarder à chaque génération de pièce s'il y a Game over, et que la pièce est insérée correctement
  {

    //Création de la piece à placer
    pieceEnCours = pieces[rand()%12]; //Avoir une pièce au hasard

    i = rand()%4; //Lui donner une couleur

    if(i == 0)
      pieceEnCours.couleur = ROUGE; 
    else if(i == 1)
      pieceEnCours.couleur = JAUNE;
    else if(i == 2)
      pieceEnCours.couleur = VIOLET;
    else if(i == 3)
      pieceEnCours.couleur = VERT;

    for ( i = 0; i < rand()%4; i++) //Effectuer une rotation sur la pièce
    {
      RotationPiece(&pieceEnCours);
    }

    for(i = 0; i < pieceEnCours.nbCases; i++) //On dessine la pièce, dans la zone de pièce a droite
    {
        DessineDiamant((pieceEnCours.cases[i].ligne) + 3, (pieceEnCours.cases[i].colonne) + 14, pieceEnCours.couleur);
    }

    //Vérification de la possibilité d'introduire la piece
    if(gameOver())
    {
      setMessage("Game over       ", false);
      pthread_exit(NULL);
    }    



    //Boucle jusqu'à ce que la piece soit bien encodé
    do
    {
      //Attende de l'insertion de la piece par l'utilisateur
      pthread_mutex_lock(&mutexCasesInserees);
      nbCasesInserees = 0;
      while(nbCasesInserees < pieceEnCours.nbCases) //On attend à chaque fois que l'utiliser entre un carré de la pièce.
      {
          pthread_cond_wait(&condCasesInserees, &mutexCasesInserees); //On déverrouille le mutex temporairement, 
          //pour que le thread event puisse accéder à la case en cours
      }
      pthread_mutex_unlock(&mutexCasesInserees);

    
      //----------------- Vérification de la correspondance de la piece ------------
    
      pthread_mutex_lock(&mutexCasesInserees);
      TriCases(casesInserees, 0, pieceEnCours.nbCases-1); //On vérifie que si la pièce est un L, il a bien entré un L et pas un carré

      cmin = casesInserees[0].colonne; //On initialise la colonne et la ligne sur la première case de la pièce
      lmin = casesInserees[0].ligne;

      for(i = 1; i < pieceEnCours.nbCases; i++) //On recherche la colonne minimum de la pièce
      {
        if(casesInserees[i].colonne < cmin) //Si la colonne est plus petite, on rechange cmin pour obtenir pour la colonne minimum
        {
          cmin = casesInserees[i].colonne;
        }
      }

      for(i = 0; i < pieceEnCours.nbCases; i++) //Déplacer la pièce en haut a gauche dans l'aperçu, en diminuant la ligne et la colonne par leur 
      //ligne et colonne minimum
      {
        casesInserees[i].ligne = casesInserees[i].ligne - lmin;
        casesInserees[i].colonne = casesInserees[i].colonne - cmin;
      }

      validation = true; //vérifier que l'utilisateur a bien encodé la pièce
      for(i = 0; i < pieceEnCours.nbCases; i++)
      {
        if( CompareCases(casesInserees[i],pieceEnCours.cases[i]) != 0) //On vérifie que la pièce soit correctement encodée
        {   
            //Si la piece n'est pas bien entrée on la supprime
            for(j = pieceEnCours.nbCases-1; j >= 0; j--)
            {
              ligneGraphique = (casesInserees[j].ligne) + lmin;
              colGraphique = (casesInserees[j].colonne) + cmin;
              EffaceCarre(ligneGraphique, colGraphique);
              tab[ligneGraphique][colGraphique] = VIDE;
              casesInserees[j].ligne = VIDE;
              casesInserees[j].colonne = VIDE;
            }
            validation = false;
            break;
        }
      }

      if(validation) //Si elle est bien encodée on la dessine en brique bleue
      {
        //On remplace les diamant par des briques
        for(j = 0; j < pieceEnCours.nbCases ; j++)
        {
          ligneGraphique = (casesInserees[j].ligne) + lmin;
          colGraphique = (casesInserees[j].colonne) + cmin;
          tab[ligneGraphique][colGraphique] = BRIQUE;
          DessineBrique(ligneGraphique, colGraphique, false);
        }

        // Effacement de la piece dans la piece à poser
        for(int ligne = 3; ligne < 7; ligne++)
        {
          for(int col = 14; col < 18; col++)
          {
            EffaceCarre(ligne,col);
          } 
        }

        //Mise à jour du score
        pthread_mutex_lock(&mutexScore);
        score = score + pieceEnCours.nbCases;
        MAJScore = true;
        pthread_mutex_unlock(&mutexScore);

        pthread_cond_signal(&condScore); //On envoie un signal au thread score

        //Lancement du traitement
        pthread_mutex_lock(&mutextraitementEnCours); //On fait l'analyse s'il y a eu un combo ou pas
        traitementEnCours = true;
        DessineVoyant(8,10,BLEU); 
        pthread_mutex_unlock(&mutextraitementEnCours);

        for(j = 0; j < pieceEnCours.nbCases; j++) //On lance un signal à chaque thread correspondant aux cases que l'utilisateur vient d'encoder
        {
          ligneGraphique = (casesInserees[j].ligne) + lmin;
          colGraphique = (casesInserees[j].colonne) + cmin;
          pthread_kill(tabThreadCase[ligneGraphique][colGraphique], SIGUSR1);
        }

        pthread_mutex_unlock(&mutexCasesInserees);
        
        //Attente de la fin du traitement
        pthread_mutex_lock(&mutextraitementEnCours);
        while(traitementEnCours)
        {
          pthread_cond_wait(&condtraitementEnCours,&mutextraitementEnCours); //On se met en attente d'un signal de thread nettoyeur
        }
        pthread_mutex_unlock(&mutextraitementEnCours);
      }
      else
        pthread_mutex_unlock(&mutexCasesInserees);

    }while(!validation); //Tant que la pièce encodée n'est pas valide

  }
}


//----------- EVENT -------------
void* threadEvent()
{
  /* Masque le signal ALARM et SIGUSR1*/
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGALRM);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_SETMASK, &mask, NULL);

  pthread_mutex_lock(&mutexSortie); //La variable sortie et event sont devenue globales, alors on les protèges avec un mutex
  //C'était pour résoudre le bug d'appuyer deux fois sur la croix
  while(!sortie)//On boucle jusqu'à ce qu'il y ai un clic
  {
    event = ReadEvent(); //Fonction qui attend un clic

    if (event.type == CROIX) //Croix, on quitte la boucle
      sortie = true;
      

    else
    {
      //Ajout d'un diamant dans la fenêtre
      if (event.type == CLIC_GAUCHE)
      {
        pthread_mutex_lock(&mutextraitementEnCours);
        if(event.ligne < 9 && event.ligne >= 0 && event.colonne < 9 && event.colonne >= 0 && tab[event.ligne][event.colonne] == VIDE && traitementEnCours == false)
        //Condition qui vérifie qu'on soit dans la zone du tableau a cliquer, et qu'on ne clique pas sur une case déjà complétée, et qu'on ne clique pendant que c'est en cours de traitement
        {
          DessineDiamant(event.ligne,event.colonne,pieceEnCours.couleur);
          tab[event.ligne][event.colonne] = DIAMANT; //On place le diamant de la tableau tab

          pthread_mutex_lock(&mutexCasesInserees);
          casesInserees[nbCasesInserees].ligne = event.ligne; 
          casesInserees[nbCasesInserees].colonne = event.colonne; //On dessine le diamant là où on a cliqué, on réutilise ce tableau là pour vérifier
          nbCasesInserees++;
          pthread_mutex_unlock(&mutexCasesInserees);

          pthread_cond_signal(&condCasesInserees); //Condwait dans thread piece pour dire qu'on vient d'insérer une case
        }

        else
        {
          DessineVoyant(8,10,ROUGE); //Clic sur une brique ou hors de la zone de jeu

          struct timespec sec;
          sec.tv_sec = 0;
          sec.tv_nsec = 400000000l;
          nanosleep(&sec, NULL);

          if(traitementEnCours) //Si il a cliqué pendant qu'il y avait traitement en cours, en redessine le voyant en bleu, sinon en vert
            DessineVoyant(8,10,BLEU);
          else
            DessineVoyant(8,10,VERT);
        }
        pthread_mutex_unlock(&mutextraitementEnCours);

      }

      //Effacement des diamants posé
      else if (event.type == CLIC_DROIT)
      {
        pthread_mutex_lock(&mutexCasesInserees);
        for(int j = nbCasesInserees-1; j >= 0; j--) //On remet a vide les pièces qu'on désirait insérée
        {
          EffaceCarre(casesInserees[j].ligne, casesInserees[j].colonne);

          tab[casesInserees[j].ligne][casesInserees[j].colonne] = VIDE;
          casesInserees[j].ligne = VIDE;
          casesInserees[j].colonne = VIDE;
        }
        nbCasesInserees = 0;
        pthread_mutex_unlock(&mutexCasesInserees);
      }   
    } 
  }
  pthread_mutex_unlock(&mutexSortie);
  //Fermeture de la fenetre
  pthread_cancel(threadHandlePiece); //Vu que le main attend la fin du thread pièce, quand on va cliquer sur la croix alors qu'on est pas en game over,
  //on tue le thread pièce pour que le main reprenne la main et termine le programme
  pthread_exit(NULL);
}


//----------- SCORE -------------
void* threadScore()
{
  /* Masque le signal ALARM et SIGUSR1*/
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGALRM);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_SETMASK, &mask, NULL);

  int i;

  for(i=14; i<=17; i++)
    DessineChiffre(1,i, 0);
      
  while (1) //Le score tourne en continu
  {
    pthread_mutex_lock(&mutexScore);
    MAJScore = false; //Lorsqu'on envoie un signal à thread score, on met la variable majscore à true
    MAJCombos = false; 
   
    // Attente de la mise à jour d'un score ou d'un combo
    while(!MAJScore && !MAJCombos)
    {
      pthread_cond_wait(&condScore ,&mutexScore); //On attend un signal de thread piece
    }
    pthread_mutex_unlock(&mutexScore);


    // Mise à jour de l'affichage du score ou du combo
    pthread_mutex_lock(&mutexScore);

    if(MAJCombos) //On dessine le combos en question, le combo est incrémentée mais le score aussi
    {
      DessineChiffre(8,14, combo/1000);
      DessineChiffre(8,15, (combo%1000)/100);
      DessineChiffre(8,16, (combo%100)/10);
      DessineChiffre(8,17, combo%10);

      DessineChiffre(1,14, score/1000);
      DessineChiffre(1,15, (score%1000)/100);
      DessineChiffre(1,16, (score%100)/10);
      DessineChiffre(1,17, score%10);
    }

    if(MAJScore) //On dessine le score
    {
      DessineChiffre(1,14, score/1000);
      DessineChiffre(1,15, (score%1000)/100);
      DessineChiffre(1,16, (score%100)/10);
      DessineChiffre(1,17, score%10);
    }

    pthread_mutex_unlock(&mutexScore);
  }
  
}


//----------- CASE --------------
void* threadCases(CASE* ca) //Chaque case est un thread, on le lance 81 une fois
{
  /* Masque le signal ALARM*/
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGALRM); //On masque juste alarm, on veut interpréter les SIGUSR1
  sigprocmask(SIG_SETMASK, &mask, NULL);

  pthread_once(&controleur, initcle); //On vérifie qu'on a déjà initialisé la clé, pour ne pas écraser la variable spécifique
  
  pthread_setspecific(key1, ca); //On met la variable dans le coffre fort du thread, ca à une valeur différente pour chaque case
  //On peut pas la mettre en globale, car elle ne peut pas avoir plusieurs variable différente, et aussi on ne saurait pas la passer
  //dans le handler sigusr1

  while(1) //On attend juste de recevoir un signal
  {
    pause();
  }
}


//----------- NETTOYEUR ---------
void* threadNettoyeur()
{
  /* Masque le signal ALARM et SIGUSR1*/
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGALRM);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_SETMASK, &mask, NULL);

  struct timespec sec;
  int i, j;
  while(1)
  {      
    //Attente de la fin de l'analyse pour chaque case
    pthread_mutex_lock(&mutexAnalyse);
    while(nbAnalyses < pieceEnCours.nbCases)
    {
      pthread_cond_wait(&condAnalyse, &mutexAnalyse); //Attends que chaque case a été kill, s'il y a eu un combo on appelle le thread nettoyeur 
      //On attend un signal de SIGUSR1 Handler
    }
    pthread_mutex_unlock(&mutexAnalyse);



    pthread_mutex_lock(&mutexAnalyse);
    //Si il y a un combo
    if(nbCarresComplets > 0 || nbColonnesCompletes > 0 || nbLignesCompletes > 0)
    {
      // Transformation des lignes complètes en brique en fusion
      for(i = 0; i < nbLignesCompletes; i++)
      {
        for(j = 0; j < 9 && tab[lignesCompletes[i]][j] == BRIQUE; j++)
        {
          DessineBrique(lignesCompletes[i], j, true);
        }
      }

      // Transformation des collones complètes en brique en fusion
      for(i = 0; i < nbColonnesCompletes; i++)
      {  
        for(j = 0; j < 9 && tab[j][colonnesCompletes[i]] == BRIQUE; j++)
        {
          
          DessineBrique(j, colonnesCompletes[i], true);
        }
      }


      // Transformation des carre complets en brique en fusion
      for(i = 0; i < nbCarresComplets; i++)
      {
        for(int l = carres[carresComplets[i]].ligne; l < (carres[carresComplets[i]].ligne+3); l++)
          {
            for(int c = carres[carresComplets[i]].colonne; c < (carres[carresComplets[i]].colonne+3); c++)
            {
              DessineBrique(l, c, true);
            }
          }
      }
    
      //mise à jour du score et du combo
      pthread_mutex_lock(&mutexScore);
      MAJCombos= true;

      if(nbCarresComplets + nbColonnesCompletes + nbLignesCompletes == 1) //1 simple combo
      {
        setMessage("Simple combo !  ", true);
        score += 10;
      }
      else if(nbCarresComplets + nbColonnesCompletes + nbLignesCompletes == 2) //2 double etc...
      {
        setMessage("Double combo !  ", true);
        score += 25;
      }
      else if(nbCarresComplets + nbColonnesCompletes + nbLignesCompletes == 3)
      {
        setMessage("Triple combo !  ", true);
        score += 40;
      }
      else if(nbCarresComplets + nbColonnesCompletes + nbLignesCompletes == 4)
      {
        setMessage("Quadruple combo !", true);
        score += 55;
      }
      combo += nbCarresComplets + nbColonnesCompletes + nbLignesCompletes; //Incrémenter la variable combo
      
      pthread_mutex_unlock(&mutexScore);
      pthread_cond_signal(&condScore); //On envoit un signal au thread score


      //Attende de 2 sec puis suppression des briques en fusions
      sec.tv_sec = 2;
      nanosleep(&sec, NULL);

      //Effacement des lignes
      for(i = 0; i < nbLignesCompletes; i++)
      {
        for(j = 0; j < 9 && tab[lignesCompletes[i]][j] == BRIQUE; j++)
        {
          EffaceCarre(lignesCompletes[i], j);
          tab[lignesCompletes[i]][j] = VIDE;
        }
      }


      //Effacement des colonnes
      for(i = 0; i < nbColonnesCompletes; i++)
      {
        for(j = 0; j < 9; j++)
        {
          EffaceCarre(j, colonnesCompletes[i]);
          tab[j][colonnesCompletes[i]] = VIDE;
        }
      }


      //Effacement des carrés
      for(i = 0; i < nbCarresComplets; i++)
      {
        for(int l = carres[carresComplets[i]].ligne; l < (carres[carresComplets[i]].ligne+3); l++)
          {
            for(int c = carres[carresComplets[i]].colonne; c < (carres[carresComplets[i]].colonne+3); c++)
            {
              EffaceCarre(l, c);
              tab[l][c] = VIDE;
            }
          }
      }
    }

    //Fin du traitement
    pthread_mutex_lock(&mutextraitementEnCours);
    traitementEnCours = false;
    DessineVoyant(8,10,VERT); //Le traitement est terminé le voyant devient vert
    pthread_mutex_unlock(&mutextraitementEnCours);

    pthread_cond_signal(&condtraitementEnCours); //On envoie un signal à thread pièce, pour lui dire que le traitement est fini et qu'il peut mettre
    //la nouvelle pièce
    
    nbAnalyses = 0;
    nbLignesCompletes = 0;
    nbColonnesCompletes = 0;
    nbCarresComplets = 0; // On remet les variables globales qui permettent de faire l'analyse de combo à zéro pour le prochain traitement

    for(i = 0; i < NB_CASES; i++) // On remet les lignes, les colonnes, les carré qui permettent de faire l'analyse de combo à zéro pour le prochain traitement
    {
      lignesCompletes[i] = -1;
      colonnesCompletes[i] = -1;
      carresComplets[i] = -1;
    }
    pthread_mutex_unlock(&mutexAnalyse);
  }
}


// ----------------------------------- HANDLER SIGNAL ---------------------------------------

//----------- ALARM ---------
void HandlerAlarm(int sig)
{
  puts("Reception d'un signal alarm.");
  setMessage("Jeux en cours   ", false); //Changer le message, si c'est true on va envoyer un signal pour remettre le message jeu en cours
}


//A chaque fois qu'on entre une pièce, on entre dans le signal pour chaque case de la pièce
//----------- SIGUSR1 ---------
void HandlerSIGUSR1(int sig)
{
  puts("Reception d'un signal SIGUSR1.");

  int i = 0, j = 0, k = 0;
  
  CASE* ca = (CASE*)pthread_getspecific(key1); //On récupère la variable qu'on a mise en variable spécifique en thread case

  //Pour les récupérer, on ne peut pas passer les cases en paramètre, et chaque case est différente pour chaque thread case

  pthread_mutex_lock(&mutexAnalyse);
  
  //-------- Vérfication de la ligne ----------

  while(ca->ligne != lignesCompletes[i] && i < 4) //Il regarde si cette ligne là fait partie des lignes complètes
    i++;
  //Si la ligne ne fait pas encore partie des lignes complètes
  if(i == 4)
  {
    i = ca->ligne; //Ca initialise i à la ligne
    j = 0;
    //Vérification que la ligne est complète
    while(j < 9 && tab[i][j] == BRIQUE)
      j++; //On verifie qu'à chaque fois il y a une brique
    
    if(j == 9) //La ligne est complète
    {
      j = 0;
      while(lignesCompletes[j] != -1)
        j++;
      
      lignesCompletes[j] = i; //On a ajoute la ligne dans le tableau des lignes complètes
      nbLignesCompletes++;
    }
  }




  //-------- Vérfication de la Colonne ----------
  i = 0;
  j = 0;
  while(ca->colonne != colonnesCompletes[i] && i < 4)
    i++;
  //Si la colonne ne fait pas encore partie des colonne complètes
  if(i == 4)
  {
    i = ca->colonne;
    j = 0;
    //Vérification que la colonne est complète
    while(j < 9 && tab[j][i] == BRIQUE)
      j++;
    
    if(j == 9)
    {
      j = 0;
      while(colonnesCompletes[j] != -1)
        j++;
      
      colonnesCompletes[j] = i;
      nbColonnesCompletes++;
    }
  }





  //-------- Vérfication du carre ----------
  i = 0;
  j = 0;
  k = 0;
  bool ok = true;
  //On determine dans quel carre se trouve la case
  for ( i = 0; i < 9; i++)
  {
    if(ca->ligne >= carres[i].ligne && ca->colonne >= carres[i].colonne && ca->ligne < (carres[i].ligne+3) && ca->colonne < (carres[i].colonne+3))
      break;
  }
  if(i != 9)
  {
    //On vérifie que le carre ne fait pas parti des carre complet
    while(i != carresComplets[j] && j < 4)
      j++;

    if(j == 4)
    {
      //On vérifie si le carre est complet
      for ( j = carres[i].ligne; j < (carres[i].ligne+3); j++)
      {
        for ( k = carres[i].colonne; k < (carres[i].colonne+3); k++)
        {
          if(tab[j][k] != BRIQUE)
            ok = false;
        }
      }
      //Si le carre est complet on l'ajoute
      if(ok)
      {
        j = 0;
        while(carresComplets[j] != -1)
          j++;
        
        carresComplets[j] = i;
        nbCarresComplets++;
        printf("carre complet! \n");
      }
    }
  }
  

  nbAnalyses++;
  pthread_mutex_unlock(&mutexAnalyse);
  pthread_cond_signal(&condAnalyse); //On envoie un signal à nettoyeur pour voir s'il y a eu un combo ou pas

}
