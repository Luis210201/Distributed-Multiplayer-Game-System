typedef enum direction_t {UP, DOWN, LEFT, RIGHT} direction_t;

typedef struct lizard_t{   
    int pos_x;
    int pos_y;
    char id;
    int score;  
    struct lizard_t *next;
    direction_t orientation;
    /*
    
        If the lizard moved UP: direction of the body will be UP
                          DOWN:                               DOWN
                          .
                          .
                          .
    
    */
}lizard_t;

typedef struct cockroach_t{
    int pos_x;
    int pos_y;
    int id;
    int score;
}cockroach_t;

typedef struct wasp_t{
    int pos_x;
    int pos_y;
    int id;
}wasp_t;

typedef struct remote_char_t{
    bool msg_type; //TRUE - connection; FALSE - movement;
    bool terminate; // TRUE - terminates; FALSE - does not terminate
    bool entity_type; // true - lizard; false - cockroach
    bool bot_type; // TRUE - roach    FALSE - wasp
    lizard_t lizard;
    cockroach_t cockroach;
    wasp_t wasp;
    direction_t direction;
}remote_char_t;
