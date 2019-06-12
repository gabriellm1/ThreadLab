#define  _GNU_SOURCE
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>
#include "raylib.h"


typedef struct  {
   char *ad1;
   char *ad2;
   int f_total;
   int f_prog;
   int f_atual;
   int d_total;
   int d_atual;
   int d_prog;
}ad;


void rmfile(const char file[]){
  struct stat s_file;
  printf("\n%s\n",file);

  stat(file, &s_file);
  if (unlink(file) == 0){
      printf("Arquivo removido: %s\n", file);
      return;
  }else{
    printf("Erro ao remover arquivo: %s\n", file);
    return;
  }
}

void rm_dir(const char dir_p[]){
  DIR *dir;
  struct dirent *dp;
  char path[200];

  dir = opendir(dir_p);

  while((dp = readdir(dir)) != NULL) {
      if (!(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))){
        sprintf(path,"%s/%s", dir_p,dp->d_name);
        printf("Path: %s \n",path);
        rmfile(path);
      }
    }

    if (rmdir(dir_p) == 0){
        printf("Diretório removido: %s\n", dir_p);
    }else{
        printf("Erro ao remover arquivo: %s\n", dir_p);
     }
}



void copy_file(void *_arg){

  ad *ads = (ad*) _arg;

  char buffer[50];
  int  status;
  mode_t permissao;
  int origem_fd = open(ads->ad1, O_RDONLY);

  struct stat fs_ad1,fs_ad2;
  fstat(origem_fd,&fs_ad1);
  permissao = fs_ad1.st_mode;

  int destino_fd = open(ads->ad2, O_CREAT | O_WRONLY ,permissao);
  fstat(destino_fd,&fs_ad2);
  ads->f_total = fs_ad1.st_size;

  while (fs_ad2.st_size<ads->f_total) {
    status = read(origem_fd, buffer, 10);
    if (status == -1) {
        printf("Erro ao ler o arquivo\n");
        exit(1);
    }
    write(destino_fd, buffer, status);
    fstat(destino_fd,&fs_ad2);
    ads->f_prog = (fs_ad2.st_size*700)/ads->f_total;
    ads->f_atual = fs_ad2.st_size;
  }
  close(origem_fd);
  close(destino_fd);
}

void copy_dir(void *_arg){

  ad *ads = (ad*) _arg;

  DIR *dir1;
  DIR *dir2;
  struct dirent *dp1;
  struct stat s_dp2;
  struct stat fs_ad1;
  struct stat fs_ad2;

  char pathOrigem[100];
  char pathDestino[100];
  char buffer[50];

  int fd_dir1;
  int fd_dir2;
  int status;

  mode_t permissao;

  if ((dir2 = opendir(ads->ad2)) == NULL) {
      status = mkdir(ads->ad2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
      dir2 = opendir(ads->ad2);
      if(status!=0){
        if( stat(ads->ad2,&s_dp2) == 0 ){
            if( !(s_dp2.st_mode & S_IFDIR) ){
                printf("Erro: Destino nao e um diretorio!\n");
                return;
            }
        }else{
          printf("Erro ao criar diretorio\n");
          return;
         }
      }
  }


  if ((dir1 = opendir(ads->ad1)) == NULL) {
        perror("Erro ao abrir o diretorio");
        return;
    }else{
      ads->d_total = 0;
      while((dp1=readdir(dir1)) != NULL){
        if (!(!strcmp(dp1->d_name, ".") || !strcmp(dp1->d_name, "..") || !strcmp(dp1->d_name, ".DS_Store"))){
          ads->d_total++;
        }
      }
      closedir(dir1);
    }


    dir1 = opendir(ads->ad1);
    ads->d_prog = 0;



    do {

        if ((dp1 = readdir(dir1)) != NULL) {


            sprintf(pathOrigem,"%s/%s", ads->ad1,dp1->d_name);
            printf("Origem: %s \n",pathOrigem);
            fd_dir1 = open(pathOrigem, O_RDONLY);
            printf("Arquivo: %s\n",dp1->d_name);

            fstat(fd_dir1,&fs_ad1);
            ads->f_total = fs_ad1.st_size;
            permissao = fs_ad1.st_size;

            // skipa arquivos . e .. e .DS_STORE(macOS)
            if (!(!strcmp(dp1->d_name, ".") || !strcmp(dp1->d_name, "..") || !strcmp(dp1->d_name, ".DS_Store"))){

                sprintf(pathDestino,"%s/%s", ads->ad2,dp1->d_name);
                printf("Destino: %s\n",pathDestino);

                ads->f_prog = 0;
                fd_dir2 = open(pathDestino, O_CREAT | O_WRONLY , permissao);
                fstat(fd_dir2,&fs_ad2);

                while(fs_ad2.st_size<ads->f_total){
                    status = read(fd_dir1,buffer,10);
                    write(fd_dir2,buffer,status);
                    fstat(fd_dir2,&fs_ad2);
                    ads->f_atual = fs_ad2.st_size;
                    ads->f_prog = (fs_ad2.st_size*700)/ads->f_total;
                }
                ads->d_atual++;
                ads->d_prog = (ads->d_atual*700)/ads->d_total;
                close(fd_dir2);
                close(fd_dir1);
            }
        }

    }while (dp1 != NULL);

    closedir(dir2);
    closedir(dir1);
}


int main(int argc, char const *argv[]) {

  // cria threads
  pthread_t thread_copia = malloc(sizeof(pthread_t));

  ad *ads = malloc(sizeof(ad));
  ads->ad1 = argv[1];
  ads->ad2 = argv[2];
  ads->f_prog = 0;

  char texto[100];
  int prog;
  int prog_dir;
  int caso;

  struct stat s_ad1,s_ad2;

  if( stat(ads->ad1,&s_ad1) != 0 ){
    caso = 0 ;
  }else{
    if(s_ad1.st_mode & S_IFDIR){
      caso = 1;
    }else{
      caso = 2;
    }
  }

  int screenWidth = 800;
  int screenHeight = 800;

  InitWindow(screenWidth, screenHeight, "Copia de arquivos/diretorios por Gabriel Monteiro");

  int boxPositionX = 50;
  int boxPositionY = 200;

  Texture2D button = LoadTexture("button.png"); // Load button texture


  Rectangle sourceRec = { 0, 0, button.width, button.height };

  Rectangle btnBounds = { 350,600, button.width, button.height};

  Vector2 mousePoint = { 0.8f, 0.8f };


  SetTargetFPS(60);


  switch (caso) {
    case 0: printf("Erro!!!\n");
            break;
    case 1: printf("caso1\n");

    pthread_create(&thread_copia, NULL,copy_dir , ads);
            while (!WindowShouldClose())  {
                mousePoint = GetMousePosition();
                BeginDrawing();

                    DrawTextureRec(button, sourceRec, (Vector2){ btnBounds.x, btnBounds.y }, RAYWHITE);
                    prog = ads->f_prog;
                    prog_dir = ads->d_prog;
                    ClearBackground(RAYWHITE);

                    DrawRectangle(boxPositionX, boxPositionY, prog, 80, SKYBLUE);
                    DrawRectangle(boxPositionX, boxPositionY+200, prog_dir, 80, LIME);

                    sprintf(texto,"Progresso %d / %d - Arquivo %d / %d",ads->f_atual,ads->f_total,ads->d_atual,ads->d_total);
                    DrawText(texto, 10, 10, 20, GRAY);
                EndDrawing();
                if (CheckCollisionPointRec(mousePoint, btnBounds)&&IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
                  printf("\n\n BOTAO \n\n");
                  pthread_cancel(thread_copia);
                  rm_dir(ads->ad2);
                  break;
                }
                if(ads->d_atual/ads->d_total == 1){
                  break;
                }
            }
            UnloadTexture(button);
            pthread_join(thread_copia, NULL);
            //CloseWindow();
            break;

    case 2: printf("caso2\n");
            pthread_create(&thread_copia, NULL,copy_file , ads);
            while (!WindowShouldClose())  {
              mousePoint = GetMousePosition();
              BeginDrawing();
                  DrawTextureRec(button, sourceRec, (Vector2){ btnBounds.x, btnBounds.y }, WHITE);
                  prog = ads->f_prog;
                  ClearBackground(RAYWHITE);
                  DrawRectangle(boxPositionX, boxPositionY, prog, 80, LIME);
                  sprintf(texto,"Progresso %d / %d ",ads->f_atual,ads->f_total);
                  DrawText(texto, 10, 10, 20, GRAY);
              EndDrawing();

              if (CheckCollisionPointRec(mousePoint, btnBounds) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){

                pthread_cancel(thread_copia);
                rmfile(ads->ad2);
                break;
              }
              if(ads->f_atual/ads->f_total == 1){
                break;
              }
            }
            UnloadTexture(button);
            pthread_join(thread_copia, NULL);
            //CloseWindow();
            break;


            
  }
  return 0;
}
