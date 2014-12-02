#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;

string sharedFilePath;
string trackerFilePath;
int MAX_RECV_LENGTH = 1024;

struct TrackerFile {
	string filename;
	string filesize;
	string description;
	string md5;
	string ip;
	string port;
};

struct Config {
	int port_num;
	string ip_addr;
	int update_time;
};

Config configFile;

void createDirectories();

void loadConfig();

int setupConnections();

string requestTrackerFile(int sockid, string file);

TrackerFile parseTrackerFile(string tfile);

void downloadFile(TrackerFile tf, int sockid);

void processCreateTrackerCommand(int sockid);

void processUpdateTrackerCommand(int sockid);

void processListCommand(int sockid);

void processGetCommand(int sockid, string file);

int main(int argc,char *argv[]){	
	int sockid;
	string command;

	loadConfig();
    
	system("clear");

	createDirectories();

	while(true) {
		sockid = setupConnections();
		cout << "Connected to server and ready for communication..." << endl;
		cout << endl;
		cout << "Choose an action:" << endl;
		cout << "1) Create Trackers" << endl;
		cout << "2) Update Trackers" << endl;
		cout << "3) List Tracker Files from Server" << endl;
		cout << "4) Get File" << endl;
		cout << "5) Exit" << endl;
		cin >> command;
		system("clear");

		if(command == "1"){
			cout << "Sending create tracker request..." << endl;
			processCreateTrackerCommand(sockid);
		} else if(command == "2") {
			cout << "Sending update tracker request..." << endl;
			processUpdateTrackerCommand(sockid);
		} else if(command == "3") {
			cout << "Sending list request..." << endl;
			processListCommand(sockid);
		} else if(command == "4") {
			system("clear");
			cout << "Name of file to download: ";
			cin >> command;
			processGetCommand(sockid, command);
			cout << "File download complete." << endl;
			sleep(1);
		} else if(command == "5") {
			close(sockid);
			printf("Connection Closed");
			exit(0);
		} else {
			printf("Unrecognized command");
		}
		cout << endl << endl;
	}
    
    return 0;
}

void createDirectories() {
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

void loadConfig() {
	ifstream fin("config.txt");
	string line;
	while (getline(fin, line)) {
		istringstream sin(line.substr(line.find("=") + 1));
		if (line.find("PORT") != -1) {
			sin >> configFile.port_num;
		}		
		else if (line.find("IPADDRESS") != -1) {
			sin >> configFile.ip_addr;
		}
		else if (line.find("UPDATETIME") != -1) {
			sin >> configFile.update_time;
		}
	}
}

int setupConnections() {
	int sockid;
	struct sockaddr_in server_addr;
	// int server_port=5001;

	if ((sockid = socket(AF_INET,SOCK_STREAM,0))==-1){//create socket
		printf("socket cannot be created\n"); exit(0);
	}

    server_addr.sin_family = AF_INET;//host byte order
    server_addr.sin_port = htons(configFile.port_num);// convert to network byte order
    server_addr.sin_addr.s_addr = inet_addr(configFile.ip_addr.c_str());
    if (connect(sockid ,(struct sockaddr *) &server_addr, sizeof(struct sockaddr))==-1){//connect and error check
		cout << errno << endl; printf("Cannot connect to server\n"); exit(0);
	}

	return sockid;
}

void processCreateTrackerCommand(int sockid) {	
	DIR* FD;
	struct dirent* in_file;
	char * FullName;  	
	struct stat statbuf;
	char buffer[20];
	int length;

	if(NULL == (FD = opendir(sharedFilePath.c_str()))) {
		cout << "error" << endl;
	}

	while((in_file = readdir(FD))) {
		string list_req = "createtracker";
		char msg[101];		
		stringstream ss;
		ss << configFile.port_num;
		string port = ss.str();
		string md5Sum;		
	
		if(strncmp(in_file->d_name, ".", 1) != 0) {
			FullName = (char*) malloc(strlen(sharedFilePath.c_str()) + strlen(in_file->d_name) + 2);
			strcpy(FullName, sharedFilePath.c_str());
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

			//Calculate the md5 checksum and insert it into tracker file
			char funct_call[50];	//store function call for file open
			char md5[100];
			strcpy(funct_call, "md5sum ");
			strcat(funct_call, "shared/");					
			strcat(funct_call, in_file->d_name);	//append file name to function call
			FILE * pipe;
			pipe = popen(funct_call, "r");	//call md5 on file
			fgets(md5, 100, pipe);	//store output from md5sum call
			md5Sum = strtok(md5, " ");	
			
			list_req += md5Sum;

			list_req += " ";
			list_req += configFile.ip_addr;
			list_req += " ";
			list_req += port;

			cout << list_req << endl;
			if((write(sockid, list_req.c_str(), list_req.size())) < 0){//inform the server of the list request
				printf("Send_request failure\n"); exit(0);
			}

		    if((length = read(sockid, &msg, 100) < 0)){// read what server has said
				printf("Read failure\n"); exit(0); 
			}

			msg[100] = '\0';
			list_req="";
			cout << "Receiving create tracker response: " << msg << endl;
			close(sockid);		
			sockid = setupConnections();	
		}
	}
	
	close(sockid);
}

void processUpdateTrackerCommand(int sockid) {
	DIR* FD;
	struct dirent* in_file;
	char * FullName;  	
	struct stat statbuf;
	char buffer[20];
	int length;
	stringstream ss;
	ss << configFile.port_num;
	string port = ss.str();

	if(NULL == (FD = opendir(sharedFilePath.c_str()))) {
		cout << "error" << endl;
	}

	while((in_file = readdir(FD))) {
		string list_req = "updatetracker";
		char msg[101];		
		if(strncmp(in_file->d_name, ".", 1) != 0) {
			FullName = (char*) malloc(strlen(sharedFilePath.c_str()) + strlen(in_file->d_name) + 2);
			strcpy(FullName, sharedFilePath.c_str());
			strcat(FullName, "/");
			strcat(FullName, in_file->d_name);
			stat(FullName, &statbuf);
			free(FullName);
			snprintf(buffer, 20, "%d", statbuf.st_size);

			list_req += " ";
			list_req += in_file->d_name;
			list_req += " ";
			list_req += "0";
			list_req += " ";			
			list_req += buffer;
			list_req += " ";
			list_req += configFile.ip_addr;
			list_req += " ";
			list_req += port;

			cout << list_req << endl;
			if((write(sockid, list_req.c_str(), list_req.size())) < 0){//inform the server of the list request
				printf("Send_request failure\n"); exit(0);
			}

		    if((length = read(sockid, &msg, 100) < 0)){// read what server has said
				printf("Read failure\n"); exit(0); 
			}

			msg[100] = '\0';
			list_req="";
			cout << "Receiving update tracker response: " << msg << endl;
			close(sockid);
			sockid = setupConnections();
		}
	}
	
	close(sockid);
}

void processListCommand(int sockid) {
	string list_req = "REQ LIST";
	char msg[1001];
	int length;

	if((write(sockid, list_req.c_str(), list_req.size())) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(1);
	}

    if((length = read(sockid, &msg, 1000))< 0){// read what server has said
		cout << errno << endl; printf("Read  failure\n"); exit(1); 
	}
	
	msg[length+1] = '\0';
	cout << msg << endl;

	close(sockid);
}

void processGetCommand(int sockid, string file) {
	string tfile = requestTrackerFile(sockid, file);
	TrackerFile tf = parseTrackerFile(tfile);

	sockid = setupConnections();
	downloadFile(tf, sockid);
	close(sockid);
}

string requestTrackerFile(int sockid, string file) {
	string list_req = "get ";
	char fpath[100];
	char recvBuf[MAX_RECV_LENGTH];
	char messageBody[MAX_RECV_LENGTH];
	int fr_block_size;
	int write_size;
	bool isBody;
	TrackerFile tf;

	list_req += file;
	strcpy(fpath, trackerFilePath.c_str());
	strcat(fpath, file.c_str());
	strcat(fpath, ".track");

	FILE *fr = fopen(fpath, "wb");
	if (fr == NULL) {
		cout << "File cannot be opened" << endl;
		exit(1);
	}	

	if((write(sockid,list_req.c_str(), list_req.size())) < 0){//inform the server of the list request
		printf("Send_request  failure\n"); exit(0);
	}

	bzero(recvBuf, MAX_RECV_LENGTH);
	bzero(messageBody, MAX_RECV_LENGTH);
	int j = 0;
	while((fr_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0))) {
		for(int i = 0; i < strlen(recvBuf); i++) {			
			if(recvBuf[i] == '<') {
				isBody = false;
			}
			if(isBody) {
				messageBody[j] = recvBuf[i];
				j++;
			}
			if(recvBuf[i] == '\n') {
				isBody = true;
			}
		}
		bzero(recvBuf, MAX_RECV_LENGTH);		
	}

	write_size = fwrite(messageBody, sizeof(char), strlen(messageBody), fr);
	fclose(fr);
	close(sockid);
	return fpath;
}

TrackerFile parseTrackerFile(string tfile) {
	ifstream in;
	TrackerFile tf;
	if(in.good()) {
		in.open(tfile.c_str());
	}

	getline(in, tf.filename);
	getline(in, tf.filesize);
	getline(in, tf.description);
	getline(in, tf.md5);
	getline(in, tf.ip);
	getline(in, tf.port);

	return tf;
}

void downloadFile(TrackerFile tf, int sockid) {
	char fpath[100];
	char recvBuf[MAX_RECV_LENGTH];
	int fd_block_size;
	int write_size;
	string download_req = "download ";
	download_req += tf.filename;

	strcpy(fpath, sharedFilePath.c_str());
	strcat(fpath, tf.filename.c_str());	
	FILE *fd = fopen(fpath, "wb");
	if(fd == NULL) {
		cout << "File cannot be opened" << endl;
		exit(1);
	}

	if((write(sockid, download_req.c_str(), download_req.size())) < 0) {
		printf("Send request failure\n");
	}

	bzero(recvBuf, MAX_RECV_LENGTH);
	cout << "Downloading..." << endl;
	while((fd_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0))) {
		write_size = fwrite(recvBuf, sizeof(char), fd_block_size, fd);
		bzero(recvBuf, MAX_RECV_LENGTH);
	}

	fseek(fd, 0, SEEK_END);
	cout << "Received " << ftell(fd) << " bytes..." << endl;
	rewind(fd);
	fclose(fd);
}
