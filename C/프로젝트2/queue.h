#define buffer_size 10000 //버퍼 최대 크기 제한
#define cache_max 5*1024*1024 //캐시 메모리 크기 5MB로 제한
#define object_max 512*1024 //캐싱 가능한 object 크기 512KB로 제한

typedef struct node{
	struct node* prev;
	struct node* next;
	char* request_message; //request message 저장
	char* response_message; //object 내용 저장
	char* object_name; //object 이름 저장
	long size; //object 크기 저장
} NODE; //Doubly Linked List를 생성하기 위한 노드 정의

typedef struct least_recently_used_queue{
	NODE* front;
	NODE* rear;
	int object_count; //큐에 저장된 object 갯수 정보
	long remain; //큐의 남은 공간 정보
} LRU_QUEUE; //LRU 정책을 적용한 큐 정의

void init(LRU_QUEUE*); //생성된 큐를 초기화하는 함수
NODE* create_node(char*, char*, char*); //노드 정보 입력 함수
NODE* search(LRU_QUEUE*, char*); //큐를 object 이름으로 검색
void push(LRU_QUEUE*, NODE*); //큐에 노드를 삽입하는 함수
NODE* evict(LRU_QUEUE*); //LRU 정책 적용하여 노드 순서 조정
NODE* pop(LRU_QUEUE*, NODE*); //큐에서 노드를 삭제하는 함수
void print_queue(LRU_QUEUE*); //큐 정보 출력 함수

void init(LRU_QUEUE* queue){
	queue->front = NULL;
	queue->rear = NULL;
	queue->remain = cache_max;
	queue->object_count = 10;
}

NODE* create_node(char* request_message, char* response_message, char* object_name){
	int requestSize = strlen(request_message);
	long responseSize = strlen(response_message);
	int objectnameSize = strlen(object_name);
	NODE* new_node = (NODE*) calloc(1, sizeof(NODE));
	new_node->prev = NULL;
	new_node->next = NULL;
	new_node->request_message = (char*) calloc(1, requestSize);
	new_node->request_message = strcpy(new_node->request_message, request_message);
	new_node->response_message = (char*) calloc(1, responseSize);
	new_node->response_message = strcpy(new_node->response_message, response_message);
	new_node->object_name = (char*) calloc(1, objectnameSize);
	new_node->object_name = strcpy(new_node->object_name, object_name);
	new_node->size = responseSize;
	return new_node;
}

NODE* search(LRU_QUEUE* queue, char* object_name){
	NODE* temp;
	for(temp = queue->front; temp != NULL; temp = temp->next){
		if(strcmp(temp->object_name, object_name) == 0 && strlen(object_name) > 1){
			return temp;
		}
	}
	return NULL;
}

void push(LRU_QUEUE* queue, NODE* new_node){
	if(queue->object_count == 10){
		queue->front = new_node;
		queue->rear = new_node;
	}
	else{
		queue->rear->next = new_node;
		new_node->prev = queue->rear;
		new_node->next = NULL;
		queue->rear = new_node;
	}
	queue->object_count--;
	queue->remain -= new_node->size;
}

NODE* evict(LRU_QUEUE* queue){
	NODE* temp = queue->front;
	queue->front = queue->front->next;
	if(queue->front != NULL){
		queue->front->prev = NULL;
	}
	else{
		queue->rear = NULL;
	}
	queue->object_count++;
	return temp;
}

NODE* pop(LRU_QUEUE* queue, NODE* node){
	NODE* temp;
	if(queue->front == node){
		return evict(queue);
	}
	else if(queue->rear == node){
		temp = queue->rear;
		queue->rear = queue->rear->prev;
		queue->rear->next = NULL;
	}
	else{
		temp = node;
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	queue->object_count++;
	return temp;
}

void print_queue(LRU_QUEUE* queue){
	NODE* temp;
	int index = 1;
	long space = cache_max;
	printf("===============================Least Recent Node===============================\n");
	for(temp = queue->front; temp != NULL; temp = temp->next){
		printf("%d: %ld, object: %s, remain: %ld \n", index, temp->size, temp->object_name, space -= temp->size);
		index++;
	}
	printf("===============================Most Recent Node================================\n");
}
