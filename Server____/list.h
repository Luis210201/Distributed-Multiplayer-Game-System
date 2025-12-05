#ifndef _LISTA_H
#define _LISTA_H

/* type definition for structure to hold list item */
typedef struct lizard_t lizard_t;
typedef enum direction_t direction_t;
typedef struct wasp_t wasp_t;
typedef struct info info;
typedef struct display_message display_message;
typedef struct cockroach_t cockroach_t;


void show_lizard_scores(lizard_t *head, display_message *display);
lizard_t  *iniLista(void);
lizard_t  *criaNovoNoLista (lizard_t* lp, char id, int pos_x, int pos_y, int score);
char get_id_lizard (lizard_t *l);
lizard_t *remove_lizard(lizard_t *lizard_head, char id);
lizard_t *getnextElementoLista(lizard_t *l);
void mostra_lista(lizard_t *lp);
lizard_t* search_lizard(lizard_t *lizard_head, char id);

#endif
