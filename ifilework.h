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
#ifndef _IFILEWORKS_H 
#define _IFILEWORKS_H 


#define ENV_CONFIG_FILE "/etc/ifileworks.conf"

#define IFW_VERSION             "0.0.1" 
#define IFW_DEBUG(fmt, ...)	ifw_debug(__FILE__, __LINE__, fmt, __VA_ARGS__) 
#define IFW_ARR_LEN(arr)	(sizeof(arr)/sizeof(arr[0])) 
#define IFW_DEF_SRV_PORT	21 
#define IFW_LISTEN_QU_LEN	8 
#define IFW_LINE_END	"\r\n" 
 
#define IFW_OK 	0 
#define IFW_ERR	(-1) 
 
#define IFW_CHECK_LOGIN()\
		do {	\
		if (!ifw_cur_user) {\
		ifw_send_resp(ctrlfd, 530, "first please");\
		return IFW_ERR;\
                 }	\
		} while(0) 
 


#define MAX_LINE  4096
# define CONF_LINE_SEP ":"

typedef struct _user_info {
    char *login;
    char *pwd;
    uid_t uid;
    gid_t gid;
    char *home;
    char *fconf;
} u_info;


struct ifw_cmd_struct { 
        char *cmd_name; 
        int (*cmd_handler)(int ctrlfd, char *cmd_line); 
}; 
 

 
char * ifw_crypt(const char *pwd);
 void strip_lf(char *str);
 int parse_config_line(char *line, u_info *user_info);
int fetch_conf_account(char*file, u_info *user_info, char* line, size_t sizeof_line, char *login);

#endif


