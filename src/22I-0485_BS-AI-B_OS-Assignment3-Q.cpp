#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>     // For usleep()
#include <termios.h>    // For terminal I/O control
#include <fcntl.h>      // For setting non-blocking input
#include <pthread.h>    // For pthreads
#include <cstring>      // For memset

using namespace std;

int boardSize;
char** board;
int playerScore = 0, player2Score = 0;
int playerX = 0, playerY = 0;  // Player 1's initial position
int player2X = 0, player2Y = 0;  // Player 2's initial position
int pipefd[2], pipefd2[2];  // Using unnamed pipes for communication between threads

pthread_t inputThread, inputThread2;

void initializeBoard(int size) 
{
    board = new char*[size];

    for (int i = 0; i < size; ++i) 
    {
        board[i] = new char[size];
    
        for (int j = 0; j < size; ++j) 
            board[i][j] = (rand() % 4 == 0) ? '*' : '.';  // 25% chance of item '*'
    }
}

void displayBoard() 
{
    system("clear");  // Clear the screen for updated display

    cout << "Board:" << endl;
    for (int i = 0; i < boardSize; ++i) 
    {
        for (int j = 0; j < boardSize; ++j) 
        {
            if (i == playerX && j == playerY) 
                cout << 'P' << ' ';  // Player 1's position
            else if (i == player2X && j == player2Y)
                cout << 'Q' << ' ';  // Player 2's position
            else
                cout << board[i][j] << ' ';
        }
        cout << endl;
    }
    cout << "\nCurrent Scores: " << endl;
    cout << "> (Player 1) Score: " << playerScore << " | (Player 2) Score: " << player2Score << endl;
}

// Setting terminal to non-blocking mode
void enableNonBlockingInput() 
{
    struct termios term;

    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag &= ~(ICANON | ECHO);  // Turning off canonical mode and echo

    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);  // Setting non-blocking input
}

// Restoring terminal to original mode
void disableNonBlockingInput() 
{
    struct termios term;

    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag |= (ICANON | ECHO);  // Turning on canonical mode and echo

    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// void movePlayer(char direction, int player) 
// {
//     int* playerXPtr = (player == 1) ? &playerX : &player2X;
//     int* playerYPtr = (player == 1) ? &playerY : &player2Y;
//     int* scorePtr = (player == 1) ? &playerScore : &player2Score;

//     int newX = *playerXPtr, newY = *playerYPtr;

//     if (direction == 'w' && newX > 0) newX--;          // Up
//     else if (direction == 's' && newX < boardSize - 1) newX++; // Down
//     else if (direction == 'a' && newY > 0) newY--;      // Left
//     else if (direction == 'd' && newY < boardSize - 1) newY++; // Right



//     else if (direction == 65 && newX > 0) newX--;      // Arrow Up (Player 2)
//     else if (direction == 66 && newX < boardSize - 1) newX++; // Arrow Down (Player 2)
//     else if (direction == 67 && newY < boardSize - 1) newY++; // Arrow Right (Player 2)
//     else if (direction == 68 && newY > 0) newY--;      // Arrow Left (Player 2)

//     // Collect item if present
//     if (board[newX][newY] == '*') {
//         (*scorePtr)++;
//         board[newX][newY] = '.';
//     }

//     // Update player position
//     *playerXPtr = newX;
//     *playerYPtr = newY;
// }

void movePlayer(char direction) 
{
    int newX = playerX, newY = playerY;

    if (direction == 'w' && playerX > 0) newX--;          // Up
    else if (direction == 's' && playerX < boardSize - 1) newX++; // Down
    else if (direction == 'a' && playerY > 0) newY--;      // Left
    else if (direction == 'd' && playerY < boardSize - 1) newY++; // Right

    // Collecting item if present
    if (board[newX][newY] == '*') 
    {
        playerScore++;
    
        board[newX][newY] = '.';
    }

    // Updating player position
    playerX = newX;
    playerY = newY;
}

void movePlayer2(char direction) 
{
    int newX = player2X, newY = player2Y;

    if (direction == 'i' && player2X > 0) newX--;          // Up
    else if (direction == 'k' && player2X < boardSize - 1) newX++; // Down
    else if (direction == 'j' && player2Y > 0) newY--;      // Left
    else if (direction == 'l' && player2Y < boardSize - 1) newY++; // Right

    // Collecting item if present
    if (board[newX][newY] == '*') 
    {
        player2Score++;
    
        board[newX][newY] = '.';
    }

    // Updating player position
    player2X = newX;
    player2Y = newY;
}


// Input handling for both players
void* inputThreadFunc(void* arg) 
{
    char key;

    while (true) 
    {
        // Reading a key press
        key = getchar();
        
        if (key != EOF) 
        {
            // Sending the keypress to the main thread through the pipe
            write(pipefd[1], &key, sizeof(key));

            if (key == 27)
                break;  // Exiting the input thread; if escape key pressed
        }

        usleep(10000);  // Small delay for better input handling
    }
}

void* inputThreadFunc2(void* arg) 
{
    char key;

    while (true) 
    {
        // Reading a key press
        key = getchar();
        
        if (key != EOF) 
        {
            // Sending the keypress to the main thread through the pipe
            write(pipefd2[1], &key, sizeof(key));

            if (key == 27)
                break;  // Exiting the input thread; if escape key is pressed
        }

        usleep(10000);  // Small delay for better input handling
    }
}

// Function to set thread priority
void setThreadPriority(pthread_t thread, int score) {
    pthread_attr_t attr;
    struct sched_param sched;

    // Initializing thread attributes
    pthread_attr_init(&attr);
    
    // Setting scheduling policy (SCHED_FIFO gives more control over priority)
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    // Setting thread priority based on player score
    // The priority scale for SCHED_FIFO is typically from 1 (lowest) to 99 (highest)
    sched.sched_priority = std::min(99, std::max(1, score)); // Ensuring priority is within valid range
    pthread_attr_setschedparam(&attr, &sched);

    // Applying the attributes to the thread
    pthread_setschedparam(thread, SCHED_FIFO, &sched);
}

int main() 
{
    srand(time(0));

    int rollNumber = 485;
    int randomNum = rand() % 90 + 10;

    boardSize = (rollNumber % randomNum) % 25;

    int player2X = boardSize - 1, player2Y = boardSize - 1;    

    if (boardSize < 10) boardSize += 15;

    initializeBoard(boardSize);
    
    // Creating pipes for communication between threads
    if (pipe(pipefd) == -1 || pipe(pipefd2) == -1) 
    {
        std::cerr << "Pipe creation failed!" << std::endl;
    
        return 1;
    }

    // Creating the input threads for both players
    if (pthread_create(&inputThread, nullptr, inputThreadFunc, nullptr) != 0 || 
        pthread_create(&inputThread2, nullptr, inputThreadFunc2, nullptr) != 0) 
    {
        std::cerr << "Error creating input threads" << std::endl;
        
        return 1;
    }

    // Setting initial priorities based on scores
    setThreadPriority(inputThread, playerScore);
    setThreadPriority(inputThread2, player2Score);

    enableNonBlockingInput();

    while (true) 
    {
        displayBoard();

        char key;

        // Checking if there is any key pressed for Player 1
        if (read(pipefd[0], &key, sizeof(key)) > 0) 
        {
            if (key == 27) 
            {
                cout << "\nFinal Scores: " << endl;
                cout << "> Player 1 Final Score: " << playerScore << endl;
                cout << "> Player 2 Final Score: " << player2Score << endl;

                cout << "\nFinal Result: " << endl;

                if (playerScore > player2Score)
                    cout << "> Player 1 wins!" << endl;
                else if (playerScore < player2Score)
                    cout << "> Player 2 wins!" << endl;
                else
                    cout << "> It's a tie!" << endl;

                break;  
            }

            movePlayer(key);  // Moving player 1 based on the key pressed

            // usleep(500000);  // Delaying for better visibility

            // Dynamically adjust thread priority based on scores
            if (playerScore > player2Score)
                setThreadPriority(inputThread, playerScore);   // Player 1 has higher priority
            else
                setThreadPriority(inputThread2, player2Score); // Player 2 has higher priority
        }

        // Checking if there is any key pressed for Player 2
        if (read(pipefd2[0], &key, sizeof(key)) > 0) 
        {
            if (key == 27) 
            {
                cout << "\nFinal Scores: " << endl;
                cout << "> Player 1 Final Score: " << playerScore << endl;
                cout << "> Player 2 Final Score: " << player2Score << endl;
            
                cout << "\nFinal Result: " << endl;

                if (playerScore > player2Score)
                    cout << "> Player 1 wins!" << endl;
                else if (playerScore < player2Score)
                    cout << "> Player 2 wins!" << endl;
                else
                    cout << "> It's a tie!" << endl;
    
                break;
            }

            movePlayer2(key);

            // usleep(500000);  // Delaying for better visibility

            // Dynamically adjust thread priority based on scores
            if (player2Score > playerScore)
                setThreadPriority(inputThread2, player2Score); // Player 2 has higher priority
            else
                setThreadPriority(inputThread, playerScore);   // Player 1 has higher priority
        }

        usleep(500000);  // Delaying for better visibility
    }

    disableNonBlockingInput();

    pthread_cancel(inputThread);  // Canceling the input thread when exiting
    
    pthread_cancel(inputThread2);  // Canceling the second input thread

    // Cleaning up dynamically allocated memory
    for (int i = 0; i < boardSize; ++i)
        delete[] board[i];
    delete[] board;

    return 0;
}
