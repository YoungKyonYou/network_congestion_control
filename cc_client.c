#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<process.h>
#include <conio.h>
#include <winsock2.h>
#define BUF_SIZE 30
#define BLUE 9
#define GREEN 10
#define RED 12
#define YEL 14
#define ORI 15
WSADATA wsaData;
unsigned threadID;
SOCKET sock;
HANDLE hThread;
UINT m_nTimerID;
char message[BUF_SIZE], m[10];
int strLen;
static HANDLE sem;
SOCKADDR_IN servAdr;

int flag = 0, packNum = 1, packCnt = 1, packDup = -1, packTime = -1, window = 1, threshold = 0x7fffffff;
int rcvPackNum = 0, rcvPackCnt = 0, timeFlag = 0, timeFlag2 = 0;;
int mainFlag = 0;

void ErrorHandling(char* message);
void textcolor(int color_number);
void conn();
void print(int pn);
char scan();
void CALLBACK TimeProc(UINT m_nTimerID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD d2);
unsigned WINAPI subMain(void* arg);

int main(int argc, char* argv[])
{
	//conn은 UDP의 연결을 설정하는 함수이다.
	conn();
	//클라이언트로부터 들어오는 ACK 등의 패킷을 수신하기 위해서 쓰레드를 생성한다
	hThread = (HANDLE)_beginthreadex(NULL, 0, subMain, NULL, 0, &threadID);
	//while 루프를 돌면서 패킷을 전송하거나 다양한 이벤트를 발생 시킨다.
	while (1)
	{
		//scan 함수는 입력을 받을 수 있다. 이때 입력한 숫자는 화면에 나타나지 않게끔 구현이 되어 있다.
		//1번을 치고 엔터하면 slow start가 진행되는 모습을 볼 수 있고 
		//2번은 손실을 원하는 패킷 번호를 추가로 입력하면 그 패킷에 대해 3dup 사건이 발생한다
		//3번을 입력하고 추가로 타임아웃이 될 패킷 번호를 입력하면 그 패킷에 대해 timeout 사건이 발생한다.
		char in = scan();
		//1번 입력은 slow start를 시작한다.
		if (in == '1')
		{
			printf("window:%d\n", window);
			//초기 threshold의 값은 무한대이므로 이벤트 발생으로 설정이 되면 출력되기 시작한다.
			if (threshold != 0x7fffffff)
				printf("threshold:%d\n", threshold);
			flag = 1;
			window = packCnt;
			//전송될 패킷에 각종 정보를 넣어서 서버로 보낸다.
			message[0] = (int)flag;
			message[4] = (int)packNum;
			message[8] = (int)packCnt;
			message[12] = (int)packDup;
			message[16] = (int)packTime;
			message[20] = (int)timeFlag;
			//윈도우 크기에 맞게 for 문을 이용하여 패킷을 전송한다
			for (int i = 0; i < window; i++)
			{
				message[4] = (int)packNum;
				send(sock, message, 24, 0);
				print(packNum);
				packNum++;
			}
			mainFlag = 1;
		}
		//2번 입력은 3 dup 액션을 취한다.
		else if (in == '2')
		{
			//추가로 손실될 패킷에 대한 번호를 입력하면 그 패킷이 손실되는 상황을 만들 수 있다.
			scanf("%d", &packDup);
			printf("window:%d\n", window);
			if (threshold != 0x7fffffff)
				printf("threshold:%d\n", threshold);
			flag = 2;
			//각종 정보를 배열에 담아 서버로 보낸다.
			window = packCnt;
			message[0] = (int)flag;
			message[4] = (int)packNum;
			message[8] = (int)packCnt;
			message[12] = (int)packDup;
			message[16] = (int)packTime;
			message[20] = (int)timeFlag;
			for (int i = 0; i < window; i++)
			{
				message[4] = (int)packNum;
				send(sock, message, 24, 0);
				print(packNum);
				packNum++;
			}
			printf("LOSS PACKET: %d\n", packDup);
			mainFlag = 2;
		}
		//timeout 사건 발생을 일으킨다.
		else if (in == '3')
		{
			//추가로 타임 아웃이 발생할 패킷을 입력 받는다.
			scanf("%d", &packTime);
			printf("window:%d\n", window);
			if (threshold != 0x7fffffff)
				printf("threshold:%d\n", threshold);
			flag = 3;
			//각종 정보를 패킷에 담아 보낸다.
			window = packCnt;
			message[0] = (int)flag;
			message[4] = (int)packNum;
			message[8] = (int)packCnt;
			message[12] = (int)packDup;
			message[16] = (int)packTime;
			message[20] = (int)timeFlag;
			for (int i = 0; i < window; i++)
			{
				message[4] = (int)packNum;
				send(sock, message, 24, 0);
				print(packNum);
				packNum++;
			}
			mainFlag = 3;
		}
	}
	return 0;
}
//입력을 받는 함수
char scan()
{
	char in;
	in = _getch();
	fflush(stdin);
	return in;

}
//타임아웃이 발생할 경우 실행할 함수
void CALLBACK TimeProc(UINT m_nTimerID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD d2)
{
	timeFlag = 1;
	timeKillEvent(m_nTimerID);
}
//쓰레드가 실행하는 함수
unsigned WINAPI subMain(void* arg)
{
	char recvMsg[BUF_SIZE];
	while (1)
	{
		//timeFlag는 타임아웃 사건의 시나리오에서 일정 시간이 지나도 ACK가 오지 않는 경우
		//timeout 함수가 실행되면서 이 flag를 1로 설정해줌으로써 실행이 된다.
		if (timeFlag == 1)
		{
			printf("window:%d\n", window);
			if (threshold != 0x7fffffff)
				printf("threshold:%d\n", threshold);
			flag = 4;
			window = packCnt;
			message[0] = (int)flag;
			message[4] = (int)packNum;
			message[8] = (int)packCnt;
			message[12] = (int)packDup;
			message[16] = (int)packTime;
			message[20] = (int)timeFlag;
			for (int i = 0; i < window - 1; i++)
			{
				message[4] = (int)packNum;
				send(sock, message, 24, 0);
				print(packNum);
				packNum++;
			}
			if (threshold > packCnt)
				packCnt *= 2;
			else
				packCnt++;


			message[4] = (int)packTime;
			send(sock, message, 24, 0);
			textcolor(RED);
			printf("time out: %d packet\n", packTime);
			textcolor(ORI);


			message[4] = (int)packTime;
			send(sock, message, 24, 0);
			textcolor(GREEN);
			printf("----------->패킷 %d\n", packTime);
			textcolor(ORI);
			mainFlag = 1;
		}
		//slow start에서 ack 패킷을 받는 역할을 한다.
		if (mainFlag == 1)
		{
			if (timeFlag2 == 1)
			{
				recv(sock, recvMsg, 24, 0);
				textcolor(RED);
				printf("<--- ACK %d 재수신\n", (int)recvMsg[4]);
				textcolor(ORI);
				timeFlag2 = 0;
			}
			recv(sock, recvMsg, 24, 0);
			flag = (int)recvMsg[0];
			rcvPackCnt = (int)recvMsg[8];
			for (int i = 0; i < rcvPackCnt; i++)
			{
				rcvPackNum = (int)recvMsg[4];
				textcolor(YEL);
				printf("<--- ACK %d 수신\n", rcvPackNum);
				textcolor(ORI);
				if (i + 1 < rcvPackCnt)
					recv(sock, recvMsg, 24, 0);
			}
			if (timeFlag == 1)
			{
				timeFlag2 = 1;
				timeFlag = 0;
				threshold = window;
				threshold /= 2;
				packCnt = 1;
				window = 1;
			}
			else
			{
				if (threshold > packCnt)
				{
					if (threshold < (packCnt * 2))
						packCnt = threshold;
					else
						packCnt *= 2;
				}
				else
					packCnt++;
			}
			window = packCnt;
			mainFlag = 0;
			printf("\n\n");
		}
		//3dup이 발생 했을 때 실행되는 조건문이다.
		else if (mainFlag == 2)
		{
			int dupCnt = 0;
			recv(sock, recvMsg, 24, 0);
			flag = (int)recvMsg[0];
			rcvPackCnt = (int)recvMsg[8];
			for (int i = 0; i < rcvPackCnt; i++)
			{
				rcvPackNum = (int)recvMsg[4];
				textcolor(YEL);
				if (rcvPackNum != -1)
					printf("<--- ACK %d 수신\n", rcvPackNum);
				else if (rcvPackNum == -1 && dupCnt < 2)
				{
					printf("<--- ACK %d 수신\n", packDup - 1);
					dupCnt++;
				}
				else
					printf("trash!!\n");
				textcolor(ORI);
				if (i + 1 < rcvPackCnt)
					recv(sock, recvMsg, 24, 0);
			}
			threshold = window;
			threshold /= 2;
			packCnt /= 2;
			packNum = packDup;
			window = packCnt;
			printf("\n\n");
			mainFlag = 0;
		}
		//timeout의 상황에서 실행되는 함수이다. 
		else if (mainFlag == 3)
		{
			int prePackNum = rcvPackNum;
			recv(sock, recvMsg, 24, 0);
			flag = (int)recvMsg[0];
			rcvPackCnt = (int)recvMsg[8];
			for (int i = 0; i < rcvPackCnt - 1; i++)
			{
				rcvPackNum = (int)recvMsg[4];

				/// TIME 
				m_nTimerID = timeSetEvent(3000, 0, TimeProc, (DWORD)0, TIME_PERIODIC);
				/// 

				textcolor(YEL);
				printf("<--- ACK %d 수신\n", rcvPackNum);
				textcolor(ORI);
				if (rcvPackNum - prePackNum == 1)
				{
					timeKillEvent(m_nTimerID);
				}
				if (i + 1 < rcvPackCnt - 1)
				{
					recv(sock, recvMsg, 24, 0);
				}

			}
			if (threshold > packCnt)
			{
				if (threshold < (packCnt * 2))
					packCnt = threshold;
				else
					packCnt *= 2;
			}
			else
				packCnt++;
			window = packCnt;
			printf("\n\n");
			mainFlag = 0;
		}
	}
}
//화면에 찍힐 텍스트 부분 함수
void print(int pn)
{
	textcolor(GREEN);
	printf("----------->패킷 %d\n", pn);
	textcolor(ORI);
}
//화면에 찍힐 텍스트의 색깔을 바꿔주는 함수
void textcolor(int color_number)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color_number);
}
//UDP 통신을 위한 초기 설정
void conn()
{
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET)
		ErrorHandling("socket() error");

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servAdr.sin_port = htons(9190);

	connect(sock, (SOCKADDR*)&servAdr, sizeof(servAdr));
	m[0] = 'c';
}
//safe coding을 위한 에러 처리 함수
void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
