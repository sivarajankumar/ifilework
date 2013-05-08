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
 

/* Server multi-client use select() to this */

#include <string.h>
#include <stdio.h>
#include <strings.h>/* memset */
#include <unistd.h>/*to read and write*/
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

#include <io.h> 
#include <fcntl.h>
#include <errno.h>

// Windows exclusive header
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <winsock.h>

#include <Wininet.h>

#define MAXLINE 4096
#define MAXSUB  200


/*Server version*/
char *version="0.0.1";
/*Global variable */
char buf_path[255];

int tabsockfd[FD_SETSIZE];                               
int size=0;  
 
#define ACK   2         


 /* Prototype */
int control_read( int f_read, int index);


/*Return the size of file*/

long int size_of_file(char* filename){

 struct stat buf;
 
   // Get data associated with "crt_stat.c": 
   stat( filename, &buf );

      // Output some of the statistics: 
      //printf( "File size     : %ld\n", buf.st_size ); 
      return   (long int)  buf.st_size;   /*64 bit*/
            
}
   
int print_dir(int sock,char* path){
    

   char msg[1024];
       WIN32_FIND_DATA FindFileData;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   char DirSpec[MAX_PATH];  // directory specification
   DWORD dwError;
   unsigned char buff_confirm[255];
   
   printf ("Target directory is %s\n", path);
   strncpy (DirSpec, path, strlen(path) + 1 );
   strncat (DirSpec, "\\*", 3);

   hFind = FindFirstFile(DirSpec, &FindFileData);

   if (hFind == INVALID_HANDLE_VALUE) 
   {
      printf ("Invalid file handle. Error is %u\n", GetLastError());
      send(sock,"stop", strlen("stop"),0) ;
      return (-1);
   } 
   else 
   {
        
      /* First file name is */
      if (send(sock, FindFileData.cFileName, strlen(FindFileData.cFileName),0) == -1 ){
      perror("send()");
      send(sock,"stop", strlen("stop"),0) ;
      }
      
      while (FindNextFile(hFind, &FindFileData) != 0) 
      {
      recv(sock,msg,1024,0);     
         /* Next file name is */ 
        char filename[255] ;
       strcpy(filename , FindFileData.cFileName );
      if (send(sock,filename, sizeof(filename),0) == -1){
       perror("send()");
       send(sock,"stop", strlen("stop"),0) ;
      }
      memset(filename, 0, sizeof(filename)); // Clean the buffer      
          
      } /* end while*/
      send(sock,"stop", strlen("stop"),0) ;
      FindClose(hFind);
      return (0);
          
   }
  
   
   return (0);
    
}

int serv_get_file(int sd,char* path,int index){
  
        char buf_file[1024];
        int fsize;
        int wsize;
        int num_blks;
        int count = 0;       
        int error;
        char buffer[1024];
        
        int ack = ACK;  
        ack = htons(ack);
    
          FILE* fp = fopen(path,"wb+"); 
          
          recv(sd,buf_file,1024,0);      
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
                  
                fwrite(buffer,1,wsize,fp);
                count++;
        } /* end while */
        
         if (rem){
               send(sd,(char *)&ack,sizeof(ack),0);
               memset(buffer, 0, sizeof(buffer));   
               error = recv(sd,buffer,1024,0);              
               fwrite(buffer,1,rem,fp);
                      
         } 
         puts("\n");
        fclose(fp);
        return 0;
    
}


int serv_remove(int sock,char* path){
    
if(DeleteFile(path)==0){
                        
	// Failed
		printf("Could not remove the program." );
		send(sock,"no_rm", strlen("no_rm"),0) ;
    return 0;
 }
 
   printf("Remove is ........... [OK]\n");
   send(sock,"rm_ok", strlen("rm_ok"),0) ;
   return 0;

}

int serv_exec(int sock,char* path){
    
  PROCESS_INFORMATION pi;
	STARTUPINFO si;
	
	// Initialize
	memset(&si,0,sizeof(si));
	si.cb = sizeof(si);

	// Execute
	if(!CreateProcess(NULL, path, NULL, NULL, false, 0, NULL,NULL,&si,&pi)) {
		// Failed
		printf("Could not run the program." );
		send(sock,"no_exec", strlen("no_exec"),0) ;
		return 0;
	}
   
  
printf("Execute is ........... [OK]\n");
send(sock,"exec_ok", strlen("exec_ok"),0) ;
   return 0;

}

int serv_send_file(int sock,char* path){

    
     char   buf_file[1024];
     int ack = 0; 
     long int size = size_of_file(path);
     memset(buf_file, 0, sizeof(buf_file));
     sprintf(buf_file,"%d", size);
     //printf(" File size : %s\n", buf_file);
    
    /*Send the file size */
    send(sock,buf_file,1024,0);
    
    FILE* fp = fopen(path,"rb+");     
       
 
               while(!feof(fp)){
                               
               recv(sock,(char *)&ack,sizeof(ack),0);
               if (ntohs(ack) != ACK) {
                printf("Server: ACK not received ...\n");
                return 0;
              }
                    memset(buf_file, 0, sizeof(buf_file));
                   fread(buf_file,sizeof(char),1024,fp);
                   
                    if (ferror(fp)){
                     send(sock,"readerr",1024,0);
                     break;
                    }
                    
                   send(sock,buf_file,1024,0);
 
               }            
            
            fclose(fp);
           
        return 0;

}

/*Return 1 if is a regular file*/
int path_is_file(char* path){
    
  struct stat st ;
 
  if ( stat(path,&st) == -1 )
    return 0;
 
 
  if (!S_ISREG(st.st_mode))
    return  0;
 
  return 1 ;

}

int path_is_dir(char* path){

  struct stat s ;
  
  if ( stat(path,&s) == -1 )
    return 0;
 
  if (!S_ISDIR(s.st_mode))
    return  0;
 
  return 1 ;
 
}

int read_path(int sock, int index){
 
    int n;
    
    send( sock, "now_path", strlen("now_path"), 0 );
    
    memset(buf_path, 0, sizeof(buf_path));
    control_read (recv(sock, buf_path , sizeof(buf_path), 0), index );
    
    if (path_is_file(buf_path)){
       /* printf("IS FILE ...\n"); */               
       send( sock, "is_file", strlen("is_file"), 0 );     
     }
   
    if (path_is_dir(buf_path)){
       send( sock, "is_dir", strlen("is_dir"), 0 );
      /*printf("IS FOLDER ...\n"); */
   
     }
   
    if(!path_is_file(buf_path) && !path_is_dir(buf_path)){
       send( sock, "unknown", strlen("unknown"), 0 );
     }  
       
}




int serv_connect(){
    
  

    int rec;
#define PORT 5000

        puts("____________________________________\n" );
        printf("Ifilework Server -- version : %s \n",version );
        puts("____________________________________" );
        
        WSADATA WSAData;
        WSAStartup(MAKEWORD(2,0), &WSAData);
        
        SOCKET sock;
        SOCKADDR_IN sin;

        struct hostent *phe;
        char buffer[64];


      //  gethostname(buffer, sizeof(buffer));
     // phe = gethostbyname(buffer);
    // memcpy(&sin.sin_addr.s_addr, phe->h_addr, phe->h_length);
sin.sin_addr.s_addr = htonl(INADDR_ANY);

char *IP;
IP = inet_ntoa(sin.sin_addr);

printf("My IP : %s \n",IP);
      
        sock = socket(AF_INET, SOCK_STREAM, 0);
        printf("My SOCKET : %i \n",sock);
        sin.sin_family    = AF_INET;
        sin.sin_port    = htons(PORT);
        
        bind(sock, (SOCKADDR *)&sin, sizeof(sin));
        
        listen(sock, 5);
        printf( "Listening port %d\n", PORT );
           
         return sock;
     
}

int  command( int sock, int index){
  
    char buffer[255];
    memset(buffer, 0, sizeof(buffer));
   control_read(recv(sock, buffer, sizeof(buffer),0), index );
   /* printf("Function command:%s\n",buffer); */
    
    if (!strcmp(buffer,"path")){
          read_path(sock,index);
          return 0;
    }  
     
    if (!strcmp(buffer,"ls")){
        print_dir(sock,buf_path);
        return 0;
    }  
    
     if (!strcmp(buffer,"exec")){
        serv_exec(sock,buf_path);
     } 
     
     if (!strcmp(buffer,"rm")){
        serv_remove(sock,buf_path);
     }
   
    if (!strcmp(buffer,"get")){
        serv_send_file(sock,buf_path);
    }
    
    if (!strcmp(buffer,"send")){
        serv_get_file(sock,buf_path, index);
    }
     
    if (!strcmp(buffer,"quit")){
    control_read( -1 , index);
    }
 
     return 0;

}
 

int control_read( int f_read, int index){
    if (f_read == -1){
          /* perror("recv()"); */
          printf("Client %i are disconnected\n",tabsockfd[index]);
          closesocket(tabsockfd[index]);
          tabsockfd[index]=tabsockfd[size-1];
          size--;  
          int i;
          for(i=1;i<size;i++)
           printf("Table socket %i :\n",tabsockfd[i]);
          return 0;        
        }
        
        if(f_read == 0) {
          printf("peer has terminated connexion\n");
          close(tabsockfd[index]); 
          tabsockfd[index]=tabsockfd[size-1]; 
          size--;
        } 
           return 0 ;
}


int main(){
    
 int sd =  serv_connect();
  
  while(1){
  
#define MAXBUFFERSIZE 1460 

  int sockfd, sockfd2;       
  fd_set readfds;             
  struct sockaddr_in client;  
  socklen_t sin_size;         
  int port;                   
  int sockmax=0;             
  int i;
  


  sin_size = sizeof(struct sockaddr_in);
 
  printf("Run Server OK\nWaiting for connection ...\n");
 
  tabsockfd[0]=sd; 
  size++;


  while(1){
  
    FD_ZERO(&readfds);        
    sockmax = tabsockfd[0];
    for(i=0;i<size;i++){     
      FD_SET(tabsockfd[i],&readfds);
      if(sockmax < tabsockfd[i]) sockmax = tabsockfd[i]; 
    }
  
    if(select(sockmax+1,&readfds,NULL,NULL,NULL) == -1){
      perror("select()");
      exit(EXIT_FAILURE);
    }
   
    if(FD_ISSET(sd,&readfds)){
      sockfd2 = accept(sd,(struct sockaddr*) &client,&sin_size);
      if((sockfd2) == -1){
        perror("accept()");
        exit(EXIT_FAILURE);
      }
      printf("Connection established with %s\n", inet_ntoa(client.sin_addr));
      tabsockfd[size]=sockfd2; 
      size++;
     
    }
     
    for(i=1;i<size;i++){
      if(FD_ISSET(tabsockfd[i],&readfds)){ 
       /*printf("First Table socket %i:\n",tabsockfd[i]); */
        command(tabsockfd[i], i);
        }
      }
    }
  }
}
