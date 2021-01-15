#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<process.h>
#include <conio.h>
#include <winsock2.h>
#define BLUE 9
#define GREEN 10
#define RED 12
#define YEL 14
#define ORI 15
#define BUF_SIZE 30
HANDLE hThread;
char in;
unsigned threadID;
WSADATA wsaData;
int strLen;
UINT m_nTimerID;
SOCKET servSock;
SOCKADDR_IN servAdr, clntAdr;
static HANDLE sem;
int clntAdrSz = sizeof(clntAdr);


int flag = 0, packNum = 0, packCnt = 0, packDup = 0, packTime = 0, timeFlag = 0;
int mainFlag = 0;
int len;


char message[BUF_SIZE];
void ErrorHandling(char* message);
void textcolor(int color_number);
void print(int cnt);
void conn();
unsigned WINAPI subMain(void* arg)
{
	while (1)
	{
		int len = recvfrom(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, &clntAdrSz);
		flag = (int)message[0];
		packCnt = (int)message[8];
		packDup = (int)message[12];
		packTime = (int)message[16];
		timeFlag = (int)message[20];
		if (flag == 1)
		{
			for (int i = 0; i < packCnt; i++)
			{
				packNum = (int)message[4];
				print(packNum);
				if (i + 1 < packCnt)
					recvfrom(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, &clntAdrSz);
			}
			printf("\n\n");
			mainFlag = 1;
		}
		else if (flag == 2)
		{
			int tempDup = packDup;
			for (int i = 0; i < packCnt; i++)
			{
				packNum = (int)message[4];
				packDup = (int)message[12];
				if (packNum == packDup)
				{
					textcolor(RED);
					printf("패킷 %d LOSS\n", packNum);
					textcolor(ORI);
				}
				else if (packNum < tempDup)
					print(packNum);
				else
				{
					textcolor(GREEN);
					printf("<---패킷 %d 수신\n", packNum);
					textcolor(ORI);

					/// //////////////////////

					textcolor(YEL);
					printf("------------->ACK %d 송신\n", tempDup - 1);
					textcolor(ORI);
				}
				if (i + 1 < packCnt)
					recvfrom(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, &clntAdrSz);
			}
			printf("\n\n");
			mainFlag = 2;
		}
		else if (flag == 3)
		{
			int tempTime = packTime;
			for (int i = 0; i < packCnt; i++)
			{
				packNum = (int)message[4];
				textcolor(GREEN);
				printf("<---패킷 %d 수신\n", packNum);
				textcolor(ORI);

				/// //////////////////////
				if (packNum == packTime)
				{
					textcolor(YEL);
					printf("------------->ACK %d 송신 (loss time-out)\n", packNum);
					textcolor(ORI);
				}
				else
				{
					textcolor(YEL);
					printf("------------->ACK %d 송신\n", packNum);
					textcolor(ORI);
				}
				if (i + 1 < packCnt)
					recvfrom(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, &clntAdrSz);
			}
			printf("\n\n");
			mainFlag = 3;
		}
		//timeflag=1로 되고나서의 행동
		else if (flag == 4)
		{
			int tempTime = packTime;
			for (int i = 0; i < packCnt - 1; i++)
			{
				packNum = (int)message[4];
				textcolor(GREEN);
				printf("<---패킷 %d 수신\n", packNum);
				textcolor(ORI);

				textcolor(YEL);
				printf("------------->ACK %d 송신\n", packNum);
				textcolor(ORI);

				if (i + 1 < packCnt)
					recvfrom(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, &clntAdrSz);
			}
			recvfrom(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, &clntAdrSz);
			textcolor(RED);
			printf("<---패킷 재수신: %d\n", packTime);
			textcolor(ORI);

			textcolor(RED);
			printf("------------->ACK %d 재송신\n", packNum);
			textcolor(ORI);

			printf("\n\n");
			mainFlag = 4;
		}
	}
}
int main(int argc, char* argv[])
{
	conn();
	hThread = (HANDLE)_beginthreadex(NULL, 0, subMain, NULL, 0, &threadID);
	while (1)
	{
		if (mainFlag == 1)
		{
			int temp = (packNum + 1) - packCnt;
			message[0] = 1;
			message[8] = (int)packCnt;
			message[12] = (int)packDup;
			message[16] = (int)packTime;
			message[20] = (int)timeFlag;
			for (int i = 0; i < packCnt; i++)
			{
				message[4] = temp;
				sendto(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, sizeof(clntAdr));
				temp++;
			}
			mainFlag = 0;
		}
		else if (mainFlag == 2)
		{
			int temp = (packNum + 1) - packCnt;
			message[0] = 2;
			message[8] = (int)packCnt;
			message[12] = (int)packDup;
			message[16] = (int)packTime;
			message[20] = (int)timeFlag;
			for (int i = 0; i < packCnt; i++)
			{
				if (temp < packDup)
					message[4] = temp;
				else
					message[4] = (int)-1;
				sendto(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, sizeof(clntAdr));
				temp++;
			}

			mainFlag = 0;
		}
		else if (mainFlag == 3)
		{
			int temp = (packNum + 1) - packCnt;
			message[0] = 2;
			message[8] = (int)packCnt;
			message[12] = (int)packDup;
			message[16] = (int)packTime;
			message[20] = (int)timeFlag;
			for (int i = 0; i < packCnt; i++)
			{
				if (temp < packTime || temp>packTime)
					message[4] = temp;
				else
				{
					temp++;
					continue;
				}
				sendto(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, sizeof(clntAdr));
				temp++;
			}
			mainFlag = 0;
		}
		else if (mainFlag == 4)
		{
			int temp = (packNum + 1) - packCnt;
			message[0] = 2;
			message[8] = (int)packCnt;
			message[12] = (int)packDup;
			message[16] = (int)packTime;
			message[20] = (int)timeFlag;
			for (int i = 0; i < packCnt; i++)
			{
				if (temp < packTime || temp>packTime)
					message[4] = temp;
				else
				{
					temp++;
					continue;
				}
				sendto(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, sizeof(clntAdr));
				temp++;
			}
			if ((int)message[20] == 1)
			{
				message[4] = (int)packNum;
				sendto(servSock, message, 24, 0, (SOCKADDR*)&clntAdr, sizeof(clntAdr));
			}
			mainFlag = 0;
		}
	}
	closesocket(servSock);
	WSACleanup();
	return 0;
}
void print(int cnt)
{
	textcolor(GREEN);
	printf("<---패킷 %d 수신\n", cnt);
	textcolor(ORI);

	/// //////////////////////

	textcolor(YEL);
	printf("------------->ACK %d 송신\n", cnt);
	textcolor(ORI);
}
void textcolor(int color_number)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color_number);
}
void conn()
{
	sem = CreateSemaphore(NULL, 1, 1, NULL);
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	servSock = socket(PF_INET, SOCK_DGRAM, 0);
	if (servSock == INVALID_SOCKET)
		ErrorHandling("UDP socket creation error");

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(9190);

	if (bind(servSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind() error");
}



void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}