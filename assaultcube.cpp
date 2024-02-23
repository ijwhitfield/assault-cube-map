#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>

using namespace std;

const uintptr_t A_PLAYER_LIST = 0x50F4F8,
A_PLAYERNUM = 0x50f500,
A_LOCALPLAYER = 0x509B74,
O_NAME = 0x225,
O_XHEAD = 0x4,
O_YHEAD = 0x8,
O_ZHEAD = 0xC,
O_YAW = 0x40;
const UINT NAME_SIZE = 16,
TIP_SIZE=6,
CORNER_SIZE=3;
const string WINDOW_NAME = "ACMap",
WINDOW_CLASS_NAME = "ACOVERLAY",
AC_WINDOW_CLASS = "SDL_app",
AC_WINDOW_NAME = "AssaultCube";
RECT client_rect = {};
RECT window_rect = {};
RECT true_rect = {};
UINT border = 0,
playercount = 0;
HDC hdc;
HBRUSH red = CreateSolidBrush(RGB(255, 0, 0)),
green = CreateSolidBrush(RGB(0, 255, 0)),
blue = CreateSolidBrush(RGB(0, 0, 255)),
black = CreateSolidBrush(RGB(0, 0, 0));

struct player {
	string name;
	float x, y, yaw;
};

vector<player> playerData; //vector of the positions of every player, 0 being localplayer, others being derived from players[]

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (hdc == NULL) {
		hdc = GetDC(hwnd);
	}
	PAINTSTRUCT ps;   // paint data for Begin/EndPaint
	RECT chroma = {};
	chroma.left = 0;
	chroma.right = true_rect.right - true_rect.left;
	chroma.top = 0;
	chroma.bottom = true_rect.bottom - true_rect.top;
	RECT radar = {};
	radar.left = 0;
	radar.right = 300;
	radar.top = 0;
	radar.bottom = 300;
	switch (uMsg) {
	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		FillRect(hdc, &chroma, green);
		FillRect(hdc, &radar, black);
		POINT triPoints[3];
		for (int i = 0; i < playercount; i++) {
			triPoints[0].x = min(300, max(0, playerData[i].x - TIP_SIZE * sin(-playerData[i].yaw*(3.14 / 180))));
			triPoints[0].y = min(300, max(0, playerData[i].y - TIP_SIZE * cos(-playerData[i].yaw*(3.14 / 180))));
			triPoints[1].x = min(300, max(0, playerData[i].x - CORNER_SIZE * sin((135  -playerData[i].yaw)*(3.14 / 180))));
			triPoints[1].y = min(300, max(0, playerData[i].y - CORNER_SIZE * cos((135  -playerData[i].yaw)*(3.14 / 180))));
			triPoints[2].x = min(300, max(0, playerData[i].x - CORNER_SIZE * sin((225  -playerData[i].yaw)*(3.14 / 180))));
			triPoints[2].y = min(300, max(0, playerData[i].y - CORNER_SIZE * cos((225  -playerData[i].yaw)*(3.14 / 180))));

			if (i == 0) {
				HGDIOBJ hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255));
				SelectObject(hdc, hPen);
				HGDIOBJ hBrush = CreateSolidBrush(RGB(0, 0, 255));
				SelectObject(hdc, (HGDIOBJ)hBrush);
			}
			else {
				HGDIOBJ hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
				SelectObject(hdc, hPen);
				HGDIOBJ hBrush = CreateSolidBrush(RGB(255, 0, 0));
				SelectObject(hdc, (HGDIOBJ)hBrush);
			}
			Polygon(hdc, triPoints, 3);
			
		}
		EndPaint(hwnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return (LRESULT)NULL;
}

int main() {
	void* test = (void*)O_NAME;
	int pid = 0;
	cout << "input pid" << endl;
	cin >> dec >> pid;
	HANDLE hproc = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (hproc == NULL) {
		cout << "process not found";
		return EXIT_FAILURE;
	}
	ReadProcessMemory(hproc, (LPCVOID)(A_PLAYERNUM), &playercount, sizeof(int), NULL);
	void* playerList = 0x0; //pointer to dynamic playerlist
	ReadProcessMemory(hproc, (LPCVOID)((void*)A_PLAYER_LIST), &playerList, sizeof(uintptr_t), NULL);
	void* localplayer = 0x0;
	ReadProcessMemory(hproc, (LPCVOID)((void*)A_LOCALPLAYER), &localplayer, sizeof(uintptr_t), NULL);
	uintptr_t* players = new uintptr_t[playercount];
	//cout << hex << localplayer << endl;
	players[0] = (uintptr_t)localplayer;
	for (int player = 1; player < playercount; player++) {
		//for every player, put its address in the players array
		ReadProcessMemory(hproc, (LPCVOID)((uintptr_t)playerList + 0x4 * (player)), &players[player], sizeof(uintptr_t), NULL);
	}
	
	//by now, every player struct is listed in players array
	HWND game_hwnd = FindWindowA((LPCSTR)AC_WINDOW_CLASS.c_str(), (LPCSTR)AC_WINDOW_NAME.c_str());
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.lpszClassName = WINDOW_CLASS_NAME.c_str();
	
	RegisterClassExA(&windowClass);
	HDC hdc;
	HWND overlay_hwnd;
	overlay_hwnd = CreateWindowExA(NULL, (LPCSTR)WINDOW_CLASS_NAME.c_str(), (LPCSTR)WINDOW_NAME.c_str(), NULL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
	
	hdc = GetDC(overlay_hwnd);
	
	SetWindowLongPtrA(overlay_hwnd, GWL_STYLE, WS_VISIBLE);
	SetWindowLongPtrA(overlay_hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);
	SetLayeredWindowAttributes(overlay_hwnd, RGB(0, 255, 0), 0, LWA_COLORKEY); //chromakey green
	float x = 0, y = 0, yaw = 0;
	
	
	
	while (true) {
		playerData.clear();
		for (int playerNum = 0; playerNum < playercount; playerNum++) {
			ReadProcessMemory(hproc, (LPCVOID)((uintptr_t)players[playerNum] + O_XHEAD), &x, sizeof(float), NULL);
			ReadProcessMemory(hproc, (LPCVOID)((uintptr_t)players[playerNum] + O_YHEAD), &y, sizeof(float), NULL);
			ReadProcessMemory(hproc, (LPCVOID)((uintptr_t)players[playerNum] + O_YAW), &yaw, sizeof(float), NULL);
			char name[NAME_SIZE]; //char buffer of size NAME_SIZE to hold name
			fill_n(name, NAME_SIZE, (char)0);
			for (int i = 0; i < NAME_SIZE; i++) {
				char letter = (char)0;
				ReadProcessMemory(hproc, (LPCVOID)((uintptr_t)players[i] + O_NAME + i), &letter, sizeof(char), NULL);
				if ((int)letter == 0) {
					break;
				}
				name[i] = letter;
			}
			playerData.push_back(player{name,x,y,yaw});
		}
		GetClientRect(game_hwnd, &client_rect);
		GetWindowRect(game_hwnd, &window_rect);
		border = ((window_rect.right - window_rect.left) - (client_rect.right - client_rect.left)) / 2;
		true_rect.top = window_rect.bottom - border - (client_rect.bottom - client_rect.top);
		true_rect.left = window_rect.left + border;
		true_rect.right = true_rect.left + (client_rect.right - client_rect.left);
		true_rect.bottom = true_rect.top + (client_rect.bottom - client_rect.top);
		SetWindowPos(overlay_hwnd, HWND_TOPMOST, true_rect.left, true_rect.top, true_rect.right-true_rect.left, true_rect.bottom-true_rect.top, SWP_SHOWWINDOW);
		InvalidateRect(overlay_hwnd, &true_rect, true);
		UpdateWindow(overlay_hwnd);
		Sleep(100);
	}
	delete[] players;
	CloseHandle(hproc);
	return EXIT_SUCCESS;
}