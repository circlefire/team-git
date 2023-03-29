#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <WS2tcpip.h>

#include <stdlib.h>
#include <iostream>

#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>
#define MAX_SIZE 1024

using std::cout;
using std::cin;
using std::endl;
using std::string;

const string server = "tcp://127.0.0.1:3306"; // �����ͺ��̽� �ּ�
const string username = "user"; // �����ͺ��̽� �����
const string password = "1234"; // �����ͺ��̽� ���� ��й�ȣ

SOCKET client_sock;
string id_pw, id_pw2;
string id;
string pw;

int chat_recv() {
	char buf[MAX_SIZE] = {};
	string msg;
	while (1) {
		ZeroMemory(&buf, MAX_SIZE);
		if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
			msg = buf;
			string user;
			std::stringstream ss(msg);
			ss >> user;
			if (user != id_pw) cout << buf << endl;
		}
		else {
			cout << "Server Off!" << endl;
			return -1;
		}
	}
}

int main() {
	sql::Driver* driver;
	sql::Connection* con{};
	sql::PreparedStatement* pstmt;
	sql::ResultSet* result;

	WSADATA wsa;
	int mod;
	int code = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (!code) {
		try {
			driver = get_driver_instance();
			con = driver->connect(server, username, password);
			con->setSchema("chat_db");
		}
		catch (sql::SQLException& e) {
			cout << "# ERR: SQLException in " << __FILE__;
			cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
			cout << "# ERR: " << e.what();
			cout << " (MySQL error code: " << e.getErrorCode();
			cout << ", SQLState: " << e.getSQLState() << " )" << endl;
			return -1;
		}
		cout << "ȸ�������� �Ϸ��� 1, �α����� �Ϸ��� 2�� �Է��ϼ���: ";
		cin >> mod;

		if (mod == 1) {
			while (1) {
				cout << "ID�� �Է��ϼ���.";
				cin >> id;
				// MySQL DB���� id�� password �� ���ϱ�
				pstmt = con->prepareStatement("SELECT count(user_id) as cnt FROM chat_table WHERE user_id='" + id + "'");
				result = pstmt->executeQuery();
				result->next();
				int count = result->getInt("cnt");
				if (count > 0) {
					cout << "�̹� �����ϴ� ���̵��Դϴ�" << endl;
				}
				else {
					cout << "PW�� �Է��ϼ���.";
					cin >> pw;
					pstmt = con->prepareStatement("INSERT INTO chat_table(user_id, user_pw) VALUES(?,?)");
					pstmt->setString(1, id);
					pstmt->setString(2, pw);
					pstmt->execute();
					cout << "ID ������ �Ϸ�Ǿ����ϴ�." << endl;
					return 0;
				}
			}
		}
		else {
			while (1) {
				cout << "����� ID�� PW�� �Է��ϼ���(/�� ����) >> ";
				cin >> id_pw;
				string id = id_pw.substr(0, id_pw.find("/")); // '/'�� �տ� �ִ� �ܾ id�� ����
				string pw = id_pw.substr(id_pw.find("/") + 1); // '/'�� �ڿ� �ִ´ܾ pw�� ����

				// MySQL DB���� id�� password �� ���ϱ�
				pstmt = con->prepareStatement("SELECT COUNT(*) as count FROM chat_table WHERE user_id='" + id + "' AND user_pw='" + pw + "'");
				result = pstmt->executeQuery();
				result->next();
				int count = result->getInt("count");

				// �α��� ��� ����ϱ�
				if (count > 0) {
					cout << "�α��� ����!" << endl;
					break;
				}
				else {
					cout << "���̵� �Ǵ� ��й�ȣ�� ��ġ���� �ʽ��ϴ�." << endl;
				}
			}
		}
		client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

		SOCKADDR_IN client_addr = {};
		client_addr.sin_family = AF_INET;
		client_addr.sin_port = htons(7777);
		InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);

		while (1) {
			if (!connect(client_sock, (SOCKADDR*)&client_addr, sizeof(client_addr))) {
				cout << "Server Connect" << endl;
				send(client_sock, id_pw.c_str(), id_pw.length(), 0);
				break;
			}
			cout << "connecting..." << endl;
		}
		std::thread th2(chat_recv);

		while (1) {
			string text;
			std::getline(cin, text);
			const char* buffer = text.c_str();
			send(client_sock, buffer, strlen(buffer), 0);
		}
		th2.join();
		closesocket(client_sock);
	}
	WSACleanup();
	return 0;
}