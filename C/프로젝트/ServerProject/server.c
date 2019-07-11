#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>

void error(char *msg){
	perror(msg);
	exit(1);
}	//error 메시지

int main(int argc, char *argv[]){
	char checkbuffer[1024]; //client가 요청한 파일이 디렉토리에 있는지 확인하기 위한 buffer
	int sockfd, newsockfd; //descriptors return from socket and accept system calls
	int portno; //port number
	socklen_t clilen;
	struct tm *t;
	time_t timer;
	char request_buffer[256]; //Client의 Request Message 저장.
	int content_index;
	char file_buffer[20480];

	/*sockaddr_in: Structure Containing an Internet Address*/
	struct sockaddr_in serv_addr, cli_addr;

	int n;
	if(argc<2){
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	/*Create a new socket
	  AF_INET: Address Domain is Internet
	  SOCK_STREAM: Socket Type is STREAM Socket*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
		error("ERROR opening socket");

	bzero((char *)&serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd, 10); //Listen for socket connections, Backlog queue (connections to wait) is 10

	clilen = sizeof(cli_addr);
	/*accept function:
	  1) Block until a new connection is established
	  2) the new socket descripter will be used for subsequent communication with the newly connected client.
	*/
	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	if(newsockfd<0) error("ERROR on accept");

	bzero(request_buffer, 256);
	n = read(newsockfd, request_buffer, 255); //client의 request message를 읽어옴.
	if(n<0)	error("ERROR reading from socket");
	printf("Here is the message: \n%s\n\n", request_buffer); //읽어온 client의 request message 출력.

	//request message를 parsing하기 위해 client_request buffer에 저장.
	char client_request[512];
	int i=0,j=0;
	client_request[j++]='.';
	while(1){
			if(request_buffer[i++]==' '){
					while(request_buffer[i]!=' '){
							client_request[j++]=request_buffer[i++];
					}
					break;
			}
	}
	client_request[j]='\0';

	/*
		 client가 요청한 파일을 서버가 가지고 있으면 정상적 response message 출력.
	 	 response message를 만드는 과정
	*/
	char* response_header;
	char response_status[] = "HTTP/1.1 200 OK\r\n";
	char tempBuffer[26];
	timer = time(NULL);
	t = localtime(&timer);
	strftime(tempBuffer, 26, "%c", t);
	char date[40] = "Date: ";
	strcat(date, tempBuffer);
	bzero(tempBuffer, 26);
	i=1, j=0;
	while(1){
		if(client_request[i++] == '.'){
			while(client_request[i] != '\0'){
				tempBuffer[j++] = client_request[i++];
			}
			break;
		}
	}
	//client가 요청한 content type을 response message에 반영
	char C_type[50] = "\nContent-Type: ";
	for(content_index=0; content_index < 5; content_index++){
		if(strcmp(tempBuffer, "html") == 0){
			strcat(C_type, "text/html");
			break;
		}
		else if(strcmp(tempBuffer, "gif") == 0){
			strcat(C_type, "image/gif");
			break;
		}
		else if(strcmp(tempBuffer, "jpeg") == 0){
			strcat(C_type, "image/jpeg");
			break;
		}
		else if(strcmp(tempBuffer, "mp3") == 0){
			strcat(C_type, "audio/mp3");
			break;
		}
		else if(strcmp(tempBuffer, "pdf") == 0){
			strcat(C_type, "application/pdf");
			break;
		}
	}
	strcat(C_type, "\r\ncharset=UTF-8\r\n");
	//response_header의 크기 동적 할당을 위해 필요한 크기의 합 구하기
	int size = sizeof(response_status) + sizeof(date) + sizeof(C_type) + 1;
	//response_header 내용 완성
	response_header = (char*) malloc(sizeof(char) * size);
	strcat(response_header, response_status);
	strcat(response_header, date);
	strcat(response_header, C_type);
	strcat(response_header, "\r\n");

	/* client가 요청한 파일이 없을 경우 client에게 전송할 404 not found 메시지를 만드는 과정. */
	char* error_header;
	char error_status[] = "HTTP/1.1 404 Not Found\r\n";
	int err_size = sizeof(error_status) + sizeof(date) + 1;
	error_header = (char*) malloc(sizeof(char) * err_size);
	strcat(error_header, error_status);
	strcat(error_header, date);

	//client가 요청한 파일이 서버에 있는지 확인하기 위해 client가 요청한 파일명을 parsing.
	char existbuffer[50];
	i=1, j=0;
	while(1){
		if(client_request[i++] == '/'){
			while(client_request[i] != '\0'){
				existbuffer[j++] = client_request[i++];
			}
			break;
		}
	}

	//현재 작업중인 디렉토리 내에 client가 요청한 파일이 있는지 없는지 검사.
	getcwd(checkbuffer, 1024); //현재 작업중인 디렉토리 주소를 받아옴.
	strcat(checkbuffer, "/");
	strcat(checkbuffer, existbuffer); //현재 작업중인 디렉토리 주소 + client가 요청한 파일명.
	int exist = access(checkbuffer, 0); //요청한 파일이 존재하면 0, 없으면 -1을 리턴.

	//client가 요청한 파일이 서버에 있으면 정상적 파일 및 response message 출력 및 전송.
	if(exist == 0){
		n = send(newsockfd, response_header, size, 0); //Server의 Response Message 전송.
		printf("%s\n", response_header);
		//client가 요청한 파일 전송
		FILE *file = fopen(client_request, "rb");
		bzero(file_buffer, sizeof(file_buffer));
		while(!feof(file)){
			fread(file_buffer, 1, sizeof(file_buffer), file);
			write(newsockfd, file_buffer, sizeof(file_buffer));
			//send(newsockfd, file_buffer, sizeof(file_buffer), 0);
			printf("%s\n", file_buffer);
			bzero(file_buffer, sizeof(file_buffer));
		}
		close(file);
		free(response_header);
	}
	//client가 요청한 파일이 서버에 없으면 404 not found error message 전송.
	else if(exist == -1){
		n = send(newsockfd, error_header, size, 0); //Server의 Error Response Message 전송.
		printf("%s\n", error_header);
		free(error_header);
	}

	if(n<0)	error("ERROR writing to socket");

	close(sockfd);
	close(newsockfd);

	return 0;
}
