
/*   2011 - This file is part of ifilework project
 *   mailto : myfilesadev@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <io.h> 
#include <fcntl.h>
#include <errno.h>

// Windows exclusive header
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock.h>


/*ifilework client version*/
char *version="0.0.1";

int cli_command(int sd,char* command,char* path);
#define ACK                   2


/* counts the number of words */
int word_count(char* buffer){
 
        int j=0;
        int i=0;
  
        do
        {
            while (buffer[i] == ' ')
            {
               i++;
           }
               j++;
           while (buffer[i] != ' ' && buffer[i] != '\0')
               i++;
       }while(buffer[i] != '\0');
 
       return j;
 }


/* Connect the client */
SOCKET cli_connect(char* host){


       int res;
       char ip[16];
       int port = 5000 ;

         puts("____________________________________\n" );
        printf("Ifilework Client -- version : %s \n",version );
         puts("____________________________________" );
        WSADATA WSAData;
        WSAStartup(MAKEWORD(2,0), &WSAData);
        
        SOCKADDR_IN sin;   
        SOCKET sd;      
        sd = socket(AF_INET, SOCK_STREAM, 0);
         
        sin.sin_addr.s_addr    = inet_addr(host);
        sin.sin_family    = AF_INET;
        sin.sin_port    = htons(port);
        
        
        if( connect(sd, (SOCKADDR *)&sin, sizeof(sin)) != 0 ){
            printf("\n Does not connect to server [ %s ]  port %i \n",host, port );
            getchar();
            return 0;
        }
        puts("\n Connected ...\n" );
                
        return  sd; 
          
}


int client_get(int sd , char *path){
    
        char buf_file[1024];
        int fsize;
        int wsize;
        int num_blks;
        int count = 0;
        int sz_recv = 0;
        int error;
         char buffer[1024];
         
        int ack = ACK;  
        ack = htons(ack);


    char *fname = strrchr (path, '\\' ) ; 
    *fname++;
    
          FILE* fp = fopen(fname,"wb+"); 
          
          recv(sd,buf_file,1024,0);
          printf("FILE NAME : %s\n", fname);
          printf("FILE SIZE : %s\n", buf_file);
          /*Converte string to int */
          fsize = atoi(buf_file);          
         
          if(fsize < 1024)
              wsize = fsize;
              else 
              wsize = 1024;
              
              num_blks = fsize / 1024;
              int rem =  fsize % 1024;
              
                while(num_blks > count){
                send(sd,(char *)&ack,sizeof(ack),0);
                memset(buffer, 0, sizeof(buffer));   
                error = recv(sd,buffer,1024,0);
                
                 
                if(strcmp(buffer,"readerr")==0){
                  printf("Server error read file.\n");    
                  break;                        
                }
               
                /*************************************/
                if(error != -1 )
                 sz_recv += wsize;
                           
                 printf("BLOCKS TRANSFER : %i/%i", sz_recv,fsize);  
                
                /* To delete the previous line */
                 printf("\r");
                 /* Flush all char in buffer */
                 fflush(stdout);
              
                fwrite(buffer,1,wsize,fp);
                count++;
        } /* end while */
        
         if (rem){
               send(sd,(char *)&ack,sizeof(ack),0);
               memset(buffer, 0, sizeof(buffer));   
               error = recv(sd,buffer,1024,0);   
               
               if(error != -1 )
                 sz_recv += rem;
                                        
                 printf("BLOCKS TRANSFER : %i/%i", sz_recv,fsize);
                          
                /* To delete the previous line */
                 printf("\r");
                 /* Flush all char in buffer */
                 fflush(stdout);
                    
                fwrite(buffer,1,rem,fp);
                  
         
         }
             
             
        puts("\n");
        fclose(fp);


}
int client_ls(int sd){
  
  char buffer[255];
  
 
  while(1){
    send(sd,"ok",1024,0);
    memset(buffer, 0, sizeof(buffer));
    recv(sd, buffer, sizeof(buffer),0);
    
    
     if(strcmp(buffer,"stop")==0) 
      break;
     else
     printf("%s \n",buffer);  
     
     }

      return 0;
}

int path_is_file(char* path){
    
 
  struct stat st ;
 
  if ( stat(path,&st) == -1 )
    return 0;
 
 
  if (!S_ISREG(st.st_mode))
    return  0;
 
  return 1 ;


}

/*Return the size of file*/

long int size_of_file(char* filename){

 struct stat buf;
 

   // Get data associated with "crt_stat.c": 
   stat( filename, &buf );

      // Output some of the statistics: 
      //printf( "File size     : %ld\n", buf.st_size ); 
      return   (long int)  buf.st_size;   /*64 bit*/
     
   
   
}

int client_send(int sd , char *path){
    
     char local_path[255];
     char   buf_file[1024];
     int ack = 0;
    
    retype:
    printf("Local file name:");
    gets(local_path);
    
    if(!path_is_file(local_path)){
       printf("File unknown: %s\n",local_path);     
       goto retype;                           
    }
    
     long int size = size_of_file(local_path);
     memset(buf_file, 0, sizeof(buf_file));
     sprintf(buf_file,"%d", size);
     //printf("GET function file size : %s\n", buf_file);
    
    /*Send the file size */
    send(sd,buf_file,1024,0);
    
    FILE* fp = fopen(local_path,"rb+");     
       
 
               while(!feof(fp)){
               recv(sd,(char *)&ack,sizeof(ack),0);
               if (ntohs(ack) != ACK) {
                printf("Client: ACK not received ...\n");
                return 0;
              }
                    memset(buf_file, 0, sizeof(buf_file));
                   fread(buf_file,sizeof(char),1024,fp);
                   
                    if (ferror(fp)){
                     send(sd,"readerr",1024,0);
                     break;
                    }
                    
                   send(sd,buf_file,1024,0);
 
               }            
            
            fclose(fp);
           
        return 0;
  
}



int send_path(int sock,char* path,char* command){


/* Send path */    
     
    if(!strcmp(command,"quit")){
        return 1;
    }
  
         char buf_path[255];
        int n ;
        char *s_path = "path";
        int result;
   
      send(sock, s_path, strlen(s_path), 0);
      
      while(1){
      memset(buf_path, 0, sizeof(buf_path));
      recv(sock, buf_path , sizeof(buf_path), 0);
      if(!strcmp(buf_path,"now_path")){
      send( sock, path, strlen(path), 0 );
       break;
      }
      }
             
             memset(buf_path, 0, sizeof(buf_path));
   
      recv(sock, buf_path , sizeof(buf_path), 0);
   
   
    if(!strcmp(command,"get") && !strcmp(buf_path,"is_file")){
        return 1;
    }
    
     if(!strcmp(command,"send") && !strcmp(buf_path,"unknown")){
       return 1;
    }
    
    if(!strcmp(command,"send") && !strcmp(buf_path,"is_file")){
        printf("File exist %s \n",path);
        return 0;
    }
    
    if(!strcmp(command,"exec") && !strcmp(buf_path,"is_file")){
        return 1;
    }
     if(!strcmp(command,"rm") && !strcmp(buf_path,"is_file")  || !strcmp(buf_path,"is_dir") ){
        return 1;
    }
   
    if(!strcmp(command,"ls") && !strcmp(buf_path,"is_dir")){
       return 1;
    }
      
    if (!strcmp(buf_path,"unknown")){

      puts("Path unknown...");
        return 0;
    }
 
    return 0;


}

int client_exec(int sd){
 
 
 char buffer[255];
 
 

    memset(buffer, 0, sizeof(buffer));
    recv(sd, buffer, sizeof(buffer),0);
    
     if(!strcmp(buffer,"no_exec" )){
     printf("exec .......... [failed]\n");
     return 0;
    }
    
    if(!strcmp(buffer,"exec_ok" )){
     printf("exec success .......... [OK]\n");
     return 0;
    }
    
    return 0;
    
}

int client_remove(int sd){
 
    char buffer[255];
 
    memset(buffer, 0, sizeof(buffer));
    recv(sd, buffer, sizeof(buffer),0);
    
     if(!strcmp(buffer,"no_rm" )){
     printf("Remove .......... [failed]\n");
     return 0;
    }
    
    if(!strcmp(buffer,"rm_ok" )){
     printf("Remove success .......... [OK]\n");
     return 0;
    }
    
    return 0;
    
}


char* is_command(char *command){
        if(!strcmp(command,"get" ))  return command; 
        if(!strcmp(command,"send"))  return command;
        if(!strcmp(command,"ls"  ))  return command;
        if(!strcmp(command,"quit"))  return command;
        if(!strcmp(command,"exec"))  return command;
        if(!strcmp(command,"rm"))    return command;
        
return 0;

}

int cli_command(int sd,char* command,char* path){

      if(!strcmp(command,"ls")){
        send(sd, command, strlen(command),0);
            
        client_ls(sd);
       }

      if(!strcmp(command,"get")){
                                 
       send(sd, command, strlen(command),0);
        client_get(sd ,path);
        } 
        
         if(!strcmp(command,"send")){
                                 
       send(sd, command, strlen(command),0);
        client_send(sd ,path);
        }
        
         if(!strcmp(command,"exec")){
                                 
       send(sd, command, strlen(command),0);
        client_exec(sd);
        }
        /* Remove*/
        if(!strcmp(command,"rm")){
                                 
         send(sd, command, strlen(command),0);
        client_remove(sd);
        }
        
     if(!strcmp(command,"quit")) {
       send(sd, command, strlen(command),0);
        close(sd) ;
       exit(0);
       
     }
       
     
}

int main(int argc, char *argv[])
{
    SOCKET sd = cli_connect(argv[1]);
    
    char  *delim= "\t \n"; 
    char  *d= "\n"; 
    char *command = NULL;
    char *path = NULL;
    char cmd_line[250];
    
while(1){
  
  printf(">");
    
  fflush(stdin);
  gets(cmd_line);

     if(word_count(cmd_line) == 1 ){
   
     command=strtok(cmd_line,delim);
     path=".";
     }
 
     if(word_count(cmd_line) >= 2 ){

     command=strtok(cmd_line,delim);
     path=strtok(NULL,d);
     }
       
    if (is_command(command)){

        if (send_path(sd,path,command))
            cli_command(sd,command,path);
        
     }else
     printf("Unknown command %s\n", command);
     
 } //end while

} 


