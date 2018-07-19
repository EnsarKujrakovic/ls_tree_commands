#include <sys/dir.h>  
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <linux/limits.h>

#define IS_IRUSR(m) ((m & S_IRUSR) ? true : false)
#define IS_IXUSR(m) ((m & S_IXUSR) ? true : false)


void tree(char*, char*);


struct stat entrystat;
int lflag = 0, lflagcounter = 0; //limit i brojac za -L flag
bool dflag = false; 			 //-d flag ukljucen/iskljucen
int c;

int dirnum = 0;					 //broj direktorija
int filenum = 0;				 //broj fajlova

char* name;					 //putanja iz proslijedjenog argumenta

int main(int argc, char** argv){
	while ((c = getopt (argc, argv, "L:d::")) != -1)//petlja za handle flagova -L i -d
	 	switch (c){
			case 'L':
				lflag = atoi(optarg);
				break;
			case 'd':
				dflag = true;
				break;
			case '?':
				printf("Wrong parameters");
				return 1;
			default:
				abort ();
			}
	name = argv[optind];
	
	if(name == NULL) name = "."; // u slucaju poziva bez argumenta
	stat(name, &entrystat);
	
	if(S_ISDIR(entrystat.st_mode)){//ako je argument direktorij poziva se tree(), ako nije javlja se greska
		printf("%s\n", name);
		tree(name, "");
		if(!dflag)
			printf("\n%d directories, %d files\n", dirnum, filenum);
		else
			printf("\n%d directories\n", dirnum);
   	}
	else{
		printf("%s [error opening dir]\n", argv[1]);
	}
	
}
void tree(char* name, char* pref){
	if(lflag == 0 || lflagcounter < lflag){	//uslov za -L flag
		int oldlflagcounter = ++lflagcounter;
		int numl = 0, numd = 0, dircounter = 0;
		int n;
		bool last, lastdir;
		char prefix[2000];
		strcpy(prefix, ""); strcat(prefix, pref);
		struct dirent **fileList;
		n = scandir(name, &fileList, NULL, alphasort);
		if(n>0)free(fileList[0]);
		for(int i = 2; i < n; ++i){		//petlja za prebrojavanje fajlova i direktorija
			if(S_ISDIR(DTTOIF(fileList[i]->d_type)))
				++numd;
			else
				++numl;
		}
		dirnum+=numd;
		filenum+=numl;
		
		int i = 0;
		while(++i < n){
			if((strcmp(fileList[i]->d_name, ".") != 0) && (strcmp(fileList[i]->d_name, "..")) != 0){ //preskakanje ../ i ./
				char path[PATH_MAX];
				strcpy(path, name); strcat(path, "/"); strcat(path, fileList[i]->d_name); //formiranje putanje za stat()
				//realpath(fileList[i]->d_name, path);
				lstat(path, &entrystat);
				last=(i==n-1) ? true : false;		//posljednji fajl ili direktorij
				if(!S_ISDIR(DTTOIF(fileList[i]->d_type))){		// ako nije direktorij vrsi se samo ispis
					if(!dflag){
						if(last)
							printf("%s└──%s", prefix, fileList[i]->d_name);
						else
			   				printf("%s├──%s", prefix, fileList[i]->d_name);
			   			//	}
						if(S_ISLNK(DTTOIF(fileList[i]->d_type))){ //ispis putanje za simbolicki link
							char lpath[PATH_MAX] = {0};
							readlink(path, lpath, PATH_MAX);
							printf(" -> %s", lpath);
							realpath(path, lpath);
							lstat(lpath, &entrystat);
							if(S_ISDIR(entrystat.st_mode)){--filenum;++dirnum;}//ako je simbolicki link na direktorij
						}
			   			printf("\n");}
		   		}
		   		else{								//ako je direktorij vrsi se ispis i rekurzivni poziv tree()
		   			++dircounter;
		   			lastdir = (dircounter+numl == n-2); //posljednji direktorij
		   			char tmp[3], oldprefix[PATH_MAX];
		   			char* err = " [error opening dir]";
		   			if((dflag&&lastdir) || last){
					printf("%s└──%s", prefix, fileList[i]->d_name);
						strcpy(tmp, "   ");		
					}else{
		   				printf("%s├──%s", prefix, fileList[i]->d_name);
		   				strcpy(tmp, "│  "); //│
		   			}
		   			if(!IS_IRUSR(entrystat.st_mode))//ako nema read permisije
		   				printf("%s", err);
		   			printf("\n");	   			
		   			strcpy(oldprefix, prefix);
		   			strcat(prefix, tmp);
		   			if(IS_IXUSR(entrystat.st_mode))// ako nema execute permisije 	
				 		tree(path, prefix);
				 	
				 	strcpy(prefix, oldprefix);
				 	lflagcounter = oldlflagcounter;
				}
			}
		free(fileList[i]);
		}free(fileList);
	}
}
