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
#include "queue.h"

char response_message[1024 * 1024 * 10]; //서버에서 클라이언트에 전송할 response message와 요청한 파일을 담기 위한 버퍼
char log_buffer[10000]; //로그에 저장할 정보를 저장하기 위한 버퍼
char current_time[10000]; //시간 정보를 저장하기 위한 버퍼
char temp_file[1024 * 1024 * 5]; //클라이언트에게 요청받은 파일을 전송하기 위해 파일 내용을 저장하는 버퍼
char hit_file[1024 * 512]; //캐시에서 hit가 났을 때 hit가 발생한 파일을 받아오기 위한 버퍼

void error(char *msg){
	perror(msg);
  exit(1);
} //에러메시지, 에러내용 출력

char* get_time(char* time_buffer){
  time_t nowtime;
  time(&nowtime);
  strftime(time_buffer, 64, "%a %d %m %Y %X %Z", localtime(&nowtime));
  return time_buffer;
} //로그에 저장하기 위한 현재시간을 받아오는 함수. "Tue 28 11 2017 23:25:04"의 형태로 시간 정보를 받아와 time_buffer에 저장한다.

void write_log(char* log_buffer){
  int logfd;
  if((logfd = open("proxy.log", O_CREAT | O_RDWR | O_APPEND, 0644)) < 0){
    error("Opening Log File: ");
  }
  if(write(logfd, log_buffer, strlen(log_buffer)) < 0){
    error("Writing Log File: ");
  }
} //로그 파일을 쓰기 위한 함수. proxy.log파일이 없으면 생성하도록 하고, proxy.log파일을 열어 log_buffer에 담긴 내용을 쓴다.

int main(int argc, char *argv[]){
	LRU_QUEUE* queue = (LRU_QUEUE*) calloc(1, sizeof(LRU_QUEUE)); //캐싱한 데이터를 저장할 LRU 큐 선언.
	init(queue); //큐 초기화
	int k;
	k = 0;
	int p_sockfd, cli_sockfd; //p_sockfd: 클라이언트 -> 프록시로 연결되는 소켓, cli_sockfd: 프록시 -> 클라이언트로 연결되는 소켓
	int portno; //포트 번호를 저장할 변수 선언
	socklen_t clilen; //cli_sockfd를 설정하기 위한 socket_t형 변수.

	char c_request_buffer[sizeof(char) * buffer_size]; //클라이언트가 보낸 request message를 프록시가 전달하기 위해 request message 저장을 위한 버퍼

	struct sockaddr_in prox_addr, cli_addr; //p_sockfd, cli_sockfd의 주소를 설정하기 위한 변수 선언

	int n;
	if(argc < 2){
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	p_sockfd = socket(AF_INET, SOCK_STREAM, 0); //p_sockfd 설정
	if(p_sockfd < 0) error("ERROR opening socket");

	bzero((char *) &prox_addr, sizeof(prox_addr));
	portno = atoi(argv[1]); //host의 url을 integer형태로 변환
	prox_addr.sin_family = AF_INET;
	prox_addr.sin_addr.s_addr = INADDR_ANY; //p_sockfd의 IP주소 설정
	prox_addr.sin_port = htons(portno); //p_sockfd의 port번호는 실행시 입력받은 값으로 설정

	if(bind(p_sockfd, (struct sockaddr*) &prox_addr, sizeof(prox_addr)) < 0) //p_sockfd 바인딩
		error("ERROR on binding");

  listen(p_sockfd,10); //queue = 10으로 설정하여 p_sockfd에 대한 연결 listening

	while(1){
		printf("\nWaiting Client...\n");
		NODE* newNode; //큐에 삽입할 노드 선언
		NODE* searchNode = (NODE*) calloc(1, sizeof(NODE)); //노드 용량 설정
		clilen = sizeof(cli_addr); //cli_sockfd 주소 설정을 위한 변수 선언
		cli_sockfd = accept(p_sockfd, (struct sockaddr *) &cli_addr, &clilen); //클라이언트와 프록시를 소켓으로 연결
		if(cli_sockfd < 0) error("ERROR on accept");

		bzero(c_request_buffer, sizeof(c_request_buffer)); //request_message를 받아올 버퍼 초기화
		n = read(cli_sockfd, c_request_buffer, sizeof(c_request_buffer)); //클라이언트가 보낸 request message를 c_request_buffer에 저장
		if(n < 0) error("ERROR reading from socket");

		//printf("======================Request message from client to proxy======================\n%s",c_request_buffer); //c_request_buffer 내용 출력

		char temp[256];
		bzero(temp, 256);
		int i=0, j=0;

		/* 클라이언트가 보낸 request message에서 host url, 요청하는 object 정보를 파싱 */
		while(1){
			i++;
			if(c_request_buffer[i] == '/'){
				break;
			}
		}
		i += 2; //예: "GET http://cnlab.hanyang.ac.kr/static/js/Clarendon_LT_Std_700.font.js HTTP/1.1 ..." 중 "GET http://"까지 제거한다.
		//현재 c_request_buffer: "cnlab.hanyang.ac.kr/static/js/Clarendon_LT_Std_700.font.js HTTP/1.1 ..."
		while(c_request_buffer[i] != ' '){
			temp[j] = c_request_buffer[i];
			j++;
			i++;
		} //temp 버퍼에 "cnlab.hanyang.ac.kr/static/js/Clarendon_LT_Std_700.font.js HTTP/1.1 ..." 중 " " 전인 "cnlab.hanyang.ac.kr/static/js/Clarendon_LT_Std_700.font.js"까지만 저장.
		temp[j] = '\0';
		//현재 temp: "cnlab.hanyang.ac.kr/static/js/Clarendon_LT_Std_700.font.js"

		char URL[256]; //end server host의 url주소를 저장하기 위한 버퍼
		char object_name[1024]; //클라이언트가 요청하는 object의 이름을 저장하기 위한 버퍼
		bzero(object_name, 1024); //object_name 초기화
		bzero(URL, 256); //URL 초기화
		object_name[0] = '/'; //object_name의 맨 처음 문자를 "/"로 설정.
		i=0, j=1;
		while(temp[i] != '\0'){
			URL[i] = temp[i];
			i++;
			if(temp[i] == '/'){
				i++;
				break;
			}
		} //"cnlab.hanyang.ac.kr/static/js/Clarendon_LT_Std_700.font.js" 중 URL에 "cnlab.hanyang.ac.kr" 까지 저장.
		//현재 URL: "cnlab.hanyang.ac.kr"
		while(temp[i] != '\0'){
			object_name[j] = temp[i];
			j++;
			i++;
		} //"cnlab.hanyang.ac.kr/static/js/Clarendon_LT_Std_700.font.js" 중 object_name에 "static/js/Clarendon_LT_Std_700.font.js"까지 저장.
		//현재 object_name: "/static/js/Clarendon_LT_Std_700.font.js"

		long int* Url_ip; //end server의 IP주소를 저장하기 위한 변수
		char* URL_ip; //end server의 url을 저장하기 위한 변수
		struct in_addr hoen;
	  struct hostent *server_struct;

		server_struct = gethostbyname(URL); //end server의 hostent 구조체 받아옴
		if(server_struct == NULL){
			error("DNS Search: ");
			exit(0);
		}

		while(*server_struct->h_addr_list != NULL){
			Url_ip = (long int *)*server_struct->h_addr_list;
			hoen.s_addr = *Url_ip;
			server_struct->h_addr_list++;
		} //end server의 정보를 server_struct에 저장

		URL_ip = inet_ntoa(hoen); //end server의 url 저장

		printf("========================parsing result========================\n");
		printf("URL\t: %s\nObject\t: %s\r\n", URL, object_name); //파싱한 url, object name 출력

		printf("========================ip address========================\n%s\n\n", URL_ip); //end server의 IP주소 출력

		searchNode = search(queue, object_name); //큐에 클라이언트가 요청한 정보가 캐싱되어 있는지 확인

		if(searchNode != NULL){ //searchNode != NULL이므로 요청한 object가 프록시 내에 저장되어 있음을 의미: 프록시에 저장된 정보를 그대로 클라이언트에 전달한다.
			int n;
			printf("HIT\n");
			push(queue, pop(queue, searchNode)); //HIT가 발생한 노드의 LRU 순서 조절
			strcat(hit_file, searchNode->response_message); //HIT가 발생한 노드에 저장된 response message를 hit_file 버퍼에 저장
			if((n = send(cli_sockfd, hit_file, sizeof(hit_file), 0)) < 0) error("HIT Sending error"); //hit_file을 cli_sockfd으로 보내준다.
			bzero(hit_file, sizeof(hit_file)); //hit_file 버퍼 초기화
		}

		else{ //요청한 object가 프록시 내에 없는 경우: end server까지 가서 object를 받아와야 한다.
			printf("MISS\n");
			int s_sockfd, n; //s_sockfd: 프록시와 end server를 연결하는 소켓.
			struct sockaddr_in addr; //s_sockfd의 주소를 설정하기 위한 struct

			s_sockfd = socket(AF_INET,SOCK_STREAM,0);
			if(s_sockfd<0) perror("ERROR: ");
			memset((char*) &addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(URL_ip); //앞에서 받아온 end server의 IP주소로 설정.
			addr.sin_port = htons(80); //HTTP connection을 위해 port번호 80으로 설정.

			if(connect(s_sockfd,(struct sockaddr*)&addr,sizeof(addr))<0) perror("ERROR: connection error"); //connection 생성

			char tmp[buffer_size];
		  sprintf(tmp, "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n%c", object_name, URL, 0); //프록시에서 end server로 보내 줄 HTTP request message 저장
		  char request_to_server[buffer_size];
		  strcpy(request_to_server, tmp);

			if((n = send(s_sockfd, request_to_server, sizeof(request_to_server), 0)) < 0) error("Sending to socket error"); //저장한 request message 전송

			int object_size;

			int history = 0;

			/*
					end server에서 받아온 response message를 클라이언트에게 전송하고, 캐싱을 위한 파싱 수행.
			*/
			while((n = read(s_sockfd, response_message, sizeof(response_message))) > 0){
				int k;
				write(cli_sockfd, response_message, n); //프록시가 end server에서 받아온 response message를 클라이언트에게 보내주기 위해 소켓에 쓴다.
				sprintf(log_buffer, "%s: %s http://%s%s %d\n%c\n", get_time(current_time), URL_ip, URL, object_name, n, 0); //로그에 작성할 내용 저장
				write_log(log_buffer); //로그 작성
				if(history == 0){
					strcpy(temp_file, (strstr(response_message, "\r\n\r\n")+4)); //read함수가 읽어온 response message에서 response header 제거.
				}
				else{
					for(k=0; k<n; k++){
						temp_file[history + k] = response_message[k];
						//read 함수가 정해진 크기가 아닌 임의의 바이트 단위로 읽어왔을 때, 읽어온 파일의 내용 손실 없이 전달하기 위해 앞서 읽은 바이트만큼 커서를 이동시키고 내용을 이어서 쓴다.
						if(response_message[k] == '^' && response_message[k+1] == 'C'){
							temp_file[history + k] = response_message[k+2];
						} //바이너리 파일로 읽어왔을 때 ^C문자를 읽어 프로세스가 종료되는 경우 방지
					}
				}
				history = strlen(temp_file); //읽어온 바이트 수 저장.
			}
			temp_file[strlen(temp_file)] = '\0'; //맨 끝에 NULL문자 삽입
			if(n < 0) error("Reading Error");

			object_size = strlen(temp_file); //받아온 object의 크기 저장

			if(object_size <= object_max && strlen(object_name) > 1){ //object의 크기가 512KB보다 작고, object의 확장자가 존재할 경우 캐싱 실행.
				newNode = create_node(c_request_buffer, temp_file, object_name); //노드에 request message, object 파일, object 이름 저장.
				if(queue->object_count == 0){ //현재 큐 내의 오브젝트 갯수가 10개일 경우, LRU 정책에 따라 노드 삭제.
					free(evict(queue));
				}
				if(queue->remain < newNode->size){ //큐의 남은 공간이 새로 삽입될 노드의 object 크기보다 작을 경우, 남은 공간이 충분해질 때까지 반복하여 LRU 정책에 따라 노드 삭제.
					while(1){
						free(evict(queue));
						if(queue->remain >= newNode->size){
							break;
						}
					}
				}
				push(queue, newNode); //큐에 새로운 노드 추가
			}

			bzero(log_buffer, 10000); //로그 버퍼 초기화
			bzero(current_time, 10000); //시간정보 버퍼 초기화

		  close(s_sockfd); //프록시와 서버 간 소켓 닫아줌.
		}

		print_queue(queue); //큐 정보 출력

		bzero(response_message, sizeof(response_message)); //response_message 버퍼 초기화
		bzero(temp_file, sizeof(temp_file)); //temp_file 버퍼 초기화

		close(cli_sockfd); //클라이언트 -> 프록시 소켓 닫아줌.
	}

  close(p_sockfd); //프록시 -> 클라이언트 소켓 닫아줌.

  return 0;
}
