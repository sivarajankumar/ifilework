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
    #include <unistd.h>   
    #include <stdio.h>   
    #include <stdlib.h>   
    #include <errno.h>   
    #include <string.h>   
    #include <sys/stat.h>   
    #include <time.h>   
    #include <sys/socket.h>   
    #include <netinet/in.h>   
    #include <arpa/inet.h>   
    #include <netdb.h>   
    #include <stdarg.h>   
    #include <signal.h>   
    #include <sys/wait.h>   
    #include <fcntl.h>   
    #include <pwd.h>   
    #include <grp.h>   
    #include <dirent.h>   
       
    #include "ifilework.h"  

     
    /* local functions declarations */   
    static int ifw_do_user(int,char*);   
    static int ifw_do_pass(int,char*);   
    static int ifw_do_pwd(int, char*);   
    static int ifw_do_cwd(int, char*);   
    static int ifw_do_syst(int,char*);   
    static int ifw_do_list(int,char*);   
    static int ifw_do_size(int,char*);   
    static int ifw_do_dele(int,char*);   
    static int ifw_do_type(int,char*);   
    static int ifw_do_port(int,char*);   
    static int ifw_do_pasv(int,char*);   
    static int ifw_do_retr(int,char*);   
    static int ifw_do_stor(int,char*);   
    static int ifw_do_quit(int,char*);   
       
    /* global variables */   
    int ifw_debug_on;   
    int ifw_srv_port = IFW_DEF_SRV_PORT;   
    int ifw_cur_child_num;   
    int ifw_quit_flag;   
    struct ifw_user_struct *ifw_cur_user;   
    int ifw_pasv_fd = -1;   
    int ifw_pasv_connfd = -1;   
    int ifw_port_connfd = -1;   
    char ifw_home_dir[PATH_MAX];   
       
    /*  
    * Currently supported ftp commands.  
    */   
    struct ifw_cmd_struct ifw_cmds[] = {   
            {"USER", ifw_do_user},   
            {"PASS", ifw_do_pass},   
            {"PWD",  ifw_do_pwd},   
            {"XPWD", ifw_do_pwd},   
            {"CWD",  ifw_do_cwd},   
            {"LIST", ifw_do_list},   
            {"NLST", ifw_do_list},   
            {"SYST", ifw_do_syst},   
            {"TYPE", ifw_do_type},   
            {"SIZE", ifw_do_size},   
            {"DELE", ifw_do_dele},   
            {"RMD",  ifw_do_dele},   
            {"PORT", ifw_do_port},   
            {"PASV", ifw_do_pasv},   
            {"RETR", ifw_do_retr},   
            {"STOR", ifw_do_stor},   
            {"QUIT", ifw_do_quit},   
            { NULL,         NULL}   
    };   
       
    /*  
    * User informations  
    */   
    u_info user_info;

   
       
    /*  
    * Ftp server's responses, and are not complete.  
    */   
    char ifw_srv_resps[][256] = {   
            "150 Begin transfer"                                                        IFW_LINE_END,   
            "200 OK"                                                                                IFW_LINE_END,   
            "213 %d"                                                                                IFW_LINE_END,   
            "215 UNIX Type: L8"                                                                IFW_LINE_END,   
            "220 ifilework " IFW_VERSION " Server"                                IFW_LINE_END,   
            "221 Goodbye"                                                                        IFW_LINE_END,   
            "226 Transfer complete"                                                        IFW_LINE_END,   
            "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)"        IFW_LINE_END,   
            "230 User %s logged in"                                                        IFW_LINE_END,   
            "250 CWD command successful"                                        IFW_LINE_END,   
            "257 \"%s\" is current directory"                                IFW_LINE_END,   
            "331 Password required for %s"                                        IFW_LINE_END,   
            "500 Unsupport command %s"                                                IFW_LINE_END,   
            "530 Login %s"                                                                        IFW_LINE_END,   
            "550 Error"                                                                                IFW_LINE_END   
    };   
       
       
       
    static void ifw_debug(const char *file, int line, const char *fmt, ...)   
    {   

          
            va_list ap;   
       
            if (!ifw_debug_on)   
                    return;   
       
            fprintf(stderr, "(%s:%d:%d)", file, line, getpid());   
            va_start(ap, fmt);   
            vfprintf(stderr, fmt, ap);   
            va_end(ap);   
    }   
       
    static void ifw_usage(void)   
    {   
            puts("____________________________________\n" );
            printf("Ifilework Server -- version : "IFW_VERSION"\n" );
            puts("____________________________________\n" );   
            printf("Usage :\n\n ifileworks -p <port> -d\n\n");   

            exit(0);
    }   
       
    static void ifw_parse_args(int argc, char *argv[])   
    {   
            int opt;   
            int err = 0;   
       
            while ((opt = getopt(argc, argv, "p:dh")) != -1) {   
                    switch (opt) {   
                            case 'p':   
                                ifw_srv_port = atoi(optarg);   
                                    err = (ifw_srv_port < 0 || ifw_srv_port > 65535);              
                                break;   
                            case 'd':   
                                ifw_debug_on = 1;   
                                break;   
                            default:   
                                err = 1;   
                                break;   
                    }   
                       
                    if (err) {   
                            ifw_usage();   
                            exit(-1);   
                    }   
            }   
       
            IFW_DEBUG("srv port:%d\n", ifw_srv_port);   
    }   
       
    static void ifw_sigchild(int signo)   
    {   
            int status;   
       
            if (signo != SIGCHLD)   
                    return;   
       
            while (waitpid(-1, &status, WNOHANG) > 0)    
                    ifw_cur_child_num--;   
       
            IFW_DEBUG("caught a SIGCHLD, ifw_cur_child_num=%d\n", ifw_cur_child_num);   
    }   
       
    static int ifw_init(void)   
    {   
            /* add all init statements here */   
       
            signal(SIGPIPE, SIG_IGN);   
            signal(SIGCHLD, ifw_sigchild);   
           
            return IFW_OK;   
    }   
       
    /*  
    * Create ftp server's listening socket.  
    */   
    static int ifw_create_srv(void)   
    {   
            int fd;   
            int on = 1;   
            int err;   
            struct sockaddr_in srvaddr;   
       
            if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {   
                    IFW_DEBUG("socket() failed: %s\n", strerror(errno));   
                    return fd;   
            }   
       
            err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));    
            if (err < 0) {   
                    IFW_DEBUG("setsockopt() failed: %s\n", strerror(errno));   
                    close(fd);   
                    return IFW_ERR;   
            }   
               
            memset(&srvaddr, 0, sizeof(srvaddr));   
            srvaddr.sin_family = AF_INET;   
            srvaddr.sin_port = htons(ifw_srv_port);   
            srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);   
            err = bind(fd, (struct sockaddr*)&srvaddr, sizeof(srvaddr));   
            if (err < 0) {   
                    IFW_DEBUG("bind() failed: %s\n", strerror(errno));   
                    close(fd);   
                    return IFW_ERR;   
            }   
       
            err = listen(fd, IFW_LISTEN_QU_LEN);   
            if (err < 0) {   
                    IFW_DEBUG("listen() failed: %s\n", strerror(errno));   
                    close(fd);   
                    return IFW_ERR;   
            }   
       
            if (ifw_debug_on) {   
                    int len = sizeof(srvaddr);   
                    getsockname(fd, (struct sockaddr*)&srvaddr, &len);   
                    IFW_DEBUG("create srv listen socket OK: %s:%d\n",   
                               inet_ntoa(srvaddr.sin_addr), ntohs(srvaddr.sin_port));   
            }   
       
            return fd;           
       
    }   
       
    /*  
    * Map server response's number to the msg pointer.  
    */   
    static char * ifw_srv_resp_num2msg(int num)   
    {   
            int i;   
            char buf[8];   
       
            snprintf(buf, sizeof(buf), "%d", num);   
            if (strlen(buf) != 3)   
                    return NULL;   
       
            for (i = 0; i < IFW_ARR_LEN(ifw_srv_resps); i++)   
                    if (strncmp(buf, ifw_srv_resps[i], strlen(buf)) == 0)   
                            return ifw_srv_resps[i];   
       
            return NULL;   
    }   
       
    static int ifw_send_msg(int fd, char *msg, int len)   
    {   
            int n, off = 0, left = len;   
       
            while (1) {   
                    n = write(fd, msg + off, left);   
                    if (n < 0) {   
                            if (errno == EINTR)   
                                    continue;   
                            return n;   
                    }   
                    if (n < left) {   
                            left -= n;   
                            off += n;   
                            continue;   
                    }   
                    return len;   
            }   
    }   
       
    static int ifw_recv_msg(int fd, char buf[], int len)   
    {   
            int n;   
       
            while (1) {   
                    n = read(fd, buf, len);   
                    if (n < 0) {   
                            if (errno == EINTR)   
                                    continue;   
                            return n;   
                    }   
                    return n;   
            }   
    }   
       
    static int ifw_send_resp(int fd, int num, ...)   
    {   
            char *cp = ifw_srv_resp_num2msg(num);   
            va_list ap;   
            char buf[BUFSIZ];   
       
            if (!cp) {   
                    IFW_DEBUG("ifw_srv_resp_num2msg(%d) failed\n", num);   
                    return IFW_ERR;   
            }   
       
            va_start(ap, num);   
            vsnprintf(buf, sizeof(buf), cp, ap);   
            va_end(ap);   
       
            IFW_DEBUG("send resp:%s\n", buf);   
            if (ifw_send_msg(fd, buf, strlen(buf)) != strlen(buf)) {   
                    IFW_DEBUG("ifw_send_msg() failed: %s\n", strerror(errno));   
                    return IFW_ERR;   
            }   
       
            return IFW_OK;   
    }   
       
    static void ifw_trim_lineend(char *cp)   
    {   
            if (cp && strlen(cp) > 0) {   
                    char *cp2 = &cp[strlen(cp) - 1];   
                       
                    while (*cp2 == '\r' || *cp2 == '\n')   
                            if (--cp2 < cp)   
                                    break;   
                    cp2[1] = '\0';   
            }   
    }   
       
    static int ifw_get_connfd(void)   
    {   
            int fd;   
       
            if (ifw_pasv_fd >= 0) {   
                    fd = accept(ifw_pasv_fd, NULL, NULL);   
                    if (fd >= 0) {   
                            close(ifw_pasv_fd);   
                            ifw_pasv_fd = -1;   
                            ifw_pasv_connfd = fd;   
                            return fd;   
                    }   
                    else   
                            IFW_DEBUG("accept() failed:%s\n", strerror(errno));   
            }   
            else if (ifw_port_connfd >= 0)   
                    return ifw_port_connfd;   
                       
            return (-1);   
    }   
       
    static int ifw_close_all_fd(void)   
    {   
            if (ifw_pasv_fd >= 0) {   
                    close(ifw_pasv_fd);   
                    ifw_pasv_fd = -1;   
            }   
            if (ifw_pasv_connfd >= 0) {   
                    close(ifw_pasv_connfd);   
                    ifw_pasv_connfd = -1;   
            }   
            if (ifw_port_connfd >= 0) {   
                    close(ifw_port_connfd);   
                    ifw_port_connfd = -1;   
            }   
            return IFW_OK;   
    }   
       

static int ifw_do_user(int ctrlfd, char *cmdline)   
    {   
           
            char line[MAX_LINE ];
            char *cp = strchr(cmdline, ' '); 
               user_info.fconf = ENV_CONFIG_FILE ; 
       
            if (cp) {   
                     user_info.login = cp + 1 ;
                      

                    if(do_check_user(&user_info)) {
                        IFW_DEBUG("user(%s) is found\n", user_info.login);
                        ifw_cur_user = user_info.login;

                  if (fetch_conf_account(user_info.fconf,&user_info, line, sizeof line,user_info.login) != 0) {
                      IFW_DEBUG("Unable to fetch info about user [%s] in file [%s]\n",
                      user_info.login, user_info.fconf);

                     }  
                         
                         strcpy(ifw_home_dir,user_info.home);
                         chdir(ifw_home_dir);
                    /*  
                     * If user name is bad, we still don't close the connection   
                     * and send back the 331 response to ask for password.  
                     */   
                    return ifw_send_resp(ctrlfd, 331, cp + 1);   
            }   
               
            return ifw_send_resp(ctrlfd, 550);   
    }  
     
}  
  
    static int ifw_do_pass(int ctrlfd, char *cmdline)   
    {   
            char *pass = strchr(cmdline, ' ');   
            char *res = crypt(pass+1, user_info.pwd); 
            if (user_info.login && pass) {   
                    if (strlen(user_info.pwd) == 0 ||   
                            strcmp(res, user_info.pwd) == 0) {   
                            IFW_DEBUG("password for %s OK\n", user_info.login);   
                            return ifw_send_resp(ctrlfd, 230, user_info.login);   
                    }   
                    IFW_DEBUG("password for %s ERR\n", user_info.login);   
            }   
                       
             
            return ifw_send_resp(ctrlfd, 530, "incorrect");   
    }   
       
    static int ifw_do_pwd(int ctrlfd, char *cmdline)   
    {   
            char curdir[PATH_MAX];   
            char *cp;   
               
            IFW_CHECK_LOGIN();   
       
            getcwd(curdir, sizeof(curdir));   
            cp = &curdir[strlen(ifw_home_dir)];   
            return ifw_send_resp(ctrlfd, 257, (*cp == '\0') ? "/" : cp);   
    }   
       
    static int ifw_do_cwd(int ctrlfd, char *cmdline)   
    {   
            char *space = strchr(cmdline, ' ');   
            char curdir[PATH_MAX];   
       
            IFW_CHECK_LOGIN();   
       
            if (!space)   
                    return ifw_send_resp(ctrlfd, 550);   
       
            getcwd(curdir, sizeof(curdir));   
            if (strcmp(curdir, ifw_home_dir) == 0 &&   
                    space[1] == '.' &&   
                    space[2] == '.')   
                    return ifw_send_resp(ctrlfd, 550);   
       
            /* Absolute path */           
            if (space[1] == '/') {   
                    if (chdir(ifw_home_dir) == 0) {   
                            if (space[2] == '\0' || chdir(space+2) == 0)   
                                    return ifw_send_resp(ctrlfd, 250);           
                    }   
                    chdir(curdir);   
                    return ifw_send_resp(ctrlfd, 550);   
            }   
       
            /* Relative path */   
            if (chdir(space+1) == 0)   
                    return ifw_send_resp(ctrlfd, 250);   
       
            chdir(curdir);   
            return ifw_send_resp(ctrlfd, 550);   
    }   
       
    /*  
    * This function acts as a implementation like 'ls -l' shell command.  
    */   
    static int ifw_get_list(char buf[], int len)   
    {   
            DIR *dir;   
            struct dirent *ent;   
            int off = 0;   
       
            if ((dir = opendir(".")) < 0) {   
                    IFW_DEBUG("opendir() failed:%s\n", strerror(errno));   
                    return IFW_ERR;   
            }   
       
            buf[0] = '\0';   
       
            while ((ent = readdir(dir)) != NULL) {   
                    char *filename = ent->d_name;   
                    struct stat st;   
                    char mode[] = "----------";   
                    struct passwd *pwd;   
                    struct group *grp;   
                    struct tm *ptm;   
                    char timebuf[BUFSIZ];    
                    int timelen;   
       
                    if (strcmp(filename, ".") == 0 ||   
                            strcmp(filename, "..") == 0)   
                            continue;   
       
                    if (stat(filename, &st) < 0) {   
                            closedir(dir);   
                            IFW_DEBUG("stat() failed:%s\n", strerror(errno));   
                            return IFW_ERR;   
                    }   
       
                    if (S_ISDIR(st.st_mode))   
                            mode[0] = 'd';   
                    if (st.st_mode & S_IRUSR)   
                            mode[1] = 'r';   
                    if (st.st_mode & S_IWUSR)   
                            mode[2] = 'w';   
                    if (st.st_mode & S_IXUSR)   
                            mode[3] = 'x';   
                    if (st.st_mode & S_IRGRP)   
                            mode[4] = 'r';   
                    if (st.st_mode & S_IWGRP)   
                            mode[5] = 'w';   
                    if (st.st_mode & S_IXGRP)   
                            mode[6] = 'x';   
                    if (st.st_mode & S_IROTH)   
                            mode[7] = 'r';   
                    if (st.st_mode & S_IWOTH)   
                            mode[8] = 'w';   
                    if (st.st_mode & S_IXOTH)   
                            mode[9] = 'x';   
                    mode[10] = '\0';   
                    off += snprintf(buf + off, len - off, "%s ", mode);   
       
                    /* hard link number, this field is nonsense for ftp */   
                    off += snprintf(buf + off, len - off, "%d ", 1);   
       
                    /* user */   
                    if ((pwd = getpwuid(st.st_uid)) == NULL) {   
                            closedir(dir);   
                            return IFW_ERR;   
                    }   
                    off += snprintf(buf + off, len - off, "%s ", pwd->pw_name);   
       
                    /* group */   
                    if ((grp = getgrgid(st.st_gid)) == NULL) {   
                            closedir(dir);   
                            return IFW_ERR;   
                    }   
                    off += snprintf(buf + off, len - off, "%s ", grp->gr_name);   
       
                    /* size */   
                    off += snprintf(buf + off, len - off, "%*d ", 10, st.st_size);   
       
                    /* mtime */   
                    ptm = localtime(&st.st_mtime);   
                    if (ptm && (timelen = strftime(timebuf, sizeof(timebuf), "%b %d %H:%S", ptm)) > 0) {   
                            timebuf[timelen] = '\0';   
                            off += snprintf(buf + off, len - off, "%s ", timebuf);   
                    }   
                    else {   
                            closedir(dir);   
                            return IFW_ERR;   
                    }   
                       
                    off += snprintf(buf + off, len - off, "%s\r\n", filename);   
                       
            }   
       
            return off;   
    }   
       
    static int ifw_do_list(int ctrlfd, char *cmdline)   
    {   
            char buf[BUFSIZ];   
            int n;   
            int fd;   
       
            IFW_CHECK_LOGIN();   
       
            if ((fd = ifw_get_connfd()) < 0) {   
                    IFW_DEBUG("LIST cmd:no available fd%s", "\n");   
                    goto err_label;   
            }   
       
            ifw_send_resp(ctrlfd, 150);   
       
            /*   
             * Get the 'ls -l'-like result and send it to client.  
             */   
            n = ifw_get_list(buf, sizeof(buf));   
            if (n >= 0) {   
                    if (ifw_send_msg(fd, buf, n) != n) {   
                            IFW_DEBUG("ifw_send_msg() failed: %s\n", strerror(errno));   
                            goto err_label;   
                    }   
            }   
            else {   
                    IFW_DEBUG("ifw_get_list() failed %s", "\n");   
                    goto err_label;   
            }   
               
            ifw_close_all_fd();   
            return ifw_send_resp(ctrlfd, 226);   
       
    err_label:   
            ifw_close_all_fd();   
            return ifw_send_resp(ctrlfd, 550);   
    }   
       
    static int ifw_do_syst(int ctrlfd, char *cmdline)   
    {   
            IFW_CHECK_LOGIN();   
            return ifw_send_resp(ctrlfd, 215);   
    }   
       
    static int ifw_do_size(int ctrlfd, char *cmdline)   
    {   
            char *space = strchr(cmdline, ' ');   
            struct stat st;   
       
            IFW_CHECK_LOGIN();   
       
            if (!space || lstat(space + 1, &st) < 0) {   
                    IFW_DEBUG("SIZE cmd err: %s\n", cmdline);   
                    return ifw_send_resp(ctrlfd, 550);   
            }   
       
            return ifw_send_resp(ctrlfd, 213, st.st_size);   
    }   
       
    static int ifw_do_dele(int ctrlfd, char *cmdline)   
    {   
            char *space = strchr(cmdline, ' ');   
            struct stat st;   
       
            IFW_CHECK_LOGIN();   
       
            if (!space || lstat(space+1, &st) < 0 ||   
                    remove(space+1) < 0) {   
                    IFW_DEBUG("DELE cmd err: %s\n", cmdline);   
                    return ifw_send_resp(ctrlfd, 550);   
            }   
       
            return ifw_send_resp(ctrlfd, 200);   
    }   
       
    static int ifw_do_type(int ctrlfd, char *cmdline)   
    {   
            IFW_CHECK_LOGIN();   
       
            /*  
             * Just send back 200 response and do nothing  
             */   
            return ifw_send_resp(ctrlfd, 200);   
    }   
       
    /*  
    * Parse PORT cmd and fetch the ip and port,   
    * and both in network byte order.  
    */   
    static int ifw_get_port_mode_ipport(char *cmdline, unsigned int *ip, unsigned short *port)   
    {   
            char *cp = strchr(cmdline, ' ');   
            int i;   
            unsigned char buf[6];   
       
            if (!cp)   
                    return IFW_ERR;   
       
            for (cp++, i = 0; i < IFW_ARR_LEN(buf); i++) {   
                    buf[i] = atoi(cp);   
                    cp = strchr(cp, ',');   
                    if (!cp && i < IFW_ARR_LEN(buf) - 1)   
                            return IFW_ERR;   
                    cp++;   
            }   
       
            if (ip)    
                    *ip = *(unsigned int*)&buf[0];   
       
            if (port)    
                    *port = *(unsigned short*)&buf[4];   
       
            return IFW_OK;   
    }   
       
     
    static int ifw_do_port(int ctrlfd, char *cmdline)   
    {   
            unsigned int ip;   
            unsigned short port;   
            struct sockaddr_in sin;   
       
            IFW_CHECK_LOGIN();   
       
            if (ifw_get_port_mode_ipport(cmdline, &ip, &port) != IFW_OK) {   
                    IFW_DEBUG("ifw_get_port_mode_ipport() failed%s", "\n");   
                    goto err_label;   
            }   
       
            memset(&sin, 0, sizeof(sin));   
            sin.sin_family = AF_INET;   
            sin.sin_addr.s_addr = ip;   
            sin.sin_port = port;   
               
            IFW_DEBUG("PORT cmd:%s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));   
       
            if (ifw_port_connfd >= 0) {   
                    close(ifw_port_connfd);   
                    ifw_port_connfd = -1;   
            }   
       
            ifw_port_connfd = socket(AF_INET, SOCK_STREAM, 0);   
            if (ifw_port_connfd < 0) {   
                    IFW_DEBUG("socket() failed:%s\n", strerror(errno));   
                    goto err_label;   
            }   
       
            if (connect(ifw_port_connfd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {   
                    IFW_DEBUG("bind() failed:%s\n", strerror(errno));   
                    goto err_label;   
            }   
       
            IFW_DEBUG("PORT mode connect OK%s", "\n");   
            return ifw_send_resp(ctrlfd, 200);   
       
    err_label:   
            if (ifw_port_connfd >= 0) {   
                    close(ifw_port_connfd);   
                    ifw_port_connfd = -1;   
            }   
            return ifw_send_resp(ctrlfd, 550);   
       
    }   
       
    static int ifw_do_pasv(int ctrlfd, char *cmdline)   
    {   
            struct sockaddr_in pasvaddr;   
            int len;   
            unsigned int ip;   
            unsigned short port;   
       
            IFW_CHECK_LOGIN();   
       
            if (ifw_pasv_fd >= 0) {   
                    close(ifw_pasv_fd);   
                    ifw_pasv_fd = -1;   
            }   
       
            ifw_pasv_fd = socket(AF_INET, SOCK_STREAM, 0);   
            if (ifw_pasv_fd < 0) {   
                    IFW_DEBUG("socket() failed: %s\n", strerror(errno));   
                    return ifw_send_resp(ctrlfd, 550);   
            }   
       
            /*   
             * must bind to the same interface as ctrl connectin came from.  
             */   
            len = sizeof(pasvaddr);   
            getsockname(ctrlfd, (struct sockaddr*)&pasvaddr, &len);   
            pasvaddr.sin_port = 0;   
       
            if (bind(ifw_pasv_fd, (struct sockaddr*)&pasvaddr, sizeof(pasvaddr)) < 0) {   
                    IFW_DEBUG("bind() failed: %s\n", strerror(errno));   
                    close(ifw_pasv_fd);   
                    ifw_pasv_fd = -1;   
                    return ifw_send_resp(ctrlfd, 550);   
            }   
       
            if (listen(ifw_pasv_fd, IFW_LISTEN_QU_LEN) < 0) {   
                    IFW_DEBUG("listen() failed: %s\n", strerror(errno));   
                    close(ifw_pasv_fd);   
                    ifw_pasv_fd = -1;   
                    return ifw_send_resp(ctrlfd, 550);   
            }   
                       
            len = sizeof(pasvaddr);   
            getsockname(ifw_pasv_fd, (struct sockaddr*)&pasvaddr, &len);   
            ip = ntohl(pasvaddr.sin_addr.s_addr);   
            port = ntohs(pasvaddr.sin_port);   
            IFW_DEBUG("local bind: %s:%d\n", inet_ntoa(pasvaddr.sin_addr), port);   
       
            /*   
             * write local ip/port into response msg  
             * and send to client.  
             */   
            return ifw_send_resp(ctrlfd, 227, (ip>>24)&0xff, (ip>>16)&0xff,   
                            (ip>>8)&0xff, ip&0xff, (port>>8)&0xff, port&0xff);   
       
    }   
       
    static int ifw_do_retr(int ctrlfd, char *cmdline)   
    {   
            char buf[BUFSIZ];   
            char *space = strchr(cmdline, ' ');   
            struct stat st;   
            int fd = -1, n;   
            int connfd;   
       
            IFW_CHECK_LOGIN();   
       
            if (!space || lstat(space + 1, &st) < 0) {   
                    IFW_DEBUG("RETR cmd err: %s\n", cmdline);   
                    goto err_label;   
            }   
       
            if ((connfd = ifw_get_connfd()) < 0) {   
                    IFW_DEBUG("ifw_get_connfd() failed%s", "\n");   
                    goto err_label;   
            }   
       
            ifw_send_resp(ctrlfd, 150);   
       
            /* begin to read file and write it to conn socket */   
            if ((fd = open(space + 1, O_RDONLY)) < 0) {   
                    IFW_DEBUG("open() failed: %s\n", strerror(errno));   
                    goto err_label;   
            }   
       
            while (1) {   
                    if ((n = read(fd, buf, sizeof(buf))) < 0) {   
                            if (errno == EINTR)   
                                    continue;   
                            IFW_DEBUG("read() failed: %s\n", strerror(errno));   
                            goto err_label;   
                    }   
       
                    if (n == 0)   
                            break;   
                               
                    if (ifw_send_msg(connfd, buf, n) != n) {   
                            IFW_DEBUG("ifw_send_msg() failed: %s\n", strerror(errno));   
                            goto err_label;   
                    }   
            }   
       
            IFW_DEBUG("RETR(%s) OK\n", space + 1);   
            if (fd >= 0)   
                    close(fd);   
            ifw_close_all_fd();   
            return ifw_send_resp(ctrlfd, 226);   
       
    err_label:   
            if (fd >= 0)   
                    close(fd);   
            ifw_close_all_fd();   
            return ifw_send_resp(ctrlfd, 550);   
    }   
       
    static int ifw_do_stor(int ctrlfd, char *cmdline)   
    {   
            char buf[BUFSIZ];   
            char *space = strchr(cmdline, ' ');   
            struct stat st;   
            int fd = -1, n;   
            int left, off;   
            int connfd;   
       
            IFW_CHECK_LOGIN();   
       
            /*  
             * Should add some permission control mechanism here.  
             */   
            if (!space || lstat(space + 1, &st) == 0) {   
                    IFW_DEBUG("STOR cmd err: %s\n", cmdline);   
                    goto err_label;   
            }   
               
            if ((connfd = ifw_get_connfd()) < 0) {   
                    IFW_DEBUG("ifw_get_connfd() failed%s", "\n");   
                    goto err_label;   
            }   
       
            ifw_send_resp(ctrlfd, 150);   
                       
            if ((fd = open(space + 1, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0) {   
                    IFW_DEBUG("open() failed: %s\n", strerror(errno));   
                    goto err_label;   
            }   
       
            /* begin to read data from socket and wirte to disk file */   
            while (1) {   
                    if ((n = ifw_recv_msg(connfd, buf, sizeof(buf))) < 0) {   
                            IFW_DEBUG("ifw_recv_msg() failed: %s\n", strerror(errno));   
                            goto err_label;   
                    }           
       
                    if (n == 0)   
                            break;   
       
                    left = n;   
                    off = 0;   
                    while (left > 0) {   
                            int nwrite;   
       
                            if ((nwrite = write(fd, buf + off, left)) < 0) {   
                                    if (errno == EINTR)   
                                            continue;   
                                    IFW_DEBUG("write() failed:%s\n", strerror(errno));   
                                    goto err_label;   
                            }   
                            off += nwrite;   
                            left -= nwrite;   
                    }   
            }   
       
            IFW_DEBUG("STOR(%s) OK\n", space+1);   
            if (fd >= 0)   
                    close(fd);   
            ifw_close_all_fd();   
            sync();   
            return ifw_send_resp(ctrlfd, 226);   
       
    err_label:   
            if (fd >= 0) {   
                    close(fd);   
                    unlink(space+1);   
            }   
            ifw_close_all_fd();   
            return ifw_send_resp(ctrlfd, 550);   
    }   
       
    static int ifw_do_quit(int ctrlfd, char *cmdline)   
    {   
            ifw_send_resp(ctrlfd, 221);   
            ifw_quit_flag = 1;   
            return IFW_OK;   
    }   
       
    static int ifw_do_request(int ctrlfd, char buf[])   
    {   
            char *end = &buf[strlen(buf) - 1];   
            char *space = strchr(buf, ' ');   
            int i;   
            char save;   
            int err;   
       
            if (*end == '\n' || *end == '\r') {   
                    /*   
                     * this is a valid ftp request.  
                     */   
                    ifw_trim_lineend(buf);   
       
                    if (!space)    
                            space = &buf[strlen(buf)];   
       
                    save = *space;   
                    *space = '\0';   
                    for (i = 0; ifw_cmds[i].cmd_name; i++) {   
                            if (strcmp(buf, ifw_cmds[i].cmd_name) == 0) {   
                                    *space = save;   
                                    IFW_DEBUG("recved a valid ftp cmd:%s\n", buf);   
                                    return ifw_cmds[i].cmd_handler(ctrlfd, buf);   
                            }   
                    }   
       
                    /*  
                     * unrecognized cmd  
                     */   
                    *space = save;   
                    IFW_DEBUG("recved a unsupported ftp cmd:%s\n", buf);   
               
                    *space = '\0';   
                    err = ifw_send_resp(ctrlfd, 500, buf);   
                    *space = save;   
       
                    return err;           
            }   
       
            IFW_DEBUG("recved a invalid ftp cmd:%s\n", buf);   
       
            /*   
             * Even if it's a invalid cmd, we should also send    
             * back a response to prevent client from blocking.  
             */   
            return ifw_send_resp(ctrlfd, 550);   
    }   
       
    static int ifw_ctrl_conn_handler(int connfd)   
    {   
            char buf[BUFSIZ];   
            int buflen;   
            int err = IFW_OK;   
       
            /*   
             * Control connection has set up,  
             * we can send out the first ftp msg.  
             */   
            if (ifw_send_resp(connfd, 220) != IFW_OK) {   
                    close(connfd);   
                    IFW_DEBUG("Close the ctrl connection OK%s", "\n");   
                    return IFW_ERR;   
            }   
       
            /*  
             * Begin to interact with client and one should implement  
             * a state machine here for full compatibility. But we only  
             * show a demonstration ftp server and i do my best to   
             * simplify it. Base on this skeleton, you can write a  
             * full-funtional ftp server if you like. ;-)  
             */   
            while (1) {   
                    buflen = ifw_recv_msg(connfd, buf, sizeof(buf));   
                    if (buflen < 0) {   
                            IFW_DEBUG("ifw_recv_msg() failed: %s\n", strerror(errno));   
                            err = IFW_ERR;   
                            break;   
                    }   
               
                    if (buflen == 0)    
                            /* this means client launch a active close */   
                            break;   
       
                    buf[buflen] = '\0';   
                    ifw_do_request(connfd, buf);   
                       
                    /*   
                     * The negative return value from ifw_do_request   
                     * should not cause the breaking of ctrl connection.  
                     * Only when the client send QUIT cmd, we exit and  
                     * launch a active close.  
                     */   
                       
                    if (ifw_quit_flag)   
                            break;   
            }   
       
            close(connfd);   
            IFW_DEBUG("Close the ctrl connection OK%s", "\n");   
            return err;   
    }   
       
    static int ifw_do_loop(int listenfd)   
    {   
            int connfd;   
            int pid;   
       
            while (1) {   
                    IFW_DEBUG("Server ready, wait client's connection...%s", "\n");   
       
                    connfd = accept(listenfd, NULL, NULL);   
                    if (connfd < 0) {   
                            IFW_DEBUG("accept() failed: %s\n", strerror(errno));   
                            continue;   
                    }   
                       
                    if (ifw_debug_on) {   
                            struct sockaddr_in cliaddr;   
                            int len = sizeof(cliaddr);   
                            getpeername(connfd, (struct sockaddr*)&cliaddr, &len);   
                            IFW_DEBUG("accept a conn from %s:%d\n",   
                                            inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));   
                    }   
       
                    if ((pid = fork()) < 0) {   
                            IFW_DEBUG("fork() failed: %s\n", strerror(errno));   
                            close(connfd);   
                            continue;   
                    }   
                    if (pid > 0) {   
                            /* parent */   
                            close(connfd);   
                            ifw_cur_child_num++;   
                            continue;   
                    }   
                       
                    /* child */   
                    close(listenfd);   
                    signal(SIGCHLD, SIG_IGN);   
                    if (ifw_ctrl_conn_handler(connfd) != IFW_OK)   
                            exit(-1);   
                    exit(0);   
            }   
       
            return IFW_OK;   
    }   
       
    int main(int argc, char *argv[])   
    {   
        if(argc < 2)
          ifw_usage();

        puts("____________________________________\n" );
        printf("Ifilework Server -- version : "IFW_VERSION" \n");
        puts("____________________________________" );

            int listenfd;   
       
            ifw_parse_args(argc, argv);   
            ifw_init();   
               
            listenfd = ifw_create_srv();   
            if (listenfd >= 0)   
                    ifw_do_loop(listenfd);   
       
            exit(0);   
    }  
