#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
using namespace std;

string sharedFilePath;
string trackerFilePath;
int MAX_RECV_LENGTH = 2000;
int CLIENT_ID = 0;
int CHUNK_SIZE = 0;
int CURRENT_CHUNK_BEGIN = 0;
int CURRENT_CHUNK_END = 0;
int QUARTER_CHUNK_SIZE = 0;
int TOTAL_FILE_SIZE = 0;
int R1 = 0;
int R2 = 0; 
string IPADDRESS = "127.0.0.1";
string PORT = "3456";
char THREAD1_RECVBUF[10000];
char THREAD2_RECVBUF[10000];
char THREAD3_RECVBUF[10000];
char THREAD4_RECVBUF[10000];
char THREAD5_RECVBUF[10000];
int THREAD1_RECVSIZE = 0;
int THREAD2_RECVSIZE = 0;
int THREAD3_RECVSIZE = 0;
int THREAD4_RECVSIZE = 0;
int THREAD5_RECVSIZE = 0;

struct PeerInfo {
	string ip;
	string port;
	string start_byte;
	string end_byte;
	string timestamp;
	string client_id;
};

struct TrackerFile {
	string filename;
	string filesize;
	string description;
	string md5;
	vector<PeerInfo> peerlist;
};

struct Config {
	int port_num;
	string ip_addr;
	int update_time;
};

struct ThreadParams {
	string name;
	string start_byte;
	string end_byte;
	string client_id;
	int sockid;
	int threadid;	
};

Config configFile;

void getWorkingDirectory();

void loadConfig();

int setupConnections();

string requestTrackerFile(int sockid, string file);

TrackerFile parseTrackerFile(string tfile);

void downloadFile(string filename, string start_byte, string end_byte, int sockid, int threadid);

void processCreateTrackerCommand(int sockid);

void processUpdateTrackerCommand(int sockid);

string processListCommand(int sockid);

void processGetCommand(int sockid, string filename, string start_byte, string end_byte, int threadid);

void main_rcv(int sock_id);

void main_snd(int sock_id);

void calculateChunk(int iteration);

void writeToFile(string filename);

void *run(void *);

int pthread_yield(void);

int main(int argc, char *argv[]){	
	int sockid;
	string command;

	// loadConfig();
    
	system("clear");

	sockid = setupConnections();

	CLIENT_ID = atoi(argv[2]);
	getWorkingDirectory();

	if(strcmp(argv[1], "rcv") == 0)
		main_rcv(sockid);
	else if(strcmp(argv[1], "snd") == 0)
		main_snd(sockid);
	else
		printf("No such client type.\n"); exit(1);

	// while(true) {
	// 	sockid = setupConnections();
	// 	cout << "Connected to server and ready for communication..." << endl;
	// 	cout << endl;
	// 	cout << "Choose an action:" << endl;
	// 	cout << "1) Create Trackers" << endl;
	// 	cout << "2) Update Trackers" << endl;
	// 	cout << "3) List Tracker Files from Server" << endl;
	// 	cout << "4) Get File" << endl;
	// 	cout << "5) Exit" << endl;
	// 	cin >> command;
	// 	system("clear");

	// 	if(command == "1"){
	// 		cout << "Sending create tracker request..." << endl;
	// 		processCreateTrackerCommand(sockid);
	// 	} else if(command == "2") {
	// 		cout << "Sending update tracker request..." << endl;
	// 		processUpdateTrackerCommand(sockid);
	// 	} else if(command == "3") {
	// 		cout << "Sending list request..." << endl;
	// 		processListCommand(sockid);
	// 	} else if(command == "4") {
	// 		system("clear");
	// 		cout << "Name of file to download: ";
	// 		cin >> command;
	// 		processGetCommand(sockid, command);
	// 		cout << "File download complete." << endl;
	// 		sleep(1);
	// 	} else if(command == "5") {
	// 		close(sockid);
	// 		printf("Connection Closed");
	// 		exit(0);
	// 	} else {
	// 		printf("Unrecognized command");
	// 	}
	// 	cout << endl << endl;
	// }
    
    return 0;
}

void main_rcv(int sock_id) {
	pthread_t t1, t2, t3, t4, t5;
	int prev_byte = 999999;
	int j = 1;
	ThreadParams p[5];
	string msg = "";
	while(msg.find("picture-wallpaper.jpg") == string::npos) {
		msg = processListCommand(sock_id);		
		sock_id = setupConnections();
		sleep(5);
	}	

	bzero(THREAD1_RECVBUF, 10000);
	bzero(THREAD2_RECVBUF, 10000);
	bzero(THREAD3_RECVBUF, 10000);
	bzero(THREAD4_RECVBUF, 10000);
	bzero(THREAD5_RECVBUF, 10000);

	while(j < 5) {
		// cout << "requesting" << endl;
		string tfile = requestTrackerFile(sock_id, "picture-wallpaper.jpg");
		sock_id = setupConnections();
		// cout << "tracker file received" << endl;
		TrackerFile tf = parseTrackerFile(tfile);
		cout << prev_byte << " " << tf.peerlist[0].end_byte << endl;		
		if(prev_byte != atoi(tf.peerlist[0].end_byte.c_str())) {
			prev_byte = atoi(tf.peerlist[0].end_byte.c_str());
			for(int i = 0; i < 5; i++) {
				p[i].name = tf.filename;
				p[i].start_byte = tf.peerlist[i].start_byte;
				p[i].end_byte = tf.peerlist[i].end_byte;
				p[i].sockid = setupConnections();
				p[i].threadid = i+1;
			}
			pthread_create(&t1, NULL, run, &p[0]);
			pthread_create(&t2, NULL, run, &p[1]);
			pthread_create(&t3, NULL, run, &p[2]);
			pthread_create(&t4, NULL, run, &p[3]);
			pthread_create(&t5, NULL, run, &p[4]);

			if(pthread_join(t1, NULL) == 0 &&
			pthread_join(t2, NULL) == 0 &&
			pthread_join(t3, NULL) == 0 &&
			pthread_join(t4, NULL) == 0 &&
			pthread_join(t5, NULL) == 0) {
				cout << "I am client_" << CLIENT_ID << ", and I received the file correctly!" << endl;
				cout << "All threads complete." << endl;
			}			
			j++;
		}
		sleep(1);
	}

	writeToFile("picture-wallpaper.jpg");

	// processGetCommand(sock_id, "picture-wallpaper.jpg");
}

void main_snd(int sock_id) {
	// sharedFilePath += CLIENT_ID;	
	processCreateTrackerCommand(sock_id);	

	for(int i = 0; i < 4; i++) {
		calculateChunk(i);
		cout << "I am client_" << CLIENT_ID << ", and I am advertising the following chunk of the file: ";
		cout << ceil((CURRENT_CHUNK_BEGIN / float(TOTAL_FILE_SIZE))*100) << "% to "; 
		cout << ceil((CURRENT_CHUNK_END / float(TOTAL_FILE_SIZE))*100)  << "%" << endl;
		sock_id = setupConnections();
		processUpdateTrackerCommand(sock_id);
		sleep(10);
	}
}

void *run(void *param) {
	struct ThreadParams *reformedParam = (struct ThreadParams *) param;
	processGetCommand(reformedParam->sockid, reformedParam->name, reformedParam->start_byte, reformedParam->end_byte, reformedParam->threadid);
	// pthread_yield();	
	close(reformedParam->sockid);
	pthread_exit(0);
}

void getWorkingDirectory() {
	char cwd[100];	
	stringstream ss;	
	// struct stat st = {0};

	if(getcwd(cwd, sizeof(cwd))==NULL) {
		exit(1);
	}

	sharedFilePath = cwd;
	// sharedFilePath.erase(sharedFilePath.length() - 3, 3);
	sharedFilePath += "/test_clients/client_";
	ss << CLIENT_ID;
	sharedFilePath += ss.str();
	// trackerFilePath 
	// trackerFilePath += "/trackers";
	// if(stat(sharedFilePath.c_str(), &st)) {
		// cout << "mkdir" << endl;
		// mkdir(sharedFilePath.c_str(), 0700);
		// printf("Could not determine filepath: %s\n", sharedFilePath.c_str());
	// }
	sharedFilePath += "/";

	// if(stat(trackerFilePath.c_str(), &st)) {
	// 	mkdir(trackerFilePath.c_str(), 0700);
	// }
	// trackerFilePath += "/";
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
    server_addr.sin_port = htons(3456);// convert to network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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

	FD = opendir(sharedFilePath.c_str());
	if(FD == NULL) {
		cout << "error: "<< errno << endl; exit(1);
	}

	while((in_file = readdir(FD))) {		
		string list_req = "createtracker";
		char msg[101];		
		stringstream ss;
		// ss << configFile.port_num;
		// string port = ss.str();
		// string md5Sum;		
	
		if(strncmp(in_file->d_name, ".", 1) != 0) {
			FullName = (char*) malloc(strlen(sharedFilePath.c_str()) + strlen(in_file->d_name) + 2);
			strcpy(FullName, sharedFilePath.c_str());
			// strcat(FullName, "/");
			strcat(FullName, in_file->d_name);
			cout << FullName << endl;
			stat(FullName, &statbuf);
			free(FullName);
			list_req += " ";
			list_req += in_file->d_name;
			list_req += " ";
			snprintf(buffer, 20, "%d", statbuf.st_size);
			sscanf(buffer, "%d", &TOTAL_FILE_SIZE);					
			list_req += buffer;
			list_req += " ";
			list_req += "description";
			list_req += " ";

			//Calculate the md5 checksum and insert it into tracker file
			// char funct_call[50];	//store function call for file open
			// char md5[100];
			// strcpy(funct_call, "md5sum ");
			// strcat(funct_call, "shared/");					
			// strcat(funct_call, in_file->d_name);	//append file name to function call
			// FILE * pipe;
			// pipe = popen(funct_call, "r");	//call md5 on file
			// fgets(md5, 100, pipe);	//store output from md5sum call
			// md5Sum = strtok(md5, " ");	
			
			// list_req += md5Sum;

			// list_req += " ";
			list_req += IPADDRESS;
			list_req += " ";			
			list_req += PORT;	
			list_req += " ";
			ss << CLIENT_ID;
			list_req += ss.str();

			cout << list_req << endl;
			if((write(sockid, list_req.c_str(), list_req.size())) < 0){//inform the server of the list request
				printf("Send_request failure\n"); exit(0);
			}

		    if((length = read(sockid, &msg, 100) < 0)){// read what server has said
				printf("Read failure\n"); exit(0); 
			}

			msg[100] = '\0';
			list_req="";
			// cout << "Receiving create tracker response: " << msg << endl;
			close(sockid);				
			sockid = setupConnections();	
		}
	}
	
	close(sockid);
}

void calculateChunk(int iteration) {
	int client_chunk_begin;
	int client_chunk_end;

	R1 = TOTAL_FILE_SIZE % 5;
	CHUNK_SIZE = TOTAL_FILE_SIZE / 5;
	// cout << TOTAL_FILE_SIZE << endl;
	// cout << CHUNK_SIZE << endl;
	R2 = CHUNK_SIZE % 4;
	QUARTER_CHUNK_SIZE = CHUNK_SIZE / 4;

	client_chunk_begin = (QUARTER_CHUNK_SIZE * 4) * (CLIENT_ID - 1);
	if(client_chunk_begin != 0)
		client_chunk_begin++;
	client_chunk_end = (QUARTER_CHUNK_SIZE * 4) * CLIENT_ID;
	// if(CLIENT_ID == 5) 
	// 	client_chunk_end += 3;

	CURRENT_CHUNK_BEGIN = client_chunk_begin + (QUARTER_CHUNK_SIZE * iteration);
	if(CURRENT_CHUNK_BEGIN != 0)
		CURRENT_CHUNK_BEGIN++;
	CURRENT_CHUNK_END = client_chunk_begin + (QUARTER_CHUNK_SIZE * (iteration + 1));
	if(iteration == 3 && CLIENT_ID == 5)
		CURRENT_CHUNK_END = TOTAL_FILE_SIZE;
}

void processUpdateTrackerCommand(int sockid) {
	DIR* FD;
	struct dirent* in_file;
	char * FullName;  	
	struct stat statbuf;
	char buffer[20];
	int length;
	stringstream ss;
	// ss << configFile.port_num;
	// string port = ss.str();

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
			ss << CURRENT_CHUNK_BEGIN;
			list_req += ss.str();
			ss.clear();
			ss.str(string());
			list_req += " ";
			ss << CURRENT_CHUNK_END;			
			list_req += ss.str();
			ss.clear();
			ss.str(string());
			list_req += " ";
			list_req += IPADDRESS;
			list_req += " ";
			list_req += PORT;
			list_req += " ";
			ss << CLIENT_ID;
			list_req += ss.str();
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

string processListCommand(int sockid) {
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

	return msg;
}

void processGetCommand(int sockid, string filename, string start_byte, string end_byte, int threadid) {
	// string tfile = requestTrackerFile(sockid, file);
	// TrackerFile tf = parseTrackerFile(tfile);

	// sockid = setupConnections();
	downloadFile(filename, start_byte, end_byte, sockid, threadid);
	// close(sockid);
}

string requestTrackerFile(int sockid, string file) {
	string list_req = "get ";
	char fpath[300];
	char recvBuf[MAX_RECV_LENGTH];
	char messageBody[MAX_RECV_LENGTH];
	int fr_block_size;
	int write_size;
	bool isBody;
	TrackerFile tf;

	list_req += file;
	strcpy(fpath, sharedFilePath.c_str());
	strcat(fpath, file.c_str());
	strcat(fpath, ".track");

	cout << fpath << endl;
	FILE *fr = fopen(fpath, "wb");
	if (fr == NULL) {
		cout << "File cannot be opened" << endl;
		exit(1);
	}	

	cout << list_req << endl;
	if((write(sockid,list_req.c_str(), list_req.size())) < 0){
		printf("Send_request  failure\n"); exit(0);
	}
	// cout << "wrote request" << endl;

	bzero(recvBuf, MAX_RECV_LENGTH);
	bzero(messageBody, MAX_RECV_LENGTH);
	int j = 0;
	while((fr_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0))) {
		// if(fr_block_size!=-1) cout << fr_block_size << endl;
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
			// cout << "for loop" << endl;
		}
		// cout << "while loop" << endl;
		bzero(recvBuf, MAX_RECV_LENGTH);		
	}
	// cout << "done looping" << endl;
	write_size = fwrite(messageBody, sizeof(char), strlen(messageBody), fr);
	// cout << "written" << endl;
	fclose(fr);
	// cout << "closed" << endl;
	close(sockid);
	// cout << "closed" << endl;
	return fpath;
}

TrackerFile parseTrackerFile(string tfile) {
	ifstream in;
	TrackerFile tf;
	PeerInfo pi;
	if(in.good()) {
		in.open(tfile.c_str());
	}

	getline(in, tf.filename);
	getline(in, tf.filesize);
	getline(in, tf.description);
	getline(in, tf.md5);
	while(!in.eof()) {
		getline(in, pi.ip, ':');
		getline(in, pi.port, ':');
		getline(in, pi.start_byte, ':');
		getline(in, pi.end_byte, ':');
		getline(in, pi.timestamp, ':');
		getline(in, pi.client_id);
		if(!in.eof())
			tf.peerlist.push_back(pi);
	}

	return tf;
}

void downloadFile(string filename, string start_byte, string end_byte, int sockid, int threadid) {
	// char fpath[100];
	// char recvBuf[MAX_RECV_LENGTH];
	int fd_block_size = 0;
	// int write_size;
	stringstream ss;
	ss << threadid;
	string download_req = "download " + filename + " " + start_byte + " " + end_byte + " " + ss.str();

	// strcpy(fpath, sharedFilePath.c_str());
	// strcat(fpath, filename.c_str());	
	// FILE *fd = fopen(fpath, "wb");
	// if(fd == NULL) {
		// cout << "File cannot be opened" << endl;
		// exit(1);
	// }

	if((write(sockid, download_req.c_str(), download_req.size())) < 0) {
		printf("Send request failure\n");
	}

	// bzero(recvBuf, MAX_RECV_LENGTH);
	cout << "Downloading..." << endl;

	switch(threadid) {
		case 1:
			while((fd_block_size = recv(sockid, THREAD1_RECVBUF, MAX_RECV_LENGTH, 0))) { THREAD1_RECVSIZE += fd_block_size; }
			break;
		case 2:
			while((fd_block_size = recv(sockid, THREAD2_RECVBUF, MAX_RECV_LENGTH, 0))) { THREAD2_RECVSIZE += fd_block_size; }
			break;
		case 3:
			while((fd_block_size = recv(sockid, THREAD3_RECVBUF, MAX_RECV_LENGTH, 0))) { THREAD3_RECVSIZE += fd_block_size; }
			break;
		case 4:
			while((fd_block_size = recv(sockid, THREAD4_RECVBUF, MAX_RECV_LENGTH, 0))) { THREAD4_RECVSIZE += fd_block_size; }
			break;
		case 5:
			while((fd_block_size = recv(sockid, THREAD5_RECVBUF, MAX_RECV_LENGTH, 0))) { THREAD5_RECVSIZE += fd_block_size; }
			break;
	}

	// while((fd_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0))) {
	// 	// write_size = fwrite(recvBuf, sizeof(char), fd_block_size, fd);
	// 	// bzero(recvBuf, MAX_RECV_LENGTH);
	// }

	// fseek(fd, 0, SEEK_END);
	// cout << "Received " << ftell(fd) << " bytes..." << endl;
	// rewind(fd);
	// fclose(fd);
	// writeToFile("picture-wallpaper.jpg");
}

void writeToFile(string filename) {
	char fpath[300];
	strcpy(fpath, sharedFilePath.c_str());
	strcat(fpath, filename.c_str());	

	FILE *fd = fopen(fpath, "wb");
	if(fd == NULL) {
		cout << "File cannot be opened" << endl;
		exit(1);
	}

	fwrite(THREAD1_RECVBUF, sizeof(char), THREAD1_RECVSIZE,fd);
	fwrite(THREAD2_RECVBUF, sizeof(char), THREAD2_RECVSIZE,fd);
	fwrite(THREAD3_RECVBUF, sizeof(char), THREAD3_RECVSIZE,fd);
	fwrite(THREAD4_RECVBUF, sizeof(char), THREAD4_RECVSIZE,fd);
	fwrite(THREAD5_RECVBUF, sizeof(char), THREAD5_RECVSIZE,fd);

	fseek(fd, 0, SEEK_END);
	cout << "Received " << ftell(fd) << " bytes..." << endl;
	rewind(fd);
	fclose(fd);
}
