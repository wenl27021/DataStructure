/*
Campus delivery path planning - Win32 interactive version.
The module keeps the graph algorithms required by the assignment:
adjacency matrix construction, DFS connectivity check, stack-based DFS path
search, queue-based BFS path search, and dynamic road-block simulation.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>

#define MIN_NODES 10
#define MAX_NODES 15
#define INF 9999999
#define ROAD_BLOCK 9999

#define IDC_START_EDIT 101
#define IDC_TARGET_EDIT 102
#define IDC_GENERATE_BTN 201
#define IDC_CONNECT_BTN 202
#define IDC_DFS_BTN 203
#define IDC_BFS_BTN 204
#define IDC_BLOCK_BTN 205
#define IDC_MATRIX_BTN 206
#define IDC_FEWEST_MODE_RADIO 210
#define IDC_DISTANCE_MODE_RADIO 211
#define IDC_ROAD_ADD_BTN 212
#define IDC_BFS_ALGORITHM_RADIO 213
#define IDC_ASTAR_ALGORITHM_RADIO 214
#define IDC_DIJKSTRA_ALGORITHM_RADIO 215
#define IDC_RESULT_EDIT 301
#define IDC_ROAD_FROM_EDIT 302
#define IDC_ROAD_TO_EDIT 303
#define IDC_ROAD_DIST_EDIT 304
#define IDC_CHANGE_MINUS_BASE 400

#define VIEW_GRAPH 0
#define VIEW_CONNECTIVITY_DIAGRAM 1
#define VIEW_DFS_DIAGRAM 2
#define VIEW_ROUTE_DIAGRAM 3

#define ROUTE_MODE_FEWEST_NODES 0
#define ROUTE_MODE_SHORTEST_DISTANCE 1

#define ROUTE_ALGORITHM_BFS 0
#define ROUTE_ALGORITHM_ASTAR 1
#define ROUTE_ALGORITHM_DIJKSTRA 2

#define MAX_ROAD_CHANGES 6
#define ROAD_CHANGE_ADD 1
#define ROAD_CHANGE_BLOCK 2

typedef struct CampusGraph {
    int nodeCount;
    char names[MAX_NODES][40];
    int posX[MAX_NODES];
    int posY[MAX_NODES];
    int matrix[MAX_NODES][MAX_NODES];
} CampusGraph;

typedef struct Stack {
    int data[MAX_NODES];
    int top;
} Stack;

typedef struct Queue {
    int data[MAX_NODES];
    int front;
    int rear;
} Queue;

typedef struct RoadChange {
    int type;
    int from;
    int to;
    int distance;
} RoadChange;

static CampusGraph g_graph;
static int g_baseMatrix[MAX_NODES][MAX_NODES];
static int g_generated = 0;
static int g_lastParent[MAX_NODES];
static int g_firstDfsParent[MAX_NODES];
static int g_hasFirstDfsRoute = 0;
static int g_lastStart = -1;
static int g_lastTarget = -1;
static int g_viewMode = VIEW_GRAPH;
static int g_routeMode = ROUTE_MODE_FEWEST_NODES;
static int g_routeAlgorithm = ROUTE_ALGORITHM_BFS;
static int g_visitOrder[MAX_NODES];
static int g_bfsLevel[MAX_NODES];
static int g_shortestDistance[MAX_NODES];
static int g_selectedNode = -1;
static int g_nodeX[MAX_NODES];
static int g_nodeY[MAX_NODES];
static int g_animationStep = 0;
static int g_animationMax = 0;
static RoadChange g_roadChanges[MAX_ROAD_CHANGES];
static int g_roadChangeCount = 0;
/* Handles for the input and output controls in the campus delivery window. */
static HWND g_startEdit = NULL;   /* Start node input box. */
static HWND g_targetEdit = NULL;  /* Target node input box. */
static HWND g_resultEdit = NULL;  /* Multi-line result display box. */
static HWND g_roadFromEdit = NULL;
static HWND g_roadToEdit = NULL;
static HWND g_roadDistEdit = NULL;
static HWND g_changeMinusButtons[MAX_ROAD_CHANGES];
static HWND g_changeLabels[MAX_ROAD_CHANGES];

/* Shared UI resources. They are created when the window opens and released when it closes. */
static HFONT g_font = NULL;       /* Font used by controls. */
static HBRUSH g_backBrush = NULL; /* Light background brush. */
static int g_campusRunning = 0;

static void initGraph(CampusGraph* graph);
static void generateCampusMap(CampusGraph* graph);
static void ensureConnected(CampusGraph* graph);
static void dfsVisit(CampusGraph* graph, int vertex, int visited[]);
static int isConnected(CampusGraph* graph);
static int firstUnvisitedNode(CampusGraph* graph, int visited[]);
static int firstVisitedNode(CampusGraph* graph, int visited[]);
static void push(Stack* stack, int value);
static int pop(Stack* stack);
static int isStackEmpty(Stack* stack);
static void enqueue(Queue* queue, int value);
static int dequeue(Queue* queue);
static int isQueueEmpty(Queue* queue);
static int searchPathByDFS(CampusGraph* graph, int start, int target, int bestParent[], int firstParent[], int routeMode);
static int searchPathByBFSRoute(CampusGraph* graph, int start, int target, int parent[], int routeMode);
static int searchPathByAStarRoute(CampusGraph* graph, int start, int target, int parent[], int routeMode);
static int searchPathByDijkstraRoute(CampusGraph* graph, int start, int target, int parent[], int routeMode);
static void dfsRouteExplore(CampusGraph* graph, int cur, int target, int routeMode,
    int visited[], int curParent[], int depth, int distance,
    int firstParent[], int* hasFirst, int bestParent[], int* hasBest,
    int* bestPrimary, int* bestSecondary, int* order);
static void copyParentArray(int dest[], int src[]);
static const char* routeAlgorithmName(void);
static int betterRouteCandidate(int candPrimary, int candSecondary, int bestPrimary, int bestSecondary);
static int nodeStraightDistance(CampusGraph* graph, int a, int b);
static int maxRoadCoordinateSpan(CampusGraph* graph);
static int minRoadWeight(CampusGraph* graph);
static int heuristicDistance(CampusGraph* graph, int node, int target);
static int heuristicHop(CampusGraph* graph, int node, int target);
static void resetVisualization(void);
static void buildConnectivityVisualization(CampusGraph* graph);
static void computeShortestDistances(CampusGraph* graph, int start, int dist[]);
static int pathDistance(CampusGraph* graph, int start, int target, int parent[]);
static void copyCurrentMatrixToBase(CampusGraph* graph);
static void clearRoadChanges(void);
static void applyRoadChangesToGraph(CampusGraph* graph);
static void updateRoadChangeControls(HWND hwnd);
static int appendRoadChange(HWND hwnd, int type, char* out, int outSize);
static void deleteRoadChange(HWND hwnd, int index, char* out, int outSize);
static void buildRoadChangeText(char* out, int outSize);
static void appendText(char* out, int outSize, const char* text);
static void appendFormat(char* out, int outSize, const char* fmt, ...);
static void buildNodeText(CampusGraph* graph, char* out, int outSize);
static void buildMatrixText(CampusGraph* graph, char* out, int outSize);
static void buildPathText(CampusGraph* graph, int start, int target, int parent[], const char* title, char* out, int outSize);
static int readIndexFromEdit(HWND edit, int* value);
static void setResultText(const char* text);
static void createControls(HWND hwnd);
static void updateAllFonts(HWND hwnd);
static void drawGraph(HWND hwnd, HDC hdc);
static void drawProcessDiagram(HWND hwnd, HDC hdc);
static void drawFlowBox(HDC hdc, int x, int y, int w, int h, const char* text, COLORREF fill);
static void drawArrow(HDC hdc, int x1, int y1, int x2, int y2);
static int shouldShowProcessPanel(void);
static int calculateResultBoxHeight(HWND hwnd);
static void updateResultLayout(HWND hwnd);
static void appendCenteredCell(char* out, int outSize, const char* text, int width);
static void buildProcessText(int mode, int routeMode, char* out, int outSize);
static void appendProcessText(int mode, int routeMode, char* out, int outSize);
static COLORREF levelColor(int level);
static int hitTestNode(int x, int y);
static void startAnimation(HWND hwnd);
/*
CampusWndProc is the window procedure for this module.
Windows calls it automatically when the campus window receives messages such as
WM_CREATE, WM_COMMAND, WM_PAINT, and WM_DESTROY.
*/
static LRESULT CALLBACK CampusWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int runCampusDeliverySystem(void)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    HINSTANCE instance;

    instance = GetModuleHandleA(NULL);
    srand((unsigned int)time(NULL));
    initGraph(&g_graph);
    g_generated = 0;
    g_lastStart = -1;
    g_lastTarget = -1;
    g_viewMode = VIEW_GRAPH;
    g_routeMode = ROUTE_MODE_FEWEST_NODES;
    g_routeAlgorithm = ROUTE_ALGORITHM_BFS;
    clearRoadChanges();
    resetVisualization();
    g_animationStep = 0;
    g_animationMax = 0;

    g_backBrush = CreateSolidBrush(RGB(246, 250, 255));
    g_font = CreateFontA(
        18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN, "Consolas");

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = CampusWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = "CampusDeliveryPlannerWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_backBrush;

    RegisterClassA(&wc);

    hwnd = CreateWindowExA(
        0,
        "CampusDeliveryPlannerWindow",
        "Campus Delivery Path Planning",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1100,
        760,
        NULL,
        NULL,
        instance,
        NULL);

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Unable to create Campus Delivery window.", "Error", MB_ICONERROR);
        return 1;
    }

    g_campusRunning = 1;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    while (g_campusRunning && GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (g_font != NULL) {
        DeleteObject(g_font);
        g_font = NULL;
    }
    if (g_backBrush != NULL) {
        DeleteObject(g_backBrush);
        g_backBrush = NULL;
    }

    return 0;
}

static LRESULT CALLBACK CampusWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char output[12000];
    int start;
    int target;
    int found;
    int clicked;

    switch (msg) {
    case WM_CREATE:
        createControls(hwnd);
        setResultText("Click Generate Map to create a random connected campus graph.\r\nUse node indexes as start and target.");
        updateResultLayout(hwnd);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) >= IDC_CHANGE_MINUS_BASE
            && LOWORD(wParam) < IDC_CHANGE_MINUS_BASE + MAX_ROAD_CHANGES) {
            output[0] = '\0';
            deleteRoadChange(hwnd, LOWORD(wParam) - IDC_CHANGE_MINUS_BASE, output, sizeof(output));
            setResultText(output);
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_GENERATE_BTN:
            generateCampusMap(&g_graph);
            ensureConnected(&g_graph);
            copyCurrentMatrixToBase(&g_graph);
            clearRoadChanges();
            updateRoadChangeControls(hwnd);
            g_generated = 1;
            g_lastStart = -1;
            g_lastTarget = -1;
            g_viewMode = VIEW_GRAPH;
            resetVisualization();
            output[0] = '\0';
            appendText(output, sizeof(output), "Campus map generated.\r\n\r\n");
            buildNodeText(&g_graph, output, sizeof(output));
            setResultText(output);
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_CONNECT_BTN:
            output[0] = '\0';
            if (!g_generated) {
                appendText(output, sizeof(output), "Please generate the campus map first.");
                g_viewMode = VIEW_GRAPH;
                resetVisualization();
            }
            else {
                if (isConnected(&g_graph)) {
                    appendText(output, sizeof(output), "DFS connectivity test: all nodes are connected.");
                }
                else {
                    appendText(output, sizeof(output), "Disconnected nodes found. Fixing graph...\r\n");
                    ensureConnected(&g_graph);
                    appendText(output, sizeof(output), "Connectivity fixed.");
                }
                buildConnectivityVisualization(&g_graph);
                g_viewMode = VIEW_CONNECTIVITY_DIAGRAM;
                startAnimation(hwnd);
            }
            setResultText(output);
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_DFS_BTN:
        case IDC_BFS_BTN:
            output[0] = '\0';
            if (!g_generated) {
                g_viewMode = VIEW_GRAPH;
                resetVisualization();
                setResultText("Please generate the campus map first.");
                updateResultLayout(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }
            if (!readIndexFromEdit(g_startEdit, &start) || !readIndexFromEdit(g_targetEdit, &target)
                || start < 0 || target < 0 || start >= g_graph.nodeCount || target >= g_graph.nodeCount || start == target) {
                g_viewMode = VIEW_GRAPH;
                resetVisualization();
                setResultText("Invalid start or target index.");
                updateResultLayout(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }

            if (LOWORD(wParam) == IDC_DFS_BTN) {
                found = searchPathByDFS(&g_graph, start, target, g_lastParent, g_firstDfsParent, g_routeMode);
                if (found && g_hasFirstDfsRoute) {
                    buildPathText(&g_graph, start, target, g_firstDfsParent,
                        "DFS first route", output, sizeof(output));
                    appendText(output, sizeof(output), "\r\n");
                    buildPathText(&g_graph, start, target, g_lastParent,
                        g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE
                        ? "DFS best shortest-distance route; tie breaker: fewest nodes"
                        : "DFS best fewest-node route; tie breaker: shortest distance",
                        output, sizeof(output));
                }
                else {
                    buildPathText(&g_graph, start, target, g_lastParent, "DFS route", output, sizeof(output));
                }
                g_viewMode = VIEW_DFS_DIAGRAM;
            }
            else {
                if (g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR) {
                    found = searchPathByAStarRoute(&g_graph, start, target, g_lastParent, g_routeMode);
                }
                else if (g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA) {
                    found = searchPathByDijkstraRoute(&g_graph, start, target, g_lastParent, g_routeMode);
                }
                else {
                    found = searchPathByBFSRoute(&g_graph, start, target, g_lastParent, g_routeMode);
                }

                if (g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
                    buildPathText(&g_graph, start, target, g_lastParent,
                        routeAlgorithmName(),
                        output, sizeof(output));
                }
                else {
                    buildPathText(&g_graph, start, target, g_lastParent,
                        routeAlgorithmName(),
                        output, sizeof(output));
                }
                g_viewMode = VIEW_ROUTE_DIAGRAM;
            }

            if (found) {
                g_lastStart = start;
                g_lastTarget = target;
                g_selectedNode = -1;
                computeShortestDistances(&g_graph, start, g_shortestDistance);
                setResultText(output);
                startAnimation(hwnd);
            }
            else {
                g_lastStart = -1;
                g_lastTarget = -1;
                g_viewMode = VIEW_GRAPH;
                resetVisualization();
                setResultText("No available path found.");
            }
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_ROAD_ADD_BTN:
        case IDC_BLOCK_BTN:
            output[0] = '\0';
            if (!g_generated) {
                appendText(output, sizeof(output), "Please generate the campus map first.");
            }
            else {
                if (appendRoadChange(hwnd,
                    LOWORD(wParam) == IDC_ROAD_ADD_BTN ? ROAD_CHANGE_ADD : ROAD_CHANGE_BLOCK,
                    output, sizeof(output))) {
                    appendText(output, sizeof(output), "\r\nRun Route Search again to view the changed route.");
                }
                g_lastStart = -1;
                g_lastTarget = -1;
                g_viewMode = VIEW_GRAPH;
                resetVisualization();
            }
            setResultText(output);
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_MATRIX_BTN:
            output[0] = '\0';
            if (!g_generated) {
                appendText(output, sizeof(output), "Please generate the campus map first.");
            }
            else {
                buildMatrixText(&g_graph, output, sizeof(output));
            }
            g_viewMode = VIEW_GRAPH;
            setResultText(output);
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_FEWEST_MODE_RADIO:
            g_routeMode = ROUTE_MODE_FEWEST_NODES;
            CheckRadioButton(hwnd, IDC_FEWEST_MODE_RADIO, IDC_DISTANCE_MODE_RADIO, IDC_FEWEST_MODE_RADIO);
            g_viewMode = VIEW_GRAPH;
            resetVisualization();
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_DISTANCE_MODE_RADIO:
            g_routeMode = ROUTE_MODE_SHORTEST_DISTANCE;
            CheckRadioButton(hwnd, IDC_FEWEST_MODE_RADIO, IDC_DISTANCE_MODE_RADIO, IDC_DISTANCE_MODE_RADIO);
            g_viewMode = VIEW_GRAPH;
            resetVisualization();
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_BFS_ALGORITHM_RADIO:
            g_routeAlgorithm = ROUTE_ALGORITHM_BFS;
            CheckRadioButton(hwnd, IDC_BFS_ALGORITHM_RADIO, IDC_DIJKSTRA_ALGORITHM_RADIO, IDC_BFS_ALGORITHM_RADIO);
            g_viewMode = VIEW_GRAPH;
            resetVisualization();
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_ASTAR_ALGORITHM_RADIO:
            g_routeAlgorithm = ROUTE_ALGORITHM_ASTAR;
            CheckRadioButton(hwnd, IDC_BFS_ALGORITHM_RADIO, IDC_DIJKSTRA_ALGORITHM_RADIO, IDC_ASTAR_ALGORITHM_RADIO);
            g_viewMode = VIEW_GRAPH;
            resetVisualization();
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;

        case IDC_DIJKSTRA_ALGORITHM_RADIO:
            g_routeAlgorithm = ROUTE_ALGORITHM_DIJKSTRA;
            CheckRadioButton(hwnd, IDC_BFS_ALGORITHM_RADIO, IDC_DIJKSTRA_ALGORITHM_RADIO, IDC_DIJKSTRA_ALGORITHM_RADIO);
            g_viewMode = VIEW_GRAPH;
            resetVisualization();
            updateResultLayout(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (g_generated && g_lastStart >= 0
            && (g_viewMode == VIEW_DFS_DIAGRAM || g_viewMode == VIEW_ROUTE_DIAGRAM)) {
            clicked = hitTestNode(LOWORD(lParam), HIWORD(lParam));
            if (clicked >= 0) {
                g_selectedNode = clicked;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            if (g_animationStep < g_animationMax) {
                ++g_animationStep;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            else {
                KillTimer(hwnd, 1);
            }
        }
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wParam, RGB(246, 250, 255));
        SetTextColor((HDC)wParam, RGB(33, 47, 61));
        return (LRESULT)g_backBrush;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        drawGraph(hwnd, hdc);
        drawProcessDiagram(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        g_campusRunning = 0;
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void createControls(HWND hwnd)
{
    int child;

    CreateWindowExA(0, "STATIC", "Start:", WS_CHILD | WS_VISIBLE,
        24, 24, 50, 24, hwnd, NULL, NULL, NULL);
    g_startEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        78, 22, 60, 26, hwnd, (HMENU)IDC_START_EDIT, NULL, NULL);

    CreateWindowExA(0, "STATIC", "Target:", WS_CHILD | WS_VISIBLE,
        154, 24, 60, 24, hwnd, NULL, NULL, NULL);
    g_targetEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "1",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        218, 22, 60, 26, hwnd, (HMENU)IDC_TARGET_EDIT, NULL, NULL);

    CreateWindowExA(0, "STATIC", "Algorithm:", WS_CHILD | WS_VISIBLE,
        304, 24, 90, 24, hwnd, NULL, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "BFS", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        394, 22, 56, 26, hwnd, (HMENU)IDC_BFS_ALGORITHM_RADIO, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "A*", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        454, 22, 50, 26, hwnd, (HMENU)IDC_ASTAR_ALGORITHM_RADIO, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Dijkstra", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        508, 22, 112, 26, hwnd, (HMENU)IDC_DIJKSTRA_ALGORITHM_RADIO, NULL, NULL);
    CheckRadioButton(hwnd, IDC_BFS_ALGORITHM_RADIO, IDC_DIJKSTRA_ALGORITHM_RADIO, IDC_BFS_ALGORITHM_RADIO);

    CreateWindowExA(0, "STATIC", "Method:", WS_CHILD | WS_VISIBLE,
        634, 24, 72, 24, hwnd, NULL, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Fewest nodes", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        706, 22, 132, 26, hwnd, (HMENU)IDC_FEWEST_MODE_RADIO, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Shortest distance", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        842, 22, 180, 26, hwnd, (HMENU)IDC_DISTANCE_MODE_RADIO, NULL, NULL);
    CheckRadioButton(hwnd, IDC_FEWEST_MODE_RADIO, IDC_DISTANCE_MODE_RADIO, IDC_FEWEST_MODE_RADIO);

    CreateWindowExA(0, "BUTTON", "Generate Map", WS_CHILD | WS_VISIBLE,
        24, 62, 120, 32, hwnd, (HMENU)IDC_GENERATE_BTN, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Connectivity", WS_CHILD | WS_VISIBLE,
        154, 62, 110, 32, hwnd, (HMENU)IDC_CONNECT_BTN, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "DFS Route", WS_CHILD | WS_VISIBLE,
        274, 62, 100, 32, hwnd, (HMENU)IDC_DFS_BTN, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Route Search", WS_CHILD | WS_VISIBLE,
        384, 62, 110, 32, hwnd, (HMENU)IDC_BFS_BTN, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Matrix", WS_CHILD | WS_VISIBLE,
        504, 62, 90, 32, hwnd, (HMENU)IDC_MATRIX_BTN, NULL, NULL);

    CreateWindowExA(0, "STATIC", "Road Change:", WS_CHILD | WS_VISIBLE,
        24, 106, 120, 24, hwnd, NULL, NULL, NULL);
    CreateWindowExA(0, "STATIC", "From", WS_CHILD | WS_VISIBLE,
        146, 106, 44, 24, hwnd, NULL, NULL, NULL);
    g_roadFromEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        190, 102, 42, 26, hwnd, (HMENU)IDC_ROAD_FROM_EDIT, NULL, NULL);
    CreateWindowExA(0, "STATIC", "To", WS_CHILD | WS_VISIBLE,
        240, 106, 28, 24, hwnd, NULL, NULL, NULL);
    g_roadToEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "1",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        268, 102, 42, 26, hwnd, (HMENU)IDC_ROAD_TO_EDIT, NULL, NULL);
    CreateWindowExA(0, "STATIC", "Dist", WS_CHILD | WS_VISIBLE,
        318, 106, 42, 24, hwnd, NULL, NULL, NULL);
    g_roadDistEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "10",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        360, 102, 52, 26, hwnd, (HMENU)IDC_ROAD_DIST_EDIT, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Road Adding", WS_CHILD | WS_VISIBLE,
        420, 100, 120, 32, hwnd, (HMENU)IDC_ROAD_ADD_BTN, NULL, NULL);
    CreateWindowExA(0, "BUTTON", "Road Block", WS_CHILD | WS_VISIBLE,
        550, 100, 110, 32, hwnd, (HMENU)IDC_BLOCK_BTN, NULL, NULL);

    g_resultEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
        24, 148, 470, 526, hwnd, (HMENU)IDC_RESULT_EDIT, NULL, NULL);

    for (child = 0; child < MAX_ROAD_CHANGES; ++child) {
        g_changeMinusButtons[child] = CreateWindowExA(0, "BUTTON", "-",
            WS_CHILD,
            24, 142, 22, 22, hwnd, (HMENU)(INT_PTR)(IDC_CHANGE_MINUS_BASE + child), NULL, NULL);
        g_changeLabels[child] = CreateWindowExA(0, "STATIC", "",
            WS_CHILD,
            50, 144, 220, 22, hwnd, NULL, NULL, NULL);
    }

    updateAllFonts(hwnd);
    updateRoadChangeControls(hwnd);
}

static void updateAllFonts(HWND hwnd)
{
    HWND child;

    if (g_font == NULL) {
        return;
    }

    child = GetWindow(hwnd, GW_CHILD);
    while (child != NULL) {
        SendMessageA(child, WM_SETFONT, (WPARAM)g_font, TRUE);
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

static void drawGraph(HWND hwnd, HDC hdc)
{
    RECT rect;
    HPEN edgePen;
    HPEN pathPen;
    HPEN dfsFirstPen;
    HPEN dfsBestPen;
    HPEN oldPen;
    HBRUSH nodeBrush;
    HBRUSH centerBrush;
    HBRUSH pathBrush;
    HBRUSH oldBrush;
    int x[MAX_NODES] = { 0 };
    int y[MAX_NODES] = { 0 };
    int i;
    int j;
    int cx;
    int cy;
    int radius;
    int areaLeft;
    int areaTop;
    int areaRight;
    int areaBottom;
    char label[64];
    char weightText[16];
    double angle;
    HBRUSH useBrush;
    HBRUSH dynamicBrush;
    int cur;
    int prev;
    COLORREF fillColor;
    int visualVisible;

    GetClientRect(hwnd, &rect);
    areaLeft = 530;
    areaTop = 210;
    areaRight = rect.right - 36;
    areaBottom = rect.bottom - 58;
    cx = (areaLeft + areaRight) / 2;
    cy = (areaTop + areaBottom) / 2;
    radius = (areaRight - areaLeft < areaBottom - areaTop ? areaRight - areaLeft : areaBottom - areaTop) / 2 - 52;

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, g_font);
    SetTextColor(hdc, RGB(33, 47, 61));
    if (g_viewMode == VIEW_CONNECTIVITY_DIAGRAM) {
        TextOutA(hdc, areaLeft, 152, "Connectivity process on generated map", 37);
        TextOutA(hdc, areaLeft, 178, "Green nodes are reached by DFS connectivity traversal.", 52);
    }
    else if (g_viewMode == VIEW_DFS_DIAGRAM) {
        TextOutA(hdc, areaLeft, 152, "DFS route process on generated map", 34);
        TextOutA(hdc, areaLeft, 178, "Blue edges show first DFS route; red edges show DFS best route.", 63);
    }
    else if (g_viewMode == VIEW_ROUTE_DIAGRAM) {
        if (g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR && g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
            TextOutA(hdc, areaLeft, 152, "A* shortest-distance process on generated map",
                (int)strlen("A* shortest-distance process on generated map"));
            TextOutA(hdc, areaLeft, 178, "Purple nodes show A* f(n)=g(n)+h(n); equal distance uses fewer nodes.",
                (int)strlen("Purple nodes show A* f(n)=g(n)+h(n); equal distance uses fewer nodes."));
        }
        else if (g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR) {
            TextOutA(hdc, areaLeft, 152, "A* fewest-node process on generated map",
                (int)strlen("A* fewest-node process on generated map"));
            TextOutA(hdc, areaLeft, 178, "Purple nodes show A* hop estimate h(n); equal hops use shorter distance.",
                (int)strlen("Purple nodes show A* hop estimate h(n); equal hops use shorter distance."));
        }
        else if (g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA && g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
            TextOutA(hdc, areaLeft, 152, "Dijkstra shortest-distance process on generated map",
                (int)strlen("Dijkstra shortest-distance process on generated map"));
            TextOutA(hdc, areaLeft, 178, "Blue nodes show Dijkstra expansion; equal distance uses fewer nodes.",
                (int)strlen("Blue nodes show Dijkstra expansion; equal distance uses fewer nodes."));
        }
        else if (g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA) {
            TextOutA(hdc, areaLeft, 152, "Dijkstra fewest-node process on generated map",
                (int)strlen("Dijkstra fewest-node process on generated map"));
            TextOutA(hdc, areaLeft, 178, "Blue nodes show Dijkstra expansion; equal hops use shorter distance.",
                (int)strlen("Blue nodes show Dijkstra expansion; equal hops use shorter distance."));
        }
        else if (g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
            TextOutA(hdc, areaLeft, 152, "BFS shortest-distance process on generated map",
                (int)strlen("BFS shortest-distance process on generated map"));
            TextOutA(hdc, areaLeft, 178, "Queue relaxation prefers lower distance; equal distance uses fewer nodes.",
                (int)strlen("Queue relaxation prefers lower distance; equal distance uses fewer nodes."));
        }
        else {
            TextOutA(hdc, areaLeft, 152, "BFS fewest-node process on generated map",
                (int)strlen("BFS fewest-node process on generated map"));
            TextOutA(hdc, areaLeft, 178, "Same-hop nodes share a color; equal hops use shorter distance.",
                (int)strlen("Same-hop nodes share a color; equal hops use shorter distance."));
        }
    }
    else {
        TextOutA(hdc, areaLeft, 152, "Campus Graph Preview", 20);
        TextOutA(hdc, areaLeft, 178, "Light edges are roads; orange edges show the last route.", 55);
    }

    if (!g_generated) {
        TextOutA(hdc, areaLeft, 250, "Click Generate Map to draw the campus graph.", 45);
        return;
    }

    if (g_graph.nodeCount <= 0 || g_graph.nodeCount > MAX_NODES) {
        TextOutA(hdc, areaLeft, 250, "Invalid graph data.", 19);
        return;
    }

    for (i = 0; i < g_graph.nodeCount; ++i) {
        angle = 6.28318530718 * i / g_graph.nodeCount - 1.57079632679;
        x[i] = cx + (int)(radius * cos(angle));
        y[i] = cy + (int)(radius * sin(angle));
    }

    edgePen = CreatePen(PS_SOLID, 1, RGB(160, 190, 215));
    pathPen = CreatePen(PS_SOLID, 4, RGB(245, 151, 66));
    dfsFirstPen = CreatePen(PS_SOLID, 3, RGB(37, 99, 235));
    dfsBestPen = CreatePen(PS_SOLID, 5, RGB(239, 68, 68));
    oldPen = (HPEN)SelectObject(hdc, edgePen);

    for (i = 0; i < g_graph.nodeCount; ++i) {
        for (j = i + 1; j < g_graph.nodeCount; ++j) {
            if (g_graph.matrix[i][j] > 0 && g_graph.matrix[i][j] < ROAD_BLOCK) {
                MoveToEx(hdc, x[i], y[i], NULL);
                LineTo(hdc, x[j], y[j]);
                sprintf_s(weightText, sizeof(weightText), "%d", g_graph.matrix[i][j]);
                TextOutA(hdc, (x[i] + x[j]) / 2, (y[i] + y[j]) / 2, weightText, (int)strlen(weightText));
            }
        }
    }

    if (g_lastStart >= 0 && g_lastTarget >= 0
        && (g_animationMax == 0 || g_animationStep >= g_animationMax)) {
        if (g_viewMode == VIEW_DFS_DIAGRAM && g_hasFirstDfsRoute) {
            cur = g_lastTarget;
            SelectObject(hdc, dfsFirstPen);
            while (cur != g_lastStart) {
                if (cur < 0 || cur >= g_graph.nodeCount) {
                    break;
                }
                prev = g_firstDfsParent[cur];
                if (prev < 0 || prev >= g_graph.nodeCount) {
                    break;
                }
                MoveToEx(hdc, x[prev], y[prev], NULL);
                LineTo(hdc, x[cur], y[cur]);
                cur = prev;
            }

            cur = g_lastTarget;
            SelectObject(hdc, dfsBestPen);
            while (cur != g_lastStart) {
                if (cur < 0 || cur >= g_graph.nodeCount) {
                    break;
                }
                prev = g_lastParent[cur];
                if (prev < 0 || prev >= g_graph.nodeCount) {
                    break;
                }
                MoveToEx(hdc, x[prev], y[prev], NULL);
                LineTo(hdc, x[cur], y[cur]);
                cur = prev;
            }
        }
        else {
            cur = g_lastTarget;
            SelectObject(hdc, pathPen);
            while (cur != g_lastStart) {
                if (cur < 0 || cur >= g_graph.nodeCount) {
                    break;
                }
                prev = g_lastParent[cur];
                if (prev < 0 || prev >= g_graph.nodeCount) {
                    break;
                }
                MoveToEx(hdc, x[prev], y[prev], NULL);
                LineTo(hdc, x[cur], y[cur]);
                cur = prev;
            }
        }
    }

    nodeBrush = CreateSolidBrush(RGB(224, 242, 254));
    centerBrush = CreateSolidBrush(RGB(187, 247, 208));
    pathBrush = CreateSolidBrush(RGB(254, 215, 170));

    for (i = 0; i < g_graph.nodeCount; ++i) {
        g_nodeX[i] = x[i];
        g_nodeY[i] = y[i];
        useBrush = nodeBrush;
        dynamicBrush = NULL;
        fillColor = RGB(224, 242, 254);
        visualVisible = g_visitOrder[i] > 0 && (g_animationMax == 0 || g_visitOrder[i] <= g_animationStep);

        if (g_viewMode == VIEW_CONNECTIVITY_DIAGRAM && visualVisible) {
            fillColor = RGB(187, 247, 208);
            dynamicBrush = CreateSolidBrush(fillColor);
            useBrush = dynamicBrush;
        }
        else if (g_viewMode == VIEW_DFS_DIAGRAM && visualVisible) {
            fillColor = RGB(254, 240, 138);
            dynamicBrush = CreateSolidBrush(fillColor);
            useBrush = dynamicBrush;
        }
        else if (g_viewMode == VIEW_ROUTE_DIAGRAM && g_routeAlgorithm == ROUTE_ALGORITHM_BFS && visualVisible && g_bfsLevel[i] >= 0) {
            fillColor = levelColor(g_bfsLevel[i]);
            dynamicBrush = CreateSolidBrush(fillColor);
            useBrush = dynamicBrush;
        }
        else if (g_viewMode == VIEW_ROUTE_DIAGRAM && g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR && visualVisible) {
            fillColor = RGB(221, 214, 254);
            dynamicBrush = CreateSolidBrush(fillColor);
            useBrush = dynamicBrush;
        }
        else if (g_viewMode == VIEW_ROUTE_DIAGRAM && g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA && visualVisible) {
            fillColor = RGB(191, 219, 254);
            dynamicBrush = CreateSolidBrush(fillColor);
            useBrush = dynamicBrush;
        }

        if (i == 0) {
            useBrush = centerBrush;
        }
        if (i == g_lastStart || i == g_lastTarget) {
            useBrush = pathBrush;
        }
        oldBrush = (HBRUSH)SelectObject(hdc, useBrush);
        Ellipse(hdc, x[i] - 20, y[i] - 20, x[i] + 20, y[i] + 20);
        SelectObject(hdc, oldBrush);
        sprintf_s(label, sizeof(label), "%d", i);
        TextOutA(hdc, x[i] - 5, y[i] - 9, label, (int)strlen(label));
        if (visualVisible && g_viewMode != VIEW_GRAPH) {
            sprintf_s(label, sizeof(label), "#%d", g_visitOrder[i]);
            TextOutA(hdc, x[i] + 16, y[i] - 28, label, (int)strlen(label));
        }
        if (g_viewMode == VIEW_ROUTE_DIAGRAM && g_routeAlgorithm == ROUTE_ALGORITHM_BFS && visualVisible && g_bfsLevel[i] >= 0) {
            sprintf_s(label, sizeof(label), "L%d", g_bfsLevel[i]);
            TextOutA(hdc, x[i] - 34, y[i] - 28, label, (int)strlen(label));
        }
        if (i == g_selectedNode && g_lastStart >= 0) {
            if (g_shortestDistance[i] >= INF) {
                sprintf_s(label, sizeof(label), "dist: --");
            }
            else {
                sprintf_s(label, sizeof(label), "dist: %d", g_shortestDistance[i]);
            }
            TextOutA(hdc, x[i] - 35, y[i] - 52, label, (int)strlen(label));
        }
        TextOutA(hdc, x[i] - 38, y[i] + 24, g_graph.names[i], (int)strlen(g_graph.names[i]));
        if (dynamicBrush != NULL) {
            DeleteObject(dynamicBrush);
        }
    }

    SelectObject(hdc, oldPen);
    DeleteObject(edgePen);
    DeleteObject(pathPen);
    DeleteObject(dfsFirstPen);
    DeleteObject(dfsBestPen);
    DeleteObject(nodeBrush);
    DeleteObject(centerBrush);
    DeleteObject(pathBrush);
}

static int shouldShowProcessPanel(void)
{
    return g_viewMode == VIEW_CONNECTIVITY_DIAGRAM
        || g_viewMode == VIEW_DFS_DIAGRAM
        || g_viewMode == VIEW_ROUTE_DIAGRAM;
}

static int calculateResultBoxHeight(HWND hwnd)
{
    RECT client;
    HDC hdc;
    HFONT oldFont;
    TEXTMETRICA tm;
    int lineCount;
    int lineHeight;
    int extraHeight;
    int height;
    int maxHeight;

    if (g_resultEdit == NULL) {
        return 38;
    }

    lineHeight = 18;
    hdc = GetDC(hwnd);
    if (hdc != NULL) {
        oldFont = NULL;
        if (g_font != NULL) {
            oldFont = (HFONT)SelectObject(hdc, g_font);
        }
        if (GetTextMetricsA(hdc, &tm)) {
            lineHeight = tm.tmHeight + tm.tmExternalLeading;
        }
        if (oldFont != NULL) {
            SelectObject(hdc, oldFont);
        }
        ReleaseDC(hwnd, hdc);
    }

    lineCount = (int)SendMessageA(g_resultEdit, EM_GETLINECOUNT, 0, 0);
    if (lineCount < 1) {
        lineCount = 1;
    }

    extraHeight = 16;
    height = lineCount * lineHeight + extraHeight;

    GetClientRect(hwnd, &client);
    maxHeight = client.bottom - 152 - 20 - 282 - 18;
    if (maxHeight < lineHeight + extraHeight) {
        maxHeight = lineHeight + extraHeight;
    }
    if (height > maxHeight) {
        height = maxHeight;
    }

    return height;
}

static void updateResultLayout(HWND hwnd)
{
    RECT dirty;
    RECT client;
    int resultHeight;
    int resultTop;

    if (g_resultEdit == NULL) {
        return;
    }

    GetClientRect(hwnd, &client);
    if (shouldShowProcessPanel()) {
        resultHeight = calculateResultBoxHeight(hwnd);
        MoveWindow(g_resultEdit, 24, 152, 470, resultHeight, TRUE);
    }
    else {
        resultTop = g_roadChangeCount == 0 ? 148 : 256;
        resultHeight = client.bottom - resultTop - 28;
        if (resultHeight < 120) {
            resultHeight = 120;
        }
        MoveWindow(g_resultEdit, 24, resultTop, 470, resultHeight, TRUE);
    }

    dirty.left = 16;
    dirty.top = 136;
    dirty.right = 510;
    dirty.bottom = client.bottom;
    InvalidateRect(hwnd, &dirty, TRUE);
}

static void drawProcessDiagram(HWND hwnd, HDC hdc)
{
    RECT panel;
    int left;
    int top;
    int rightX;
    int boxW;
    int boxH;
    int resultHeight;
    const char* title;
    const char* a;
    const char* b;
    const char* c;
    const char* d;
    const char* e;
    const char* note;
    HBRUSH brush;
    HBRUSH oldBrush;
    HPEN pen;
    HPEN oldPen;

    if (!shouldShowProcessPanel()) {
        return;
    }

    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, g_font);
    resultHeight = calculateResultBoxHeight(hwnd);

    panel.left = 24;
    panel.top = 152 + resultHeight + 20;
    panel.right = 494;
    panel.bottom = panel.top + 282;

    brush = CreateSolidBrush(RGB(248, 252, 255));
    pen = CreatePen(PS_SOLID, 1, RGB(148, 163, 184));
    oldBrush = (HBRUSH)SelectObject(hdc, brush);
    oldPen = (HPEN)SelectObject(hdc, pen);
    RoundRect(hdc, panel.left, panel.top, panel.right, panel.bottom, 10, 10);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    left = 38;
    rightX = 272;
    top = panel.top + 38;
    boxW = 198;
    boxH = 46;

    if (g_viewMode == VIEW_CONNECTIVITY_DIAGRAM) {
        title = "Connectivity implementation process";
        a = "Start from node 0";
        b = "DFS marks\nreachable nodes";
        c = "Check unvisited\nnodes";
        d = "Add random edge\nif needed";
        e = "All nodes connected";
        note = "The map colors each node when DFS reaches it.";
    }
    else if (g_viewMode == VIEW_DFS_DIAGRAM) {
        title = "DFS route implementation process";
        a = "Start DFS search";
        b = "First target hit";
        c = "Continue DFS\nbacktracking";
        d = "Compare route\npriority";
        e = "Trace best route";
        note = "Blue=first route; red=best route under the selected method.";
    }
    else {
        if (g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR && g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
            title = "A* shortest-distance route process";
        }
        else if (g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR) {
            title = "A* fewest-node route process";
        }
        else if (g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA && g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
            title = "Dijkstra shortest-distance route process";
        }
        else if (g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA) {
            title = "Dijkstra fewest-node route process";
        }
        else if (g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
            title = "BFS shortest-distance route process";
        }
        else {
            title = "BFS fewest-node route process";
        }
        a = "Initialize start node";
        b = g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
            ? "Pick lowest\nf=g+h"
            : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
            ? "Pick lowest\npriority"
            : "Queue next candidate";
        c = g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? "Tie by fewer nodes" : "Tie by lower distance";
        d = "Record parent links";
        e = "Trace best route";
        if (g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
            note = g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
                ? "A* distance mode: f(n)=g(n)+h(n); equal distance chooses fewer nodes."
                : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
                ? "Dijkstra distance mode: lower distance wins; equal distance chooses fewer nodes."
                : "BFS distance mode: queue relaxation keeps lower distance first.";
        }
        else {
            note = g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
                ? "A* node mode: f(n)=g(n)+h(n); equal hops choose lower distance."
                : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
                ? "Dijkstra node mode: hop count wins; equal hops choose lower distance."
                : "BFS node mode: hop count wins; equal hops choose lower distance.";
        }
    }

    SetTextColor(hdc, RGB(31, 41, 55));
    TextOutA(hdc, left, panel.top + 12, title, (int)strlen(title));

    drawFlowBox(hdc, left, top, boxW, boxH, a, RGB(219, 234, 254));
    drawArrow(hdc, left + boxW, top + boxH / 2, rightX, top + boxH / 2);
    drawFlowBox(hdc, rightX, top, boxW, boxH, b, RGB(224, 242, 254));
    drawArrow(hdc, rightX + boxW / 2, top + boxH, rightX + boxW / 2, top + 78);
    drawFlowBox(hdc, rightX, top + 84, boxW, boxH, c, RGB(220, 252, 231));
    drawArrow(hdc, rightX, top + 84 + boxH / 2, left + boxW, top + 84 + boxH / 2);
    drawFlowBox(hdc, left, top + 84, boxW, boxH, d, RGB(254, 243, 199));
    drawArrow(hdc, left + boxW / 2, top + 84 + boxH, left + boxW / 2, top + 162);
    drawFlowBox(hdc, left, top + 168, boxW, boxH, e, RGB(254, 215, 170));

    SetTextColor(hdc, RGB(71, 85, 105));
    TextOutA(hdc, left, top + 230, note, (int)strlen(note));
}

static void drawFlowBox(HDC hdc, int x, int y, int w, int h, const char* text, COLORREF fill)
{
    RECT rc;
    HBRUSH brush;
    HBRUSH oldBrush;
    HPEN pen;
    HPEN oldPen;

    rc.left = x;
    rc.top = y;
    rc.right = x + w;
    rc.bottom = y + h;

    brush = CreateSolidBrush(fill);
    pen = CreatePen(PS_SOLID, 1, RGB(96, 125, 139));
    oldBrush = (HBRUSH)SelectObject(hdc, brush);
    oldPen = (HPEN)SelectObject(hdc, pen);
    RoundRect(hdc, x, y, x + w, y + h, 12, 12);
    InflateRect(&rc, -6, -4);
    DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

static void drawArrow(HDC hdc, int x1, int y1, int x2, int y2)
{
    HPEN pen;
    HPEN oldPen;
    int dx;
    int dy;

    pen = CreatePen(PS_SOLID, 2, RGB(100, 116, 139));
    oldPen = (HPEN)SelectObject(hdc, pen);
    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);

    dx = x2 - x1;
    dy = y2 - y1;
    if (abs(dx) >= abs(dy)) {
        LineTo(hdc, x2 - (dx > 0 ? 10 : -10), y2 - 6);
        MoveToEx(hdc, x2, y2, NULL);
        LineTo(hdc, x2 - (dx > 0 ? 10 : -10), y2 + 6);
    }
    else {
        LineTo(hdc, x2 - 6, y2 - (dy > 0 ? 10 : -10));
        MoveToEx(hdc, x2, y2, NULL);
        LineTo(hdc, x2 + 6, y2 - (dy > 0 ? 10 : -10));
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static void initGraph(CampusGraph* graph)
{
    int i;
    int j;

    graph->nodeCount = 0;
    for (i = 0; i < MAX_NODES; ++i) {
        graph->names[i][0] = '\0';
        graph->posX[i] = 0;
        graph->posY[i] = 0;
        for (j = 0; j < MAX_NODES; ++j) {
            graph->matrix[i][j] = INF;
        }
    }
}

static void generateCampusMap(CampusGraph* graph)
{
    const char* baseNames[] = {
        "Express Center", "Dorm A", "Dorm B", "Dorm C", "Dorm D",
        "Library", "Canteen", "Teaching Building", "Lab Building",
        "Gym", "Gate", "Playground", "Clinic", "Admin Building", "Shop"
    };
    const int baseX[] = { 50, 78, 92, 90, 78, 62, 48, 34, 22, 12, 10, 24, 44, 66, 84 };
    const int baseY[] = { 16, 22, 36, 52, 66, 78, 82, 76, 62, 48, 32, 18, 30, 46, 60 };
    int i;
    int j;
    int weight;

    initGraph(graph);
    graph->nodeCount = MIN_NODES + rand() % (MAX_NODES - MIN_NODES + 1);

    for (i = 0; i < graph->nodeCount; ++i) {
        strcpy_s(graph->names[i], sizeof(graph->names[i]), baseNames[i]);
        graph->posX[i] = baseX[i];
        graph->posY[i] = baseY[i];
        graph->matrix[i][i] = 0;
    }

    for (i = 0; i < graph->nodeCount - 1; ++i) {
        weight = 3 + rand() % 18;
        graph->matrix[i][i + 1] = weight;
        graph->matrix[i + 1][i] = weight;
    }

    for (i = 0; i < graph->nodeCount; ++i) {
        for (j = i + 2; j < graph->nodeCount; ++j) {
            if (rand() % 100 < 28) {
                weight = 3 + rand() % 25;
                graph->matrix[i][j] = weight;
                graph->matrix[j][i] = weight;
            }
        }
    }
}

static void ensureConnected(CampusGraph* graph)
{
    int visited[MAX_NODES];
    int i;
    int connectedNode;
    int isolatedNode;
    int weight;

    while (!isConnected(graph)) {
        for (i = 0; i < graph->nodeCount; ++i) {
            visited[i] = 0;
        }
        dfsVisit(graph, 0, visited);

        isolatedNode = firstUnvisitedNode(graph, visited);
        connectedNode = firstVisitedNode(graph, visited);

        if (isolatedNode < 0 || isolatedNode >= graph->nodeCount
            || connectedNode < 0 || connectedNode >= graph->nodeCount) {
            return;
        }

        weight = 3 + rand() % 20;
        graph->matrix[isolatedNode][connectedNode] = weight;
        graph->matrix[connectedNode][isolatedNode] = weight;
    }
}

static void dfsVisit(CampusGraph* graph, int vertex, int visited[])
{
    int i;

    visited[vertex] = 1;
    for (i = 0; i < graph->nodeCount; ++i) {
        if (!visited[i] && graph->matrix[vertex][i] > 0 && graph->matrix[vertex][i] < ROAD_BLOCK) {
            dfsVisit(graph, i, visited);
        }
    }
}

static int isConnected(CampusGraph* graph)
{
    int visited[MAX_NODES];
    int i;

    if (graph->nodeCount == 0) {
        return 0;
    }

    for (i = 0; i < graph->nodeCount; ++i) {
        visited[i] = 0;
    }
    dfsVisit(graph, 0, visited);

    for (i = 0; i < graph->nodeCount; ++i) {
        if (!visited[i]) {
            return 0;
        }
    }
    return 1;
}

static int firstUnvisitedNode(CampusGraph* graph, int visited[])
{
    int i;

    for (i = 0; i < graph->nodeCount; ++i) {
        if (!visited[i]) {
            return i;
        }
    }
    return -1;
}

static int firstVisitedNode(CampusGraph* graph, int visited[])
{
    int i;

    for (i = 0; i < graph->nodeCount; ++i) {
        if (visited[i]) {
            return i;
        }
    }
    return -1;
}

static void resetVisualization(void)
{
    int i;

    for (i = 0; i < MAX_NODES; ++i) {
        g_visitOrder[i] = 0;
        g_bfsLevel[i] = -1;
        g_shortestDistance[i] = INF;
        g_nodeX[i] = 0;
        g_nodeY[i] = 0;
        g_lastParent[i] = -1;
        g_firstDfsParent[i] = -1;
    }
    g_hasFirstDfsRoute = 0;
    g_selectedNode = -1;
    g_animationStep = 0;
    g_animationMax = 0;
}

static void buildConnectivityVisualization(CampusGraph* graph)
{
    int visited[MAX_NODES];
    int stack[MAX_NODES];
    int top;
    int cur;
    int i;
    int order;

    resetVisualization();
    if (graph->nodeCount <= 0) {
        return;
    }

    for (i = 0; i < graph->nodeCount; ++i) {
        visited[i] = 0;
    }

    top = -1;
    order = 0;
    stack[++top] = 0;
    visited[0] = 1;

    while (top >= 0) {
        cur = stack[top--];
        g_visitOrder[cur] = ++order;
        for (i = graph->nodeCount - 1; i >= 0; --i) {
            if (!visited[i] && graph->matrix[cur][i] > 0 && graph->matrix[cur][i] < ROAD_BLOCK) {
                visited[i] = 1;
                stack[++top] = i;
            }
        }
    }
}

static void computeShortestDistances(CampusGraph* graph, int start, int dist[])
{
    int used[MAX_NODES];
    int i;
    int j;
    int cur;
    int best;

    for (i = 0; i < graph->nodeCount; ++i) {
        dist[i] = INF;
        used[i] = 0;
    }

    if (start < 0 || start >= graph->nodeCount) {
        return;
    }

    dist[start] = 0;
    for (i = 0; i < graph->nodeCount; ++i) {
        cur = -1;
        best = INF;
        for (j = 0; j < graph->nodeCount; ++j) {
            if (!used[j] && dist[j] < best) {
                best = dist[j];
                cur = j;
            }
        }
        if (cur == -1) {
            break;
        }
        used[cur] = 1;
        for (j = 0; j < graph->nodeCount; ++j) {
            if (graph->matrix[cur][j] > 0 && graph->matrix[cur][j] < ROAD_BLOCK
                && dist[cur] + graph->matrix[cur][j] < dist[j]) {
                dist[j] = dist[cur] + graph->matrix[cur][j];
            }
        }
    }
}

static void push(Stack* stack, int value)
{
    if (stack->top < MAX_NODES - 1) {
        stack->data[++stack->top] = value;
    }
}

static int pop(Stack* stack)
{
    if (isStackEmpty(stack)) {
        return -1;
    }
    return stack->data[stack->top--];
}

static int isStackEmpty(Stack* stack)
{
    return stack->top == -1;
}

static void enqueue(Queue* queue, int value)
{
    if (queue->rear < MAX_NODES - 1) {
        queue->data[++queue->rear] = value;
    }
}

static int dequeue(Queue* queue)
{
    if (isQueueEmpty(queue)) {
        return -1;
    }
    return queue->data[queue->front++];
}

static int isQueueEmpty(Queue* queue)
{
    return queue->front > queue->rear;
}

static int searchPathByDFS(CampusGraph* graph, int start, int target, int bestParent[], int firstParent[], int routeMode)
{
    int visited[MAX_NODES];
    int curParent[MAX_NODES];
    int i;
    int order;
    int hasBest;
    int bestPrimary;
    int bestSecondary;

    resetVisualization();
    order = 0;
    hasBest = 0;
    g_hasFirstDfsRoute = 0;
    bestPrimary = INF;
    bestSecondary = INF;

    for (i = 0; i < graph->nodeCount; ++i) {
        visited[i] = 0;
        curParent[i] = -1;
        bestParent[i] = -1;
        firstParent[i] = -1;
    }

    visited[start] = 1;
    dfsRouteExplore(graph, start, target, routeMode,
        visited, curParent, 0, 0,
        firstParent, &g_hasFirstDfsRoute,
        bestParent, &hasBest, &bestPrimary, &bestSecondary, &order);

    return hasBest;
}

static void dfsRouteExplore(CampusGraph* graph, int cur, int target, int routeMode,
    int visited[], int curParent[], int depth, int distance,
    int firstParent[], int* hasFirst, int bestParent[], int* hasBest,
    int* bestPrimary, int* bestSecondary, int* order)
{
    int i;
    int candPrimary;
    int candSecondary;

    if (g_visitOrder[cur] == 0) {
        g_visitOrder[cur] = ++(*order);
    }

    if (cur == target) {
        candPrimary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? distance : depth;
        candSecondary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? depth : distance;

        if (!(*hasFirst)) {
            copyParentArray(firstParent, curParent);
            *hasFirst = 1;
        }
        if (!(*hasBest) || betterRouteCandidate(candPrimary, candSecondary, *bestPrimary, *bestSecondary)) {
            copyParentArray(bestParent, curParent);
            *bestPrimary = candPrimary;
            *bestSecondary = candSecondary;
            *hasBest = 1;
        }
        return;
    }

    for (i = 0; i < graph->nodeCount; ++i) {
        if (!visited[i] && graph->matrix[cur][i] > 0 && graph->matrix[cur][i] < ROAD_BLOCK) {
            visited[i] = 1;
            curParent[i] = cur;
            dfsRouteExplore(graph, i, target, routeMode,
                visited, curParent, depth + 1, distance + graph->matrix[cur][i],
                firstParent, hasFirst, bestParent, hasBest, bestPrimary, bestSecondary, order);
            curParent[i] = -1;
            visited[i] = 0;
        }
    }
}

static void copyParentArray(int dest[], int src[])
{
    int i;

    for (i = 0; i < MAX_NODES; ++i) {
        dest[i] = src[i];
    }
}

static const char* routeAlgorithmName(void)
{
    if (g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR) {
        return g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE
            ? "A* shortest-distance route; tie breaker: fewest nodes"
            : "A* fewest-node route; tie breaker: shortest distance";
    }
    if (g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA) {
        return g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE
            ? "Dijkstra shortest-distance route; tie breaker: fewest nodes"
            : "Dijkstra fewest-node route; tie breaker: shortest distance";
    }
    return g_routeMode == ROUTE_MODE_SHORTEST_DISTANCE
        ? "BFS shortest-distance route; tie breaker: fewest nodes"
        : "BFS fewest-node route; tie breaker: shortest distance";
}

static int betterRouteCandidate(int candPrimary, int candSecondary, int bestPrimary, int bestSecondary)
{
    return candPrimary < bestPrimary
        || (candPrimary == bestPrimary && candSecondary < bestSecondary);
}

static int nodeStraightDistance(CampusGraph* graph, int a, int b)
{
    int dx;
    int dy;

    dx = graph->posX[a] - graph->posX[b];
    dy = graph->posY[a] - graph->posY[b];
    return (int)sqrt((double)(dx * dx + dy * dy));
}

static int maxRoadCoordinateSpan(CampusGraph* graph)
{
    int i;
    int j;
    int span;
    int maxSpan;

    maxSpan = 1;
    for (i = 0; i < graph->nodeCount; ++i) {
        for (j = i + 1; j < graph->nodeCount; ++j) {
            if (graph->matrix[i][j] > 0 && graph->matrix[i][j] < ROAD_BLOCK) {
                span = nodeStraightDistance(graph, i, j);
                if (span > maxSpan) {
                    maxSpan = span;
                }
            }
        }
    }
    return maxSpan;
}

static int minRoadWeight(CampusGraph* graph)
{
    int i;
    int j;
    int minWeight;

    minWeight = INF;
    for (i = 0; i < graph->nodeCount; ++i) {
        for (j = i + 1; j < graph->nodeCount; ++j) {
            if (graph->matrix[i][j] > 0 && graph->matrix[i][j] < ROAD_BLOCK
                && graph->matrix[i][j] < minWeight) {
                minWeight = graph->matrix[i][j];
            }
        }
    }
    return minWeight == INF ? 0 : minWeight;
}

static int heuristicHop(CampusGraph* graph, int node, int target)
{
    int straight;
    int maxSpan;

    straight = nodeStraightDistance(graph, node, target);
    if (straight <= 0) {
        return 0;
    }

    maxSpan = maxRoadCoordinateSpan(graph);
    return (straight + maxSpan - 1) / maxSpan;
}

static int heuristicDistance(CampusGraph* graph, int node, int target)
{
    int minWeight;

    minWeight = minRoadWeight(graph);
    if (minWeight <= 0) {
        return 0;
    }

    return heuristicHop(graph, node, target) * minWeight;
}

static int searchPathByBFSRoute(CampusGraph* graph, int start, int target, int parent[], int routeMode)
{
    int queueData[MAX_NODES * MAX_NODES];
    int inQueue[MAX_NODES];
    int bestHops[MAX_NODES];
    int bestDist[MAX_NODES];
    int i;
    int j;
    int cur;
    int order;
    int candHops;
    int candDist;
    int candPrimary;
    int candSecondary;
    int bestPrimary;
    int bestSecondary;
    int front;
    int rear;

    resetVisualization();
    order = 0;
    front = 0;
    rear = 0;
    for (i = 0; i < graph->nodeCount; ++i) {
        inQueue[i] = 0;
        bestHops[i] = INF;
        bestDist[i] = INF;
        parent[i] = -1;
    }

    bestHops[start] = 0;
    bestDist[start] = 0;
    g_bfsLevel[start] = 0;
    queueData[rear++] = start;
    inQueue[start] = 1;

    while (front < rear) {
        cur = queueData[front++];
        inQueue[cur] = 0;
        if (g_visitOrder[cur] == 0) {
            g_visitOrder[cur] = ++order;
        }
        g_bfsLevel[cur] = bestHops[cur];

        for (j = 0; j < graph->nodeCount; ++j) {
            if (graph->matrix[cur][j] > 0 && graph->matrix[cur][j] < ROAD_BLOCK) {
                candHops = bestHops[cur] + 1;
                candDist = bestDist[cur] + graph->matrix[cur][j];

                if (routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
                    candPrimary = candDist;
                    candSecondary = candHops;
                    bestPrimary = bestDist[j];
                    bestSecondary = bestHops[j];
                }
                else {
                    candPrimary = candHops;
                    candSecondary = candDist;
                    bestPrimary = bestHops[j];
                    bestSecondary = bestDist[j];
                }

                if (betterRouteCandidate(candPrimary, candSecondary, bestPrimary, bestSecondary)) {
                    bestHops[j] = candHops;
                    bestDist[j] = candDist;
                    parent[j] = cur;
                    g_bfsLevel[j] = candHops;
                    if (!inQueue[j] && rear < MAX_NODES * MAX_NODES) {
                        queueData[rear++] = j;
                        inQueue[j] = 1;
                    }
                }
            }
        }
    }

    return start == target || parent[target] != -1;
}

static int searchPathByDijkstraRoute(CampusGraph* graph, int start, int target, int parent[], int routeMode)
{
    int used[MAX_NODES];
    int bestDist[MAX_NODES];
    int bestHops[MAX_NODES];
    int i;
    int j;
    int cur;
    int order;
    int primary;
    int secondary;
    int curPrimary;
    int curSecondary;
    int candDist;
    int candHops;
    int candPrimary;
    int candSecondary;
    int bestPrimary;
    int bestSecondary;

    resetVisualization();
    order = 0;
    for (i = 0; i < graph->nodeCount; ++i) {
        used[i] = 0;
        bestDist[i] = INF;
        bestHops[i] = INF;
        parent[i] = -1;
    }

    bestDist[start] = 0;
    bestHops[start] = 0;
    g_bfsLevel[start] = 0;

    while (1) {
        cur = -1;
        for (i = 0; i < graph->nodeCount; ++i) {
            if (!used[i] && bestDist[i] < INF && bestHops[i] < INF) {
                primary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? bestDist[i] : bestHops[i];
                secondary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? bestHops[i] : bestDist[i];
                if (cur == -1) {
                    cur = i;
                }
                else {
                    curPrimary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? bestDist[cur] : bestHops[cur];
                    curSecondary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? bestHops[cur] : bestDist[cur];
                    if (betterRouteCandidate(primary, secondary, curPrimary, curSecondary)) {
                        cur = i;
                    }
                }
            }
        }

        if (cur == -1) {
            return 0;
        }
        used[cur] = 1;
        g_visitOrder[cur] = ++order;
        g_bfsLevel[cur] = bestHops[cur];

        if (cur == target) {
            return 1;
        }

        for (j = 0; j < graph->nodeCount; ++j) {
            if (!used[j] && graph->matrix[cur][j] > 0 && graph->matrix[cur][j] < ROAD_BLOCK) {
                candDist = bestDist[cur] + graph->matrix[cur][j];
                candHops = bestHops[cur] + 1;

                if (routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
                    candPrimary = candDist;
                    candSecondary = candHops;
                    bestPrimary = bestDist[j];
                    bestSecondary = bestHops[j];
                }
                else {
                    candPrimary = candHops;
                    candSecondary = candDist;
                    bestPrimary = bestHops[j];
                    bestSecondary = bestDist[j];
                }

                if (betterRouteCandidate(candPrimary, candSecondary, bestPrimary, bestSecondary)) {
                    bestDist[j] = candDist;
                    bestHops[j] = candHops;
                    parent[j] = cur;
                    g_bfsLevel[j] = candHops;
                }
            }
        }
    }
}

static int searchPathByAStarRoute(CampusGraph* graph, int start, int target, int parent[], int routeMode)
{
    int used[MAX_NODES];
    int bestDist[MAX_NODES];
    int bestHops[MAX_NODES];
    int i;
    int j;
    int cur;
    int order;
    int candDist;
    int candHops;
    int primary;
    int secondary;
    int curPrimary;
    int curSecondary;
    int candPrimary;
    int candSecondary;
    int bestPrimary;
    int bestSecondary;

    resetVisualization();
    order = 0;
    for (i = 0; i < graph->nodeCount; ++i) {
        used[i] = 0;
        bestDist[i] = INF;
        bestHops[i] = INF;
        parent[i] = -1;
    }

    bestDist[start] = 0;
    bestHops[start] = 0;
    g_bfsLevel[start] = 0;

    while (1) {
        cur = -1;
        for (i = 0; i < graph->nodeCount; ++i) {
            if (!used[i] && bestDist[i] < INF && bestHops[i] < INF) {
                primary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE
                    ? bestDist[i] + heuristicDistance(graph, i, target)
                    : bestHops[i] + heuristicHop(graph, i, target);
                secondary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? bestHops[i] : bestDist[i];
                if (cur == -1) {
                    cur = i;
                }
                else {
                    curPrimary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE
                        ? bestDist[cur] + heuristicDistance(graph, cur, target)
                        : bestHops[cur] + heuristicHop(graph, cur, target);
                    curSecondary = routeMode == ROUTE_MODE_SHORTEST_DISTANCE ? bestHops[cur] : bestDist[cur];
                    if (betterRouteCandidate(primary, secondary, curPrimary, curSecondary)) {
                        cur = i;
                    }
                }
            }
        }

        if (cur == -1) {
            return 0;
        }
        used[cur] = 1;
        g_visitOrder[cur] = ++order;
        g_bfsLevel[cur] = bestHops[cur];

        if (cur == target) {
            return 1;
        }

        for (j = 0; j < graph->nodeCount; ++j) {
            if (!used[j] && graph->matrix[cur][j] > 0 && graph->matrix[cur][j] < ROAD_BLOCK) {
                candDist = bestDist[cur] + graph->matrix[cur][j];
                candHops = bestHops[cur] + 1;

                if (routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
                    candPrimary = candDist;
                    candSecondary = candHops;
                    bestPrimary = bestDist[j];
                    bestSecondary = bestHops[j];
                }
                else {
                    candPrimary = candHops;
                    candSecondary = candDist;
                    bestPrimary = bestHops[j];
                    bestSecondary = bestDist[j];
                }

                if (betterRouteCandidate(candPrimary, candSecondary, bestPrimary, bestSecondary)) {
                    bestDist[j] = candDist;
                    bestHops[j] = candHops;
                    parent[j] = cur;
                    g_bfsLevel[j] = candHops;
                }
            }
        }
    }
}

static int pathDistance(CampusGraph* graph, int start, int target, int parent[])
{
    int total;
    int cur;
    int prev;

    total = 0;
    cur = target;
    while (cur != start && parent[cur] != -1) {
        prev = parent[cur];
        total += graph->matrix[prev][cur];
        cur = prev;
    }
    return total;
}

static void copyCurrentMatrixToBase(CampusGraph* graph)
{
    int i;
    int j;

    for (i = 0; i < MAX_NODES; ++i) {
        for (j = 0; j < MAX_NODES; ++j) {
            g_baseMatrix[i][j] = graph->matrix[i][j];
        }
    }
}

static void clearRoadChanges(void)
{
    int i;

    g_roadChangeCount = 0;
    for (i = 0; i < MAX_ROAD_CHANGES; ++i) {
        g_roadChanges[i].type = 0;
        g_roadChanges[i].from = 0;
        g_roadChanges[i].to = 0;
        g_roadChanges[i].distance = 0;
    }
}

static void applyRoadChangesToGraph(CampusGraph* graph)
{
    int i;
    int j;
    int a;
    int b;
    int value;

    for (i = 0; i < MAX_NODES; ++i) {
        for (j = 0; j < MAX_NODES; ++j) {
            graph->matrix[i][j] = g_baseMatrix[i][j];
        }
    }

    for (i = 0; i < g_roadChangeCount; ++i) {
        a = g_roadChanges[i].from;
        b = g_roadChanges[i].to;
        if (a < 0 || b < 0 || a >= graph->nodeCount || b >= graph->nodeCount || a == b) {
            continue;
        }

        value = g_roadChanges[i].distance;
        graph->matrix[a][b] = value;
        graph->matrix[b][a] = value;
    }
}

static void updateRoadChangeControls(HWND hwnd)
{
    int i;
    int x;
    int y;
    char label[128];

    for (i = 0; i < MAX_ROAD_CHANGES; ++i) {
        if (g_changeMinusButtons[i] == NULL || g_changeLabels[i] == NULL) {
            continue;
        }

        if (i < g_roadChangeCount) {
            if (i < 3) {
                x = 274;
                y = 142 + i * 24;
            }
            else {
                x = 24;
                y = 178 + (i - 3) * 24;
            }

            MoveWindow(g_changeMinusButtons[i], x, y, 22, 22, TRUE);
            MoveWindow(g_changeLabels[i], x + 28, y + 2, 220, 22, TRUE);
            if (g_roadChanges[i].type == ROAD_CHANGE_BLOCK) {
                sprintf_s(label, sizeof(label), "Block %d-%d weight %d",
                    g_roadChanges[i].from, g_roadChanges[i].to, g_roadChanges[i].distance);
            }
            else {
                sprintf_s(label, sizeof(label), "Add %d-%d weight %d",
                    g_roadChanges[i].from, g_roadChanges[i].to, g_roadChanges[i].distance);
            }
            SetWindowTextA(g_changeLabels[i], label);
            ShowWindow(g_changeMinusButtons[i], SW_SHOW);
            ShowWindow(g_changeLabels[i], SW_SHOW);
        }
        else {
            ShowWindow(g_changeMinusButtons[i], SW_HIDE);
            ShowWindow(g_changeLabels[i], SW_HIDE);
        }
    }

    InvalidateRect(hwnd, NULL, TRUE);
}

static int appendRoadChange(HWND hwnd, int type, char* out, int outSize)
{
    int from;
    int to;
    int distance;

    if (g_roadChangeCount >= MAX_ROAD_CHANGES) {
        appendFormat(out, outSize, "Road Change list is full. Delete one change first. Max changes: %d", MAX_ROAD_CHANGES);
        return 0;
    }

    if (!readIndexFromEdit(g_roadFromEdit, &from)
        || !readIndexFromEdit(g_roadToEdit, &to)
        || !readIndexFromEdit(g_roadDistEdit, &distance)) {
        appendText(out, outSize, "Please input numeric From, To, and Dist values.");
        return 0;
    }

    if (from < 0 || to < 0 || from >= g_graph.nodeCount || to >= g_graph.nodeCount || from == to) {
        appendText(out, outSize, "Invalid road endpoints. From and To must be different valid node indexes.");
        return 0;
    }

    if (type == ROAD_CHANGE_ADD && (distance <= 0 || distance >= ROAD_BLOCK)) {
        appendText(out, outSize, "Road Adding needs a distance between 1 and 9998.");
        return 0;
    }

    if (type == ROAD_CHANGE_BLOCK && distance < ROAD_BLOCK) {
        distance = ROAD_BLOCK;
    }

    g_roadChanges[g_roadChangeCount].type = type;
    g_roadChanges[g_roadChangeCount].from = from;
    g_roadChanges[g_roadChangeCount].to = to;
    g_roadChanges[g_roadChangeCount].distance = distance;
    ++g_roadChangeCount;

    applyRoadChangesToGraph(&g_graph);
    updateRoadChangeControls(hwnd);

    if (type == ROAD_CHANGE_BLOCK) {
        appendFormat(out, outSize, "Road Block added: %d-%d, distance changed to %d.\r\n\r\n", from, to, distance);
    }
    else {
        appendFormat(out, outSize, "Road Adding added: %d-%d, distance changed to %d.\r\n\r\n", from, to, distance);
    }
    buildRoadChangeText(out, outSize);
    return 1;
}

static void deleteRoadChange(HWND hwnd, int index, char* out, int outSize)
{
    int i;

    if (index < 0 || index >= g_roadChangeCount) {
        appendText(out, outSize, "Invalid road change index.");
        return;
    }

    appendFormat(out, outSize, "Deleted road change %d.\r\n\r\n", index + 1);
    for (i = index; i < g_roadChangeCount - 1; ++i) {
        g_roadChanges[i] = g_roadChanges[i + 1];
    }
    --g_roadChangeCount;

    applyRoadChangesToGraph(&g_graph);
    updateRoadChangeControls(hwnd);
    g_lastStart = -1;
    g_lastTarget = -1;
    g_viewMode = VIEW_GRAPH;
    resetVisualization();
    buildRoadChangeText(out, outSize);
}

static void buildRoadChangeText(char* out, int outSize)
{
    int i;

    appendText(out, outSize, "Current Road Change list:\r\n");
    if (g_roadChangeCount == 0) {
        appendText(out, outSize, "No road changes.\r\n");
        return;
    }

    for (i = 0; i < g_roadChangeCount; ++i) {
        if (g_roadChanges[i].type == ROAD_CHANGE_BLOCK) {
            appendFormat(out, outSize, "%d. Road Block: %d <-> %d, distance %d\r\n",
                i + 1, g_roadChanges[i].from, g_roadChanges[i].to, g_roadChanges[i].distance);
        }
        else {
            appendFormat(out, outSize, "%d. Road Adding: %d <-> %d, distance %d\r\n",
                i + 1, g_roadChanges[i].from, g_roadChanges[i].to, g_roadChanges[i].distance);
        }
    }
}

static void appendText(char* out, int outSize, const char* text)
{
    size_t used;

    used = strlen(out);
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

static void appendCenteredCell(char* out, int outSize, const char* text, int width)
{
    int len;
    int left;
    int right;
    int i;

    len = (int)strlen(text);
    if (len > width) {
        len = width;
    }
    left = (width - len) / 2;
    right = width - len - left;

    for (i = 0; i < left; ++i) {
        appendText(out, outSize, " ");
    }
    appendText(out, outSize, text);
    for (i = 0; i < right; ++i) {
        appendText(out, outSize, " ");
    }
}

static void buildProcessText(int mode, int routeMode, char* out, int outSize)
{
    out[0] = '\0';
    appendProcessText(mode, routeMode, out, outSize);
}

static void appendProcessText(int mode, int routeMode, char* out, int outSize)
{
    if (strlen(out) > 0) {
        appendText(out, outSize, "\r\n\r\n");
    }

    if (mode == VIEW_CONNECTIVITY_DIAGRAM) {
        appendText(out, outSize, "Connectivity implementation process\r\n");
        appendText(out, outSize, "-----------------------------------\r\n");
        appendText(out, outSize, "1. Start DFS from node 0.\r\n");
        appendText(out, outSize, "2. Mark every reachable node on the map.\r\n");
        appendText(out, outSize, "3. If unvisited nodes remain, add a random edge.\r\n");
        appendText(out, outSize, "4. Repeat until all nodes are reachable.\r\n");
        appendText(out, outSize, "\r\nRight map: green nodes show DFS connectivity traversal order.");
    }
    else if (mode == VIEW_DFS_DIAGRAM) {
        appendText(out, outSize, "DFS route implementation process\r\n");
        appendText(out, outSize, "--------------------------------\r\n");
        appendText(out, outSize, "[Start DFS search] -> [First target hit]\r\n");
        appendText(out, outSize, "                          |\r\n");
        appendText(out, outSize, "                          v\r\n");
        appendText(out, outSize, "[Compare route priority] <- [Continue DFS backtracking]\r\n");
        appendText(out, outSize, "          |\r\n");
        appendText(out, outSize, "          v\r\n");
        appendText(out, outSize, "[Trace best route]\r\n");
        appendText(out, outSize, "\r\nRight map: blue edges show first DFS route; red edges show best DFS route.");
    }
    else if (routeMode == ROUTE_MODE_SHORTEST_DISTANCE) {
        appendText(out, outSize,
            g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
            ? "A* shortest-distance route implementation process\r\n"
            : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
            ? "Dijkstra shortest-distance route implementation process\r\n"
            : "BFS shortest-distance route implementation process\r\n");
        appendText(out, outSize, "------------------------------------------------\r\n");
        appendText(out, outSize,
            g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
            ? "[Initialize open set] -> [Pick lowest f=g+h]\r\n"
            : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
            ? "[Initialize distances] -> [Pick lowest priority]\r\n"
            : "[Enqueue start node] -> [Queue relax roads]\r\n");
        appendText(out, outSize, "                            |\r\n");
        appendText(out, outSize, "                            v\r\n");
        appendText(out, outSize, "[Record parent links]  <- [Tie by fewer nodes]\r\n");
        appendText(out, outSize, "          |\r\n");
        appendText(out, outSize, "          v\r\n");
        appendText(out, outSize, "[Trace best route]\r\n");
        appendText(out, outSize,
            g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
            ? "\r\nRight map: purple nodes show A* expansion with f(n)=g(n)+h(n). Click a node to show shortest distance from start."
            : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
            ? "\r\nRight map: blue nodes show Dijkstra expansion. Click a node to show shortest distance from start."
            : "\r\nRight map: same-hop nodes share colors while BFS queue relaxation keeps the shortest distance.");
    }
    else {
        appendText(out, outSize,
            g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
            ? "A* fewest-node route implementation process\r\n"
            : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
            ? "Dijkstra fewest-node route implementation process\r\n"
            : "BFS fewest-node route implementation process\r\n");
        appendText(out, outSize, "-------------------------------------------\r\n");
        appendText(out, outSize,
            g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
            ? "[Initialize open set] -> [Pick lowest f=g+h]\r\n"
            : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
            ? "[Initialize priorities] -> [Pick lowest priority]\r\n"
            : "[Enqueue start node] -> [Dequeue front node]\r\n");
        appendText(out, outSize, "                            |\r\n");
        appendText(out, outSize, "                            v\r\n");
        appendText(out, outSize, "[Record parent links]  <- [Tie by lower distance]\r\n");
        appendText(out, outSize, "          |\r\n");
        appendText(out, outSize, "          v\r\n");
        appendText(out, outSize, "[Trace best route]\r\n");
        appendText(out, outSize,
            g_routeAlgorithm == ROUTE_ALGORITHM_ASTAR
            ? "\r\nRight map: purple nodes show A* hop expansion with h(n). Click a node to show shortest distance from start."
            : g_routeAlgorithm == ROUTE_ALGORITHM_DIJKSTRA
            ? "\r\nRight map: blue nodes show Dijkstra expansion. Click a node to show shortest distance from start."
            : "\r\nRight map: same-hop nodes share the same color. Click a node to show shortest distance from start.");
    }
}

static void buildNodeText(CampusGraph* graph, char* out, int outSize)
{
    int i;

    appendText(out, outSize, "Campus node list:\r\n");
    for (i = 0; i < graph->nodeCount; ++i) {
        appendFormat(out, outSize, "%2d. %s\r\n", i, graph->names[i]);
    }
}

static void buildMatrixText(CampusGraph* graph, char* out, int outSize)
{
    int i;
    int j;
    int k;
    char cell[32];
    const int cellWidth = 8;

    appendText(out, outSize, "Adjacency matrix table (9999 means road block, -- means no direct road):\r\n\r\n");

    appendText(out, outSize, "+");
    for (k = 0; k <= graph->nodeCount; ++k) {
        for (j = 0; j < cellWidth; ++j) {
            appendText(out, outSize, "-");
        }
        appendText(out, outSize, "+");
    }
    appendText(out, outSize, "\r\n|");
    appendCenteredCell(out, outSize, "Node", cellWidth);
    appendText(out, outSize, "|");
    for (i = 0; i < graph->nodeCount; ++i) {
        sprintf_s(cell, sizeof(cell), "%d", i);
        appendCenteredCell(out, outSize, cell, cellWidth);
        appendText(out, outSize, "|");
    }
    appendText(out, outSize, "\r\n");
    appendText(out, outSize, "+");
    for (k = 0; k <= graph->nodeCount; ++k) {
        for (j = 0; j < cellWidth; ++j) {
            appendText(out, outSize, "-");
        }
        appendText(out, outSize, "+");
    }
    appendText(out, outSize, "\r\n");

    for (i = 0; i < graph->nodeCount; ++i) {
        appendText(out, outSize, "|");
        sprintf_s(cell, sizeof(cell), "%d", i);
        appendCenteredCell(out, outSize, cell, cellWidth);
        appendText(out, outSize, "|");
        for (j = 0; j < graph->nodeCount; ++j) {
            if (graph->matrix[i][j] >= INF) {
                strcpy_s(cell, sizeof(cell), "--");
            }
            else {
                sprintf_s(cell, sizeof(cell), "%d", graph->matrix[i][j]);
            }
            appendCenteredCell(out, outSize, cell, cellWidth);
            appendText(out, outSize, "|");
        }
        appendText(out, outSize, "\r\n");
    }
    appendText(out, outSize, "+");
    for (k = 0; k <= graph->nodeCount; ++k) {
        for (j = 0; j < cellWidth; ++j) {
            appendText(out, outSize, "-");
        }
        appendText(out, outSize, "+");
    }
    appendText(out, outSize, "\r\n");
}

static void buildPathText(CampusGraph* graph, int start, int target, int parent[], const char* title, char* out, int outSize)
{
    int path[MAX_NODES];
    int count;
    int cur;
    int i;

    count = 0;
    cur = target;
    while (cur != -1 && count < MAX_NODES) {
        path[count++] = cur;
        if (cur == start) {
            break;
        }
        cur = parent[cur];
    }

    appendFormat(out, outSize, "%s:\r\n", title);
    for (i = count - 1; i >= 0; --i) {
        appendText(out, outSize, graph->names[path[i]]);
        if (i > 0) {
            appendText(out, outSize, " -> ");
        }
    }
    appendFormat(out, outSize, "\r\n\r\nNode count: %d\r\nHop count: %d\r\nTotal route weight: %d\r\n",
        count, count - 1, pathDistance(graph, start, target, parent));
}

static int readIndexFromEdit(HWND edit, int* value)
{
    char text[32];

    GetWindowTextA(edit, text, sizeof(text));
    return sscanf_s(text, "%d", value) == 1;
}

static COLORREF levelColor(int level)
{
    static COLORREF colors[] = {
        RGB(254, 215, 170),
        RGB(191, 219, 254),
        RGB(187, 247, 208),
        RGB(254, 240, 138),
        RGB(221, 214, 254),
        RGB(204, 251, 241),
        RGB(252, 165, 165)
    };
    int count;

    count = (int)(sizeof(colors) / sizeof(colors[0]));
    if (level < 0) {
        return RGB(224, 242, 254);
    }
    return colors[level % count];
}

static int hitTestNode(int x, int y)
{
    int i;
    int dx;
    int dy;

    if (!g_generated) {
        return -1;
    }

    for (i = 0; i < g_graph.nodeCount; ++i) {
        dx = x - g_nodeX[i];
        dy = y - g_nodeY[i];
        if (dx * dx + dy * dy <= 28 * 28) {
            return i;
        }
    }
    return -1;
}

static void startAnimation(HWND hwnd)
{
    int i;

    g_animationMax = 0;
    for (i = 0; i < g_graph.nodeCount; ++i) {
        if (g_visitOrder[i] > g_animationMax) {
            g_animationMax = g_visitOrder[i];
        }
    }

    if (g_animationMax > 0) {
        g_animationStep = 0;
        KillTimer(hwnd, 1);
        SetTimer(hwnd, 1, 350, NULL);
    }
    else {
        g_animationStep = 0;
    }
}

static void setResultText(const char* text)
{
    if (g_resultEdit != NULL) {
        SetWindowTextA(g_resultEdit, text);
    }
}
