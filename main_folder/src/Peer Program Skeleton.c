#include <algorithm>
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
pthread_mutex_t dwnld_lock;
pthread_mutex_t connection_lock;

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
	bool isNULL;
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

int setupConnections();

string requestTrackerFile(int sockid, string file);

TrackerFile parseTrackerFile(string tfile, int prev_byte_1, int prev_byte_2, int prev_byte_3, int prev_byte_4, int prev_byte_5);

void downloadFile(string filename, string start_byte, string end_byte, int sockid, int threadid);

void processCreateTrackerCommand(int sockid);

void processUpdateTrackerCommand(int sockid);

string processListCommand(int sockid);

void processGetCommand(int sockid, string filename, string start_byte, string end_byte, int threadid);

void main_rcv();

void main_snd();

void calculateChunk(int iteration);

void writeToFile(string filename);

void *run(void *);

int pthread_yield(void);

bool sort_func(PeerInfo p1, PeerInfo p2) { return atoi(p1.client_id.c_str()) < atoi(p2.client_id.c_str()); }

int main(int argc, char *argv[]){	
	CLIENT_ID = atoi(argv[2]);
	getWorkingDirectory();

	if(strcmp(argv[1], "rcv") == 0)
		main_rcv();
	else if(strcmp(argv[1], "snd") == 0)
		main_snd();
	else {
		printf("No such client type.\n"); 
		exit(1);
	}
    
    return 0;
}

void main_rcv() {
	int sock_id;
	pthread_t t1, t2, t3, t4, t5;
	int prev_byte_1 = 0;
	int prev_byte_2 = 7144;
	int prev_byte_3 = 14288;
	int prev_byte_4 = 21432;
	int prev_byte_5 = 28576;
	int j = 1;
	ThreadParams p[5];
	string msg = "";
	while(msg.find("picture-wallpaper.jpg") == string::npos) {
		sock_id = setupConnections();
		msg = processListCommand(sock_id);	
		sleep(5);
	}	

	bzero(THREAD1_RECVBUF, 10000);
	bzero(THREAD2_RECVBUF, 10000);
	bzero(THREAD3_RECVBUF, 10000);
	bzero(THREAD4_RECVBUF, 10000);
	bzero(THREAD5_RECVBUF, 10000);

	pthread_mutex_init(&dwnld_lock, NULL);
	pthread_mutex_init(&connection_lock, NULL);

	while(j < 5) {
		// cout << "looping" << endl;		
		sock_id = setupConnections();		
 		string tfile = requestTrackerFile(sock_id, "picture-wallpaper.jpg");
		TrackerFile tf = parseTrackerFile(tfile, prev_byte_1, prev_byte_2, prev_byte_3, prev_byte_4, prev_byte_5);
		if(tf.isNULL == true) {
			// cout << "continuing" << endl;
			usleep(500000);
			continue;			
		}
		// cout << "Prev byte 1: " << prev_byte_1 << " new byte: " << tf.peerlist[0].end_byte << endl; 
		// cout << "Prev byte 2: " << prev_byte_2 << " new byte: " << tf.peerlist[1].end_byte << endl; 
		// cout << "Prev byte 3: " << prev_byte_3 << " new byte: " << tf.peerlist[2].end_byte << endl; 
		// cout << "Prev byte 4: " << prev_byte_4 << " new byte: " << tf.peerlist[3].end_byte << endl; 
		// cout << "Prev byte 5: " << prev_byte_5 << " new byte: " << tf.peerlist[4].end_byte << endl;
		// cout << tf.peerlist.size() << endl;
		// cout << "new byte1: " << tf.peerlist[0]		
		if(prev_byte_1 != atoi(tf.peerlist[0].end_byte.c_str()) &&
			prev_byte_2 != atoi(tf.peerlist[1].end_byte.c_str()) &&
			prev_byte_3 != atoi(tf.peerlist[2].end_byte.c_str()) &&
			prev_byte_4 != atoi(tf.peerlist[3].end_byte.c_str()) &&
			prev_byte_5 != atoi(tf.peerlist[4].end_byte.c_str())) {

			prev_byte_1 = atoi(tf.peerlist[0].end_byte.c_str()) + 1;
			prev_byte_2 = atoi(tf.peerlist[1].end_byte.c_str()) + 1;
			prev_byte_3 = atoi(tf.peerlist[2].end_byte.c_str()) + 1;
			prev_byte_4 = atoi(tf.peerlist[3].end_byte.c_str()) + 1;
			prev_byte_5 = atoi(tf.peerlist[4].end_byte.c_str()) + 1;

			if(CLIENT_ID == 6) {
				cout << "CHUNK " << j << ":" << endl; 
			}
			for(int i = 0; i < 5; i++) {
				p[i].name = tf.filename;
				p[i].start_byte = tf.peerlist[i].start_byte;
				p[i].end_byte = tf.peerlist[i].end_byte;				
				if(CLIENT_ID == 6) {

					cout << "Client " << CLIENT_ID << " thread " << i+1 << " peer " << i << " start byte " << p[i].start_byte << endl;				
					cout << "Client " << CLIENT_ID << " thread " << i+1 << " peer " << i << " end byte " << p[i].end_byte << endl;								
				}

				p[i].sockid = setupConnections();
				p[i].threadid = i+1;
			}
			if(CLIENT_ID == 6)
				cout << "*******************************" << endl << endl;
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
			}			
			j++;
		}
		// usleep(500000);
		sleep(1);		
	}

	pthread_mutex_destroy(&dwnld_lock);	
	pthread_mutex_destroy(&connection_lock);						
	writeToFile("picture-wallpaper.jpg");
}

void main_snd() {
	int sock_id = setupConnections();
	processCreateTrackerCommand(sock_id);	

	for(int i = 0; i < 4; i++) {
		calculateChunk(i);
		cout << "I am client_" << CLIENT_ID << ", and I am advertising the following chunk of the file: ";
		cout << ceil((CURRENT_CHUNK_BEGIN / float(TOTAL_FILE_SIZE))*100) + 1 << "% to "; 
		cout << ceil((CURRENT_CHUNK_END / float(TOTAL_FILE_SIZE))*100)  << "%" << endl;
		sock_id = setupConnections();
		processUpdateTrackerCommand(sock_id);
		sleep(10);
	}
}

void *run(void *param) {
	struct ThreadParams *reformedParam = (struct ThreadParams *) param;
	processGetCommand(reformedParam->sockid, reformedParam->name, reformedParam->start_byte, reformedParam->end_byte, reformedParam->threadid);
	close(reformedParam->sockid);
	pthread_exit(0);
}

void getWorkingDirectory() {
	char cwd[100];	
	stringstream ss;	

	if(getcwd(cwd, sizeof(cwd))==NULL) {
		exit(1);
	}

	sharedFilePath = cwd;
	sharedFilePath += "/test_clients/client_";
	ss << CLIENT_ID;
	sharedFilePath += ss.str();
	sharedFilePath += "/";
}

int setupConnections() {
	int sockid;
	bool successful = false;
	struct sockaddr_in server_addr;

	if ((sockid = socket(AF_INET,SOCK_STREAM,0))==-1){//create socket
		printf("socket cannot be created\n"); exit(0);
	}

    server_addr.sin_family = AF_INET;//host byte order
    server_addr.sin_port = htons(3456);// convert to network byte order
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    pthread_mutex_lock(&connection_lock);
    while(!successful) {
	    if (connect(sockid ,(struct sockaddr *) &server_addr, sizeof(struct sockaddr))==-1){//connect and error check
			cout << errno << endl; printf("Cannot connect to server\n");
			if(errno == 22)
				exit(1);
		} else {
			successful = true;
		}		
	}
	pthread_mutex_unlock(&connection_lock);

	return sockid;
}

void processCreateTrackerCommand(int sockid) {	
	DIR* FD;
	struct dirent* in_file;
	char * FullName;  	
	struct stat statbuf;
	char buffer[20];
	int length;
	bool createSuccessful = false;

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
			strcat(FullName, in_file->d_name);
			stat(FullName, &statbuf);
			free(FullName);
			list_req += " ";
			list_req += in_file->d_name;
			list_req += " ";
			snprintf(buffer, 20, "%d", (int)statbuf.st_size);
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

			while(!createSuccessful) {
				if((write(sockid, list_req.c_str(), list_req.size())) < 0){//inform the server of the list request
					printf("Send_request failure\n"); exit(0);
				} else {
					createSuccessful = true;
				}
			}

		    if((length = recv(sockid, &msg, 100, 0) < 0)){// read what server has said
				printf("Read failure (create tracker) with error %d\n", errno); exit(0); 
			}

			cout << msg << endl;
			msg[100] = '\0';
			list_req="";
			close(sockid);				
			sockid = setupConnections();	
		}
	}
	
	close(sockid);
}

void calculateChunk(int iteration) {
	int client_chunk_begin;

	CHUNK_SIZE = TOTAL_FILE_SIZE / 5;
	QUARTER_CHUNK_SIZE = CHUNK_SIZE / 4;

	client_chunk_begin = (QUARTER_CHUNK_SIZE * 4) * (CLIENT_ID - 1);
	CURRENT_CHUNK_BEGIN = client_chunk_begin + (QUARTER_CHUNK_SIZE * iteration);
	CURRENT_CHUNK_END = client_chunk_begin + (QUARTER_CHUNK_SIZE * (iteration + 1)) - 1;

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
	bool updateSuccessful = false;
	int totalBytesSent = 0;
	int bytesSentThisTime = 0;

	if(NULL == (FD = opendir(sharedFilePath.c_str()))) {
		cout << "error opening file for update" << endl;
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
			snprintf(buffer, 20, "%d", (int)statbuf.st_size);

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
			// cout << CURRENT_CHUNK_BEGIN << endl;
			// cout << CURRENT_CHUNK_END << endl;
			cout << list_req << endl;
			while(!updateSuccessful && totalBytesSent < list_req.size()) {
				bytesSentThisTime = write(sockid, list_req.c_str(), list_req.size());
				totalBytesSent += bytesSentThisTime;
				// cout << "Bytes sent: " << totalBytesSent << " && " << " Bytes desired: " << list_req.size();
				if(bytesSentThisTime < 0){//inform the server of the list request
					printf("Send_request failure\n");	
					updateSuccessful = false;														
				} else {
					updateSuccessful = true;
				}
			}

		    if((length = recv(sockid, &msg, 100, 0) < 0)){// read what server has said
				printf("Read failure (update tracker) with error %d\n", errno); exit(0); 
			}

			cout << msg << endl;
			msg[100] = '\0';
			list_req="";
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
	close(sockid);

	return msg;
}

void processGetCommand(int sockid, string filename, string start_byte, string end_byte, int threadid) {
	downloadFile(filename, start_byte, end_byte, sockid, threadid);
}

string requestTrackerFile(int sockid, string file) {
	string list_req = "get ";
	char fpath[300];
	char recvBuf[MAX_RECV_LENGTH];
	char messageBody[MAX_RECV_LENGTH];
	int fr_block_size;
	int write_size;
	bool isBody;
	FILE *fr;

	list_req += file;
	strcpy(fpath, sharedFilePath.c_str());
	strcat(fpath, file.c_str());
	strcat(fpath, ".track");


	if((write(sockid,list_req.c_str(), list_req.size())) < 0){
		printf("Send_request  failure\n"); exit(0);
	}

	bzero(recvBuf, MAX_RECV_LENGTH);
	bzero(messageBody, MAX_RECV_LENGTH);
	int j = 0;
	while((fr_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0)) > 0) {
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

	if(strlen(messageBody) > 0) {
		fr = fopen(fpath, "wb");

		// cout << "Received message of size: " << messageBody << endl;
		if (fr == NULL) {
			cout << "File cannot be opened" << endl;
			return "";
		}

		write_size = fwrite(messageBody, sizeof(char), strlen(messageBody), fr);
		fclose(fr);
	}

	close(sockid);
	return fpath;
}

TrackerFile parseTrackerFile(string tfile, int prev_byte_1, int prev_byte_2, int prev_byte_3, int prev_byte_4, int prev_byte_5) {
	ifstream in;
	TrackerFile tf;
	PeerInfo pi;
	int peerCount = 0;
	if(in.good()) {
		in.open(tfile.c_str());
	}

	if(in.eof()) {
		tf.isNULL = true; 
		return tf;
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
		// cout << "start: " << pi.start_byte << endl;
		// cout << "end: " << pi.end_byte << endl;
		if(!in.eof() && (
			(atoi(pi.end_byte.c_str()) == prev_byte_1 + 1785) ||
			(atoi(pi.end_byte.c_str()) == prev_byte_2 + 1785) ||
			(atoi(pi.end_byte.c_str()) == prev_byte_3 + 1785) ||
			(atoi(pi.end_byte.c_str()) == prev_byte_4 + 1785) ||
			(atoi(pi.end_byte.c_str()) == (prev_byte_5 == 33934 ? prev_byte_5 + 1804 : prev_byte_5 + 1785)))) {
			if(!((atoi(pi.start_byte.c_str()) == 0) && atoi(pi.end_byte.c_str()) == 35738)) {
				peerCount++;
				tf.peerlist.push_back(pi);
			}
		}
	}

	sort(tf.peerlist.begin(), tf.peerlist.end(), sort_func);
	if(peerCount != 5)
		tf.isNULL = true;
	else
		tf.isNULL = false;

	return tf;
}

void downloadFile(string filename, string start_byte, string end_byte, int sockid, int threadid) {
	char recvBuf[10000];
	int fd_block_size = 0;
	int rcvdThisTime = 0;
	bool downloadInProgress = true;
	stringstream ss;
	ss << threadid;
	string download_req = "download " + filename + " " + start_byte + " " + end_byte + " " + ss.str();	


	pthread_mutex_lock(&dwnld_lock);
	while(downloadInProgress) {

		if((write(sockid, download_req.c_str(), download_req.size())) < 0) {
			printf("Send request failure\n"); 
			continue;
		}
	
		bzero(recvBuf, 10000);
		switch(threadid) {
			case 1:
				while((fd_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0)) > 0) { 
					THREAD1_RECVSIZE += fd_block_size;
					rcvdThisTime += fd_block_size; 
					memcpy(&THREAD1_RECVBUF[THREAD1_RECVSIZE - fd_block_size], &recvBuf, fd_block_size); 
				}			
				if(fd_block_size == 0)
					downloadInProgress = false;//cout << "error downloading, errno " << errno << endl;
				else 
					THREAD1_RECVSIZE -= rcvdThisTime;
				cout << "Client " << CLIENT_ID << " thread " << threadid << " downloaded " << fd_block_size << " bytes." << endl;
				break;
			case 2:
				while((fd_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0)) > 0) { 
					THREAD2_RECVSIZE += fd_block_size;
					rcvdThisTime += fd_block_size;
					memcpy(&THREAD2_RECVBUF[THREAD2_RECVSIZE - fd_block_size], &recvBuf, fd_block_size); 
				}
				if(fd_block_size == 0) 
					downloadInProgress = false;//cout << "error downloading, errno " << errno << endl;
				else
					THREAD2_RECVSIZE -= rcvdThisTime;
				cout << "Client " << CLIENT_ID << " thread " << threadid << " downloaded " << fd_block_size << " bytes." << endl;
				break;
			case 3:
				while((fd_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0)) > 0) { 
					THREAD3_RECVSIZE += fd_block_size; 
					rcvdThisTime += fd_block_size;
					memcpy(&THREAD3_RECVBUF[THREAD3_RECVSIZE - fd_block_size], &recvBuf, fd_block_size); 
				}
				if(fd_block_size == 0) 
					downloadInProgress = false; //cout << "error downloading, errno " << errno << endl;
				else
					THREAD3_RECVSIZE -= rcvdThisTime;
				cout << "Client " << CLIENT_ID << " thread " << threadid << " downloaded " << fd_block_size << " bytes." << endl;
				break;
			case 4:
				while((fd_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0)) > 0) { 
					THREAD4_RECVSIZE += fd_block_size; 
					rcvdThisTime += fd_block_size;
					memcpy(&THREAD4_RECVBUF[THREAD4_RECVSIZE - fd_block_size], &recvBuf, fd_block_size); 
				}
				if(fd_block_size == 0) 
					downloadInProgress = false; //cout << "error downloading, errno " << errno << endl;
				else
					THREAD4_RECVSIZE -= rcvdThisTime;
				cout << "Client " << CLIENT_ID << " thread " << threadid << " downloaded " << fd_block_size << " bytes." << endl;
				break;
			case 5:
				while((fd_block_size = recv(sockid, recvBuf, MAX_RECV_LENGTH, 0)) > 0) { 
					THREAD5_RECVSIZE += fd_block_size; 
					rcvdThisTime += fd_block_size;
					memcpy(&THREAD5_RECVBUF[THREAD5_RECVSIZE - fd_block_size], &recvBuf, fd_block_size); 
				}
				if(fd_block_size == 0) 
					downloadInProgress = false; //cout << "error downloading, errno " << errno << endl;
				else
					THREAD5_RECVSIZE -= rcvdThisTime;
				cout << "Client " << CLIENT_ID << " thread " << threadid << " downloaded " << fd_block_size << " bytes." << endl;
				break;
		}
		close(sockid);
		sockid = setupConnections();
	}
	pthread_mutex_unlock(&dwnld_lock);
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

	fwrite(THREAD1_RECVBUF, sizeof(char), THREAD1_RECVSIZE, fd);
	cout << "1st chunk size " << THREAD1_RECVSIZE << endl;
	fwrite(THREAD2_RECVBUF, sizeof(char), THREAD2_RECVSIZE, fd);
	cout << "2nd chunk size " << THREAD2_RECVSIZE << endl;
	fwrite(THREAD3_RECVBUF, sizeof(char), THREAD3_RECVSIZE, fd);
	cout << "3rd chunk size " << THREAD3_RECVSIZE << endl;
	fwrite(THREAD4_RECVBUF, sizeof(char), THREAD4_RECVSIZE, fd);
	cout << "4th chunk size " << THREAD4_RECVSIZE << endl;
	fwrite(THREAD5_RECVBUF, sizeof(char), THREAD5_RECVSIZE, fd);
	cout << "5th chunk size " << THREAD5_RECVSIZE << endl;

	fseek(fd, 0, SEEK_END);
	cout << "Received " << ftell(fd) << " bytes..." << endl;
	cout << "I am client_" << CLIENT_ID << ", and I received the file correctly!" << endl;
	rewind(fd);
	fclose(fd);
}

///@file
