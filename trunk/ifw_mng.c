/*  2004 ifilework TEAM
    mailto: myfilesadev@gmail.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.
 */

#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include "ifilework.h"



static char *do_get_passwd(void)
{
    static char password[MAX_LINE];
    
    int tries = 3;
       
    *password = 0;
 
    
   
     again:
     strcpy(password,getpass("Enter password: "));
      printf("\n");
      if (strcmp(password,getpass("Enter password again: ")) != 0){
           printf("passwords do not agree\n");
           tries--;
           goto again;
           
           }else
           printf("password OK\n");
   
    
    return password;
}


static int add_config_line(FILE *fd, u_info *user_info){
      
         
     if (fd == NULL) {
         return -1;
       }

   
  
    if (fprintf(fd, 
                "%s:"        /* account */
                "%s:"         /* password */
                "%lu:"          /* uid */
                "%lu:"          /* gid */              
                "%s:"          /* home */
                , user_info->login, user_info->pwd,
                (unsigned long) user_info->uid, (unsigned long) user_info->gid,
                user_info->home) < 0) {
        return -1;
    }
       
  if (fprintf(fd, "\n") < 0) {
        return -1;
    }
    
    return 0;
}

int do_useradd( u_info *user_info){
       
        FILE *fp;
        FILE *fp_out;
        char f_out[MAX_LINE ];
        char BUFFER[MAX_LINE ];
        
   if (user_info->login == NULL || *(user_info->login) == 0) {
        fprintf(stderr, "Missing login\n");
        return -1 ;
    }
        
    if (user_info->home == NULL || *(user_info->home) == 0) {
        fprintf(stderr, "Missing home directory\n");
        return -1 ;
    }
    sprintf(f_out,"%s%s",user_info->fconf,".tmp");
 
    if ((user_info->pwd = do_get_passwd()) == NULL) {
        fprintf(stderr, "Error with entering password - aborting\n");        
        return -1 ;
    }
     char *cleartext = user_info->pwd;

      user_info->pwd = ifw_crypt(cleartext);


    fp_out = fopen(f_out, "w+"); 
     if (!fp_out) {
         printf("\a error to open the temp file \n");
         return -1;
    }
 

    fp= fopen(user_info->fconf, "r");
       
      if (!fp) {
         printf("\a error to open the configuration file -> %s \n",user_info->fconf );
         return -1;
    }
 
    
 
     while (!feof(fp))
    {
     fscanf(fp, "%s\n", BUFFER);
     fprintf(fp_out, "%s\n", BUFFER);
    }
    
    if(add_config_line(fp_out, user_info) != 0) {
        fprintf(stderr, "Unable to append a line\n");
        fclose(fp);
        return -1;
    }
    
     fclose(fp_out);
     fclose(fp);
     
     rename(f_out,user_info->fconf);
     
      return 0;    
}

int do_userdel( u_info *user_info){
    
        FILE *fp;
        FILE *fp_out;
        char fout  [MAX_LINE];
        char buffer[MAX_LINE];
        char tmp   [MAX_LINE];

 
    sprintf(fout,"%s%s",user_info->fconf,".tmp");
  

    fp_out = fopen(fout, "w"); 
     if (!fp_out) {
         printf("\a error : not open temp file \n");
         return -1;
    }
    

    fp= fopen(user_info->fconf, "r");
       
    if (!fp) {
         printf(" error : not open config file \n");
         return -1;
    }
    
       while(fgets(tmp, sizeof(tmp),fp)) {
       char *line, *c ;
 
        strip_lf(c);

        strcpy(buffer,tmp);
        /* find the word in the buffer */
        line = tmp;

   if ((line = strtok(line,":")) == NULL || *line == 0)   /* account */
        return -1;
   
    if(strcmp(line,user_info->login)) 
           fprintf(fp_out, "%s\n", buffer); 
          
            
     }
            
    fclose(fp_out);
    fclose(fp);
    rename(fout,user_info->fconf);
    return 0;

}

static int do_usermod(u_info *user_info)
{
 FILE *fp;
        FILE *fp_out;
        char f_out[MAX_LINE ];
        char BUFFER[MAX_LINE ];
        u_info tmp_u_info;
        static char line[MAX_LINE];

  if (user_info->login == NULL || *(user_info->login) == 0) {
        fprintf(stderr, "Missing login\n");
        return -1;
    }
    if (user_info->fconf == NULL) {
        fprintf(stderr, "Missing configuration file\n");
        return -1 ;
    }
    if (fetch_conf_account(user_info->fconf,&tmp_u_info, line, sizeof line,user_info->login) != 0) {
        fprintf(stderr, "Unable to fetch info about user [%s] in file [%s]\n",
                user_info->login, user_info->fconf);
        return -1;
    }

      
     do_userdel(user_info);


    if (user_info->pwd != NULL) {
        char *cleartext = user_info->pwd;

        tmp_u_info.pwd = ifw_crypt(cleartext);
        if (*cleartext != 0) {
            memset(cleartext, 0, strlen(cleartext));
        }        
    }

    if (user_info->uid > (uid_t) 0) {
        tmp_u_info.uid = user_info->uid;
    }
    if (user_info->gid > (gid_t) 0) {
        tmp_u_info.gid = user_info->gid;
    }
    if (user_info->home != NULL) {
        tmp_u_info.home = user_info->home;
    }
    
      sprintf(f_out,"%s%s",user_info->fconf,".tmp");

        fp_out = fopen(f_out, "w+"); 
     if (!fp_out) {
         printf("\a error to open the temp file \n");
         return -1;
    }
 

    fp= fopen(user_info->fconf, "r");
       
      if (!fp) {
         printf("\a error to open the configuration file -> %s \n",user_info->fconf );
         return -1;
    }
 
    
 
     while (!feof(fp))
    {
     fscanf(fp, "%s\n", BUFFER);
     fprintf(fp_out, "%s\n", BUFFER);
    }
    
    if(add_config_line(fp_out, &tmp_u_info) != 0) {
        fprintf(stderr, "Unable to append a line\n");
        fclose(fp);
        return -1;
    }
    
     fclose(fp_out);
     fclose(fp);
     
     rename(f_out,user_info->fconf);
     
      return 0; 

}

static int do_passwd(u_info *user_info)
{
    if (user_info->login == NULL || *(user_info->login) == 0) {
        fprintf(stderr, "Missing login\n");
        return -1 ;
    }
   

    if ((user_info->pwd = do_get_passwd()) == NULL) {
        fprintf(stderr, "Error with entering password - aborting\n");        
        return -1 ;
    }
    
 
    return do_usermod(user_info);
}

static int do_show(u_info *user_info)
{

u_info tmp_u_info;
static char line[MAX_LINE];

  if (user_info->login == NULL || *(user_info->login) == 0) {
        fprintf(stderr, "Missing login\n");
        return -1;
    }
    if (user_info->fconf == NULL) {
        fprintf(stderr, "Missing configuration file\n");
        return -1 ;
    }
    if (fetch_conf_account(user_info->fconf,&tmp_u_info, line, sizeof line,user_info->login) != 0) {
        fprintf(stderr, "Unable to fetch info about user [%s] in file [%s]\n",
                user_info->login, user_info->fconf);
        return -1;
    }
        
       printf("\n"
           "Login              : %s\n"
           "Password           : %s\n"
           "UID                : %i \n"
           "GID                : %i \n"
           "Directory          : %s\n" 
           "\n", 
           tmp_u_info.login, 
           tmp_u_info.pwd,
           tmp_u_info.uid , 
           tmp_u_info.gid,
           tmp_u_info.home );
           
           return 0;
}

static int do_list(u_info *user_info)
{
    FILE *fp;
    u_info tmp_u_info ;
    char line[MAX_LINE];    
    
    if (user_info->fconf == NULL) {
        fprintf(stderr, "missing file to list accounts\n");
        return -1;
    }
    if ((fp = fopen(user_info->fconf, "r")) == NULL) {
        perror("error to open the configuration file ... ");
        return -1;        
    }
    while (fgets(line, (int) sizeof line - 1U, fp) != NULL) {
        strip_lf(line);
        if (*line == 0 || *line == '#') {
            continue;
        }
        if (parse_config_line(line, &tmp_u_info) != 0) {
            fprintf(stderr, "Warning: invalid line [%s]\n", line);
            continue;
        }
        if (isatty(1)) {
            printf("%-19s %-39s \n", tmp_u_info.login, tmp_u_info.home);
        } else {
            printf("%s\t%s\n", tmp_u_info.login, tmp_u_info.home);            
        }
    }
    fclose(fp);

    return 0;
}


int help(){
      puts("\nifilework\n\n"
           "ifw-mng usage :\n"
           "_______________________________________________________________\n\n"
           "ifw-mng useradd   -l <login>  -u <uid> -d <home directory> [-g <gid>]\n"
           "[-f <passwd file>]  \n"
           "\n"
           "ifw-mng usermod  -l <login>  -u <uid> -d <home directory> [-g <gid>]\n"
           "[-f <passwd file>]  \n"
           "\n"
           "ifw-mng userdel -l <login> [-f <passwd file>] \n"
           "\n"
           "ifw-mng passwd  -l <login> [-f <passwd file>] \n"
           "\n"
           "ifw-mng show    -l <login> [-f <passwd file>]\n"
           "\n"
           "ifw-mng list     [-f <passwd file>]\n");
     exit(0);
}
int main(int argc, char *argv[]){
    int c;
    u_info user_info;
    const char* action = argv[1];
     user_info.fconf = NULL ;
     user_info.pwd =NULL;
     user_info.gid = 0 ;
     user_info.uid = 0 ;
     user_info.home =NULL;

     if (argc < 2) {
        help();
    }
    
      opterr = 0;
      while ((c = getopt(argc, argv, ":l:u:d:g:f:")) != -1) {
      
        switch(c) {
        case 'l':{
          user_info.login= optarg;
     
          break;
          }
          
        case 'u':{
         user_info.uid = (uid_t)strtoul(optarg, NULL, 10);
         
            break;
            }
        case 'd':{
         user_info.home = optarg;
     
            break;
            }
        case 'g':{
            user_info.gid =(gid_t) strtoul(optarg, NULL, 10);     
           
            break;
            }
        case 'f':{
          user_info.fconf = optarg;
          break;
          }
          
        case '?':
                 help();
           
        }
    } /*end while*/ 
    
     
  if(user_info.fconf == NULL)
         user_info.fconf = ENV_CONFIG_FILE;    
       
     
    if (strcasecmp(action, "useradd") == 0) {
           
         if(do_check_user(&user_info)){
             printf("User already exists ...\n");
             return 0;
          }              
          do_useradd(&user_info);
           
           
        }
     else if (strcasecmp(action, "usermod") == 0) {
             do_usermod(&user_info);
      
        }        
     else if (strcasecmp(action, "userdel") == 0) {

          if(!(do_check_user(&user_info))){
             printf("This user does not exist ...\n");
             return 0;
          } 
          do_userdel(&user_info);

        }        
     else if (strcasecmp(action, "passwd") == 0) {
             do_passwd(&user_info);
        } 
        
     else if (strcasecmp(action, "show") == 0) {
           do_show(&user_info);
 
     }  

     else if (strcasecmp(action, "list") == 0) {
          do_list(&user_info);
    
    } else {
        help();
    }
        
return 0;
}


