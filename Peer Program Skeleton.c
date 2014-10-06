#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <iostream>
#include <arpa/inet.h>
#include <dirent.h>
using namespace std;

void processCreateTrackerCommand(int sockid);

void processUpdateTrackerCommand(int sockid);

void processListCommand(int sockid);

void processGetCommand(int sockid);

int main(int argc,char *argv[]){	
   	char server_address[50];
	int server_port=5000;  // you should instead read from configuration file   
	struct sockaddr_in server_addr;
	int sockid;
        
	cout << "creating socket" << endl;

	if ((sockid = socket(AF_INET,SOCK_STREAM,0))==-1){//create socket
		printf("socket cannot be created\n"); exit(0);
	}
                                              
   	cout << "connecting socket" << endl;

    server_addr.sin_family = AF_INET;//host byte order
    server_addr.sin_port = htons(server_port);// convert to network byte order
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(sockid ,(struct sockaddr *) &server_addr, sizeof(struct sockaddr))==-1){//connect and error check
		cout<< errno << endl; printf("Cannot connect to server\n"); exit(0);
	}
   /* If connected successfully*/
    cout << "connected" << endl;

	if(strcmp(argv[1],"createtracker") == 0){
		processCreateTrackerCommand(sockid);
	} else if(strcmp(argv[1], "updatetracker") == 0) {
		processUpdateTrackerCommand(sockid);
	} else if(strcmp(argv[1], "list") == 0) {
		processListCommand(sockid);
	} else if(strcmp(argv[1], "get") == 0) {
		processGetCommand(sockid);
	} else {
		printf("Unrecognized command");
	}
    
}

void processCreateTrackerCommand(int sockid) {
	// int list_req=htons(LIST);
	string list_req = "createtracker";
	char msg[100];
	DIR* FD;
	struct dirent* in_file;
	char const * DirName = "/Users/clayton/Desktop/peerfiles";
	char * FullName;  	
	struct stat statbuf;
	char buffer[20];
	int length;

	if(NULL == (FD = opendir("/Users/clayton/Desktop/peerfiles"))) {
		cout << "error" << endl;
	}

	while((in_file = readdir(FD))) {
		if(strncmp(in_file->d_name, ".", 1) != 0) {
			FullName = (char*) malloc(strlen(DirName) + strlen(in_file->d_name) + 2);
			strcpy(FullName, DirName);
			strcat(FullName, "/");
			strcat(FullName, in_file->d_name);
			stat(FullName, &statbuf);
			free(FullName);

			list_req += " ";
			list_req += in_file->d_name;
			list_req += " ";
			snprintf(buffer, 20, "%d", statbuf.st_size);
			list_req += buffer;
			list_req += " ";
			list_req += "description";
			list_req += " ";
			list_req += "132451325987";
			list_req += " ";
			list_req += "127.0.0.1";
			list_req += " ";
			list_req += "5000";

			if((write(sockid, list_req.c_str(), list_req.size())) < 0){//inform the server of the list request
				printf("Send_request failure\n"); exit(0);
			}

		    if((length = read(sockid, &msg, 100) < 0)){// read what server has said
				printf("Read  failure\n"); exit(0); 
			}

			msg[length] = '\0';
			cout << msg << endl;
		}
	}
	
	close(sockid);
	printf("Connection closed\n");
    exit(0);
}

void processUpdateTrackerCommand(int sockid) {
	char* list_req = "updatetracker";
	char* msg;
	if((write(sockid,list_req, sizeof(list_req))) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(0);
	}

    if((read(sockid, &msg, sizeof(msg)))< 0){// read what server has said
		printf("Read  failure\n"); exit(0); 
	}
	
	close(sockid);
	printf("Connection closed\n");
    exit(0);
}

void processListCommand(int sockid) {
	char* list_req = "list";
	char* msg;
	if((write(sockid,list_req, sizeof(list_req))) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(0);
	}

    if((read(sockid, &msg, sizeof(msg)))< 0){// read what server has said
		printf("Read  failure\n"); exit(0); 
	}
	
	close(sockid);
	printf("Connection closed\n");
    exit(0);
}

void processGetCommand(int sockid) {
	char* list_req = "get";
	char* msg;
	if((write(sockid,list_req, sizeof(list_req))) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(0);
	}

    if((read(sockid, &msg, sizeof(msg)))< 0){// read what server has said
		printf("Read  failure\n"); exit(0); 
	}
	
	close(sockid);
	printf("Connection closed\n");
    exit(0);
}
