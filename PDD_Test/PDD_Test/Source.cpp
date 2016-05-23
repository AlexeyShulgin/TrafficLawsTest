#include "resource.h"
#include <Windows.h>
#include <WinReg.h>
#include <tchar.h>
#include <string.h>
#include <locale.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment( lib, "GDIPlus.lib" )

#define PATH_AB_KHARKOV L"C:\\Program Files\\Тест ПДД\\Questions\\AB_KHARKOV" // Путь к билетам AB Харьков
#define PATH_AB_KIEV L"C:\\Program Files\\Тест ПДД\\Questions\\AB_KIEV" // Путь к билетам AB Киев
#define PATH_CD_KHARKOV L"C:\\Program Files\\Тест ПДД\\Questions\\CD_KHARKOV" // Путь к билетам CD Харьков

#define SIZE_COMBOBOX_TEXT 12 // Длина текста в ComboBox в диалоговом окне
#define SIZE_TIME_TEXT 6 // Длина времени
#define SIZE_TEXT 10000 // Максимальная длина текста

#define COUNT_AB_KIEV 110 // Количество билетов AB Киев
#define COUNT_AB_KHARKOV 80 // Количество билетов AB Харьков
#define COUNT_CD_KHARKOV 93 // Количество билетов CD Харьков

#define COUNT_QUESTIONS 20 // Максимальное количество вопросов в билете

#define ID_BUTTONS (WM_USER + 1) // ID кнопок с номерами вопросов

#define ID_BUTTON_ENTER (WM_USER + 2) // ID кнопки "Ответить"

#define DIALOG_CLOSED (WM_USER + 3)
#define DIALOG_OK (WM_USER + 4)

#define ID_STATIC_QUESTION (WM_USER + 5) // ID текста вопроса
#define ID_STATIC_TIME (WM_USER + 6) // ID текста таймера

#define ID_TIMER (WM_USER + 7) // ID таймера

#define SIZE_OF_KEY 16383

static HINSTANCE hinstance = NULL;
static LPCWSTR CLASS_NAME = L"PDD_TEST_WINDOW"; // Имя класса окна
static LPCWSTR WND_NAME = L"Тест ПДД"; // Имя окна

static HWND hWnd = NULL; // Дескриптор окна

const UINT ID_RADIOS[5] = { (WM_USER + 8), (WM_USER + 9), (WM_USER + 10), (WM_USER + 11), (WM_USER + 12) };
const UINT ID_STATICS[5] = { (WM_USER + 13), (WM_USER + 14), (WM_USER + 15), (WM_USER + 16), (WM_USER + 17) };

static HWND hStQuestion = NULL; // Дескриптор STATIC с вопросом

static HWND hBtns[20] = { NULL }; // Массив дескрипторов Buttons
static HWND hRadios[5] = { NULL }; // Массив дескрипторов RadioButton

static HWND hBtnEnter = NULL; // Дескриптор кнопки "Ответить"

static HWND hStatic = NULL; // Дескриптор STATIC для времени

static UINT_PTR hTimer = NULL; // Дескриптор таймера

ULONG_PTR gdiplusToken = 0;

bool Time = false;
bool t = false;
bool IsFinished = false;
bool DialogClosed = false;
wchar_t Ticket[MAX_PATH] = { NULL };

int CurrentQuestion = 1;
int HighScore = 0;
int CountQuestions = 0;

int TicketNum = 0;

// Структура для хранения вопроса
typedef struct _QUESTION
{
	wchar_t Question[SIZE_TEXT]; // Вопрос
	wchar_t Image[SIZE_COMBOBOX_TEXT]; // Имя картинки
	LPWSTR Answers[5]; // Варианты ответа
	int TrueAnswer; // Номер правильного ответа
	int buf; // Ответ, выбранный пользователем
	bool IsAnswered; // Была ли  нажата кнопка "Ответить"
} QUESTION, *LPQUESTION;

QUESTION Questions[COUNT_QUESTIONS] = { NULL }; // Структура для хранения вопроса

int Minutes = NULL; // Количество минут
int Seconds = NULL; // Количество секунд

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // Функция обработки сообщений главного окна приложения
static BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM); // Функция обработки сообщений диалогового окна

bool LoadQuestions(LPCWSTR); // Функция загрузки вопросов из файла
bool LoadQuestion(int); // Функция загрузки вопроса из структуры
bool DrawImage(int); // Функция для вывода картинки в окно
bool LoadAnswers(bool, int); // Функция для вывода вариантов ответа в окно
bool Test(int, bool); // Функция проверки ответа
bool LoadResults(void); // Функция вывода результатов теста на экран
bool ReloadTest(void); // Функция для начала нового тестирования
DWORD GetError(DWORD); // Функция обработки ошибок

int WINAPI _tWinMain(HINSTANCE _hinstance, HINSTANCE _hPrevInstance, LPTSTR _lpCommandLine, int nCmdShow)
{
	GdiplusStartupInput gdiplusStartupInput; // Структура с информацией о GDI+
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL); // Инициализация GDI+

	hinstance = _hinstance;

	WNDCLASSEX wc = { NULL };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = ::hinstance;
	wc.hIcon = LoadIcon(::hinstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(130, 110, 200));
	wc.lpszClassName = CLASS_NAME;
	wc.hIconSm = LoadIcon(::hinstance, MAKEINTRESOURCE(IDI_ICON1));

	if (!RegisterClassEx(&wc))
		return GetError(GetLastError());

	HDC hDCScreen = GetDC(NULL);
	int Horzres = GetDeviceCaps(hDCScreen, HORZRES);
	int Vertres = GetDeviceCaps(hDCScreen, VERTRES);
	ReleaseDC(NULL, hDCScreen);

	hWnd = CreateWindow(CLASS_NAME, WND_NAME, WS_OVERLAPPED | WS_SYSMENU, (Horzres / 2) - 365, (Vertres / 2) - 300, 730, 600, NULL, NULL, hinstance, NULL);
	if (!hWnd)
		return GetError(GetLastError());

	INT_PTR result_dialog = DialogBoxParam(hinstance, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DlgProc), NULL);
	if (!result_dialog)
		return GetError(GetLastError());
	else if (result_dialog == DIALOG_CLOSED)
		return 0;

	wchar_t str[3] = { NULL };
	for (int i = 0; i < 20; i++)
	{
		swprintf_s(str, L"%d", i + 1);
		hBtns[i] = CreateWindowEx(NULL, L"BUTTON", str, WS_CHILD | WS_VISIBLE | WS_BORDER | BS_OWNERDRAW, i + 5 + (i * 34.5), 10, 30, 30, hWnd, (HMENU)ID_BUTTONS, hinstance, NULL);
		if (!hBtns[i])
			return GetError(GetLastError());
	}

	hBtnEnter = CreateWindowEx(NULL, L"BUTTON", L"Ответить", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_DEFPUSHBUTTON, 315, 520, 100, 30, hWnd, (HMENU)ID_BUTTON_ENTER, hinstance, NULL);
	if (!hBtnEnter)
		return GetError(GetLastError());

	if (!LoadQuestions(Ticket))
		return GetError(GetLastError());

	if (Time)
	{
		hStatic = CreateWindowEx(NULL, L"STATIC", L"00:00", WS_CHILD | WS_VISIBLE, 630, 520, 100, 30, ::hWnd, (HMENU)ID_STATIC_TIME, hinstance, NULL);
		if (!hStatic)
			return GetError(GetLastError());

		HFONT hFnt = CreateFont(35, NULL, NULL, NULL, FW_BLACK, FALSE, FALSE, FALSE, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, L"Arial");
		if (!hFnt)
			return GetError(GetLastError());

		SendMessage(hStatic, WM_SETFONT, (WPARAM)hFnt, TRUE);

		hTimer = SetTimer(hWnd, ID_TIMER, 1000, NULL);
		if (!hTimer)
		{
			return GetError(GetLastError());
		}
	}

	hStQuestion = CreateWindowEx(NULL, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 10, 50, 700, 150, hWnd, (HMENU)ID_STATIC_QUESTION, hinstance, NULL);
	if (!hStQuestion)
		return false;

	HFONT hFnt = CreateFont(20, NULL, NULL, NULL, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, L"Arial");
	if (!hFnt)
		return GetError(GetLastError());

	SendMessage(hStQuestion, WM_SETFONT, (WPARAM)hFnt, TRUE);
	for (int i = 0; i < 5; i++)
	{
		SendMessage(hRadios[i], WM_SETFONT, (WPARAM)hFnt, TRUE);
	}

	ShowWindow(hWnd, SW_SHOW);

	if (!LoadQuestion(CurrentQuestion))
		return GetError(GetLastError());

	UpdateWindow(hWnd);

	MSG msg = { NULL };
	while (GetMessage(&msg, NULL, NULL, NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	for (int i = 0; i < COUNT_QUESTIONS; i++)
		if (hBtns[i])
			DestroyWindow(hBtns[i]);
	for (int i = 0; i < 5; i++)
		if (hRadios[i])
			DestroyWindow(hRadios[i]);
	if (hBtnEnter)
		DestroyWindow(hBtnEnter);
	if (hStQuestion)
		DestroyWindow(hStQuestion);
	if (hStatic)
		DestroyWindow(hStatic);
	if(hWnd)
		DestroyWindow(hWnd);

	GdiplusShutdown(gdiplusToken); // Освобождение ресурсов GDI+

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND _hWnd, UINT _Msg, WPARAM _wParam, LPARAM _lParam)
{
	wchar_t TimeNow[SIZE_TIME_TEXT] = { NULL };
	switch (_Msg)
	{
	case WM_TIMER:
		if (Time && t)
		{
			if (Minutes < 20 && Seconds < 60)
			{
				Seconds++;
				if (Seconds == 60)
				{
					Seconds = NULL;
					Minutes++;
				}
				if (Minutes < 10 && Seconds < 10)
				{
					if (swprintf_s(TimeNow, L"0%d:0%d", Minutes, Seconds) == -1)
						PostQuitMessage(GetError(GetLastError()));
				}
				else if (Minutes < 10)
				{
					if (swprintf_s(TimeNow, L"0%d:%d", Minutes, Seconds) == -1)
						PostQuitMessage(GetError(GetLastError()));
				}
				else if (Seconds < 10)
				{
					if (swprintf_s(TimeNow, L"%d:0%d", Minutes, Seconds) == -1)
						PostQuitMessage(GetError(GetLastError()));
				}
				else
				{
					if (swprintf_s(TimeNow, L"%d:%d", Minutes, Seconds) == -1)
						PostQuitMessage(GetError(GetLastError()));
				}
				SendMessage(hStatic, WM_SETTEXT, NULL, (LPARAM)TimeNow);
			}
			else if (Minutes == 20)
			{
				if (!LoadResults())
					PostQuitMessage(GetError(GetLastError()));
				LoadQuestion(CurrentQuestion);
			}
		}
		break;
	case WM_CTLCOLORSTATIC:
		if ((HWND)_lParam == hRadios[0] || (HWND)_lParam == hRadios[1] || (HWND)_lParam == hRadios[2] || (HWND)_lParam == hRadios[3] || (HWND)_lParam == hRadios[4])
		{
			if (SetBkColor((HDC)_wParam, RGB(130, 110, 200)) == CLR_INVALID)
				PostQuitMessage(GetError(GetLastError()));
			if (SetTextColor((HDC)_wParam, RGB(255, 255, 255)) == CLR_INVALID)
				PostQuitMessage(GetError(GetLastError()));
			if (Questions[CurrentQuestion - 1].IsAnswered)
			{
				if ((HWND)_lParam == hRadios[Questions[CurrentQuestion - 1].TrueAnswer - 1])
				{
					if (SetBkColor((HDC)_wParam, RGB(0, 255, 0)) == CLR_INVALID)
						PostQuitMessage(GetError(GetLastError()));
					return (LRESULT)CreateSolidBrush(RGB(9, 131, 0));
				}
				else if ((HWND)_lParam == hRadios[Questions[CurrentQuestion - 1].buf - 1] && Questions[CurrentQuestion - 1].buf != Questions[CurrentQuestion - 1].TrueAnswer)
				{
					if (SetBkColor((HDC)_wParam, RGB(255, 0, 0)) == CLR_INVALID)
						PostQuitMessage(GetError(GetLastError()));
					return (LRESULT)CreateSolidBrush(RGB(255, 0, 0));
				}
			}
			return (LRESULT)CreateSolidBrush(RGB(130, 110, 200));
		}
		else
		{
			if (SetBkColor((HDC)_wParam, RGB(130, 110, 200)) == CLR_INVALID)
				PostQuitMessage(GetError(GetLastError()));
			if (SetTextColor((HDC)_wParam, RGB(255, 255, 255)) == CLR_INVALID)
				PostQuitMessage(GetError(GetLastError()));
			return (LRESULT)CreateSolidBrush(RGB(130, 110, 200));
		}
		break;
	case WM_CTLCOLORBTN:
	{
		wchar_t _txt[3] = { NULL };
		GetWindowText((HWND)_lParam, _txt, 3);
		int q = _wtoi(_txt);
		q--;
		COLORREF Rgb = NULL;
		if (!Questions[q].IsAnswered && q != CurrentQuestion - 1)
			Rgb = RGB(153, 153, 153);
		else if (q == CurrentQuestion - 1)
			Rgb = RGB(255, 255, 0);
		else
		{
			if (Questions[q].IsAnswered && Questions[q].buf == Questions[q].TrueAnswer)
				Rgb = RGB(0, 255, 0);
			else if (Questions[q].IsAnswered && Questions[q].buf != Questions[q].TrueAnswer)
				Rgb = RGB(255, 0, 0);
		}

		if (SetBkColor((HDC)_wParam, Rgb) == CLR_INVALID)
			PostQuitMessage(GetError(GetLastError()));
		if (SetTextColor((HDC)_wParam, RGB(0, 0, 0)) == CLR_INVALID)
			PostQuitMessage(GetError(GetLastError()));
		
		TextOut((HDC)_wParam, 6, 6, _txt, wcslen(_txt));
		return (LRESULT)CreateSolidBrush(Rgb);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(NULL);
		break;
	case WM_COMMAND:
		switch (_wParam)
		{
		case ID_BUTTON_ENTER:
			if (!IsFinished)
			{
				Test(CurrentQuestion - 1, true);

				if (CurrentQuestion > 0 && CurrentQuestion < 20)
				{
					CurrentQuestion++;
					if (!LoadQuestion(CurrentQuestion))
						PostQuitMessage(GetError(GetLastError()));
				}
				if (CountQuestions == 20)
				{
					if (!LoadResults())
						PostQuitMessage(GetError(GetLastError()));
					LoadQuestion(CurrentQuestion);
				}

				SetFocus(hBtns[CurrentQuestion - 1]);
			}
			break;
		case ID_BUTTONS:
			wchar_t _buf[3] = { NULL };
			GetWindowText((HWND)_lParam, _buf, 3);
			CurrentQuestion = _wtoi(_buf);
			if (!LoadQuestion(CurrentQuestion))
				PostQuitMessage(GetError(GetLastError()));
			break;
		}
		if (_wParam == ID_RADIOS[0])
			Questions[CurrentQuestion - 1].buf = 1;
		else if (_wParam == ID_RADIOS[1])
			Questions[CurrentQuestion - 1].buf = 2;
		else if (_wParam == ID_RADIOS[2])
			Questions[CurrentQuestion - 1].buf = 3;
		else if (_wParam == ID_RADIOS[3])
			Questions[CurrentQuestion - 1].buf = 4;
		else if (_wParam == ID_RADIOS[4])
			Questions[CurrentQuestion - 1].buf = 5;
		break;
	case WM_CLOSE:
		t = false;
		if (IsFinished)
			if (MessageBox(_hWnd, L"Хотите пройти тест ещё раз?", L"Тест ПДД", MB_ICONQUESTION | MB_YESNO | MB_APPLMODAL) == IDYES)
			{
				if (!ReloadTest())
					PostQuitMessage(GetError(GetLastError()));
				else
					if (DialogClosed)
						PostQuitMessage(NULL);
			}
			else
				PostQuitMessage(NULL);
		else if (MessageBox(_hWnd, L"Вы действительно хотите окончить тестирование?", L"Тест ПДД", MB_ICONQUESTION | MB_YESNO | MB_APPLMODAL) == IDYES)
		{
			for (int i = 0; i < 20; i++)
				if (!Test(i, false))
					PostQuitMessage(GetError(GetLastError()));
			if (!LoadResults())
				PostQuitMessage(GetError(GetLastError()));
			PostQuitMessage(NULL);
		}
		else
			t = true;
		break;
	default:
		return DefWindowProc(_hWnd, _Msg, _wParam, _lParam);
	}
	return 0;
}

BOOL CALLBACK DlgProc(HWND _hWnd, UINT _Msg, WPARAM _wParam, LPARAM _lParam)
{
	HWND hComb = hComb = GetDlgItem(_hWnd, IDC_COMBO1);
	LRESULT CurSel = NULL;
	wchar_t ComboBoxItems[SIZE_COMBOBOX_TEXT] = { NULL };
	switch (_Msg)
	{
	case WM_COMMAND:
		switch (_wParam)
		{
		case IDC_RADIO1:
			if (SendMessage(GetDlgItem(_hWnd, IDC_RADIO1), BM_GETCHECK, _wParam, _lParam) == BST_CHECKED)
			{
				SendMessage(hComb, CB_RESETCONTENT, NULL, NULL);

				for (int i = 1; i <= COUNT_AB_KIEV; i++)
				{
					swprintf_s(ComboBoxItems, L"Билет № %d", i);
					SendMessage(hComb, CB_ADDSTRING, NULL, (LPARAM)ComboBoxItems);
				}
				SendMessage(hComb, CB_SETCURSEL, NULL, NULL);

				SendDlgItemMessage(_hWnd, IDC_COMBO1, CB_SETCURSEL, NULL, NULL);
			}
			break;
		case IDC_RADIO2:
			if (SendMessage(GetDlgItem(_hWnd, IDC_RADIO2), BM_GETCHECK, _wParam, _lParam) == BST_CHECKED)
			{
				SendMessage(hComb, CB_RESETCONTENT, NULL, NULL);

				for (int i = 1; i <= COUNT_AB_KHARKOV; i++)
				{
					swprintf_s(ComboBoxItems, L"Билет № %d", i);
					SendMessage(hComb, CB_ADDSTRING, NULL, (LPARAM)ComboBoxItems);
				}
				SendMessage(hComb, CB_SETCURSEL, NULL, NULL);

				SendDlgItemMessage(_hWnd, IDC_COMBO1, CB_SETCURSEL, NULL, NULL);
			}
			break;
		case IDC_RADIO3:
			if (SendMessage(GetDlgItem(_hWnd, IDC_RADIO3), BM_GETCHECK, _wParam, _lParam) == BST_CHECKED)
			{
				SendMessage(hComb, CB_RESETCONTENT, NULL, NULL);

				for (int i = 1; i <= COUNT_CD_KHARKOV; i++)
				{
					swprintf_s(ComboBoxItems, L"Билет № %d", i);
					SendMessage(hComb, CB_ADDSTRING, NULL, (LPARAM)ComboBoxItems);
				}
				SendMessage(hComb, CB_SETCURSEL, NULL, NULL);

				SendDlgItemMessage(_hWnd, IDC_COMBO1, CB_SETCURSEL, NULL, NULL);
			}
			break;
		case IDC_BUTTON1:
			CurSel = SendMessage(hComb, CB_GETCURSEL, NULL, NULL);
			if (CurSel > 0)
				SendMessage(hComb, CB_SETCURSEL, (WPARAM)CurSel - 1, NULL);
			break;
		case IDC_BUTTON2:
			CurSel = SendMessage(hComb, CB_GETCURSEL, NULL, NULL);
			if (CurSel < SendMessage(hComb, CB_GETCOUNT, NULL, NULL))
				SendMessage(hComb, CB_SETCURSEL, (WPARAM)CurSel + 1, NULL);
			break;
		case IDOK:
			if (SendMessage(GetDlgItem(_hWnd, IDC_RADIO1), BM_GETCHECK, _wParam, _lParam) == BST_UNCHECKED && SendMessage(GetDlgItem(_hWnd, IDC_RADIO2), BM_GETCHECK, _wParam, _lParam) == BST_UNCHECKED && SendMessage(GetDlgItem(_hWnd, IDC_RADIO3), BM_GETCHECK, _wParam, _lParam) == BST_UNCHECKED)
			{
				MessageBox(_hWnd, L"Для начала тестирования необходимо выбрать билет!", L"Тест ПДД", MB_ICONASTERISK);
				break;
			}
			if (SendMessage(GetDlgItem(_hWnd, IDC_CHECK2), BM_GETCHECK, _wParam, _lParam) == BST_CHECKED)
			{
				Time = true;
				t = true;
			}
			else
				Time = false;
			if (SendMessage(GetDlgItem(_hWnd, IDC_RADIO1), BM_GETCHECK, _wParam, _lParam) == BST_CHECKED)
			{
				TicketNum = 1;
				swprintf_s(Ticket, L"%s\\%d.txt", PATH_AB_KIEV, SendMessage(hComb, CB_GETCURSEL, NULL, NULL) + 1);
			}
			else if (SendMessage(GetDlgItem(_hWnd, IDC_RADIO2), BM_GETCHECK, _wParam, _lParam) == BST_CHECKED)
			{
				TicketNum = 2;
				swprintf_s(Ticket, L"%s\\%d.txt", PATH_AB_KHARKOV, SendMessage(hComb, CB_GETCURSEL, NULL, NULL) + 1);
			}
			else if (SendMessage(GetDlgItem(_hWnd, IDC_RADIO3), BM_GETCHECK, _wParam, _lParam) == BST_CHECKED)
			{
				TicketNum = 3;
				swprintf_s(Ticket, L"%s\\%d.txt", PATH_CD_KHARKOV, SendMessage(hComb, CB_GETCURSEL, NULL, NULL) + 1);
			}
			EndDialog(_hWnd, DIALOG_OK);
			break;
		}
		break;
	case WM_CLOSE:
		if (MessageBox(_hWnd, L"Вы действительно хотите закрыть приложение \"Тест ПДД\"?", L"Тест ПДД", MB_ICONQUESTION | MB_YESNO | MB_APPLMODAL) == IDYES)
			EndDialog(_hWnd, DIALOG_CLOSED);
		break;
		return FALSE;
	}
	return FALSE;
}

bool LoadQuestions(LPCWSTR _Path)
{
	HANDLE hFile = NULL; // Файл с вопросами
	hFile = CreateFile(Ticket, GENERIC_READ, 2, NULL, OPEN_EXISTING, NULL, NULL); // Создание файла
	if (hFile == INVALID_HANDLE_VALUE) // Если файл с вопросами не найден
		return false;
	DWORD size_file = GetFileSize(hFile, NULL); // Размер файла

	if (size_file <= 0) // Если файл пустой
	{
		SetLastError(ERROR_FILE_CORRUPT); // Установка ошибки "Файл повреждён"
		return false;
	}

	LPWSTR buf = NULL; // Буффер для хранения данных из файла
	DWORD read = NULL; // Число прочитанных байтов

	buf = (LPWSTR)VirtualAlloc(NULL, size_file, MEM_COMMIT, PAGE_READWRITE); // Выделение памяти под данные из файла
	if (!buf)
		return false;

	if (!ReadFile(hFile, buf, size_file, &read, NULL)) // Чтение из файла
		return false;

	int i = 0;
	wchar_t *buf1 = NULL; // Указатель на следующий токен
	wchar_t *next_buf1 = NULL; // Информация позиции между вызовами wcstok_s
	
	*buf++;

	buf1 = wcstok_s(buf, L"\r\n", &next_buf1);
	while (buf1)
	{
		ZeroMemory(&Questions[i], sizeof(QUESTION)); // Обнуление структуры

		int j = 0;
		LPWSTR buf2 = NULL;
		LPWSTR next_buf2 = NULL;
		buf2 = wcstok_s(buf1, L"\u2702", &next_buf2);
		while (buf2)
		{
			if (j == 0)
				if (!wmemcpy(Questions[i].Image, buf2, wcslen(buf2)))
					return false;
			if (j == 1)
				if (!wmemcpy(Questions[i].Question, buf2, wcslen(buf2)))
					return false;
			if (j == 2)
			{
				int k = 0;
				LPWSTR buf3 = NULL;
				LPWSTR next_buf3 = NULL;
				buf3 = wcstok_s(buf2, L"\u2704", &next_buf3);
				while (buf3)
				{
					Questions[i].Answers[k] = buf3;
					buf3 = wcstok_s(NULL, L"\u2704", &next_buf3);
					k++;
				}
			}
			if (j == 3)
				Questions[i].TrueAnswer = _wtoi(buf2);
			buf2 = wcstok_s(NULL, L"\u2702", &next_buf2);
			j++;
		}
		buf1 = wcstok_s(NULL, L"\r\n", &next_buf1);
		i++;
	}
	CloseHandle(hFile);
	return true;
}

bool LoadQuestion(int _num)
{
	if (_num <= 0 || _num > 20)
		return false;

	_num--;

	if (Questions[_num].Question == NULL || Questions[_num].Answers == NULL || Questions[_num].TrueAnswer == NULL)
	{
		SetLastError(13);
		return false;
	}

	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);

	SendMessage(hStQuestion, WM_SETTEXT, NULL, (LPARAM)Questions[_num].Question);

	if (!_wcsicmp(Questions[_num].Image, L"NULL")) // Если картинки нет
	{
		if (!LoadAnswers(false, _num))
			return false;
	}
	else // Если картинка есть
	{
		if (!LoadAnswers(true, _num))
			return false;
		if (!DrawImage(_num))
			return false;
	}

	return true;
}

bool DrawImage(int _num)
{
	wchar_t str[MAX_PATH] = { NULL }; // Полный путь к картинке
	if(TicketNum == 1)
		swprintf_s(str, L"%s\\Images\\%s", PATH_AB_KIEV, Questions[_num].Image); // Формирование полного пути к картинке
	else if(TicketNum == 2)
		swprintf_s(str, L"%s\\Images\\%s", PATH_AB_KHARKOV, Questions[_num].Image); // Формирование полного пути к картинке
	else if(TicketNum == 3)
		swprintf_s(str, L"%s\\Images\\%s", PATH_CD_KHARKOV, Questions[_num].Image); // Формирование полного пути к картинке

	HDC hDC = GetWindowDC(hWnd);
	Graphics gr(hDC);
	Image *image = new Image(str);

	float w = image->GetWidth(); // Ширина картинки
	float h = image->GetHeight(); // Высота картинки

	SIZE sz = { NULL };
	GetTextExtentPoint32(GetWindowDC(hStQuestion), Questions[_num].Question, wcslen(Questions[_num].Question), &sz);
	int Cnt = sz.cx / 18 * 20 / 700;
	if ((float)Cnt < (float)sz.cx / 18 * 20 / 700)
		Cnt++;

	if (w < 200 && h < 200)
	{
		float tmp = w / (200);
		w /= tmp;
		h /= tmp;
	}
	if (w > 345)
	{
		float tmp = w / (345);
		w /= tmp;
		h /= tmp;
	}
	if (h > 300)
	{
		float tmp = h / (300);
		w /= tmp;
		h /= tmp;
	}

	gr.DrawImage(image, 15, 105 + (sz.cy + 10)*Cnt, (int)w, (int)h); // Вывод картинки в окно
	
	delete image; // Освобождение памяти
	ReleaseDC(hWnd, hDC);
	return true;
}

bool LoadAnswers(bool _IsImage, int _num)
{
	for (int k = 0; k < 5; k++)
	{
		if (hRadios[k])
			DestroyWindow(hRadios[k]);
		hRadios[k] = NULL;
	}

	int i = 0;

	HFONT hFnt = CreateFont(17, NULL, NULL, NULL, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, L"Arial");
	if (!hFnt)
		return GetError(GetLastError());

	SIZE sz = { NULL };
	GetTextExtentPoint32(GetWindowDC(hStQuestion), Questions[_num].Question, wcslen(Questions[_num].Question), &sz);
	int Cnt = sz.cx / 18 * 20 / 700;
	if ((float)Cnt < (float)sz.cx / 18 * 20 / 700)
		Cnt++;
	int pxls = 70 + (sz.cy + 10)*Cnt;

	if (_IsImage)
	{
		while (Questions[_num].Answers[i] && i < 5 && i >= 0)
		{
			if (i > 0)
			{
				GetTextExtentPoint32(GetWindowDC(hRadios[i - 1]), Questions[_num].Answers[i - 1], wcslen(Questions[_num].Answers[i - 1]), &sz);
				Cnt = sz.cx / 350;
				if ((float)Cnt < (float)sz.cx / 350)
					Cnt++;
				pxls += 20 + (sz.cy + 5)*Cnt;
			}

			GetTextExtentPoint32(GetWindowDC(hRadios[i]), Questions[_num].Answers[i], wcslen(Questions[_num].Answers[i]), &sz);
			Cnt = sz.cx / 350;
			if ((float)Cnt < (float)sz.cx / 350)
				Cnt++;
			Cnt++;

			hRadios[i] = CreateWindowEx(NULL, L"BUTTON", Questions[_num].Answers[i], WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | BS_MULTILINE | BS_VCENTER, 360, pxls, 350, 18 * Cnt, hWnd, (HMENU)ID_RADIOS[i], hinstance, NULL);
			if (!hRadios[i])
				return false;
			SendMessage(hRadios[i], WM_SETFONT, (WPARAM)hFnt, TRUE);

			Cnt = 0;

			i++;
		}
	}
	else
	{
		while (Questions[_num].Answers[i] && i < 5 && i >= 0)
		{
			if (i > 0)
			{
				GetTextExtentPoint32(GetWindowDC(hRadios[i - 1]), Questions[_num].Answers[i - 1], wcslen(Questions[_num].Answers[i - 1]), &sz);
				Cnt = sz.cx / 675;
				if ((float)Cnt < (float)sz.cx / 675)
					Cnt++;
				pxls += 20 + (sz.cy + 5)*Cnt;
			}

			GetTextExtentPoint32(GetWindowDC(hRadios[i]), Questions[_num].Answers[i], wcslen(Questions[_num].Answers[i]), &sz);
			Cnt = sz.cx / 675;
			if ((float)Cnt < (float)sz.cx / 675)
				Cnt++;
			Cnt++;

			hRadios[i] = CreateWindowEx(NULL, L"BUTTON", Questions[_num].Answers[i], WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | BS_MULTILINE | BS_VCENTER, 10, pxls, 700, 18 * Cnt, hWnd, (HMENU)ID_RADIOS[i], hinstance, NULL);
			if (!hRadios[i])
				return false;
			SendMessage(hRadios[i], WM_SETFONT, (WPARAM)hFnt, TRUE);
			i++;
		}
	}
	SendMessage(hRadios[Questions[_num].buf - 1], BM_CLICK, NULL, NULL);
	if (Questions[_num].IsAnswered || IsFinished)
		for (int k = 0; k < i; k++)
			EnableWindow(hRadios[k], FALSE);
	UpdateWindow(hWnd);
	return true;
}

bool Test(int _num, bool _ShowTrueAnswer)
{
	if (!Questions[_num].IsAnswered)
	{
		if (Questions[_num].buf == Questions[_num].TrueAnswer)
			HighScore++;
		if (_ShowTrueAnswer)
			CountQuestions++;
		Questions[_num].IsAnswered = true;
	}
	return true;
}

bool LoadResults()
{
	wchar_t str[SIZE_TEXT] = { NULL };

	IsFinished = true;

	swprintf_s(str, L"Правильных ответов: %d (%d%%)", HighScore, (int)(HighScore * 100 / 20));
	if (Time)
	{
		if (hTimer)
			if (!KillTimer(hWnd, ID_TIMER))
				return false;

		wchar_t time[SIZE_TIME_TEXT] = { NULL };
		int len = GetWindowTextLength(hStatic); // Длина текста в поле ввода
		if (len)
		{
			SendMessage(hStatic, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)&time[0]);
		}
		swprintf_s(str, L"%s\r\nВремя: %s", str, time);
	}

	if (HighScore < 18)
		swprintf_s(str, L"%s\r\nРезультат: Не сдал.", str);
	else
		swprintf_s(str, L"%s\r\nРезультат: Сдал.", str);

	MessageBox(hWnd, str, L"Результат тестирования", NULL);
	return true;
}

bool ReloadTest(void)
{
	ShowWindow(hWnd, SW_HIDE);

	hTimer = NULL;

	if (Time)
		if (hStatic)
		{
			DestroyWindow(hStatic);
			hStatic = NULL;
		}

	ZeroMemory(&Questions, sizeof(QUESTION) * COUNT_QUESTIONS);

	Minutes = NULL;
	Seconds = NULL;

	SendMessage(hStQuestion, WM_SETTEXT, NULL, (LPARAM)L"");

	for (int i = 0; i < 5; i++)
		if (hRadios[i])
			DestroyWindow(hRadios[i]);

	Time = false;
	t = false;
	IsFinished = false;
	ZeroMemory(&Ticket, sizeof(Ticket));
	CurrentQuestion = 1;
	HighScore = 0;
	CountQuestions = 0;

	INT_PTR result_dialog = DialogBoxParam(hinstance, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DlgProc), NULL);
	if (!result_dialog)
		return false;
	else if (result_dialog == DIALOG_CLOSED)
	{
		DialogClosed = true;
		return true;
	}

	if (Time)
	{
		hStatic = CreateWindowEx(NULL, L"STATIC", L"00:00", WS_CHILD | WS_VISIBLE, 630, 520, 100, 30, ::hWnd, (HMENU)ID_STATIC_TIME, hinstance, NULL);
		if (!hStatic)
			return false;

		HFONT hFnt = CreateFont(35, NULL, NULL, NULL, FW_BLACK, FALSE, FALSE, FALSE, OEM_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_ROMAN, L"Arial");
		if (!hFnt)
			return false;

		SendMessage(hStatic, WM_SETFONT, (WPARAM)hFnt, TRUE);

		hTimer = SetTimer(hWnd, ID_TIMER, 1000, NULL);
		if (!hTimer)
			return false;
	}

	if (!LoadQuestions(Ticket))
		return false;

	ShowWindow(hWnd, SW_SHOW);

	if (!LoadQuestion(CurrentQuestion))
		return false;

	UpdateWindow(hWnd);

	return true;
}

DWORD GetError(DWORD _Error)
{
	LPVOID lperr = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, _Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lperr, NULL, NULL);
	MessageBox(hWnd, (LPCWSTR)lperr, NULL, MB_ICONERROR);
	LocalFree(lperr);

	return _Error;
}