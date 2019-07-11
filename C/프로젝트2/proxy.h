#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>

#define buffer_size 64 * 1024

void error(char*);
void proxy_address(struct sockaddr_in*, in_port_t);
void end_address(struct sockaddr_in*, struct hostent*);
void parse_request(char**, char**, char*);
char* get_time(char*);
void write_log(char*);

void error(char *msg){
	perror(msg);
  exit(1);
}

void proxy_address(struct sockaddr_in* prox_address, in_port_t prox_port){
  bzero((char *) prox_address, sizeof(sockaddr_in));
	prox_address->sin_family = AF_INET;
	prox_address->sin_addr.s_addr = htonl(INADDR_ANY);
	prox_address->sin_port = htons(server_port);
}

void end_address(struct sockaddr_in* server_address, struct hostent* server){
  memset(server_address, 0, sizeof(server_address));
  server_address->sin_family = AF_INET;
  memcpy((char*)&(server_address->sin_addr.s_addr), (char*)server->h_addr, server->h_length);
  server_address->sin_port = htons(80);
}

void parse_request(char** server_url, char** server_host_name, char* buffer){
  int length;
  *server_url = NULL;
  *server_host_name = NULL;
  char* temp = strtok(buffer, "\r\n");
  while(temp != NULL){
    if(strstr(temp, "GET ") == temp && *server_url == NULL){
      length = strstr(temp+4, " ") - temp - 3;
      *server_url = (char*) malloc(sizeof(char) * length);
      bzero(*server_url, length);
      strncpy(*server_url, temp+4, length-1);
    }
    else if(strstr(temp, "Host: ") == temp && *server_host_name == NULL){
      length = strlen(temp+5);
      *server_host_name = (char*) malloc(sizeof(char) * length);
      bzero(*server_host_name, length);
      strncpy(*server_host_name, temp+6, length-1);
    }
    if(*server_url != NULL && *server_host_name != NULL){
      break;
    }
    temp = strtok(NULL, "\r\n");
  }
}

char* get_time(char* time_buffer){
  time_t nowtime;
  time(&nowtime);
  strftime(time_buffer, 64, "%a %d %m %Y %X %Z", localtime(&nowtime));
  return time_buffer;
}

void write_log(char* log_buffer){
  int logfd;
  if((logfd == open("proxy.log", O_CREAT | O_RDWR | O_APPEND, 0644)) < 0){
    error("Opening Log File: ");
  }
  if(write(logfd, log_buffer, strlen(log_buffer)) < 0){
    error("Writing Log File: ");
  }
}
