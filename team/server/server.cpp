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

const string server = "tcp://127.0.0.1:3306"; // �����ͺ��̽� �ּ�
const string username = "user"; // �����ͺ��̽� �����
const string password = "1234"; // �����ͺ��̽� ���� ��й�ȣ

struct SOCKET_INFO {
	SOCKET sck;
	string user;
};

std::vector<SOCKET_INFO> sck_list;
SOCKET_INFO server_sock;
int client_count = 0;

// 1. ���� �ʱ�ȭ
// socket(), bind(), listen();
// ������ �����ϰ�, �ּҸ� �����ְ�, Ȱ��ȭ -> ������
void server_init();

// 2. Ŭ���̾�Ʈ �߰�
// accept(), recv() 
// ������ �����ϰ� Ŭ���̾�Ʈ�� ������ �г����� ����
void add_client();

// 3. Ŭ���̾�Ʈ�� msg ������
// send()
void send_msg(const char* msg);

// 4. Ŭ���̾�Ʈ���� ä�� ������ ����
// �����ߴٸ� �����߽��ϴ� ���� �����
void recv_msg(int idx);

// 5. ���� �ݾ���
void del_client(int idx);

int main() {
	WSADATA wsa;

	int code = WSAStartup(MAKEWORD(2, 2), &wsa);
	// winsock version 2.2 ���
	// winsock �ʱ�ȭ �ϴ� �Լ�
	// ���� �����ϸ� 0��ȯ, �����ϸ� 0�̿��� �� ��ȯ

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

/*�Լ� ������*/
//1. ���� �ʱ�ȭ
void server_init() {
	server_sock.sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	/*
	socket(int AF, int ���Ÿ��, int protocol)
	- �ּ� ü�� ����
	- ��� Ÿ�� ����
	- � �������� �������
	*/

	SOCKADDR_IN server_addr = {};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(7777);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//localhost=127.0.0.1

	bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr));
	/*
	bind(SOCKET s, const sockaddr *name, int namelen)
	- socket() ���� ������� ����
	- ���ϰ� ������ �ּ� ������ ��� �ִ� ����ü
	(�ڵ忡���� server_addr)
	- �� ��° �Ű������� ũ��
	*/
	listen(server_sock.sck, SOMAXCONN);
	/*	listen(SOCKET s, int backlog)*/
	server_sock.user = "server";
	// ������ ������ ���� ����
	cout << "Server On!" << endl;
}
//2. Ŭ���̾�Ʈ �ʱ�ȭ
void add_client() {
	SOCKADDR_IN addr = {};
	int addrsize = sizeof(addr);
	char buf[MAX_SIZE] = {};

	ZeroMemory(&addr, addrsize); // addr 0���� �ʱ�ȭ

	SOCKET_INFO new_client = {};
	new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
	/*
	accept(SOCKET s, sockaddr* addr, &addrsize)
	- socket() ���� ������� ����
	- client�� �ּ� ������ ������ ����ü
	- 2�� �Ű������� ũ��
	*/
	recv(new_client.sck, buf, MAX_SIZE, 0);
	/*
	recv()
	- accept() ������� ����(����� ���� ����)
	- �����͸� ���� ���۸� ���� ����
	- �� ��° �Ű������� ����
	- flag
	*/
	new_client.user = string(buf);
	// buf�� string���� ��ȯ�ؼ� �����
	//string str = new_client.user.substr(0, new_client.user.find("/"));
	string msg = "[����] " + new_client.user.substr(0, new_client.user.find("/")) + "���� �����߽��ϴ�.";
	cout << msg << endl;

	sck_list.push_back(new_client);

	std::thread th(recv_msg, client_count);

	client_count++;
	cout << "[����] ���� ������ ��: " << client_count << "��" << endl;
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
			//���� ���������� �޾Ҵٸ�
			msg = sck_list[idx].user.substr(0, sck_list[idx].user.find("/")) + ':' + buf;
			cout << msg << endl;
			send_msg(msg.c_str());
		}
		else {
			msg = "[����] " + sck_list[idx].user.substr(0, sck_list[idx].user.find("/")) + "���� �����߽��ϴ�.";
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