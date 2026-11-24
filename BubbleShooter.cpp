#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
using namespace std;

//// wardan///////
const int ROWS = 10;
const int COLS = 10;
const int QUEUE_SIZE = 10;
const int STACK_SIZE = 20;
const int COLORS = 5;
const int START_ROWS = 4;        // rows filled with bubbles when a game starts
const int START_QUEUE = 5;       // how many upcoming bubbles the queue holds
const int MIN_MATCH = 3;         // smallest connected group that gets popped
const int DANGER_ROW = ROWS - 2; // any bubble on this row or below = game over
int board[ROWS][COLS];
bool visited[ROWS][COLS];
int score = 0;
int shotsTaken = 0;
int currentBubble = 1;
int nextGameID = 1;
bool gameActive = false;

// ============================ QUEUE (upcoming bubbles) ============================
class Queue {
  public:
    int bubbles[QUEUE_SIZE];
    int front = 0;
    int rear = -1;
    int count = 0;
};
Queue bubbleQueue;
bool isQueueEmpty() { return bubbleQueue.count == 0; }
bool isQueueFull() { return bubbleQueue.count == QUEUE_SIZE; }
void enqueue(int color) {
    if (isQueueFull()) {
        cout << "Queue is full!\n";
        return;
    }
    bubbleQueue.rear = (bubbleQueue.rear + 1) % QUEUE_SIZE;
    bubbleQueue.bubbles[bubbleQueue.rear] = color;
    bubbleQueue.count++;
}
int dequeue() {
    if (isQueueEmpty()) {
        return -1;
    }
    int color = bubbleQueue.bubbles[bubbleQueue.front];
    bubbleQueue.front = (bubbleQueue.front + 1) % QUEUE_SIZE;
    bubbleQueue.count--;
    return color;
}
int getFront() {
    if (isQueueEmpty())
        return -1;
    return bubbleQueue.bubbles[bubbleQueue.front];
}
int randomColor() { return rand() % COLORS + 1; } // any colour 1-5 (used to fill the starting board)
bool colorOnBoard(int color) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] == color)
                return true;
        }
    }
    return false;
}
// only colours still on the board, so every bubble can help clear it (no endless games)
int generateRandomBubble() {
    int available[COLORS];
    int n = 0;
    for (int color = 1; color <= COLORS; color++) {
        if (colorOnBoard(color))
            available[n++] = color;
    }
    if (n == 0) // empty board (game already won): any colour
        return randomColor();
    return available[rand() % n];
}
void initializeQueue() {
    bubbleQueue.front = 0;
    bubbleQueue.rear = -1;
    bubbleQueue.count = 0;
    for (int i = 0; i < START_QUEUE; i++) {
        enqueue(generateRandomBubble());
    }
}
// take the next colour out of the queue as the current bubble and refill the queue
void nextBubble() {
    // a colour may have been cleared from the board since it was queued: replace it
    for (int i = 0; i < bubbleQueue.count; i++) {
        int index = (bubbleQueue.front + i) % QUEUE_SIZE;
        if (!colorOnBoard(bubbleQueue.bubbles[index]))
            bubbleQueue.bubbles[index] = generateRandomBubble();
    }
    currentBubble = dequeue();
    if (currentBubble == -1) {
        currentBubble = generateRandomBubble();
    }
    enqueue(generateRandomBubble());
}

// ============================ STACK (undo) ============================
class GameState {
  public:
    int savedBoard[ROWS][COLS];
    int savedScore;
    int savedShots;
    int savedCurrentBubble;
    Queue savedQueue;
};
class Stack {
  public:
    GameState states[STACK_SIZE];
    int top = -1;
};
Stack undoStack;
void initializeStack() { undoStack.top = -1; }
bool isStackEmpty() { return undoStack.top == -1; }
bool isStackFull() { return undoStack.top == STACK_SIZE - 1; }
void push() {
    if (isStackFull()) {
        // history is full: throw away the OLDEST state (bottom of the stack)
        // so the most recent moves can still be undone one at a time
        for (int i = 0; i < STACK_SIZE - 1; i++) {
            undoStack.states[i] = undoStack.states[i + 1];
        }
        undoStack.top--;
    }
    undoStack.top++;
    GameState &state = undoStack.states[undoStack.top];
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            state.savedBoard[i][j] = board[i][j];
        }
    }
    state.savedScore = score;
    state.savedShots = shotsTaken;
    state.savedCurrentBubble = currentBubble;
    state.savedQueue = bubbleQueue;
}
bool pop(GameState &state) {
    if (isStackEmpty())
        return false;
    state = undoStack.states[undoStack.top--];
    return true;
}
bool peek(GameState &state) {
    if (isStackEmpty())
        return false;
    state = undoStack.states[undoStack.top];
    return true;
}
void undoMove() {
    if (!gameActive) {
        cout << "No active game to undo!\n";
        return;
    }
    GameState state;
    if (!pop(state)) {
        cout << "Nothing to undo!\n";
        return;
    }
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board[i][j] = state.savedBoard[i][j];
        }
    }
    score = state.savedScore;
    shotsTaken = state.savedShots;
    currentBubble = state.savedCurrentBubble;
    bubbleQueue = state.savedQueue;
    cout << "Previous move restored!\n";
}

// ============================ BOARD (2D array) ============================
bool isValidCell(int row, int col) { return row >= 0 && row < ROWS && col >= 0 && col < COLS; }
bool isEmpty(int row, int col) { return isValidCell(row, col) && board[row][col] == 0; }
void initializeBoard() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board[i][j] = 0;
        }
    }
    for (int i = 0; i < START_ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board[i][j] = randomColor();
        }
    }
}
void resetBoard() { initializeBoard(); }
int countBubbles() {
    int count = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] != 0)
                count++;
        }
    }
    return count;
}
void placeBubble(int row, int col, int color) {
    if (isValidCell(row, col) && isEmpty(row, col)) {
        board[row][col] = color;
    }
}
void removeBubble(int row, int col) {
    if (isValidCell(row, col)) {
        board[row][col] = 0;
    }
}
bool isBoardEmpty() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] != 0)
                return false;
        }
    }
    return true;
}
// a shot bubble flies upward, so it can only stop when it touches the top wall
// or a bubble above / beside it - it can never hang in empty space
bool isAttached(int row, int col) {
    if (row == 0)
        return true;
    if (board[row - 1][col] != 0)
        return true;
    if (col > 0 && board[row][col - 1] != 0)
        return true;
    if (col < COLS - 1 && board[row][col + 1] != 0)
        return true;
    return false;
}

// ============================ SCORING ============================
int calculateScore(int matches) { return matches * 10; }
void updateScore(int points) { score += points; }
void resetScore() { score = 0; }

// ============================ RECURSION (matching) ============================
int matchRows[ROWS * COLS];
int matchCols[ROWS * COLS];
int matchCount = 0;
void resetVisited() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            visited[i][j] = false;
        }
    }
}
bool isMatchingColor(int row, int col, int color) {
    return isValidCell(row, col) && !visited[row][col] && color != 0 && board[row][col] == color;
}
void checkConnected(int row, int col, int color) {
    if (!isMatchingColor(row, col, color))
        return;
    visited[row][col] = true;
    matchRows[matchCount] = row;
    matchCols[matchCount] = col;
    matchCount++;
    checkConnected(row - 1, col, color);
    checkConnected(row + 1, col, color);
    checkConnected(row, col - 1, color);
    checkConnected(row, col + 1, color);
}
int findMatches(int row, int col) {
    resetVisited();
    matchCount = 0;
    if (!isValidCell(row, col) || isEmpty(row, col))
        return 0;
    int color = board[row][col];
    checkConnected(row, col, color);
    return matchCount;
}
int countMatches() { return matchCount; }
// removes the group found by findMatches() only if it is big enough; returns how many popped
int removeMatches() {
    if (matchCount < MIN_MATCH)
        return 0;
    for (int i = 0; i < matchCount; i++) {
        removeBubble(matchRows[i], matchCols[i]);
    }
    cout << matchCount << " bubbles removed!\n";
    return matchCount;
}
void processMatches(int row, int col) {
    int totalMatches = findMatches(row, col);
    cout << "Connected bubbles: " << totalMatches << endl;
    int removed = removeMatches();
    if (removed > 0) {
        updateScore(calculateScore(removed));
        cout << "Score updated: " << score << endl;
    } else {
        cout << "No matching cluster found.\n";
    }
}

// ============================ WIN / LOSE ============================
bool checkWin() { return isBoardEmpty(); }
bool checkLose() {
    for (int i = DANGER_ROW; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board[i][j] != 0)
                return true;
        }
    }
    return false;
}






/////faizan//////
class GameRecord {
  public:
    int gameID;
    int score;
    int shots;
    bool won;
    GameRecord *next;
};
GameRecord *head = NULL;
void insertRecord(int id, int points, int shots, bool result) {
    GameRecord *newNode = new GameRecord;
    newNode->gameID = id;
    newNode->score = points;
    newNode->shots = shots;
    newNode->won = result;
    newNode->next = NULL;
    if (head == NULL) {
        head = newNode;
        return;
    }
    GameRecord *temp = head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newNode;
}
void displayRecords() {
    if (head == NULL) {
        cout << "No game records found!\n";
        return;
    }

    GameRecord *temp = head;
    cout << "\n========== GAME HISTORY ==========\n";
    while (temp != NULL) {
        cout << "Game ID: " << temp->gameID << " | Score: " << temp->score
             << " | Shots: " << temp->shots << " | Result: " << (temp->won ? "Win" : "Loss")
             << endl;
        temp = temp->next;
    }
}
GameRecord *searchRecord(int id) {
    GameRecord *temp = head;
    while (temp != NULL) {
        if (temp->gameID == id)
            return temp;
        temp = temp->next;
    }
    return NULL;
}
void deleteRecord(int id) {
    if (head == NULL)
        return;
    if (head->gameID == id) {
        GameRecord *temp = head;
        head = head->next;
        delete temp;
        cout << "Record deleted!\n";
        return;
    }
    GameRecord *temp = head;
    while (temp->next != NULL && temp->next->gameID != id) {
        temp = temp->next;
    }
    if (temp->next == NULL) {
        cout << "Record not found!\n";
        return;
    }
    GameRecord *del = temp->next;
    temp->next = del->next;
    delete del;
    cout << "Record deleted!\n";
}
int countRecords() {
    int count = 0;
    GameRecord *temp = head;
    while (temp != NULL) {
        count++;
        temp = temp->next;
    }
    return count;
}
GameRecord *getMiddle(GameRecord *start) {
    if (start == NULL)

        return NULL;
    GameRecord *slow = start;
    GameRecord *fast = start->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}
GameRecord *mergeLists(GameRecord *a, GameRecord *b) {
    if (a == NULL)
        return b;
    if (b == NULL)
        return a;
    GameRecord *result = NULL;
    if (a->score >= b->score) {
        result = a;
        result->next = mergeLists(a->next, b);
    } else {
        result = b;
        result->next = mergeLists(a, b->next);
    }
    return result;
}
GameRecord *mergeSort(GameRecord *start) {
    if (start == NULL || start->next == NULL)
        return start;
    GameRecord *middle = getMiddle(start);
    GameRecord *secondHalf = middle->next;
    middle->next = NULL;
    GameRecord *left = mergeSort(start);
    GameRecord *right = mergeSort(secondHalf);
    return mergeLists(left, right);
}
void sortRecords() {
    head = mergeSort(head);
    cout << "Game records sorted by score!\n";
}

// ============================ BST (leaderboard) ============================
class TreeNode {
  public:
    int score;
    int gameID;
    TreeNode *left;
    TreeNode *right;
};
TreeNode *root = NULL;
bool comesBefore(int score1, int id1, int score2, int id2) {
    if (score1 != score2)
        return score1 < score2;
    return id1 < id2;
}
TreeNode *insertNode(TreeNode *node, int points, int id) {
    if (node == NULL) {
        TreeNode *newNode = new TreeNode;
        newNode->score = points;
        newNode->gameID = id;
        newNode->left = NULL;
        newNode->right = NULL;
        return newNode;
    }
    if (comesBefore(points, id, node->score, node->gameID)) {
        node->left = insertNode(node->left, points, id);
    } else {
        node->right = insertNode(node->right, points, id);
    }
    return node;
}
TreeNode *searchNode(TreeNode *node, int points, int id) {
    if (node == NULL)
        return NULL;
    if (node->score == points && node->gameID == id)
        return node;
    if (comesBefore(points, id, node->score, node->gameID))

        return searchNode(node->left, points, id);
    return searchNode(node->right, points, id);
}
TreeNode *findMin(TreeNode *node) {
    if (node == NULL)
        return NULL;
    while (node->left != NULL) {
        node = node->left;
    }
    return node;
}
TreeNode *findMax(TreeNode *node) {
    if (node == NULL)
        return NULL;
    while (node->right != NULL) {
        node = node->right;
    }
    return node;
}
void inorder(TreeNode *node) {
    if (node == NULL)
        return;
    inorder(node->left);
    cout << "Game " << node->gameID << " | Score: " << node->score << endl;
    inorder(node->right);
}
void preorder(TreeNode *node) {
    if (node == NULL)
        return;
    cout << node->score << " ";
    preorder(node->left);
    preorder(node->right);
}
void postorder(TreeNode *node) {
    if (node == NULL)
        return;
    postorder(node->left);

    postorder(node->right);
    cout << node->score << " ";
}
TreeNode *deleteNode(TreeNode *node, int points, int id) {
    if (node == NULL)
        return NULL;
    if (comesBefore(points, id, node->score, node->gameID)) {
        node->left = deleteNode(node->left, points, id);
    } else if (comesBefore(node->score, node->gameID, points, id)) {
        node->right = deleteNode(node->right, points, id);
    } else {
        if (node->left == NULL) {
            TreeNode *temp = node->right;
            delete node;
            return temp;
        }
        if (node->right == NULL) {
            TreeNode *temp = node->left;
            delete node;
            return temp;
        }
        TreeNode *successor = findMin(node->right);
        node->score = successor->score;
        node->gameID = successor->gameID;
        node->right = deleteNode(node->right, successor->score, successor->gameID);
    }
    return node;
}
int height(TreeNode *node) {
    if (node == NULL)
        return 0;

    int leftHeight = height(node->left);
    int rightHeight = height(node->right);
    if (leftHeight > rightHeight)
        return leftHeight + 1;
    return rightHeight + 1;
}
int countNodes(TreeNode *node) {
    if (node == NULL)
        return 0;
    return 1 + countNodes(node->left) + countNodes(node->right);
}
// highest score ever recorded = right-most node of the BST
int getHighScore() {
    TreeNode *best = findMax(root);
    if (best == NULL)
        return 0;
    return best->score;
}

// ============================ SEARCHING (binary search) ============================
// inorder traversal of a BST visits keys in ascending order -> fills a sorted array
void storeInorder(TreeNode *node, int arr[], int &index) {
    if (node == NULL)
        return;
    storeInorder(node->left, arr, index);
    arr[index++] = node->score;
    storeInorder(node->right, arr, index);
}
int binarySearch(int arr[], int size, int key) {
    int low = 0;
    int high = size - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (arr[mid] == key)
            return mid;
        if (arr[mid] < key)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}

// ============================ RECORDS + LEADERBOARD (kept in sync) ============================
void updateRecord(int id, int newScore) {
    GameRecord *temp = searchRecord(id);
    if (temp == NULL) {
        cout << "Record not found!\n";
        return;
    }
    // the BST is ordered by score, so the old key must be removed and the new one inserted
    root = deleteNode(root, temp->score, temp->gameID);
    temp->score = newScore;
    root = insertNode(root, temp->score, temp->gameID);

    cout << "Record updated!\n";
}
void saveGameResult(bool won) {
    int id = nextGameID++;
    insertRecord(id, score, shotsTaken, won);
    root = insertNode(root, score, id);
    cout << "\nGame saved successfully!\n";
    cout << "Game ID: " << id << endl;
    cout << "Final score: " << score << endl;
    cout << "Best score so far: " << getHighScore() << endl;
}

// ============================ GAME CONTROL ============================
bool shootBubble(int row, int col) {
    if (!gameActive) {
        cout << "Start a game first!\n";
        return false;
    }
    if (!isValidCell(row, col) || !isEmpty(row, col)) {
        cout << "Invalid position!\n";
        return false;
    }
    if (!isAttached(row, col)) {
        cout << "A bubble must touch the top wall or another bubble (above or beside it)!\n";
        return false;
    }
    push();
    placeBubble(row, col, currentBubble);
    shotsTaken++;
    cout << "Bubble placed: " << currentBubble << endl;
    processMatches(row, col);
    nextBubble();
    return true;
}
void startGame() {
    initializeBoard();
    initializeQueue();

    initializeStack();
    resetScore();
    shotsTaken = 0;
    nextBubble();
    gameActive = true;
    cout << "\nNew game started!\n";
    cout << "Current bubble: " << currentBubble << endl;
}
void restartGame() {
    cout << "\nCurrent game abandoned (it is not saved).\n";
    startGame();
}
void endGame(bool won) {
    gameActive = false;
    saveGameResult(won);
    initializeStack(); // a finished game is final - its undo history is cleared
}
void clearRecords() {
    while (head != NULL) {
        GameRecord *temp = head;
        head = head->next;
        delete temp;
    }
}
void clearTree(TreeNode *node) {
    if (node == NULL)
        return;
    clearTree(node->left);
    clearTree(node->right);
    delete node;
}
void quitGame() {
    clearRecords();
    clearTree(root);
    root = NULL;
}





////nofal///
