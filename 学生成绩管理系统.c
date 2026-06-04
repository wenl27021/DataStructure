/*
Student score management - Win32 version.
Core data structure: singly linked list with a head node.
Data file: students.txt.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define STUDENT_FILE "students.txt"
#define STUDENT_COUNT 30
#define COURSE_COUNT 3

#define IDC_STU_OUTPUT 1001
#define IDC_STU_NAME 1002
#define IDC_STU_ID 1003
#define IDC_STU_COURSE 1004
#define IDC_STU_SCORE 1005
#define IDC_STU_RANK 1006
#define IDC_STU_THRESHOLD 1007

#define IDC_STU_GENERATE 1101
#define IDC_STU_DISPLAY 1102
#define IDC_STU_SORT_NAME 1103
#define IDC_STU_SORT_TOTAL 1104
#define IDC_STU_SEARCH_NAME 1105
#define IDC_STU_SEARCH_RANK 1106
#define IDC_STU_MODIFY 1107
#define IDC_STU_DELETE 1108
#define IDC_STU_STATS 1109
#define IDC_STU_SAVE 1110

typedef struct Student {
    char id[20];
    char name[40];
    int score[COURSE_COUNT];
    int total;
    double average;
} Student;

typedef struct Node {
    Student data;
    struct Node* next;
} Node;

static Node* g_head = NULL;
static HWND g_stuOutput = NULL;
static HWND g_stuName = NULL;
static HWND g_stuId = NULL;
static HWND g_stuCourse = NULL;
static HWND g_stuScore = NULL;
static HWND g_stuRank = NULL;
static HWND g_stuThreshold = NULL;
static HFONT g_stuFont = NULL;
static HBRUSH g_stuBrush = NULL;
static int g_stuRunning = 0;

static void calculate(Student* stu);
static void generateData(void);
static Node* createList(void);
static void destroyList(Node* head);
static int fileExists(const char* fileName);
static void saveToFile(Node* head);
static void appendText(char* out, int outSize, const char* text);
static void appendFormat(char* out, int outSize, const char* fmt, ...);
static void appendHeader(char* out, int outSize);
static void appendStudent(char* out, int outSize, const Student* stu);
static void buildAllText(char* out, int outSize);
static void sortByName(Node* head);
static void sortByTotal(Node* head);
static void searchByName(Node* head, const char* name, char* out, int outSize);
static void searchByRank(Node* head, int rank, char* out, int outSize);
static int modifyScore(Node* head, const char* id, int courseNo, int newScore);
static int deleteBelow(Node* head, int threshold);
static void statistics(Node* head, char* out, int outSize);
static int listLength(Node* head);
static void reloadList(void);
static int readEditInt(HWND edit, int* value);
static void readEditText(HWND edit, char* text, int size);
static void setOutput(const char* text);
static void createStudentControls(HWND hwnd);
static void setStudentFonts(HWND hwnd);
static LRESULT CALLBACK StudentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int runStudentScoreSystem(void)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    HINSTANCE instance;

    instance = GetModuleHandleA(NULL);
    srand((unsigned int)time(NULL));

    if (!fileExists(STUDENT_FILE)) {
        generateData();
    }
    reloadList();

    g_stuBrush = CreateSolidBrush(RGB(248, 251, 255));
    g_stuFont = CreateFontA(
        17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Consolas");

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = StudentWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = "StudentScoreWin32Window";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_stuBrush;
    RegisterClassA(&wc);

    hwnd = CreateWindowExA(0, "StudentScoreWin32Window", "Student Score Management",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1050, 720,
        NULL, NULL, instance, NULL);

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Unable to create student window.", "Error", MB_ICONERROR);
        return 1;
    }

    g_stuRunning = 1;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    while (g_stuRunning && GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    destroyList(g_head);
    g_head = NULL;
    if (g_stuFont != NULL) {
        DeleteObject(g_stuFont);
        g_stuFont = NULL;
    }
    if (g_stuBrush != NULL) {
        DeleteObject(g_stuBrush);
        g_stuBrush = NULL;
    }
    return 0;
}

static LRESULT CALLBACK StudentWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char out[12000];
    char text[80];
    int course;
    int score;
    int rank;
    int threshold;
    int deleted;

    (void)lParam;
    out[0] = '\0';

    switch (msg) {
    case WM_CREATE:
        createStudentControls(hwnd);
        buildAllText(out, sizeof(out));
        setOutput(out);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_STU_GENERATE:
            generateData();
            reloadList();
            buildAllText(out, sizeof(out));
            setOutput(out);
            break;

        case IDC_STU_DISPLAY:
            buildAllText(out, sizeof(out));
            setOutput(out);
            break;

        case IDC_STU_SORT_NAME:
            sortByName(g_head);
            buildAllText(out, sizeof(out));
            setOutput(out);
            break;

        case IDC_STU_SORT_TOTAL:
            sortByTotal(g_head);
            buildAllText(out, sizeof(out));
            setOutput(out);
            break;

        case IDC_STU_SEARCH_NAME:
            readEditText(g_stuName, text, sizeof(text));
            searchByName(g_head, text, out, sizeof(out));
            setOutput(out);
            break;

        case IDC_STU_SEARCH_RANK:
            if (readEditInt(g_stuRank, &rank)) {
                searchByRank(g_head, rank, out, sizeof(out));
                setOutput(out);
            }
            else {
                setOutput("Invalid rank.");
            }
            break;

        case IDC_STU_MODIFY:
            readEditText(g_stuId, text, sizeof(text));
            if (readEditInt(g_stuCourse, &course) && readEditInt(g_stuScore, &score)
                && modifyScore(g_head, text, course, score)) {
                saveToFile(g_head);
                buildAllText(out, sizeof(out));
                appendText(out, sizeof(out), "\r\nScore modified and saved.");
                setOutput(out);
            }
            else {
                setOutput("Modify failed. Check id, course number, and score.");
            }
            break;

        case IDC_STU_DELETE:
            if (readEditInt(g_stuThreshold, &threshold)) {
                deleted = deleteBelow(g_head, threshold);
                saveToFile(g_head);
                buildAllText(out, sizeof(out));
                appendFormat(out, sizeof(out), "\r\nDeleted students: %d", deleted);
                setOutput(out);
            }
            else {
                setOutput("Invalid threshold.");
            }
            break;

        case IDC_STU_STATS:
            statistics(g_head, out, sizeof(out));
            setOutput(out);
            break;

        case IDC_STU_SAVE:
            saveToFile(g_head);
            setOutput("Student data saved to students.txt.");
            break;
        }
        return 0;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wParam, RGB(248, 251, 255));
        SetTextColor((HDC)wParam, RGB(31, 41, 55));
        return (LRESULT)g_stuBrush;

    case WM_DESTROY:
        g_stuRunning = 0;
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void createStudentControls(HWND hwnd)
{
    CreateWindowExA(0, "STATIC", "Name:", WS_CHILD | WS_VISIBLE, 22, 18, 55, 24, hwnd, NULL, NULL, NULL);
    g_stuName = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "Zhang", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        80, 16, 140, 26, hwnd, (HMENU)IDC_STU_NAME, NULL, NULL);

    CreateWindowExA(0, "STATIC", "ID:", WS_CHILD | WS_VISIBLE, 236, 18, 35, 24, hwnd, NULL, NULL, NULL);
    g_stuId = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "20260001", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        270, 16, 100, 26, hwnd, (HMENU)IDC_STU_ID, NULL, NULL);

    CreateWindowExA(0, "STATIC", "Course:", WS_CHILD | WS_VISIBLE, 386, 18, 65, 24, hwnd, NULL, NULL, NULL);
    g_stuCourse = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "1", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        452, 16, 42, 26, hwnd, (HMENU)IDC_STU_COURSE, NULL, NULL);

    CreateWindowExA(0, "STATIC", "Score:", WS_CHILD | WS_VISIBLE, 510, 18, 55, 24, hwnd, NULL, NULL, NULL);
    g_stuScore = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "90", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        565, 16, 52, 26, hwnd, (HMENU)IDC_STU_SCORE, NULL, NULL);

    CreateWindowExA(0, "STATIC", "Rank:", WS_CHILD | WS_VISIBLE, 632, 18, 50, 24, hwnd, NULL, NULL, NULL);
    g_stuRank = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "1", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        680, 16, 52, 26, hwnd, (HMENU)IDC_STU_RANK, NULL, NULL);

    CreateWindowExA(0, "STATIC", "Threshold:", WS_CHILD | WS_VISIBLE, 748, 18, 88, 24, hwnd, NULL, NULL, NULL);
    g_stuThreshold = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "180", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        840, 16, 66, 26, hwnd, (HMENU)IDC_STU_THRESHOLD, NULL, NULL);

    CreateWindowExA(0, "BUTTON", "Generate", WS_CHILD | WS_VISIBLE, 22, 58, 90, 32, hwnd, (HMENU)IDC_STU_GENERATE, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Display", WS_CHILD | WS_VISIBLE, 120, 58, 82, 32, hwnd, (HMENU)IDC_STU_DISPLAY, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Sort Name", WS_CHILD | WS_VISIBLE, 210, 58, 95, 32, hwnd, (HMENU)IDC_STU_SORT_NAME, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Sort Total", WS_CHILD | WS_VISIBLE, 313, 58, 95, 32, hwnd, (HMENU)IDC_STU_SORT_TOTAL, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Search Name", WS_CHILD | WS_VISIBLE, 416, 58, 110, 32, hwnd, (HMENU)IDC_STU_SEARCH_NAME, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Rank", WS_CHILD | WS_VISIBLE, 534, 58, 70, 32, hwnd, (HMENU)IDC_STU_SEARCH_RANK, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Modify", WS_CHILD | WS_VISIBLE, 612, 58, 80, 32, hwnd, (HMENU)IDC_STU_MODIFY, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Delete", WS_CHILD | WS_VISIBLE, 700, 58, 80, 32, hwnd, (HMENU)IDC_STU_DELETE, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Stats", WS_CHILD | WS_VISIBLE, 788, 58, 70, 32, hwnd, (HMENU)IDC_STU_STATS, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Save", WS_CHILD | WS_VISIBLE, 866, 58, 70, 32, hwnd, (HMENU)IDC_STU_SAVE, NULL, NULL);

    g_stuOutput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL | WS_HSCROLL,
        22, 108, 980, 540, hwnd, (HMENU)IDC_STU_OUTPUT, NULL, NULL);

    setStudentFonts(hwnd);
}

static void setStudentFonts(HWND hwnd)
{
    HWND child;
    child = GetWindow(hwnd, GW_CHILD);
    while (child != NULL) {
        SendMessageA(child, WM_SETFONT, (WPARAM)g_stuFont, TRUE);
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

static void calculate(Student* stu)
{
    stu->total = stu->score[0] + stu->score[1] + stu->score[2];
    stu->average = stu->total / 3.0;
}

static void generateData(void)
{
    const char* surnames[] = { "Zhang", "Wang", "Li", "Zhao", "Chen", "Liu", "Yang", "Huang", "Zhou", "Wu" };
    const char* givenNames[] = { "San", "Si", "Wei", "Fang", "Ming", "Lei", "Jie", "Na", "Qiang", "Yan" };
    FILE* fp = NULL;
    int i;
    char id[20] = { 0 };

    if (fopen_s(&fp, STUDENT_FILE, "w") != 0 || fp == NULL) {
        return;
    }

    for (i = 0; i < STUDENT_COUNT; ++i) {
        sprintf_s(id, sizeof(id), "2026%04d", i + 1);
        fprintf(fp, "%s %s %s %d %d %d\n", id,
            surnames[rand() % 10], givenNames[rand() % 10],
            50 + rand() % 51, 50 + rand() % 51, 50 + rand() % 51);
    }
    fclose(fp);
}

static Node* createList(void)
{
    FILE* fp = NULL;
    Node* head;
    Node* tail;
    char id[20] = { 0 };
    char surname[20] = { 0 };
    char givenName[20] = { 0 };
    int s1;
    int s2;
    int s3;

    if (fopen_s(&fp, STUDENT_FILE, "r") != 0 || fp == NULL) {
        return NULL;
    }

    head = (Node*)malloc(sizeof(Node));
    if (head == NULL) {
        fclose(fp);
        return NULL;
    }
    head->next = NULL;
    tail = head;

    while (fscanf_s(fp, "%19s %19s %19s %d %d %d",
        id, (unsigned int)sizeof(id),
        surname, (unsigned int)sizeof(surname),
        givenName, (unsigned int)sizeof(givenName),
        &s1, &s2, &s3) == 6) {
        Node* node = (Node*)malloc(sizeof(Node));
        if (node == NULL) {
            break;
        }
        strcpy_s(node->data.id, sizeof(node->data.id), id);
        sprintf_s(node->data.name, sizeof(node->data.name), "%s %s", surname, givenName);
        node->data.score[0] = s1;
        node->data.score[1] = s2;
        node->data.score[2] = s3;
        calculate(&node->data);
        node->next = NULL;
        tail->next = node;
        tail = node;
    }

    fclose(fp);
    return head;
}

static void destroyList(Node* head)
{
    while (head != NULL) {
        Node* next = head->next;
        free(head);
        head = next;
    }
}

static int fileExists(const char* fileName)
{
    FILE* fp = NULL;
    if (fopen_s(&fp, fileName, "r") != 0 || fp == NULL) {
        return 0;
    }
    fclose(fp);
    return 1;
}

static void saveToFile(Node* head)
{
    FILE* fp = NULL;
    Node* p;

    if (fopen_s(&fp, STUDENT_FILE, "w") != 0 || fp == NULL || head == NULL) {
        return;
    }
    for (p = head->next; p != NULL; p = p->next) {
        fprintf(fp, "%s %s %d %d %d\n", p->data.id, p->data.name,
            p->data.score[0], p->data.score[1], p->data.score[2]);
    }
    fclose(fp);
}

static void appendText(char* out, int outSize, const char* text)
{
    size_t used = strlen(out);
    if ((int)used < outSize - 1) {
        strncat_s(out, outSize, text, outSize - used - 1);
    }
}

static void appendFormat(char* out, int outSize, const char* fmt, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    appendText(out, outSize, buffer);
}

static void appendHeader(char* out, int outSize)
{
    appendFormat(out, outSize, "%-11s %-17s %6s  %6s  %6s  %6s  %8s\r\n",
        "ID", "Name", "C1", "C2", "C3", "Total", "Average");
    appendText(out, outSize, "----------------------------------------------------------------------\r\n");
}

static void appendStudent(char* out, int outSize, const Student* stu)
{
    appendFormat(out, outSize, "%-11s %-17s %6d  %6d  %6d  %6d  %8.2f\r\n",
        stu->id, stu->name, stu->score[0], stu->score[1], stu->score[2], stu->total, stu->average);
}

static void buildAllText(char* out, int outSize)
{
    Node* p;
    out[0] = '\0';
    if (g_head == NULL || g_head->next == NULL) {
        appendText(out, outSize, "No student data.");
        return;
    }
    appendHeader(out, outSize);
    for (p = g_head->next; p != NULL; p = p->next) {
        appendStudent(out, outSize, &p->data);
    }
}

static void sortByName(Node* head)
{
    Node* sorted = NULL;
    Node* cur;
    if (head == NULL) {
        return;
    }
    cur = head->next;
    while (cur != NULL) {
        Node* next = cur->next;
        if (sorted == NULL || strcmp(cur->data.name, sorted->data.name) < 0) {
            cur->next = sorted;
            sorted = cur;
        }
        else {
            Node* p = sorted;
            while (p->next != NULL && strcmp(p->next->data.name, cur->data.name) <= 0) {
                p = p->next;
            }
            cur->next = p->next;
            p->next = cur;
        }
        cur = next;
    }
    head->next = sorted;
}

static void sortByTotal(Node* head)
{
    Node* sorted = NULL;
    Node* cur;
    if (head == NULL) {
        return;
    }
    cur = head->next;
    while (cur != NULL) {
        Node* next = cur->next;
        if (sorted == NULL || cur->data.total > sorted->data.total) {
            cur->next = sorted;
            sorted = cur;
        }
        else {
            Node* p = sorted;
            while (p->next != NULL && p->next->data.total >= cur->data.total) {
                p = p->next;
            }
            cur->next = p->next;
            p->next = cur;
        }
        cur = next;
    }
    head->next = sorted;
}

static void searchByName(Node* head, const char* name, char* out, int outSize)
{
    Node* p;
    int found = 0;
    out[0] = '\0';
    appendHeader(out, outSize);
    for (p = head->next; p != NULL; p = p->next) {
        if (strstr(p->data.name, name) != NULL) {
            appendStudent(out, outSize, &p->data);
            found = 1;
        }
    }
    if (!found) {
        appendText(out, outSize, "No matching student.");
    }
}

static void searchByRank(Node* head, int rank, char* out, int outSize)
{
    Node* p;
    int i;
    int len = listLength(head);
    out[0] = '\0';
    if (rank < 1 || rank > len) {
        appendFormat(out, outSize, "Invalid rank. Student count: %d", len);
        return;
    }
    sortByTotal(head);
    p = head->next;
    for (i = 1; i < rank; ++i) {
        p = p->next;
    }
    appendHeader(out, outSize);
    appendStudent(out, outSize, &p->data);
}

static int modifyScore(Node* head, const char* id, int courseNo, int newScore)
{
    Node* p;
    if (courseNo < 1 || courseNo > 3 || newScore < 0 || newScore > 100 || head == NULL) {
        return 0;
    }
    for (p = head->next; p != NULL; p = p->next) {
        if (strcmp(p->data.id, id) == 0) {
            p->data.score[courseNo - 1] = newScore;
            calculate(&p->data);
            return 1;
        }
    }
    return 0;
}

static int deleteBelow(Node* head, int threshold)
{
    Node* prev;
    Node* cur;
    int count = 0;
    if (head == NULL) {
        return 0;
    }
    prev = head;
    cur = head->next;
    while (cur != NULL) {
        if (cur->data.total < threshold) {
            prev->next = cur->next;
            free(cur);
            cur = prev->next;
            ++count;
        }
        else {
            prev = cur;
            cur = cur->next;
        }
    }
    return count;
}

static void statistics(Node* head, char* out, int outSize)
{
    int sum[3] = { 0, 0, 0 };
    int maxScore[3] = { 0, 0, 0 };
    int minScore[3] = { 101, 101, 101 };
    int count = 0;
    int i;
    Node* p;
    out[0] = '\0';
    if (head == NULL || head->next == NULL) {
        appendText(out, outSize, "No student data.");
        return;
    }
    for (p = head->next; p != NULL; p = p->next) {
        for (i = 0; i < 3; ++i) {
            sum[i] += p->data.score[i];
            if (p->data.score[i] > maxScore[i]) {
                maxScore[i] = p->data.score[i];
            }
            if (p->data.score[i] < minScore[i]) {
                minScore[i] = p->data.score[i];
            }
        }
        ++count;
    }
    appendText(out, outSize, "Course      Average       Max       Min\r\n");
    appendText(out, outSize, "------------------------------------------\r\n");
    for (i = 0; i < 3; ++i) {
        appendFormat(out, outSize, "Course %d   %8.2f  %8d  %8d\r\n", i + 1, sum[i] * 1.0 / count, maxScore[i], minScore[i]);
    }
}

static int listLength(Node* head)
{
    int count = 0;
    Node* p;
    if (head == NULL) {
        return 0;
    }
    for (p = head->next; p != NULL; p = p->next) {
        ++count;
    }
    return count;
}

static void reloadList(void)
{
    destroyList(g_head);
    g_head = createList();
}

static int readEditInt(HWND edit, int* value)
{
    char text[32] = { 0 };
    GetWindowTextA(edit, text, sizeof(text));
    return sscanf_s(text, "%d", value) == 1;
}

static void readEditText(HWND edit, char* text, int size)
{
    GetWindowTextA(edit, text, size);
}

static void setOutput(const char* text)
{
    if (g_stuOutput != NULL) {
        SetWindowTextA(g_stuOutput, text);
    }
}
