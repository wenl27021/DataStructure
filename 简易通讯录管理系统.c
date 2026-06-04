/*
Simple contact management - Win32 version.
Core data structure: doubly circular linked list with a head node.
Deleting a current node uses cur->prev and cur->next directly:
cur->prev->next = cur->next; cur->next->prev = cur->prev; then free(cur).
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define CONTACT_FILE "contacts.txt"
#define CONTACT_COUNT 20

#define IDC_CON_OUTPUT 2001
#define IDC_CON_ID 2002
#define IDC_CON_NAME 2003
#define IDC_CON_PHONE 2004
#define IDC_CON_GROUP 2005
#define IDC_CON_TARGET 2006
#define IDC_CON_REMARK 2007

#define IDC_CON_GENERATE 2101
#define IDC_CON_FORWARD 2102
#define IDC_CON_BACKWARD 2103
#define IDC_CON_APPEND 2104
#define IDC_CON_BEFORE 2105
#define IDC_CON_DELETE 2106
#define IDC_CON_SEARCH_NAME 2107
#define IDC_CON_SEARCH_GROUP 2108
#define IDC_CON_MODIFY 2109
#define IDC_CON_SORT 2110
#define IDC_CON_DELETE_PHONE 2111

#define IDC_PROMPT_EDIT 3001
#define IDC_PROMPT_OK 3002
#define IDC_PROMPT_CANCEL 3003
#define IDC_CHOICE_FIRST 3101
#define IDC_CHOICE_SECOND 3102
#define IDC_CHOICE_CANCEL 3103

typedef struct Contact {
    int id;
    char name[40];
    char phone1[20];
    char phone2[20];
    char group[64];
    char remark[80];
} Contact;

typedef struct DblNode {
    Contact data;
    struct DblNode* prev;
    struct DblNode* next;
} DblNode;

typedef struct TextPromptState {
    HWND edit;
    char* out;
    int outSize;
    int allowEmpty;
    int done;
    int result;
    const char* label;
} TextPromptState;

typedef struct ChoicePromptState {
    const char* message;
    const char* firstText;
    const char* secondText;
    const char* cancelText;
    int showCancel;
    int done;
    int result;
} ChoicePromptState;

static DblNode* g_contactHead = NULL;
static HWND g_conOutput = NULL;
static HWND g_conId = NULL;
static HWND g_conName = NULL;
static HWND g_conPhone = NULL;
static HWND g_conGroup = NULL;
static HWND g_conTarget = NULL;
static HWND g_conRemark = NULL;
static HFONT g_conFont = NULL;
static HBRUSH g_conBrush = NULL;
static int g_conRunning = 0;

static void generateContacts(void);
static DblNode* createDblList(void);
static void destroyList(DblNode* head);
static int fileExists(const char* fileName);
static void saveToFile(DblNode* head);
static void insertNodeBefore(DblNode* pos, DblNode* node);
static void appendText(char* out, int outSize, const char* text);
static void appendFormat(char* out, int outSize, const char* fmt, ...);
static void appendHeader(char* out, int outSize);
static void appendContact(char* out, int outSize, const Contact* contact);
static void buildForwardText(DblNode* head, char* out, int outSize);
static void buildBackwardText(DblNode* head, char* out, int outSize);
static void searchByName(DblNode* head, const char* name, char* out, int outSize);
static void searchByGroup(DblNode* head, const char* group, char* out, int outSize);
static int deleteById(DblNode* head, int id);
static int deletePhoneById(HWND hwnd, DblNode* head, int id, char* out, int outSize);
static int modifyPhone(HWND hwnd, DblNode* head, int id, const char* phone, char* out, int outSize);
static void sortById(DblNode* head);
static void sortByName(DblNode* head);
static int isValidPhone(const char* phone);
static int isValidGroup(const char* group);
static int groupMatches(const char* stored, const char* query);
static void normalizeGroup(char* group, int size);
static void sanitizeText(char* text);
static DblNode* findById(DblNode* head, int id);
static DblNode* findByName(DblNode* head, const char* name);
static int phoneExists(DblNode* head, const char* phone, DblNode* excludeNode, int excludeSlot);
static int addPhoneToContact(DblNode* node, const char* phone);
static int appendContactWithRules(HWND hwnd, DblNode* head, Contact* contact, DblNode* before, char* out, int outSize);
static void removeNode(DblNode* node);
static int choosePhoneSlot(HWND hwnd, const Contact* contact);
static int promptChoice(HWND owner, const char* title, const char* message,
    const char* firstText, const char* secondText, const char* cancelText, int showCancel);
static int promptTextInput(HWND owner, const char* title, const char* label, char* out, int outSize, int allowEmpty);
static void reloadContacts(void);
static int readEditInt(HWND edit, int* value);
static void readEditText(HWND edit, char* text, int size);
static int readContactFromEdits(Contact* contact);
static void setOutput(const char* text);
static void createContactControls(HWND hwnd);
static void setContactFonts(HWND hwnd);
static void initGroupCombo(void);
static void handleGroupSelection(HWND hwnd);
static LRESULT CALLBACK ChoicePromptProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK TextPromptProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK ContactWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int runContactSystem(void)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    HINSTANCE instance;

    instance = GetModuleHandleA(NULL);
    srand((unsigned int)time(NULL));

    if (!fileExists(CONTACT_FILE)) {
        generateContacts();
    }
    reloadContacts();

    g_conBrush = CreateSolidBrush(RGB(248, 251, 255));
    g_conFont = CreateFontA(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Consolas");

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = ContactWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = "ContactWin32Window";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_conBrush;
    RegisterClassA(&wc);

    hwnd = CreateWindowExA(0, "ContactWin32Window", "Simple Contact Management",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1220, 740,
        NULL, NULL, instance, NULL);

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Unable to create contact window.", "Error", MB_ICONERROR);
        return 1;
    }

    g_conRunning = 1;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    while (g_conRunning && GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    destroyList(g_contactHead);
    g_contactHead = NULL;
    if (g_conFont != NULL) {
        DeleteObject(g_conFont);
        g_conFont = NULL;
    }
    if (g_conBrush != NULL) {
        DeleteObject(g_conBrush);
        g_conBrush = NULL;
    }
    return 0;
}

static LRESULT CALLBACK ContactWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char out[16000];
    char text[80];
    int id;
    Contact contact;

    (void)lParam;
    out[0] = '\0';

    switch (msg) {
    case WM_CREATE:
        createContactControls(hwnd);
        buildForwardText(g_contactHead, out, sizeof(out));
        setOutput(out);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CON_GROUP && HIWORD(wParam) == CBN_SELCHANGE) {
            handleGroupSelection(hwnd);
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_CON_GENERATE:
            generateContacts();
            reloadContacts();
            buildForwardText(g_contactHead, out, sizeof(out));
            setOutput(out);
            break;

        case IDC_CON_FORWARD:
            buildForwardText(g_contactHead, out, sizeof(out));
            setOutput(out);
            break;

        case IDC_CON_BACKWARD:
            buildBackwardText(g_contactHead, out, sizeof(out));
            setOutput(out);
            break;

        case IDC_CON_APPEND:
            if (readContactFromEdits(&contact)) {
                appendContactWithRules(hwnd, g_contactHead, &contact, g_contactHead, out, sizeof(out));
                setOutput(out);
            }
            else {
                setOutput("Invalid contact data. Phone must have 11 digits and Group must be Classmate/Colleague/Friend/Family/Other(...).");
            }
            break;

        case IDC_CON_BEFORE:
            if (readEditInt(g_conTarget, &id) && readContactFromEdits(&contact)) {
                DblNode* target = findById(g_contactHead, id);
                if (target != NULL) {
                    appendContactWithRules(hwnd, g_contactHead, &contact, target, out, sizeof(out));
                    setOutput(out);
                }
                else {
                    setOutput("Target id not found.");
                }
            }
            else {
                setOutput("Invalid target id or contact data.");
            }
            break;

        case IDC_CON_DELETE:
            if (readEditInt(g_conId, &id) && deleteById(g_contactHead, id)) {
                saveToFile(g_contactHead);
                buildForwardText(g_contactHead, out, sizeof(out));
                setOutput(out);
            }
            else {
                setOutput("Delete failed. Id not found.");
            }
            break;

        case IDC_CON_SEARCH_NAME:
            readEditText(g_conName, text, sizeof(text));
            searchByName(g_contactHead, text, out, sizeof(out));
            setOutput(out);
            break;

        case IDC_CON_SEARCH_GROUP:
            readEditText(g_conGroup, text, sizeof(text));
            searchByGroup(g_contactHead, text, out, sizeof(out));
            setOutput(out);
            break;

        case IDC_CON_MODIFY:
            readEditText(g_conPhone, text, sizeof(text));
            if (readEditInt(g_conId, &id) && modifyPhone(hwnd, g_contactHead, id, text, out, sizeof(out))) {
                saveToFile(g_contactHead);
                buildForwardText(g_contactHead, out, sizeof(out));
                setOutput(out);
            }
            else {
                if (out[0] == '\0') {
                    setOutput("Modify failed. Check id and phone.");
                }
                else {
                    setOutput(out);
                }
            }
            break;

        case IDC_CON_DELETE_PHONE:
            if (readEditInt(g_conId, &id) && deletePhoneById(hwnd, g_contactHead, id, out, sizeof(out))) {
                saveToFile(g_contactHead);
                buildForwardText(g_contactHead, out, sizeof(out));
                setOutput(out);
            }
            else {
                if (out[0] == '\0') {
                    setOutput("Delete phone failed. Check id.");
                }
                else {
                    setOutput(out);
                }
            }
            break;

        case IDC_CON_SORT:
        {
            int sortChoice = promptChoice(hwnd, "Sort Contacts",
                "Choose a sort key.\nThe contacts will be sorted in ascending order.",
                "Id", "Name", NULL, 0);
            if (sortChoice == 1) {
                sortById(g_contactHead);
                saveToFile(g_contactHead);
                buildForwardText(g_contactHead, out, sizeof(out));
                appendText(out, sizeof(out), "\r\nSorted by Id in ascending order.");
                setOutput(out);
            }
            else if (sortChoice == 2) {
                sortByName(g_contactHead);
                saveToFile(g_contactHead);
                buildForwardText(g_contactHead, out, sizeof(out));
                appendText(out, sizeof(out), "\r\nSorted by Name in ascending order.");
                setOutput(out);
            }
            break;
        }
        }
        return 0;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wParam, RGB(248, 251, 255));
        SetTextColor((HDC)wParam, RGB(31, 41, 55));
        return (LRESULT)g_conBrush;

    case WM_DESTROY:
        g_conRunning = 0;
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void createContactControls(HWND hwnd)
{
    CreateWindowExA(0, "STATIC", "ID:", WS_CHILD | WS_VISIBLE, 22, 18, 35, 24, hwnd, NULL, NULL, NULL);
    g_conId = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "21", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        60, 16, 60, 26, hwnd, (HMENU)IDC_CON_ID, NULL, NULL);
    CreateWindowExA(0, "STATIC", "Name:", WS_CHILD | WS_VISIBLE, 134, 18, 55, 24, hwnd, NULL, NULL, NULL);
    g_conName = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "Zhang San", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        190, 16, 130, 26, hwnd, (HMENU)IDC_CON_NAME, NULL, NULL);
    CreateWindowExA(0, "STATIC", "Phone:", WS_CHILD | WS_VISIBLE, 334, 18, 60, 24, hwnd, NULL, NULL, NULL);
    g_conPhone = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "13800000000", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        396, 16, 125, 26, hwnd, (HMENU)IDC_CON_PHONE, NULL, NULL);
    CreateWindowExA(0, "STATIC", "Group:", WS_CHILD | WS_VISIBLE, 536, 18, 65, 24, hwnd, NULL, NULL, NULL);
    g_conGroup = CreateWindowExA(WS_EX_CLIENTEDGE, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_HASSTRINGS | WS_VSCROLL,
        600, 16, 145, 160, hwnd, (HMENU)IDC_CON_GROUP, NULL, NULL);
    CreateWindowExA(0, "STATIC", "Remark:", WS_CHILD | WS_VISIBLE, 760, 18, 70, 24, hwnd, NULL, NULL, NULL);
    g_conRemark = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        832, 16, 160, 26, hwnd, (HMENU)IDC_CON_REMARK, NULL, NULL);
    CreateWindowExA(0, "STATIC", "Target:", WS_CHILD | WS_VISIBLE, 1006, 18, 65, 24, hwnd, NULL, NULL, NULL);
    g_conTarget = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "1", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        1072, 16, 60, 26, hwnd, (HMENU)IDC_CON_TARGET, NULL, NULL);

    CreateWindowExA(0, "BUTTON", "Generate", WS_CHILD | WS_VISIBLE, 22, 58, 90, 32, hwnd, (HMENU)IDC_CON_GENERATE, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Forward", WS_CHILD | WS_VISIBLE, 120, 58, 82, 32, hwnd, (HMENU)IDC_CON_FORWARD, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Backward", WS_CHILD | WS_VISIBLE, 210, 58, 90, 32, hwnd, (HMENU)IDC_CON_BACKWARD, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Append", WS_CHILD | WS_VISIBLE, 308, 58, 80, 32, hwnd, (HMENU)IDC_CON_APPEND, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Insert Before", WS_CHILD | WS_VISIBLE, 396, 58, 115, 32, hwnd, (HMENU)IDC_CON_BEFORE, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Delete", WS_CHILD | WS_VISIBLE, 520, 58, 75, 32, hwnd, (HMENU)IDC_CON_DELETE, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Find Name", WS_CHILD | WS_VISIBLE, 604, 58, 90, 32, hwnd, (HMENU)IDC_CON_SEARCH_NAME, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Find Group", WS_CHILD | WS_VISIBLE, 702, 58, 95, 32, hwnd, (HMENU)IDC_CON_SEARCH_GROUP, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Modify", WS_CHILD | WS_VISIBLE, 806, 58, 75, 32, hwnd, (HMENU)IDC_CON_MODIFY, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Delete Phone", WS_CHILD | WS_VISIBLE, 890, 58, 110, 32, hwnd, (HMENU)IDC_CON_DELETE_PHONE, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Sort", WS_CHILD | WS_VISIBLE, 1008, 58, 65, 32, hwnd, (HMENU)IDC_CON_SORT, NULL, NULL);

    g_conOutput = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL | WS_HSCROLL,
        22, 108, 1140, 560, hwnd, (HMENU)IDC_CON_OUTPUT, NULL, NULL);

    initGroupCombo();
    setContactFonts(hwnd);
}

static void setContactFonts(HWND hwnd)
{
    HWND child = GetWindow(hwnd, GW_CHILD);
    while (child != NULL) {
        SendMessageA(child, WM_SETFONT, (WPARAM)g_conFont, TRUE);
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

static void generateContacts(void)
{
    const char* surnames[] = { "Zhang", "Wang", "Li", "Zhao", "Chen", "Liu", "Yang", "Huang", "Zhou", "Wu" };
    const char* givenNames[] = { "San", "Si", "Wei", "Fang", "Ming", "Lei", "Jie", "Na", "Qiang", "Yan" };
    const char* groups[] = { "Classmate", "Colleague", "Friend", "Family" };
    FILE* fp = NULL;
    int i;
    char name[40] = { 0 };
    char phone[20] = { 0 };

    if (fopen_s(&fp, CONTACT_FILE, "w") != 0 || fp == NULL) {
        return;
    }
    for (i = 0; i < CONTACT_COUNT; ++i) {
        sprintf_s(name, sizeof(name), "%s %s", surnames[rand() % 10], givenNames[rand() % 10]);
        sprintf_s(phone, sizeof(phone), "1%010d", 300000000 + rand() % 700000000);
        fprintf(fp, "%d,%s,%s,,%s,\n", i + 1, name, phone, groups[rand() % 4]);
    }
    fclose(fp);
}

static DblNode* createDblList(void)
{
    FILE* fp = NULL;
    DblNode* head;
    char line[256] = { 0 };
    int id;

    if (fopen_s(&fp, CONTACT_FILE, "r") != 0 || fp == NULL) {
        return NULL;
    }
    head = (DblNode*)malloc(sizeof(DblNode));
    if (head == NULL) {
        fclose(fp);
        return NULL;
    }
    head->prev = head;
    head->next = head;

    while (fgets(line, sizeof(line), fp) != NULL) {
        DblNode* node;
        char parts[6][96] = { { 0 } };
        int partCount = 0;
        int i;
        int pos = 0;
        line[strcspn(line, "\n")] = '\0';
        line[strcspn(line, "\r")] = '\0';
        for (i = 0; ; ++i) {
            if (line[i] == ',' || line[i] == '\0') {
                parts[partCount][pos] = '\0';
                ++partCount;
                pos = 0;
                if (line[i] == '\0' || partCount >= 6) {
                    break;
                }
            }
            else if (pos < (int)sizeof(parts[0]) - 1) {
                parts[partCount][pos++] = line[i];
            }
        }
        if (partCount < 4 || sscanf_s(parts[0], "%d", &id) != 1) {
            continue;
        }
        node = (DblNode*)malloc(sizeof(DblNode));
        if (node == NULL) {
            break;
        }
        ZeroMemory(node, sizeof(DblNode));
        node->data.id = id;
        strcpy_s(node->data.name, sizeof(node->data.name), parts[1]);
        strcpy_s(node->data.phone1, sizeof(node->data.phone1), parts[2]);
        if (partCount >= 6) {
            strcpy_s(node->data.phone2, sizeof(node->data.phone2), parts[3]);
            strcpy_s(node->data.group, sizeof(node->data.group), parts[4]);
            strcpy_s(node->data.remark, sizeof(node->data.remark), parts[5]);
        }
        else {
            node->data.phone2[0] = '\0';
            strcpy_s(node->data.group, sizeof(node->data.group), parts[3]);
            node->data.remark[0] = '\0';
        }
        sanitizeText(node->data.name);
        sanitizeText(node->data.phone1);
        sanitizeText(node->data.phone2);
        sanitizeText(node->data.group);
        sanitizeText(node->data.remark);
        normalizeGroup(node->data.group, sizeof(node->data.group));
        insertNodeBefore(head, node);
    }
    fclose(fp);
    return head;
}

static void destroyList(DblNode* head)
{
    DblNode* cur;
    if (head == NULL) {
        return;
    }
    cur = head->next;
    while (cur != head) {
        DblNode* next = cur->next;
        free(cur);
        cur = next;
    }
    free(head);
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

static void saveToFile(DblNode* head)
{
    FILE* fp = NULL;
    DblNode* p;
    if (head == NULL || fopen_s(&fp, CONTACT_FILE, "w") != 0 || fp == NULL) {
        return;
    }
    for (p = head->next; p != head; p = p->next) {
        fprintf(fp, "%d,%s,%s,%s,%s,%s\n",
            p->data.id, p->data.name, p->data.phone1, p->data.phone2, p->data.group, p->data.remark);
    }
    fclose(fp);
}

static void insertNodeBefore(DblNode* pos, DblNode* node)
{
    node->prev = pos->prev;
    node->next = pos;
    pos->prev->next = node;
    pos->prev = node;
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
    appendText(out, outSize, "ID      Name                 Phone1          Phone2          Group                Remark\r\n");
    appendText(out, outSize, "-----------------------------------------------------------------------------------------------\r\n");
}

static void appendContact(char* out, int outSize, const Contact* contact)
{
    appendFormat(out, outSize, "%-7d %-20s %-15s %-15s %-20s %-20s\r\n",
        contact->id, contact->name, contact->phone1, contact->phone2, contact->group, contact->remark);
}

static void buildForwardText(DblNode* head, char* out, int outSize)
{
    DblNode* p;
    out[0] = '\0';
    appendHeader(out, outSize);
    if (head == NULL) {
        return;
    }
    for (p = head->next; p != head; p = p->next) {
        appendContact(out, outSize, &p->data);
    }
}

static void buildBackwardText(DblNode* head, char* out, int outSize)
{
    DblNode* p;
    out[0] = '\0';
    appendHeader(out, outSize);
    if (head == NULL) {
        return;
    }
    for (p = head->prev; p != head; p = p->prev) {
        appendContact(out, outSize, &p->data);
    }
}

static void searchByName(DblNode* head, const char* name, char* out, int outSize)
{
    DblNode* p;
    int found = 0;
    out[0] = '\0';
    appendHeader(out, outSize);
    for (p = head->next; p != head; p = p->next) {
        if (strstr(p->data.name, name) != NULL) {
            appendContact(out, outSize, &p->data);
            found = 1;
        }
    }
    if (!found) {
        appendText(out, outSize, "No matching contact.");
    }
}

static void searchByGroup(DblNode* head, const char* group, char* out, int outSize)
{
    DblNode* p;
    int found = 0;
    out[0] = '\0';
    appendHeader(out, outSize);
    for (p = head->next; p != head; p = p->next) {
        if (groupMatches(p->data.group, group)) {
            appendContact(out, outSize, &p->data);
            found = 1;
        }
    }
    if (!found) {
        appendText(out, outSize, "No matching group.");
    }
}

static int deleteById(DblNode* head, int id)
{
    DblNode* p;
    for (p = head->next; p != head; p = p->next) {
        if (p->data.id == id) {
            removeNode(p);
            return 1;
        }
    }
    return 0;
}

static int deletePhoneById(HWND hwnd, DblNode* head, int id, char* out, int outSize)
{
    DblNode* node = findById(head, id);
    int slot;

    out[0] = '\0';
    if (node == NULL) {
        appendText(out, outSize, "Delete phone failed. Id not found.");
        return 0;
    }
    slot = choosePhoneSlot(hwnd, &node->data);
    if (slot < 0) {
        appendText(out, outSize, "Delete phone canceled.");
        return 0;
    }
    if (slot == 0) {
        if (node->data.phone1[0] == '\0') {
            appendText(out, outSize, "Phone1 is already empty.");
            return 0;
        }
        node->data.phone1[0] = '\0';
    }
    else {
        if (node->data.phone2[0] == '\0') {
            appendText(out, outSize, "Phone2 is already empty.");
            return 0;
        }
        node->data.phone2[0] = '\0';
    }

    if (node->data.phone1[0] == '\0' && node->data.phone2[0] == '\0') {
        removeNode(node);
        appendText(out, outSize, "Both phone numbers were deleted. Contact record removed.");
    }
    else {
        appendText(out, outSize, "Phone number deleted.");
    }
    return 1;
}

static int modifyPhone(HWND hwnd, DblNode* head, int id, const char* phone, char* out, int outSize)
{
    DblNode* node = findById(head, id);
    int slot;

    out[0] = '\0';
    if (node == NULL) {
        appendText(out, outSize, "Modify failed. Id not found.");
        return 0;
    }

    slot = choosePhoneSlot(hwnd, &node->data);
    if (slot < 0) {
        appendText(out, outSize, "Modify canceled.");
        return 0;
    }

    if (phone[0] == '\0') {
        if (slot == 0) {
            node->data.phone1[0] = '\0';
        }
        else {
            node->data.phone2[0] = '\0';
        }
        if (node->data.phone1[0] == '\0' && node->data.phone2[0] == '\0') {
            removeNode(node);
            appendText(out, outSize, "Both phone numbers are empty. Contact record removed.");
        }
        else {
            appendText(out, outSize, "Selected phone number cleared.");
        }
        return 1;
    }

    if (!isValidPhone(phone)) {
        appendText(out, outSize, "Invalid phone number. Phone must have 11 digits.");
        return 0;
    }
    if (phoneExists(head, phone, node, slot)) {
        appendText(out, outSize, "Invalid phone number. The phone already exists.");
        return 0;
    }

    if (slot == 0) {
        strcpy_s(node->data.phone1, sizeof(node->data.phone1), phone);
    }
    else {
        strcpy_s(node->data.phone2, sizeof(node->data.phone2), phone);
    }
    appendText(out, outSize, "Phone number modified.");
    return 1;
}

static void sortById(DblNode* head)
{
    int swapped;
    if (head == NULL || head->next == head) {
        return;
    }
    do {
        DblNode* p = head->next;
        swapped = 0;
        while (p->next != head) {
            DblNode* q = p->next;
            if (p->data.id > q->data.id) {
                Contact temp = p->data;
                p->data = q->data;
                q->data = temp;
                swapped = 1;
            }
            p = p->next;
        }
    } while (swapped);
}

static void sortByName(DblNode* head)
{
    int swapped;
    if (head == NULL || head->next == head) {
        return;
    }
    do {
        DblNode* p = head->next;
        swapped = 0;
        while (p->next != head) {
            DblNode* q = p->next;
            if (strcmp(p->data.name, q->data.name) > 0) {
                Contact temp = p->data;
                p->data = q->data;
                q->data = temp;
                swapped = 1;
            }
            p = p->next;
        }
    } while (swapped);
}

static int isValidPhone(const char* phone)
{
    int i;
    if (strlen(phone) != 11) {
        return 0;
    }
    for (i = 0; phone[i] != '\0'; ++i) {
        if (phone[i] < '0' || phone[i] > '9') {
            return 0;
        }
    }
    return 1;
}

static int isValidGroup(const char* group)
{
    size_t len = strlen(group);
    if (strcmp(group, "Classmate") == 0 || strcmp(group, "Colleague") == 0
        || strcmp(group, "Friend") == 0 || strcmp(group, "Family") == 0) {
        return 1;
    }
    return len > 7 && strncmp(group, "Other(", 6) == 0 && group[len - 1] == ')';
}

static int groupMatches(const char* stored, const char* query)
{
    if (strcmp(query, "Other") == 0) {
        return strncmp(stored, "Other", 5) == 0;
    }
    return strcmp(stored, query) == 0;
}

static void normalizeGroup(char* group, int size)
{
    if (strcmp(group, "Family") == 0 || strcmp(group, "Friend") == 0
        || strcmp(group, "Colleague") == 0 || strcmp(group, "Classmate") == 0
        || isValidGroup(group)) {
        return;
    }
    strcpy_s(group, size, "Other(Imported)");
}

static void sanitizeText(char* text)
{
    int i;
    for (i = 0; text[i] != '\0'; ++i) {
        if (text[i] == ',' || text[i] == '\r' || text[i] == '\n') {
            text[i] = ' ';
        }
    }
}

static DblNode* findById(DblNode* head, int id)
{
    DblNode* p;
    if (head == NULL) {
        return NULL;
    }
    for (p = head->next; p != head; p = p->next) {
        if (p->data.id == id) {
            return p;
        }
    }
    return NULL;
}

static DblNode* findByName(DblNode* head, const char* name)
{
    DblNode* p;
    if (head == NULL) {
        return NULL;
    }
    for (p = head->next; p != head; p = p->next) {
        if (strcmp(p->data.name, name) == 0) {
            return p;
        }
    }
    return NULL;
}

static int phoneExists(DblNode* head, const char* phone, DblNode* excludeNode, int excludeSlot)
{
    DblNode* p;
    if (head == NULL || phone[0] == '\0') {
        return 0;
    }
    for (p = head->next; p != head; p = p->next) {
        if (!(p == excludeNode && excludeSlot == 0) && strcmp(p->data.phone1, phone) == 0) {
            return 1;
        }
        if (!(p == excludeNode && excludeSlot == 1) && p->data.phone2[0] != '\0'
            && strcmp(p->data.phone2, phone) == 0) {
            return 1;
        }
    }
    return 0;
}

static int addPhoneToContact(DblNode* node, const char* phone)
{
    if (node->data.phone1[0] == '\0') {
        strcpy_s(node->data.phone1, sizeof(node->data.phone1), phone);
        return 1;
    }
    if (node->data.phone2[0] == '\0') {
        strcpy_s(node->data.phone2, sizeof(node->data.phone2), phone);
        return 1;
    }
    return 0;
}

static int appendContactWithRules(HWND hwnd, DblNode* head, Contact* contact, DblNode* before, char* out, int outSize)
{
    DblNode* sameName;
    DblNode* node;
    int answer;

    out[0] = '\0';
    sanitizeText(contact->name);
    sanitizeText(contact->phone1);
    sanitizeText(contact->phone2);
    sanitizeText(contact->group);
    sanitizeText(contact->remark);
    normalizeGroup(contact->group, sizeof(contact->group));

    if (!isValidPhone(contact->phone1) || !isValidGroup(contact->group)) {
        appendText(out, outSize, "Invalid contact data. Phone must have 11 digits and Group must be valid.");
        return 0;
    }
    if (phoneExists(head, contact->phone1, NULL, -1)) {
        appendText(out, outSize, "Invalid phone number. The phone already exists.");
        return 0;
    }

    sameName = findByName(head, contact->name);
    if (sameName != NULL) {
        answer = promptChoice(hwnd, "Existing Contact",
            "A contact with the same Name already exists.\nIs this an existing contact?",
            "Yes", "No", NULL, 0);
        if (answer == 1) {
            if (!addPhoneToContact(sameName, contact->phone1)) {
                appendText(out, outSize, "Cannot save. The existing contact already has two phone numbers.");
                return 0;
            }
            saveToFile(head);
            buildForwardText(head, out, outSize);
            appendText(out, outSize, "\r\nPhone added to the existing contact.");
            return 1;
        }

        if (!promptTextInput(hwnd, "Remark Required",
            "Same Name selected as a new contact. Please input a required remark:",
            contact->remark, sizeof(contact->remark), 0)) {
            appendText(out, outSize, "Append canceled. New same-name contacts need a remark.");
            return 0;
        }
        sanitizeText(contact->remark);
    }

    if (findById(head, contact->id) != NULL) {
        appendText(out, outSize, "Append failed. Id already exists.");
        return 0;
    }

    node = (DblNode*)malloc(sizeof(DblNode));
    if (node == NULL) {
        appendText(out, outSize, "Append failed. Memory allocation failed.");
        return 0;
    }
    ZeroMemory(node, sizeof(DblNode));
    node->data = *contact;
    insertNodeBefore(before, node);
    saveToFile(head);
    buildForwardText(head, out, outSize);
    appendText(out, outSize, "\r\nContact appended and saved.");
    return 1;
}

static void removeNode(DblNode* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node);
}

static int choosePhoneSlot(HWND hwnd, const Contact* contact)
{
    char msg[256];
    int answer;

    sprintf_s(msg, sizeof(msg),
        "Choose the phone number to modify.\n\nPhone1: %s\nPhone2: %s",
        contact->phone1[0] == '\0' ? "(empty)" : contact->phone1,
        contact->phone2[0] == '\0' ? "(empty)" : contact->phone2);
    answer = promptChoice(hwnd, "Choose Phone Number", msg, "Phone1", "Phone2", "Cancel", 1);
    if (answer == 1) {
        return 0;
    }
    if (answer == 2) {
        return 1;
    }
    return -1;
}

static LRESULT CALLBACK ChoicePromptProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ChoicePromptState* state = (ChoicePromptState*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE:
    {
        HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CREATESTRUCTA* cs = (CREATESTRUCTA*)lParam;
        state = (ChoicePromptState*)cs->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)state);
        CreateWindowExA(0, "STATIC", state->message, WS_CHILD | WS_VISIBLE,
            20, 18, 390, 72, hwnd, NULL, NULL, NULL);
        CreateWindowExA(0, "BUTTON", state->firstText, WS_CHILD | WS_VISIBLE,
            88, 108, 90, 30, hwnd, (HMENU)IDC_CHOICE_FIRST, NULL, NULL);
        CreateWindowExA(0, "BUTTON", state->secondText, WS_CHILD | WS_VISIBLE,
            196, 108, 90, 30, hwnd, (HMENU)IDC_CHOICE_SECOND, NULL, NULL);
        if (state->showCancel) {
            CreateWindowExA(0, "BUTTON", state->cancelText, WS_CHILD | WS_VISIBLE,
                304, 108, 90, 30, hwnd, (HMENU)IDC_CHOICE_CANCEL, NULL, NULL);
        }
        SendMessageA(hwnd, WM_SETFONT, (WPARAM)font, TRUE);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_CHOICE_FIRST) {
            state->result = 1;
            state->done = 1;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_CHOICE_SECOND) {
            state->result = 2;
            state->done = 1;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_CHOICE_CANCEL) {
            state->result = 0;
            state->done = 1;
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        state->result = 0;
        state->done = 1;
        DestroyWindow(hwnd);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static int promptChoice(HWND owner, const char* title, const char* message,
    const char* firstText, const char* secondText, const char* cancelText, int showCancel)
{
    static int registered = 0;
    WNDCLASSA wc;
    ChoicePromptState state;
    HWND hwnd;
    MSG msg;
    RECT ownerRect;
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;

    if (!registered) {
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = ChoicePromptProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "ContactChoicePromptWindow";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassA(&wc);
        registered = 1;
    }

    ZeroMemory(&state, sizeof(state));
    state.message = message;
    state.firstText = firstText;
    state.secondText = secondText;
    state.cancelText = cancelText == NULL ? "Cancel" : cancelText;
    state.showCancel = showCancel;

    if (owner != NULL && GetWindowRect(owner, &ownerRect)) {
        x = ownerRect.left + 210;
        y = ownerRect.top + 180;
        EnableWindow(owner, FALSE);
    }

    hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "ContactChoicePromptWindow", title,
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        x, y, showCancel ? 450 : 390, 190, owner, NULL, GetModuleHandleA(NULL), &state);
    if (hwnd == NULL) {
        if (owner != NULL) {
            EnableWindow(owner, TRUE);
        }
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    while (!state.done && GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (owner != NULL) {
        EnableWindow(owner, TRUE);
        SetForegroundWindow(owner);
    }
    return state.result;
}

static LRESULT CALLBACK TextPromptProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TextPromptState* state = (TextPromptState*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE:
    {
        HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        CREATESTRUCTA* cs = (CREATESTRUCTA*)lParam;
        state = (TextPromptState*)cs->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)state);
        CreateWindowExA(0, "STATIC", state->label, WS_CHILD | WS_VISIBLE,
            18, 16, 330, 42, hwnd, NULL, NULL, NULL);
        state->edit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            18, 62, 330, 26, hwnd, (HMENU)IDC_PROMPT_EDIT, NULL, NULL);
        CreateWindowExA(0, "BUTTON", "OK", WS_CHILD | WS_VISIBLE,
            176, 104, 78, 28, hwnd, (HMENU)IDC_PROMPT_OK, NULL, NULL);
        CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE,
            270, 104, 78, 28, hwnd, (HMENU)IDC_PROMPT_CANCEL, NULL, NULL);
        SendMessageA(state->edit, WM_SETFONT, (WPARAM)font, TRUE);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_PROMPT_OK) {
            GetWindowTextA(state->edit, state->out, state->outSize);
            sanitizeText(state->out);
            if (!state->allowEmpty && state->out[0] == '\0') {
                MessageBoxA(hwnd, "Input cannot be empty.", "Input Required", MB_OK | MB_ICONWARNING);
                return 0;
            }
            state->result = 1;
            state->done = 1;
            DestroyWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_PROMPT_CANCEL) {
            state->result = 0;
            state->done = 1;
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_CLOSE:
        state->result = 0;
        state->done = 1;
        DestroyWindow(hwnd);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static int promptTextInput(HWND owner, const char* title, const char* label, char* out, int outSize, int allowEmpty)
{
    static int registered = 0;
    WNDCLASSA wc;
    TextPromptState state;
    HWND hwnd;
    MSG msg;
    RECT ownerRect;
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;

    if (!registered) {
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = TextPromptProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "ContactTextPromptWindow";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassA(&wc);
        registered = 1;
    }

    ZeroMemory(&state, sizeof(state));
    state.out = out;
    state.outSize = outSize;
    state.allowEmpty = allowEmpty;
    state.label = label;
    out[0] = '\0';

    if (owner != NULL && GetWindowRect(owner, &ownerRect)) {
        x = ownerRect.left + 180;
        y = ownerRect.top + 160;
        EnableWindow(owner, FALSE);
    }

    hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, "ContactTextPromptWindow", title,
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        x, y, 380, 180, owner, NULL, GetModuleHandleA(NULL), &state);
    if (hwnd == NULL) {
        if (owner != NULL) {
            EnableWindow(owner, TRUE);
        }
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    while (!state.done && GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    if (owner != NULL) {
        EnableWindow(owner, TRUE);
        SetForegroundWindow(owner);
    }
    return state.result;
}

static void reloadContacts(void)
{
    destroyList(g_contactHead);
    g_contactHead = createDblList();
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

static int readContactFromEdits(Contact* contact)
{
    ZeroMemory(contact, sizeof(Contact));
    if (!readEditInt(g_conId, &contact->id)) {
        return 0;
    }
    readEditText(g_conName, contact->name, sizeof(contact->name));
    readEditText(g_conPhone, contact->phone1, sizeof(contact->phone1));
    readEditText(g_conGroup, contact->group, sizeof(contact->group));
    readEditText(g_conRemark, contact->remark, sizeof(contact->remark));
    contact->phone2[0] = '\0';
    sanitizeText(contact->name);
    sanitizeText(contact->phone1);
    sanitizeText(contact->group);
    sanitizeText(contact->remark);
    return contact->id > 0 && contact->name[0] != '\0'
        && isValidPhone(contact->phone1) && isValidGroup(contact->group);
}

static void setOutput(const char* text)
{
    if (g_conOutput != NULL) {
        SetWindowTextA(g_conOutput, text);
    }
}

static void initGroupCombo(void)
{
    SendMessageA(g_conGroup, CB_ADDSTRING, 0, (LPARAM)"Classmate");
    SendMessageA(g_conGroup, CB_ADDSTRING, 0, (LPARAM)"Colleague");
    SendMessageA(g_conGroup, CB_ADDSTRING, 0, (LPARAM)"Friend");
    SendMessageA(g_conGroup, CB_ADDSTRING, 0, (LPARAM)"Family");
    SendMessageA(g_conGroup, CB_ADDSTRING, 0, (LPARAM)"Other");
    SetWindowTextA(g_conGroup, "Friend");
}

static void handleGroupSelection(HWND hwnd)
{
    int index = (int)SendMessageA(g_conGroup, CB_GETCURSEL, 0, 0);
    char group[64] = { 0 };
    char note[48] = { 0 };

    if (index == CB_ERR) {
        return;
    }
    SendMessageA(g_conGroup, CB_GETLBTEXT, (WPARAM)index, (LPARAM)group);
    if (strcmp(group, "Other") == 0) {
        if (promptTextInput(hwnd, "Other Group", "Please input a note for Other group:", note, sizeof(note), 0)) {
            sprintf_s(group, sizeof(group), "Other(%s)", note);
            SetWindowTextA(g_conGroup, group);
        }
        else {
            SetWindowTextA(g_conGroup, "Friend");
        }
    }
}
