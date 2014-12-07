#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <fcntl.h>
#include <iostream>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <vector>
using namespace std;

string trackerFilePath;
string sharedFilePath;
int MAX_SEND_LENGTH = 2000;
int IP = 0;
int PORT = 0;
int numThreads = 0;
time_t timer;
struct tm newyear;
pthread_mutex_t fileWriteLock;
pthread_mutexattr_t attr;

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

struct DownloadReq {
	string filename;
	int start_byte;
	int end_byte;
	string client_id;
};

vector<TrackerFile> trackerFiles;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;

void getWorkingDirectory();

void loadTrackerFiles();

void setupTimer();

int setupSocketConnections();

void outputNetworkInfo(int sockid);

void listenForConnections(int sockid);

void peer_handler(int sock_child);

void handle_createtracker_req(int sock_child, char* read_msg);

string createTrackerFile(char* read_msg);

TrackerFile parseCreateTrackerMsg(char* read_msg);

void handle_list_req(int sock_child);

void handle_download(int sock_child, char* read_msg);

DownloadReq parseDownloadRequest(char* read_msg);

string parseGetRequest(char* read_msg);

void handle_get_req(int sock_child, char* read_msg);

void handle_updatetracker_req(int sock_child, char* read_msg);

string updateTrackerFile(char* read_msg);

PeerInfo parseUpdateTrackerMsg(char* read_msg);

bool writeTrackerFile(TrackerFile &tf);

bool appendTrackerFile(TrackerFile &tf);

int main(int argc, char* argv[]){
	int sockid;

	if(strcmp(argv[1],"localhost") == 0) 
		IP = INADDR_LOOPBACK;
	else
		IP = atoi(argv[1]);
	PORT = atoi(argv[2]);
	numThreads = atoi(argv[3]);

	system("clear");

	setupTimer();

	getWorkingDirectory();

	sockid = setupSocketConnections();	

	// outputNetworkInfo(sockid);


	while(true){
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&fileWriteLock, &attr);
		// cout << endl;
		// cout << "Tracker server listening for incoming connections..." << endl;
		
		// sockid = setupSocketConnections();

		if (listen(sockid, 100) < 0){ //(parent) process listens at sockid and check error
			printf(" Tracker SERVER CANNOT LISTEN %d\n", errno); exit(0);
		}

		listenForConnections(sockid);
		// cout << "listened" << endl;
	pthread_mutex_destroy(&fileWriteLock);
	pthread_mutexattr_destroy(&attr);
	}         
}
    
void getWorkingDirectory() {
	char cwd[100];
	struct stat st = {0};

	if(getcwd(cwd, sizeof(cwd))==NULL) {
		exit(1);
	}

	trackerFilePath = cwd;
	sharedFilePath = cwd;
	sharedFilePath += "/test_clients/client_";
	trackerFilePath += "/test_server/";
}

void loadTrackerFiles() {
	DIR* FD;
	struct dirent* in_file;
	struct stat filestat;

	if(NULL == (FD = opendir(trackerFilePath.c_str()))) {
		cout << "error" << endl;
	}

	while((in_file = readdir(FD))) {
		string temp_path = trackerFilePath + "/" + in_file->d_name;
		if(stat(temp_path.c_str(), &filestat)) continue;
		if(in_file->d_name[0] != '.') {
			TrackerFile tf;
			PeerInfo pi;
			ifstream in;	

			if(in.good()) {
				in.open(temp_path.c_str());
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
				trackerFiles.push_back(tf);
				in.close();
			}			
		}
	}
}

void setupTimer() {
	newyear = *localtime(&timer);
	newyear.tm_mday = 1;
	newyear.tm_mon = 0;
	newyear.tm_hour = 0;
	newyear.tm_min = 0;
	newyear.tm_sec = 0;
}

int setupSocketConnections() {
   int sockid;

   if ((sockid = socket(AF_INET,SOCK_STREAM,0)) < 0){//create socket connection oriented
	   printf("socket cannot be created \n"); exit(0); 
   }
    
   //socket created at this stage
   //now associate the socket with local port to allow listening incoming connections
   server_addr.sin_family = AF_INET;// assign address family
   server_addr.sin_port = htons(PORT);//change server port to NETWORK BYTE ORDER
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   

   if (bind(sockid ,(struct sockaddr *) &server_addr, sizeof(server_addr)) ==-1){//bind and check error
	   printf("bind  failure\n"); exit(0); 
   }  

   return sockid;                                      
}

void outputNetworkInfo(int sockid) {
	int family, s;
	struct ifaddrs *ifa, *ifaddr;
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	char host[NI_MAXHOST];

	bzero(host, NI_MAXHOST);

	cout << "Tracker server network info..." << endl;

	if(getsockname(sockid, (struct sockaddr *)&sin, &len) == -1) {
		cout << "Error getting port from socket" << endl;
	} else {
		cout << "Port: " << ntohs(sin.sin_port) << endl;
	}

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(0);
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 

	getifaddrs(&ifaddr);	
	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		family = ifa->ifa_addr->sa_family;
		s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, 100, NULL, 0, NI_NUMERICHOST);
		int count = 0;
		
		for(int i = 0; i < strlen(host); i++) {
			if(host[i] == '.') {
				count++;
			}
		}
		if(count == 3) {
			cout << "IP: " << host << endl;			
		}
	}

	freeifaddrs(ifaddr);
	freeifaddrs(ifa);
}

void listenForConnections(int sockid) {
	int sockchild;
	pid_t pid;

	if ((sockchild = accept(sockid ,(struct sockaddr *) &client_addr, (socklen_t*) &client_addr ))==-1){ /* accept connection and create a socket descriptor for actual work */
		   printf("Tracker Cannot accept...\n"); exit(0); 
	}

	// while(numThreads > 0) {

		if ((pid=fork())==0) {//New child process will serve the requester client. separate child will serve separate client
		   numThreads--;
		   close(sockid);   //child does not need listener
		   peer_handler(sockchild);//child is serving the client.
		   // trackerFiles.clear();
		   // cout << "peer handled" << endl;	   
		   close(sockchild);// printf("\n 1. closed");	   
		   numThreads++;
		   // cout << "about to exit" << endl;
		   // kill(getpid(), SIGKILL);
		   exit(0);         // kill the process. child process all done with work
	    }	
	close(sockchild);  // parent all done with client, only child will communicate with that client from now
// }
	// cout << "leaving" << endl;
	// close(sockchild);  // parent all done with client, only child will communicate with that client from now
}

void peer_handler(int sock_child){ // function for file transfer. child process will call this function     
    //start handiling client request	
	int length = -1;
	char read_msg[301];
	// cout << "about to read" << endl;
	while(length < 0) {
		bzero(read_msg, 301);
		length=read(sock_child, &read_msg, 300);
		cout << "length: " << length << endl;	
	}
	// cout << read_msg << endl;
	read_msg[length+1]='\0';
	// cout << "Client is: " << read_msg[length] << endl;
	pthread_mutex_lock(&fileWriteLock);
	loadTrackerFiles();

	cout << "Message received: " << read_msg << endl;

	if((!strcmp(read_msg, "REQ LIST"))||(!strcmp(read_msg, "req list"))||(!strcmp(read_msg, "<REQ LIST>"))||(!strcmp(read_msg, "<REQ LIST>\n"))){//list command received
		handle_list_req(sock_child);// handle list request
	}
	else if((strstr(read_msg,"get")!=NULL)||(strstr(read_msg,"GET")!=NULL)){// get command received
		handle_get_req(sock_child, read_msg);
	}
	else if((strstr(read_msg,"createtracker")!=NULL)||(strstr(read_msg,"Createtracker")!=NULL)||(strstr(read_msg,"CREATETRACKER")!=NULL)){// get command received
		handle_createtracker_req(sock_child, read_msg);		
	}
	else if((strstr(read_msg,"updatetracker")!=NULL)||(strstr(read_msg,"Updatetracker")!=NULL)||(strstr(read_msg,"UPDATETRACKER")!=NULL)){// get command received
		handle_updatetracker_req(sock_child, read_msg);	
	} else if(strstr(read_msg, "download") != NULL) {
		handle_download(sock_child, read_msg);
		// cout << "download handled" << endl;
	}

	trackerFiles.clear();
	pthread_mutex_unlock(&fileWriteLock);
	// trackerFiles.clear();
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

	// pthread_mutex_lock(&fileWriteLock);
	fp = fopen((trackerFilePath + "/" + tf.filename + ".track").c_str(), "r");

	if(fp) {
		for(int i = 0; i < trackerFiles.size(); i++) {
			if(trackerFiles[i].filename == tf.filename) {
				for(int j = 0; j < trackerFiles[i].peerlist.size(); j++) {
					if(trackerFiles[i].peerlist[j].client_id == tf.peerlist[0].client_id) {
						return "<createtracker ferr>";
					}
				}
			}
		}

		cout << "writing tracker from client " << tf.peerlist[0].client_id << endl;
		fp = fopen((trackerFilePath + "/" + tf.filename + ".track").c_str(), "a");
		if(fputs((tf.peerlist[0].ip.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].port.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].start_byte.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].end_byte.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].timestamp.c_str()), fp) == EOF) { return err;}
		fputs(":", fp);
		if(fputs((tf.peerlist[0].client_id.c_str()), fp) == EOF) { return err;}
		fputs("\n", fp);
	} else {
		if(!writeTrackerFile(tf)) {
			return "<createtracker fail>";
		}
	}	

	if(fp != NULL) {
		fclose(fp);	
	}
	// pthread_mutex_unlock(&fileWriteLock);

	trackerFiles.push_back(tf);
	return "<createtracker succ>";
}

TrackerFile parseCreateTrackerMsg(char* read_msg) {
	char* msg = read_msg;
	char time[100];
	TrackerFile tf;
	PeerInfo pi;

	strtok(msg, " ");
	tf.filename = strtok(NULL, " ");
	tf.filesize = strtok(NULL, " ");
	tf.description = strtok(NULL, " ");
	// tf.md5 = strtok(NULL, " ");
	pi.ip = strtok(NULL, " ");
	pi.port = strtok(NULL, " ");
	pi.start_byte = "0";
	pi.end_byte = tf.filesize;
	sprintf(time, "%.f", difftime(timer, mktime(&newyear))); 
	pi.timestamp = time;
	pi.client_id = strtok(NULL, " ");
	tf.peerlist.push_back(pi);

	return tf;
}

void handle_list_req(int sock_child) {
	string msg = "<REP LIST ";
	stringstream ss;
	ss << trackerFiles.size();
	msg = msg + ss.str();
	ss.clear();
	ss.str(string());
	msg = msg + ">\n";

	for(int i = 0; i < trackerFiles.size(); i++) {
		msg = msg + "<";
		ss << (i+1);
		msg += ss.str();
		ss.clear();
		ss.str(string());
		msg += " ";
		msg += trackerFiles[i].filename;
		msg += " ";
		msg += trackerFiles[i].filesize;
		msg += " ";
		msg += trackerFiles[i].description;
		msg += " ";
		msg += trackerFiles[i].md5;
		msg += " ";
		for(int j = 0; j < trackerFiles[i].peerlist.size(); j++) {			
			msg += trackerFiles[i].peerlist[j].ip;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].port;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].start_byte;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].end_byte;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].timestamp;
			msg += " ";
			msg += trackerFiles[i].peerlist[j].client_id;
			msg += ">\n";
		}
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
	char filePathBuf[300];
	int fileBlockSize;

	for(int i = 0; i < trackerFiles.size(); i++) {
		if(trackerFiles[i].filename == filename) {
			tf = trackerFiles[i];
		}
	}

	strcpy(filePathBuf, trackerFilePath.c_str());
	strcat(filePathBuf, filename.c_str());
	strcat(filePathBuf, ".track");
	// pthread_mutex_lock(&fileWriteLock);
	FILE *fs = fopen(filePathBuf, "rb");
	if(fs == 0) {
		cout << "error opening file " << errno << endl;
	}

	strcpy(sendBuf, "<REP GET BEGIN>\n");
	if(send(sock_child, sendBuf, strlen(sendBuf), 0) < 0) {
		cout << "Error sending GET response header" << endl;
	}
	bzero(sendBuf, MAX_SEND_LENGTH);
	// cout << "Sending tracker file to peer..." << endl;
	while((fileBlockSize = fread(sendBuf, sizeof(char), MAX_SEND_LENGTH, fs))) {
		// cout << "Sending " << fileBlockSize << " bytes of tracker file" << endl;
		if(send(sock_child, sendBuf, fileBlockSize, 0) < 0) {
			cout << "Error sending tracker file" << endl;
		}
		bzero(sendBuf, MAX_SEND_LENGTH);
	}
	strcpy(sendBuf, "\n<REP GET END ");
	strcat(sendBuf, tf.md5.c_str());
	strcat(sendBuf, ">");

	if(send(sock_child, sendBuf, strlen(sendBuf), 0) < 0) {
		cout << "Error sending GET response footer" << endl;
	}

	fclose(fs);	
	// pthread_mutex_unlock(&fileWriteLock);
}

string parseGetRequest(char* read_msg) {
	char* msg = read_msg;

	strtok(msg, " ");	
	return strtok(NULL, " ");
}

void handle_download(int sock_child, char* read_msg) {
	// strtok(read_msg, " ");
	// string filename = strtok(NULL, " ");
	DownloadReq dr = parseDownloadRequest(read_msg);
	char sendBuf[MAX_SEND_LENGTH];
	char filePathBuf[300];
	int fileBlockSize;
	int bytes_sent = 0;

	sharedFilePath += dr.client_id;
	sharedFilePath += "/";
	strcpy(filePathBuf, sharedFilePath.c_str());
	strcat(filePathBuf, dr.filename.c_str());
	FILE *fs = fopen(filePathBuf, "r+b");
	cout << filePathBuf << endl;
	if(fs == NULL) {
		cout << "error opening file" << endl; exit(1);
	}

	bzero(sendBuf, MAX_SEND_LENGTH);
	fseek(fs, dr.start_byte, SEEK_SET);
	cout << "Sending " << (dr.end_byte - dr.start_byte) + 1 << " bytes..." << endl;
	// rewind(fs);
	fileBlockSize = fread(sendBuf, sizeof(char), (dr.end_byte - dr.start_byte) + 1, fs);
	if(fileBlockSize > 1786)
		cout << "start " << dr.start_byte << " end " << dr.end_byte << endl;	
	// while((fileBlockSize = fread(sendBuf, sizeof(char), dr.end_byte - dr.start_byte, fs))) {
	// if(dr.client_id == "5")
		// cout << "read bytes " << fileBlockSize << endl;
	while(bytes_sent < fileBlockSize) {
		// if(dr.client_id == "5")
			// cout << "bytes sent " << bytes_sent << endl;
		if((bytes_sent += send(sock_child, sendBuf, fileBlockSize, 0)) < 0) {
			cout << "Error sending requested file" << endl; exit(0);
		}
		// cout << bytes_sent << endl;
	}
		bzero(sendBuf, MAX_SEND_LENGTH);

	// }
	fclose(fs);
	// close(sock_child);
	// cout << "closing the file" << endl;
}

DownloadReq parseDownloadRequest(char* read_msg) {
	char* msg = read_msg;
	DownloadReq dr;

	strtok(msg, " ");
	dr.filename = strtok(NULL, " ");
	dr.start_byte = atoi(strtok(NULL, " "));
	dr.end_byte = atoi(strtok(NULL, " "));
	dr.client_id = strtok(NULL, " ");
	return dr;
}

void handle_updatetracker_req(int sock_child, char* read_msg) {
	// pthread_mutex_lock(&fileWriteLock);
	// trackerFiles.clear();
	// loadTrackerFiles();
	string msg = updateTrackerFile(read_msg);
	cout << "Sending response: " << msg << endl;
	if((write(sock_child, msg.c_str(), 100)) < 0){
		printf("Send_request  failure\n"); exit(0);
	}
	// trackerFiles.clear();
	// pthread_mutex_unlock(&fileWriteLock);
}

bool writeTrackerFile(TrackerFile &tf) {
	FILE *fd;
	fd = fopen((trackerFilePath + "/" + tf.filename + ".track").c_str(), "w");
	if(fputs((tf.filename.c_str()), fd) == EOF) { return false;}
	fputs("\n", fd);
	if(fputs((tf.filesize.c_str()), fd) == EOF) { return false;}
	fputs("\n", fd);
	if(fputs((tf.description.c_str()), fd) == EOF) { return false;}
	fputs("\n", fd);
	if(fputs((tf.md5.c_str()), fd) == EOF) { return false;}
	fputs("\n", fd);
	for(int i = 0; i < tf.peerlist.size(); i++) {
		if(fputs((tf.peerlist[i].ip.c_str()), fd) == EOF) { return false;}
		fputs(":", fd);
		if(fputs((tf.peerlist[i].port.c_str()), fd) == EOF) { return false;}
		fputs(":", fd);
		if(fputs(tf.peerlist[i].start_byte.c_str(), fd) == EOF) { return false;}
		cout << "writing: " << tf.peerlist[i].start_byte << endl;
		fputs(":", fd);
		if(fputs(tf.peerlist[i].end_byte.c_str(), fd) == EOF) { return false;}
		cout << "writing: " << tf.peerlist[i].end_byte << endl;
		fputs(":", fd);
		if(fputs(tf.peerlist[i].timestamp.c_str(), fd) == EOF) { return false;}
		fputs(":", fd);
		if(fputs(tf.peerlist[i].client_id.c_str(), fd) == EOF) { return false;}
		fputs("\n", fd);
	}

	if(fd != NULL) {
		fclose(fd);
	}

	return true;
}

bool appendTrackerFile(PeerInfo &pi) {
	FILE *fd;
	fd = fopen((trackerFilePath + "/picture-wallpaper.jpg.track").c_str(), "a");
	// for(int i = 0; i < tf.peerlist.size(); i++) {
		if(fputs((pi.ip.c_str()), fd) == EOF) { return false;}
		fputs(":", fd);
		if(fputs((pi.port.c_str()), fd) == EOF) { return false;}
		fputs(":", fd);
		if(fputs(pi.start_byte.c_str(), fd) == EOF) { return false;}
		cout << "writing: " << pi.start_byte << endl;
		fputs(":", fd);
		if(fputs(pi.end_byte.c_str(), fd) == EOF) { return false;}
		cout << "writing: " << pi.end_byte << endl;
		fputs(":", fd);
		if(fputs(pi.timestamp.c_str(), fd) == EOF) { return false;}
		fputs(":", fd);
		if(fputs(pi.client_id.c_str(), fd) == EOF) { return false;}
		fputs("\n", fd);
	// }

	if(fd != NULL) {
		fclose(fd);
	}

	return true;
}

string updateTrackerFile(char* read_msg) {	
	cout << "updating tracker file with message: " << read_msg << endl;
	char buff[100];
	strcpy(buff, read_msg);
	string result = "<updatetracker ";
	strtok(buff, " ");
	result += strtok(NULL, " ");
	// loadTrackerFiles();
	PeerInfo pi = parseUpdateTrackerMsg(read_msg);

	// if(idx != -1) {
		// pthread_mutex_lock(&fileWriteLock);
		if(appendTrackerFile(pi)) {
			// pthread_mutex_unlock(&fileWriteLock);
			result += " succ>";
			return result;
		} else {
			// pthread_mutex_unlock(&fileWriteLock);
			result += " fail>";
			return result;
		}
	// }

	result += " ferr>";
	return result;
}

PeerInfo parseUpdateTrackerMsg(char* read_msg) {
	char* msg = read_msg;
	char timeString[100];
	string filename;
	PeerInfo pi;

	strtok(msg, " ");
	filename = strtok(NULL, " ");
	pi.start_byte = strtok(NULL, " ");
	pi.end_byte = strtok(NULL, " ");
	pi.ip = strtok(NULL, " ");
	pi.port = strtok(NULL, " ");
	pi.client_id = strtok(NULL, " ");
	time(&timer);
	sprintf(timeString, "%.f", difftime(timer, mktime(&newyear))); 
	pi.timestamp = timeString;
	// for(int i = 0; i < trackerFiles.size(); i++) {	
	// 	if(trackerFiles[i].filename == filename) {
			
	// 		// cout << pi.start_byte << endl;
	// 		// cout << pi.end_byte << endl;
	// 		for(int j = 0; j < trackerFiles[i].peerlist.size(); j++) {
	// 			// cout << pi.client_id << " = " << trackerFiles[i].peerlist[j].client_id << endl;
	// 			// cout << trackerFiles[i].peerlist.size() << endl;
	// 			if(pi.client_id == trackerFiles[i].peerlist[j].client_id) {
	// 				trackerFiles[i].peerlist[j].start_byte = pi.start_byte;
	// 				trackerFiles[i].peerlist[j].end_byte = pi.end_byte;
	// 				trackerFiles[i].peerlist[j].ip = pi.ip;
	// 				trackerFiles[i].peerlist[j].port = pi.port;
					
					// trackerFiles[i].peerlist[j].timestamp = timeString;	
					// trackerFiles[i].peerlist[j].client_id = pi.client_id;
	// 				return trackerFiles[i].peerlist[j];
	// 			} 
	// 			// else {
	// 			// 	return -1;
	// 			// }

	// 		}			
	// 	}
	// }
	return pi;
}
