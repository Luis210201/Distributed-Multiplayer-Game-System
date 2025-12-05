#define WINDOW_SIZE 30

typedef struct info{
    char symbol_array[26];
    int score_array[26]; //to avoid sending a lot of messages!

}info;

typedef struct display_message{
    int x;
    int y;
    char symbol;
    info information;
    bool informative; 

}display_message;
