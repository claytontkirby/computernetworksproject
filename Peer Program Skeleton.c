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

string peerFilePath;

void makeSharedDirectory();

void processCreateTrackerCommand(int sockid);

void processUpdateTrackerCommand(int sockid);

void processListCommand(int sockid);

void processGetCommand(int sockid);

int main(int argc,char *argv[]){	
   	char server_address[50];
	int server_port=5001;  // you should instead read from configuration file   
	struct sockaddr_in server_addr;
	int sockid;
    
	std::system("clear");

	makeSharedDirectory();

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

void makeSharedDirectory() {
	char cwd[100];
	struct stat st = {0};

	if(getcwd(cwd, sizeof(cwd))==NULL) {
		exit(1);
	}

	peerFilePath = cwd;
	peerFilePath += "/shared";
	if(stat(peerFilePath.c_str(), &st)) {
		mkdir(peerFilePath.c_str(), 0700);
	}
}

void processCreateTrackerCommand(int sockid) {
	string list_req = "createtracker";
	DIR* FD;
	struct dirent* in_file;
	char * FullName;  	
	struct stat statbuf;
	char buffer[20];
	int length;

	if(NULL == (FD = opendir(peerFilePath.c_str()))) {
		cout << "error" << endl;
	}

	while((in_file = readdir(FD))) {
		char msg[100];		
		if(strncmp(in_file->d_name, ".", 1) != 0) {
			FullName = (char*) malloc(strlen(peerFilePath.c_str()) + strlen(in_file->d_name) + 2);
			strcpy(FullName, peerFilePath.c_str());
			strcat(FullName, "/");
			strcat(FullName, in_file->d_name);
			stat(FullName, &statbuf);
			free(FullName);

			cout << "creating " << in_file->d_name << endl;
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
			list_req="";
			cout << "msg " << msg << endl;		
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
	string list_req = "REQ LIST";
	char msg[1000];
	int length;

	if((write(sockid, list_req.c_str(), list_req.size())) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(0);
	}

    if((length = read(sockid, &msg, 1000))< 0){// read what server has said
		printf("Read  failure\n"); exit(0); 
	}
	
	msg[length] = '\0';
	cout << msg << endl;

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
