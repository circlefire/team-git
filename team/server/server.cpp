#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <WinSock2.h>
#include <stdlib.h>
#include <iostream>
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#define MAX_SIZE 1024
#define MAX_CLIENT 3

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;

const string server = "tcp://127.0.0.1:3306"; // 데이터베이스 주소
const string username = "user"; // 데이터베이스 사용자
const string password = "1234"; // 데이터베이스 접속 비밀번호

struct SOCKET_INFO {
	SOCKET sck;
	string user;
};

std::vector<SOCKET_INFO> sck_list;
SOCKET_INFO server_sock;
int client_count = 0;

// 1. 소켓 초기화
// socket(), bind(), listen();
// 소켓을 생성하고, 주소를 묶어주고, 활성화 -> 대기상태
void server_init();

// 2. 클라이언트 추가
// accept(), recv() 
// 연결을 설정하고 클라이언트가 전송한 닉네임을 받음
void add_client();

// 3. 클라이언트에 msg 보내기
// send()
void send_msg(const char* msg);

// 4. 클라이언트에게 채팅 내용을 받음
// 퇴장했다면 퇴장했습니다 공지 띄워줌
void recv_msg(int idx);

// 5. 소켓 닫아줌
void del_client(int idx);

int main() {
	WSADATA wsa;

	int code = WSAStartup(MAKEWORD(2, 2), &wsa);
	// winsock version 2.2 사용
	// winsock 초기화 하는 함수
	// 실행 성공하면 0반환, 실패하면 0이외의 값 반환

	if (!code) {
		server_init();
		std::thread th1[MAX_CLIENT];
		for (int i = 0; i < MAX_CLIENT; i++) {
			th1[i] = std::thread(add_client);
		}

		while (1) {
			string text, msg = "";
			std::getline(cin, text);
			const char* buf = text.c_str();

			msg = server_sock.user + " : " + buf;

			send_msg(msg.c_str());
		}
		for (int i = 0; i < MAX_CLIENT; i++) {
			th1[i].join();
		}
		closesocket(server_sock.sck);
		WSACleanup();
		return 0;
	}
}

/*함수 구현부*/
//1. 소켓 초기화
void server_init() {
	server_sock.sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/*
	socket(int AF, int 통신타입, int protocol)
	- 주소 체계 형식
	- 통신 타입 설정
	- 어떤 프로토콜 사용할지
	*/

	SOCKADDR_IN server_addr = {};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(7777);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//localhost=127.0.0.1

	bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr));
	/*
	bind(SOCKET s, const sockaddr *name, int namelen)
	- socket() 으로 만들어준 소켓
	- 소켓과 연결할 주소 정보를 담고 있는 구조체
	(코드에서는 server_addr)
	- 두 번째 매개변수의 크기
	*/
	listen(server_sock.sck, SOMAXCONN);
	/*	listen(SOCKET s, int backlog)*/
	server_sock.user = "server";
	// 서버가 켜지면 나올 문구
	cout << "Server On!" << endl;
}
//2. 클라이언트 초기화
void add_client() {
	SOCKADDR_IN addr = {};
	int addrsize = sizeof(addr);
	char buf[MAX_SIZE] = {};

	ZeroMemory(&addr, addrsize); // addr 0으로 초기화

	SOCKET_INFO new_client = {};
	new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
	/*
	accept(SOCKET s, sockaddr* addr, &addrsize)
	- socket() 으로 만들어준 소켓
	- client의 주소 정보를 저장할 구조체
	- 2번 매개변수의 크기
	*/
	recv(new_client.sck, buf, MAX_SIZE, 0);
	/*
	recv()
	- accept() 만들어준 소켓(통신을 위한 소켓)
	- 데이터를 받을 버퍼를 받을 변수
	- 두 번째 매개변수의 길이
	- flag
	*/
	new_client.user = string(buf);
	// buf를 string으로 변환해서 담아줌
	//string str = new_client.user.substr(0, new_client.user.find("/"));
	string msg = "[공지] " + new_client.user.substr(0, new_client.user.find("/")) + "님이 입장했습니다.";
	cout << msg << endl;

	sck_list.push_back(new_client);

	std::thread th(recv_msg, client_count);

	client_count++;
	cout << "[공지] 현재 접속자 수: " << client_count << "명" << endl;
	send_msg(msg.c_str());

	th.join();
}
void send_msg(const char* msg) {
	for (int i = 0; i < client_count; i++) {
		send(sck_list[i].sck, msg, MAX_SIZE, 0);
	}
}
void recv_msg(int idx) {
	char buf[MAX_SIZE] = {};
	string msg = "";
	while (1) {
		ZeroMemory(&buf, MAX_SIZE);
		if (recv(sck_list[idx].sck, buf, MAX_SIZE, 0) > 0) {
			//만약 정상적으로 받았다면
			msg = sck_list[idx].user.substr(0, sck_list[idx].user.find("/")) + ':' + buf;
			cout << msg << endl;
			send_msg(msg.c_str());
		}
		else {
			msg = "[공지] " + sck_list[idx].user.substr(0, sck_list[idx].user.find("/")) + "님이 퇴장했습니다.";
			cout << msg << endl;
			send_msg(msg.c_str());
			del_client(idx);
			return;
		}
	}
}
void del_client(int idx) {
	closesocket(sck_list[idx].sck);
	client_count--;
}