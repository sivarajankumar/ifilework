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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

/*ifilework client version*/
char *version="0.0.1";

/**********************/
/* Global Variables   */
/**********************/
char *                   commandName;
int                      ftpPort;
char                     hostName[256];
struct sockaddr_in       hostAddr;
struct sockaddr_in       dataAddr;
struct sockaddr_in       controlAddr;
int                      controlSocket;
int                      dataSocket;
char                     last_reply [1024*8];
int                      connected; /* 1 = yes*/
int                      loginOk; /* 1 = yes*/
char                     user[64];
char                     password[64];
char                     type_Transfer;

/***************************/
/* Function prototypes     */
/***************************/

void   initialization(void);
int    Connect( char *host );
void   Close_All(void);
int    Read_Reply(void);
void   Send_Command( char * cmd );
int    Quit_FTP(void);
int    Verify_Connect(void);
int    Login( char * aUser, char * aPasswd );
int    Cwd( char * hostDirectory );
int    Pwd(void);
int    Delete_File( char * fileName );
int    Info_System( void );
int    Choose_Type( char * type );
int    Choose_Port( void );
char * Get_Local_Name( char * fileName );
int    InitDataConnect( void );
int    Read_File( char * cmd, char * fileName, int display );
int    Write_File( char * cmd, char * fileName  );
void   Exec_Command(void);
void   Print_Help(void);


/***********************/
/* Start here          */
/***********************/

int main( int argc, char ** argv )
{
  int result= 0;

  commandName = argv[0];

  initialization();


  if ( argc > 1 ) 
    result= Connect( argv[1] );
  

  if ( result) 
    Login( "", "" );


  Exec_Command();

  Close_All();

  return 0;
}



void initialization(void)
{
  struct servent *sp;

   /* Query the list of services to find the service ftp */
   /* TCP layer is required because the FTP protocol */
   /* works with it. */
  
  sp = getservbyname("ftp", "tcp");
  if (sp == NULL) {
    fprintf(stderr, "%s: service 'ftp/tcp' unknown.\n", commandName );
    exit(1);
  }

   /* Retrieve the port used by the FTP Service */
   /* The port is already in network order format, so it is not */
   /* needed to use the htons */
 
  ftpPort = sp->s_port;
}

int Connect( char *host )
{
  struct hostent *hp = 0;
  int len;

  /* If it was already connected, everything is closed */
  if ( connected )
    Close_All();

  /* We are trying to obtain information on the FTP server
     struct hostent which contains:
     the official name of the site (h_name)
     the alias list (h_aliases)
     type site address (h_addrtype)
     The address length (h_length)
     the address list (h_addr_list)
   */

  hp = gethostbyname(host);
  if (hp == NULL) {
    fprintf(stderr, "%s: %s: ", commandName, host);
    /* Print error message created by gethostbyname*/
    herror((char *)NULL);
    return 0;
  }
  /*It copies the address type in our address structure (sockaddr_in)
    to Connect to Server */
  hostAddr.sin_family = hp->h_addrtype;

  /* Then copies the contents of the first address in our structure */
  if (hp->h_length > sizeof(hostAddr.sin_addr)) {
    hp->h_length = sizeof(hostAddr.sin_addr);
  }
  memcpy(&hostAddr.sin_addr, hp->h_addr_list[0], hp->h_length);

  /* It copies the official name of the site in our global variable hostName
     to know at any time the name of the site we connecteds */
  (void) strncpy(hostName, hp->h_name, sizeof(hostName));
  hostName[sizeof(hostName)-1] = 0;


  controlSocket = socket(hostAddr.sin_family, SOCK_STREAM, 0);
  if (controlSocket < 0) {
    fprintf( stderr, "%s: socket", commandName );
    perror("");
    return 0;
  }
  
  hostAddr.sin_port = ftpPort;

  /*We try to Connect to the FTP server address and port
    we just define */
  if ( connect(controlSocket, (struct sockaddr *)&hostAddr, sizeof (hostAddr)) < 0 ) {
    fprintf( stderr, "%s: connect ", commandName );
    perror("");
    Close_All();
    return 0;
  }

  /* It recovers information about our address*/
  len = sizeof (controlAddr);
  if (getsockname(controlSocket, (struct sockaddr *)&controlAddr, &len) < 0) {
    fprintf( stderr, "%s: getsockname", commandName );
    perror("");
    Close_All();
    return 0;
  }
  /* Nous sommes a present connecteds */
  connected = 1;
  printf("connected to %s.\n", hostName);

  /* Get server response */
  if ( Read_Reply() != 220 )
    {
      Close_All();
      return 0;
    }
  /* Connection Ok */
  return 1;
}

void Close_All(void)
{
  Quit_FTP();
  close( controlSocket );
  connected = 0;
}

int Read_Reply(void)
{
  int count = 0;
  char * tmp = last_reply ;
  char * tmp2 = last_reply ;
  int keep_on = 1;

  /* Read data on the control socket */

  while( keep_on ) {
    tmp += count;
    count = recv( controlSocket, tmp, sizeof(last_reply ), 0 );

    if ( count > 0 ) 
      {
	/* Add a null character at the end of the chain,
           to mark the end*/
	if ( count < sizeof(last_reply ) )
	  tmp[count] = 0;
	else
	  last_reply [ sizeof(last_reply )-1 ] = 0;
	
	while( *tmp2 ) {
	  if ( *tmp2 == '\n' )
	    {
	      printf( "\n" );
	      keep_on =  *(tmp+3) == '-';
	    }
	  else
	    {
	      putchar( *tmp2 );
	    }
	  tmp2++;
	}
      }
    else
      /* failure */
      return -1;
  }
  /* send the reply code */
  return atoi(last_reply );
}

void Send_Command( char * cmd )
{
  char buffer[1024];

  sprintf( buffer, "%s%c%c", cmd, 13, 10 ); /* cmd + CR + LF */
  send( controlSocket, buffer, strlen(buffer), 0 );
}

int Quit_FTP(void)
{
  if ( connected )
    {
      Send_Command( "QUIT" );
      if ( Read_Reply() == 221 )
	{
	  loginOk = 0;
	  return 1;
	}
      else
	return 0;
    }
  else
    return 0;
}

int Verify_Connect(void)
{
  if ( ! connected )
    {
      fprintf( stderr, "You are not connectedd to a server.\n" );
      return 0;
    }
  if ( ! loginOk )
    {
      fprintf( stderr, "You must make a login.\n" );
      return 0;
    }
  return 1;
}

int Login( char * aUser, char * aPasswd )
{
  char cmd[1024];
  int reply;

  if ( ! connected )
    {
      fprintf( stderr, "You must first connect to a server.\n" );
      return 0;
    }

  if ( loginOk ) 
    {
      fprintf( stderr, "User '%s' on line.\n", user );
      return 0;
    }

  /* Send the user name */
  printf( "username: " );
  if ( *aUser ) {
    sprintf( user, "%s", aUser );
    printf( "%s\n", user );
  } else
    scanf( "%s", user ); 
  sprintf( cmd, "USER %s", user );
  Send_Command( cmd );
  reply= Read_Reply();
  if ( reply== 331 )
    {
      /* Since the name is accepted, it sends the password */
      printf( "password: " );
      if ( *aPasswd ) {
	 sprintf( password, "%s", aPasswd );
	 printf( "%s\n", password );
      } else
	 scanf( "%s", password );
      sprintf( cmd, "PASS %s", password );
      Send_Command( cmd );
      reply= Read_Reply();
    }
  
  if ( reply!= 230 )
    return 0; /* failure  */
  
  /* The user has been accepted */
  loginOk = 1;
  return 1;
}

int Cwd( char * hostDirectory )
{
  if ( ! Verify_Connect() )
    return 0;

  if ( strlen( hostDirectory ) > 0 )
    {
      char cmd[1024];
      int reply;
      sprintf( cmd, "CWD %s", hostDirectory );
      Send_Command( cmd );
      reply= Read_Reply();
      if ( reply!= 250 )
	return 0;
      /* success*/
      return 1;
    }
  else
    return 0;
}

int Pwd(void)
{
  int reply;

  if ( ! Verify_Connect() )
    return 0;

  Send_Command( "PWD" );
  reply= Read_Reply();
  if ( reply!= 250 )
    return 0;
  return 1;
}

int Delete_File( char * fileName )
{
  if ( ! Verify_Connect() )
    return 0;

  if ( strlen( fileName ) > 0 )
    {
      char cmd[1024];
      int reply;
      sprintf( cmd, "DELE %s", fileName );
      Send_Command( cmd );
      reply= Read_Reply();
      if ( reply!= 250 )
	return 0; /* failure  */
      /* success */
      return 1;
    }
  else
    {
      fprintf( stderr, "del: must specify a file\n" );
      return 0;
    }
}

int Info_System( void )
{
  int reply;

  if ( ! Verify_Connect() )
    return 0;

  Send_Command( "SYST" );
  reply= Read_Reply();
  if ( reply!= 250 )
    return 0; /* failure */
  /* success */
  return 1;
}

int Choose_Type( char * type )
{
  char cmd[1024];

  if ( ! Verify_Connect() )
    return 0;

  sprintf( cmd, "TYPE %s", type );
  Send_Command( cmd );
  if ( Read_Reply() != 200 )
    return 0; /* failure */

  type_Transfer = *type;
  return 1; /* success */
}

int Choose_Port(void)
{
  char buff[1024];
  char *a;
  char *p;
  int reply;

  if ( ! Verify_Connect() )
    return 0;

  /* Prepare the PORT command with the address of our socket */
  a = (char *)&dataAddr.sin_addr;
  p = (char *)&dataAddr.sin_port;
#define UC(b)   (((int)b)&0xff)
   /* We define the 4 bytes of address +
                     2 bytes port
   */
  sprintf( buff, "PORT   %d,%d,%d,%d,%d,%d",
	   UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
	   UC(p[0]), UC(p[1]));
  Send_Command( buff );
  reply= Read_Reply();
  if (reply!= 200) 
    return 0;  

  /* success */
  return 1;
}

char * Get_Local_Name( char * fileName )
{
  /* It removes any directory, to keep only the file name */
  char * result= fileName + strlen(fileName) - 1;
  while( (result!= fileName) && (*result!= '/') )
    result--;
  return result;
}

/* Prepare the channel for the transfer of data */
int InitDataConnect( void )
{
  int len;

  /* Create a socket  */

  dataAddr = controlAddr;
  dataAddr.sin_port = 0; /* the system is allowed to choose a port*/

  dataSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (dataSocket < 0) {
    fprintf( stderr, "%s: socket", commandName );
    perror("");
    return 0;
  }

  
  if (bind(dataSocket, (struct sockaddr *)&dataAddr, sizeof (dataAddr)) < 0) {
    fprintf( stderr, "%s: bind", commandName );
    perror("");
    close( dataSocket );
    dataSocket = -1;
    return 0;
  }

  /* Retrieve socket our address transfer, so that it
     PORT can send a request to the FTP server, so that it knows
     How to Connect to us
  */
  len = sizeof( dataAddr );
  if (getsockname(dataSocket, (struct sockaddr *)&dataAddr, &len) < 0) {
    fprintf( stderr, "%s: getsockname", commandName );
    perror("");
    close( dataSocket );
    dataSocket = -1;
    return 0;
  }

  if (listen(dataSocket, 1) < 0) {
    fprintf( stderr, "%s: listen", commandName );
    perror("");
    close( dataSocket );
    dataSocket = -1;
    return 0;
  }

  /* Send PORT command*/
  if ( ! Choose_Port() ) {
    close( dataSocket );
    dataSocket = -1;
    return 0;
  }

  /* Success */
  return 1;
}

int Read_File( char * cmd, char * fileName, int display  )
{
  int reply;
  int s;
  int c;
  FILE * dest, * src;
  char buff[1024];
  struct sockaddr_in from;
  size_t fromlen = sizeof(from);
  char *localFileName;

  if ( ! Verify_Connect() )
    return 0;

  if ( ! display )
    {
      /* Creer un fichier local pour stocker le transfert
	 des donnees */
      localFileName = Get_Local_Name( fileName );
      dest = fopen( localFileName, "w" );
      if ( ! dest )
	{
	  fprintf( stderr, "%s: Can not create file '%s'",
		   commandName, localFileName );
	  return 0;
	}
    }
 
   /* Create the socket transfer */
  if ( ! InitDataConnect() )
    return 0;
  

  /* Prepare the command file transfer */
  if ( strlen(fileName) > 0 )
    sprintf( buff, "%s %s", cmd, fileName );
  else
    sprintf( buff, "%s", cmd );

  /* Send order and test its success */
  Send_Command( buff );
  reply= Read_Reply();
  
  if ( reply!= 150 )
    {
      /* Server refused our command*/
      close( dataSocket );
      dataSocket = -1;
      return 0;
    }

  /* accept a new connection on the reserve Port
     transfer. */
  s = accept(dataSocket, (struct sockaddr *) &from, &fromlen);
  if (s < 0) {
    fprintf( stderr, "%s: accept", commandName );
    perror("");
    close(dataSocket);
    dataSocket = -1;
    return 0;
  }
  /* Close to the listening socket on the transfer port*/
  close(dataSocket);
  /* Recover the new connection as connection
     the transfer */
  dataSocket = s;

  src = fdopen( dataSocket, "r" );
  if ( ! src )
    {
      fprintf( stderr, "%s: fdopen", commandName );
      perror( "" );
      return 0;
    }

  /* Transfer the data file*/
  while ( (c = fgetc(src)) != EOF )
    {
      if ( display )
	putchar( c );
      else
	fputc( c, dest ); 
    }

  /* Close any open files and socket transfer */
  fclose( src );
  close( dataSocket );
  if ( ! display )
    fclose( dest );

  /* Check if the transfer went well*/
  reply= Read_Reply();
  
  if ( reply!= 226 )
    return 0; /* Failed reception File */

  /* Success */
  return 1;
}

int Write_File( char * cmd, char * fileName  )
{
  int reply;
  int s;
  int c;
  FILE * dest, * src;
  char buff[1024];
  struct sockaddr_in from;
  size_t fromlen = sizeof(from);
  char *localFileName;

  if ( ! Verify_Connect() )
    return 0;

  if ( ! ( src = fopen( fileName, "r" ) ) )
    {
      fprintf( stderr, "%s: Can not open file '%s'\n",
	       commandName, fileName );
      return 0;
    }
  localFileName = Get_Local_Name( fileName );

 /* Create the socket transfer */
  if ( ! InitDataConnect() )
    return 0;

   /* Prepare the command file transfer */
  sprintf( buff, "%s %s", cmd, localFileName );

  /* Send order and test its success */
  Send_Command( buff );
  reply= Read_Reply();
  
  if ( reply!= 150 )
    {
      /* Server refused our command */
      close( dataSocket );
      dataSocket = -1;
      return 0;
    }

    /* accept a new connection on the reserve Port
     transfer. */
  s = accept(dataSocket, (struct sockaddr *) &from, &fromlen);
  if (s < 0) {
    fprintf( stderr, "%s: accept", commandName );
    perror("");
    close(dataSocket);
    dataSocket = -1;
    return 0;
  }
 /* Close to the listening socket on the transfer port*/
  close(dataSocket);
   /* Recover the new connection as connection
     the transfer */
  dataSocket = s;

 
  dest = fdopen( dataSocket, "w" );
  if ( ! dest )
    {
      fprintf( stderr, "%s: fdopen", commandName );
      perror( "" );
      return 0;
    }

   /* Transfer the data file*/
  while ( (c = fgetc(src)) != EOF )
    fputc( c, dest ); 

   /* Close any open files and socket transfer */
  fclose( src );
  fclose( dest );
  close( dataSocket );

  /* Check if the transfer went well*/
  reply= Read_Reply();
  
  if ( reply!= 226 )
    return 0; /* Failed reception File */

  /* Succes */
  return 1;
}

void Exec_Command(void)
{
  char line[1024];
  char *cmd;
  char *arg;
  char *tmp;

      puts("____________________________________\n" );
      printf("Ifilework Client -- version : %s \n",version );
      puts("____________________________________" );

  for ( ; ; )
    {
      /* Read a line of text */
      printf( "> " );
      if ( ! fgets( line, sizeof(line), stdin ) || *line == '\n' ) {
	printf( "\n" );
	continue;
      }

      /* Determine the argument of the command */
      cmd = line;
      arg = line;
      /* Find the first space */
      while ( *arg && *arg > ' ' )
	arg++;

      *arg = 0;
      arg++;
      /* Skip separators*/
      while( *arg && *arg <= ' ' )
	arg++;
      tmp = arg;
      /* if we do not meet separator */
      while ( *tmp && *tmp >= ' ' )
	tmp++;
      *tmp = 0;

      /* Test command */
      if ( strcmp( cmd, "bye" ) == 0 )
	return;
      else if ( strcmp( cmd, "?" ) == 0 )
	Print_Help();
      else if ( strcmp( cmd, "open" ) == 0 ) {
	if ( *arg )
	  Connect( arg );
	else
	  fprintf( stderr, "open: you must provide the address of an FTP server.\n" ); 
	  }
      else if ( strcmp( cmd, "user" ) == 0 )
	Login( "", "" );
      else if ( strcmp( cmd, "bin" ) == 0 )
	Choose_Type( "I" );
      else if ( strcmp( cmd, "ascii" ) == 0 )
	Choose_Type( "A" );
      else if ( strcmp( cmd, "pwd" ) == 0 )
	Pwd();
      else if ( strcmp( cmd, "cd" ) == 0 ) {
	if (*arg)
	  Cwd( arg );
	else 
	  fprintf( stderr, "cd: you must specify a directory.\n" );
      } else if ( strcmp( cmd, "ls" ) == 0 ) {
	Choose_Type( "A" );
	Read_File( "NLST", arg, 1 );
      } else if ( strcmp( cmd, "dir" ) == 0 ) {
	Choose_Type( "A" );
	Read_File( "LIST", arg, 1 );
      } else if ( strcmp( cmd, "type" ) == 0 ) {
	Choose_Type( "A" );
	Read_File( "RETR", arg, 1 );
      } else if ( strcmp( cmd, "get" ) == 0 ) {
	if (*arg) {
	  Choose_Type( "I" );
	  Read_File( "RETR", arg, 0 );
	} else
	  fprintf( stderr, "get: you must specify a file.\n" );
      } else if ( strcmp( cmd, "put" ) == 0 ) {
	if (*arg) {
	  Choose_Type( "I" );
	  Write_File( "STOR", arg );
	} else
	  fprintf( stderr, "put: you must specify a file.\n" );
      } else if ( strcmp( cmd, "del" ) == 0 ) 
	Delete_File( arg );
      else if ( strcmp( cmd, "syst" ) == 0 )
	Info_System();
    }
}

void Print_Help(void)
{
  printf( "ifilework list of commands:\n" );
  printf( "___________________________\n" );
  printf( "\n" );
  printf( "?...............this helps\n" );
  printf( "open <server> ..Connect to the server <server>\n" );
  printf( "user............enter a login and password\n" );
  printf( "bye.............exit the program\n" );
  printf( "bin.............switch to binary mode\n" );
  printf( "ascii...........switch to text mode\n" );
  printf( "pwd.............display the current directory\n" );
  printf( "cd <dir>........go to the directory <dir>\n" );
  printf( "ls [<dir>]......display the contents of the directory<dir>\n" );
  printf( "dir [<dir>].....display the contents of the directory <dir>\n" );
  printf( "type <file>.....display the file <file>\n" );
  printf( "get <file>......get file <file>\n" );
  printf( "put <file>......send the file <file>\n" );
  printf( "del <file>......delete the file <file>\n" );
  printf( "syst............discover the system of the FTP server\n" );
  printf( "\n" );
}





