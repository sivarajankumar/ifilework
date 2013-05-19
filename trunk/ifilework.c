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

#include <crypt.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include "ifilework.h"

#define _XOPEN_SOURCE 

void strip_lf(char *str)
{
    char *f;
    
    if (str == NULL) {
        return;
    }
    if ((f = strchr(str, '\r')) != NULL) {
        *f = 0;
    }    
    if ((f = strchr(str, '\n')) != NULL) {
        *f = 0;
    }
}

int parse_config_line(char *line, u_info *user_info){
 


   user_info->pwd = NULL;
       user_info->uid = 0;         
       user_info->gid = 0;
       user_info->home = NULL; 
    
 if ((line = strtok(line, CONF_LINE_SEP)) == NULL || *line == 0)   /* account */
        return -1;
   
    user_info->login = line;
    
if ((line = strtok(NULL, CONF_LINE_SEP)) == NULL || *line == 0)    /* pwd */
        return -1;
    
    user_info->pwd = line;
    
if ((line = strtok(NULL, CONF_LINE_SEP)) == NULL || *line == 0)    /* uid */
        return -1;
    
    user_info->uid = (uid_t)strtoul(line, NULL, 10);
    
if ((line = strtok(NULL, CONF_LINE_SEP)) == NULL || *line == 0)    /* gid */
        return -1;
 
    user_info->gid =(gid_t)strtoul(line, NULL, 10); ;
    
if ((line = strtok(NULL, CONF_LINE_SEP)) == NULL || *line == 0)    /* home */
        return -1;
    

    
    user_info->home = line; 

        return 0;

}

int fetch_conf_account(char*file, u_info *user_info,
                            char* line, size_t sizeof_line, char *login)
{
    FILE *fp;
    int ret = -1;
    
    if ( file == NULL || user_info == NULL || line == NULL ||
        sizeof_line < (size_t) 2U || login == NULL ||
        *login == 0) {
        fprintf(stderr, "bad arguments to fetch account\n");
        return -1;
    }
    if ((fp = fopen(file, "r")) == NULL) {
        perror("error to open the configuration file ... ");
        return -1;
    }
    while (fgets(line, (int) sizeof_line - 1U, fp) != NULL) {
        strip_lf(line);
        if (*line == 0 || *line == '#' ) {
            continue;
        }
        if (parse_config_line(line ,user_info) != 0) {
            fprintf(stderr, "Warning: invalid line [%s]\n", line);
            continue;
        }
        if (strcmp(login, user_info->login) != 0) {
            continue;
        }
        ret = 0;
        break;
    }
    fclose(fp);

    return ret;
}

int do_check_user(u_info *user_info){

      FILE *fp;
      char buffer[MAX_LINE];
      

  
    fp= fopen(user_info->fconf, "r");
       
    if (!fp) {
         printf("error to open the configuration file ... \n");
         return -1;
    }
    
       while(fgets(buffer, sizeof(buffer),fp)!= NULL) {
       char *line, *c ;
 
        strip_lf(c);

        /* find the word in the buffer*/
        line = buffer;

         if ((line = strtok(line,":")) == NULL || *line == 0)   /* account */
              return -1;
   
          if(!strcmp(line,user_info->login)) {
           fclose(fp);
           return 1;    
           }
            
       }

      fclose(fp);
      return 0;

}


char * ifw_crypt(const char *pwd){
     unsigned long seed[2];
     char salt[] = "$1$........";
const char *const seedchars =
"./0123456789ABCDEFGHIJKLMNOPQRST"
"UVWXYZabcdefghijklmnopqrstuvwxyz";

int i;
/* Generate a (not very) random seed.
You should do it better than this... */
seed[0] = time(NULL);
seed[1] = getpid() ^ (seed[0] >> 14 & 0x30000);
/* Turn it into printable characters from `seedchars'. */
for (i = 0; i < 8; i++)
salt[3+i] = seedchars[(seed[i/5] >> (i%5)*6) & 0x3f];

return (char *) crypt(pwd, salt); 
}




