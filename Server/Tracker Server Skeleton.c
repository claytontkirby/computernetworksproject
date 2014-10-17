#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <iostream>
#include <vector>
#include <arpa/inet.h>
#include <fstream>
#include <string>
#include <fcntl.h>
using namespace std;

string trackerFilePath;
string sharedFilePath;
int MAX_SEND_LENGTH = 1028;

struct TrackerFile {
	string filename;
	string filesize;
	string description;
	string md5;
	string ip;
	string port;
};

vector<TrackerFile> trackerFiles;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

void createTrackerFileLoc();

void loadTrackerFiles();

int setupSocketConnections();

void listenForConnections(int sockid);

void peer_handler(int sock_child);

void handle_createtracker_req(int sock_child, char* read_msg);

string createTrackerFile(char* read_msg);

TrackerFile parseCreateTrackerMsg(char* read_msg);

void handle_list_req(int sock_child);

void handle_download(int sock_child, char* read_msg);

string parseGetRequest(char* read_msg);

void handle_get_req(int sock_child, char* read_msg);

// void tokenize_createmsg(char* msg);

void tokenize_updatemsg(char* msg);

void handle_updatetracker_req(int sock_child);

int main(){
   int sockid;

   std::system("clear");

   createTrackerFileLoc();

   loadTrackerFiles();

   sockid = setupSocketConnections();

   while(true){
   	cout << endl;
   	cout << "Tracker server listening for incoming connections..." << endl;
   	if (listen(sockid, 2) < 0){ //(parent) process listens at sockid and check error
	   printf(" Tracker  SERVER CANNOT LISTEN\n"); exit(0);
    }  
   	listenForConnections(sockid);
   }         
}
    
void createTrackerFileLoc() {
	char cwd[100];
	struct stat st = {0};

	if(getcwd(cwd, sizeof(cwd))==NULL) {
		exit(1);
	}

	sharedFilePath = cwd;
	trackerFilePath = cwd;
	sharedFilePath += "/shared";
	trackerFilePath += "/trackers";
	if(stat(sharedFilePath.c_str(), &st)) {
		mkdir(sharedFilePath.c_str(), 0700);
	}

	if(stat(trackerFilePath.c_str(), &st)) {
		mkdir(trackerFilePath.c_str(), 0700);
	}
	sharedFilePath += "/";
	trackerFilePath += "/";
}


void loadTrackerFiles() {
	DIR* FD;
	struct dirent* in_file;

	if(NULL == (FD = opendir(trackerFilePath.c_str()))) {
		cout << "error" << endl;
	}

	while((in_file = readdir(FD))) {
		if(in_file->d_name[0] != '.') {
			char temp[100];
			TrackerFile tf;
			ifstream in;	
			strcpy(temp, trackerFilePath.c_str());
			strcat(temp, in_file->d_name);
			if(in.good()) {
				in.open(temp);
			}

			getline(in, tf.filename);
			getline(in, tf.filesize);
			getline(in, tf.description);
			getline(in, tf.md5);
			getline(in, tf.ip);
			getline(in, tf.port);
			trackerFiles.push_back(tf);

			in.close();
		}
	}
}

int setupSocketConnections() {
   // struct sockaddr_in server_addr;
   int sockid;
   int server_port=5001;

   if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0){//create socket connection oriented
	   printf("socket cannot be created \n"); exit(0); 
   }
    
   //socket created at this stage
   //now associate the socket with local port to allow listening incoming connections
   server_addr.sin_family = AF_INET;// assign address family
   server_addr.sin_port = htons(server_port);//change server port to NETWORK BYTE ORDER
   server_addr.sin_addr.s_addr = htons(INADDR_ANY);
   client_addr.sin_family = AF_INET;
   client_addr.sin_port = htons(server_port);
   client_addr.sin_addr.s_addr = htons(INADDR_ANY); 

   if (bind(sockid ,(struct sockaddr *) &server_addr, sizeof(server_addr)) ==-1){//bind and check error
	   printf("bind  failure\n"); exit(0); 
   }  

   return sockid;                                      
}

void listenForConnections(int sockid) {
	int sockchild;
	// pid_t pid;

	if ((sockchild = accept(sockid ,(struct sockaddr *) &client_addr, (socklen_t*) &client_addr ))==-1){ /* accept connection and create a socket descriptor for actual work */
		   printf("Tracker Cannot accept...\n"); exit(0); 
	}

	// if ((pid=fork())==0) {//New child process will serve the requester client. separate child will serve separate client
	   // close(sockid);   //child does not need listener
	   peer_handler(sockchild);//child is serving the client.		   
	   close(sockchild);// printf("\n 1. closed");
	   // exit(0);         // kill the process. child process all done with work
    // }

	// close(sockchild);  // parent all done with client, only child will communicate with that client from now
}

void peer_handler(int sock_child){ // function for file transfer. child process will call this function     
    //start handiling client request	
	int length;
	char read_msg[100];
	char* fname = NULL;
	// int MAXLINE = 10;
	length=read(sock_child, &read_msg, 100);
	read_msg[length]='\0';

	cout << "Message received: " << read_msg << endl;

	if((!strcmp(read_msg, "REQ LIST"))||(!strcmp(read_msg, "req list"))||(!strcmp(read_msg, "<REQ LIST>"))||(!strcmp(read_msg, "<REQ LIST>\n"))){//list command received
		handle_list_req(sock_child);// handle list request
		printf("list request handled.\n");
	}
	else if((strstr(read_msg,"get")!=NULL)||(strstr(read_msg,"GET")!=NULL)){// get command received
		handle_get_req(sock_child, read_msg);
	}
	else if((strstr(read_msg,"createtracker")!=NULL)||(strstr(read_msg,"Createtracker")!=NULL)||(strstr(read_msg,"CREATETRACKER")!=NULL)){// get command received
		handle_createtracker_req(sock_child, read_msg);		
	}
	else if((strstr(read_msg,"updatetracker")!=NULL)||(strstr(read_msg,"Updatetracker")!=NULL)||(strstr(read_msg,"UPDATETRACKER")!=NULL)){// get command received
		// tokenize_updatemsg(read_msg);
		// handle_updatetracker_req(sock_child);		
	} else if(strstr(read_msg, "download") != NULL) {
		handle_download(sock_child, read_msg);
	}
	
}//end client handler function

void handle_createtracker_req(int sock_child, char* read_msg) {
	string msg;
	msg = createTrackerFile(read_msg);
	cout << "Sending create tracker response..." << endl;
	if((write(sock_child, msg.c_str(), 100)) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(0);
	}
}

string createTrackerFile(char* read_msg) {
	TrackerFile tf = parseCreateTrackerMsg(read_msg);
	FILE *fp;
	string err = "<createtracker fail>";

	fp = fopen((trackerFilePath + "/" + tf.filename + ".track").c_str(), "r");

	if(fp) {
		return "<createtracker ferr>";
	} else {
		fp = fopen((trackerFilePath + "/" + tf.filename + ".track").c_str(), "w");
	}	


	if(fputs((tf.filename.c_str()), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.filesize.c_str()), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.description.c_str()), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.md5.c_str()), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.ip.c_str()), fp) == EOF) { return err;}
	fputs("\n", fp);
	if(fputs((tf.port.c_str()), fp) == EOF) { return err;}

	// fflush(fp);
	fclose(fp);
	trackerFiles.push_back(tf);

	return "<createtracker succ>";
}

TrackerFile parseCreateTrackerMsg(char* read_msg) {
	char* msg = read_msg;
	TrackerFile tf;

	strtok(msg, " ");
	tf.filename = strtok(NULL, " ");
	tf.filesize = strtok(NULL, " ");
	tf.description = strtok(NULL, " ");
	tf.md5 = strtok(NULL, " ");
	tf.ip = strtok(NULL, " ");
	tf.port = strtok(NULL, " ");	

	return tf;
}

void handle_list_req(int sock_child) {
	string msg = "<REP LIST ";
	msg = msg + std::to_string(trackerFiles.size());
	msg = msg + ">\n";

	for(int i = 0; i < trackerFiles.size(); i++) {
		msg = msg + "<";
		msg += std::to_string(i+1);
		msg += " ";
		msg += trackerFiles[i].filename;
		msg += " ";
		msg += trackerFiles[i].filesize;
		msg += " ";
		msg += trackerFiles[i].description;
		msg += " ";
		msg += trackerFiles[i].md5;
		msg += " ";
		msg += trackerFiles[i].ip;
		msg += " ";
		msg += trackerFiles[i].port;
		msg += ">\n";
	}

	msg = msg + "<REP LIST END>\n";

	cout << "Sending list response..." << endl;
	if(write(sock_child, msg.c_str(), 1000) < 0) {
		printf("Send_request failure\n"); exit(0);
	}
}

void handle_get_req(int sock_child, char* read_msg) {
	string filename = parseGetRequest(read_msg);
	TrackerFile tf;
	char sendBuf[MAX_SEND_LENGTH];
	char filePathBuf[100];
	int fileBlockSize;

	strcpy(filePathBuf, trackerFilePath.c_str());
	strcat(filePathBuf, filename.c_str());
	strcat(filePathBuf, ".track");
	FILE *fs = fopen(filePathBuf, "rb");
	if(fs == 0) {
		cout << "error opening file" << endl;
	}

	bzero(sendBuf, MAX_SEND_LENGTH);
	while((fileBlockSize = fread(sendBuf, sizeof(char), MAX_SEND_LENGTH, fs))) {
		cout << "Sending tracker file to peer..." << endl;
		if(send(sock_child, sendBuf, fileBlockSize, 0) < 0) {
			cout << "Error sending tracker file" << endl;
		}
		bzero(sendBuf, MAX_SEND_LENGTH);
	}
	fclose(fs);

	for(int i = 0; i < trackerFiles.size(); i++) {
		if(trackerFiles[i].filename == filename) {
			tf = trackerFiles[i];
		}
	}
}

string parseGetRequest(char* read_msg) {
	char* msg = read_msg;

	strtok(msg, " ");	
	return strtok(NULL, " ");
}

void handle_download(int sock_child, char* read_msg) {
	strtok(read_msg, " ");
	string filename = strtok(NULL, " ");
	int sendBuf[MAX_SEND_LENGTH];
	char filePathBuf[100];
	int fileBlockSize;

	strcpy(filePathBuf, sharedFilePath.c_str());
	strcat(filePathBuf, filename.c_str());
	FILE *fs = fopen(filePathBuf, "r");
	if(fs == NULL) {
		cout << "error opening file" << endl;
	}

	bzero(sendBuf, MAX_SEND_LENGTH);
	while((fileBlockSize = fread(sendBuf, sizeof(int), MAX_SEND_LENGTH, fs))) {
		if(send(sock_child, sendBuf, fileBlockSize, 0) < 0) {
			cout << "Error sending tracker file" << endl;
		}
		bzero(sendBuf, MAX_SEND_LENGTH);
	}
	fclose(fs);
}

void tokenize_updatemsg(char* msg) {

}

void handle_updatetracker_req(int sock_child) {

}
