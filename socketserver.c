#include <sys/socket.h>
#include <netinet/in.h>
#include<signal.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "./threadpool/threadpool.h" 

#define THREAD_NUM 100
#define QUEUE_SIZE 1000
#define FORBIDDEN "HTTP/1.1 Forbidden 403\n"
#define OK "HTTP/1.1 200 OK\n"
#define UNAUTHORIZED "HTTP/1.1 Unauthorized 401\n"
#define NOTFOUND "HTTP/1.1 Not Found 404\n"

int listenfd;

void SignalFunction(int sig)
{
	switch(sig){
		case 2:
			printf("<Server>Server stop...\n");
			close(listenfd);
			exit(0);
			break;
		default:
			break;
	}
}

int AddHttpHeader(char *header,char *status)
{
	if(header == NULL || status == NULL)
		return -1;
	strcat(header, status);
	strcat(header, "Server: RcheWebServ v0.1");
	strcat(header, "Content-Type: text/html\n\n");	
	return 0;		
}

int AddHttpContent(char *content, char *address)
{
	FILE *fp;
	char line[1024];
	if(content == NULL || address == NULL)
		return -1;
	if(NULL == (fp = fopen(address,"r")))
		return -1;
	while(fgets(line,sizeof(line),fp) != NULL){
		strcat(content,line);
	}
	fclose(fp);
	return 0;
}

int HttpParser(char *httpRequest,int connfd)
{
	char header[1024] = "";
	char content[10000];
	char *method;
	char *requestFile;
	
	method = strtok(httpRequest," ");
	requestFile = strtok(NULL," ");
	
	// 401 Unauthorized
	if(requestFile[0] == '.' && requestFile[1] == '.'){
		if(-1 == AddHttpHeader(header, UNAUTHORIZED))	
			printf("<Server>Error : Create Header Failed");
		if(-1 == AddHttpContent(content, "./unauthorized.html"))
			printf("<Server>Error : Create Content Failed");
	}
	// 403 Forbidden
	else if (strncmp(method, "GET", 4) && strncmp(method, "get", 4)){
		if(-1 == AddHttpHeader(header, FORBIDDEN))	
			printf("<Server>Error : Create Header Failed");
		if(-1 == AddHttpContent(content, "./forbidden.html"))
			printf("<Server>Error : Create Content Failed");
	}
	// 404 Not Found
	else if(-1 == access((requestFile+1), F_OK)){
		if(-1 == AddHttpHeader(header, NOTFOUND))	
			printf("<Server>Error : Create Header Failed");
		if(-1 == AddHttpContent(content, "./notfound.html"))
			printf("<Server>Error : Create Content Failed");
	}
	// 200 OK
	else{
		if(-1 == AddHttpHeader(header, OK))
			printf("<Server>Error : Create Header Failed");
		if(-1 == AddHttpContent(content, (requestFile+1)))
			printf("<Server>Error : Create Content Failed");
	}					
	write(connfd, header, strlen(header));
	write(connfd, content, strlen(content));
	return 0;	
}

void communication(void *arg)
{
	int connfd = *(int *)arg;
	char buffer[10000];
	memset(buffer, '\0', sizeof(buffer)); 
	if (read(connfd, buffer, sizeof(buffer)) < 0)
	{
		printf("\n<Server>Error : Read Failed \n");
	}
	else{
		HttpParser(buffer,connfd);
	}
	close(connfd);	
}

int socket_server(int port)
{
	int connfd = 0;
	struct sockaddr_in serv_addr; 
	
	printf("<Server>Initialize...\n");
	//create socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));   
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port); 

	//creat thread pool
	threadpool_t *pool = (threadpool_t *)malloc(sizeof(threadpool_t));
	if(-1 == (threadpool_init(pool, THREAD_NUM, QUEUE_SIZE)))
		printf("<Server>Error : Thread Init Failed\n");

	//assign port number to socket
	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

	printf("<Server>Listening...\n");
	//the max number of connection for socket is 10
	listen(listenfd, 10); 

	while((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL))){
		threadpool_addQueue(pool, communication, (void *)&connfd);
	}
	close(listenfd);
	threadpool_destroy(pool);
	threadpool_freeMem(pool);
	return 0;
}

int main(int argc, char *argv[])
{

	if(signal(SIGINT, SignalFunction) == SIG_ERR){
		return 0;
	}
	if(argc < 2){
        printf("<Server>Error : must to indicate the port of server\n");
		return 0;
    }
	else
		socket_server(atoi(argv[1]));
	return 0;
}
