#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "list.h"
#include <stdbool.h>
#include <ncurses.h>
#include "remote-char.h"
#include "display_messages.h"
#include <string.h>


/******************************************************************************
 * iniLista ()
 *
 * Arguments: none
 * Returns: lizard_t *
 * Side-Effects: list is initialized
 *
 * Description: initializes list
 *****************************************************************************/

lizard_t  *iniLista()
{
  return NULL;
}


/******************************************************************************
 * criaNovoNoLista ()
 *
 * Arguments: nome - Item to save in list node
 * Returns: lizard_t  *
 * Side-Effects: none
 *
 * Description: creates and returns a new node that can later be added to the
 *              list
 *****************************************************************************/

lizard_t  *criaNovoNoLista (lizard_t* lizard_head, char id, int pos_x, int pos_y, int score)
{
  lizard_t *newNode;

  if(lizard_head == NULL){
    newNode = (lizard_t*) malloc(sizeof(lizard_t));
    if(newNode!=NULL) {
      newNode->id = id;
      newNode->pos_x = pos_x;
      newNode->pos_y = pos_y;
      newNode->score = score;
      lizard_head = newNode;
      newNode->next = NULL;
    }
  }
  else{

    newNode = (lizard_t*) malloc(sizeof(lizard_t));
    if(newNode!=NULL) {
      newNode->id = id;
      newNode->pos_x = pos_x;
      newNode->pos_y = pos_y;
      newNode->score = score;
      newNode->next = lizard_head;
    }
  }

  return newNode; //retorna a nova head da lista;
}


/******************************************************************************
 * getItemLista ()
 *
 * Arguments: this - pointer to element
 * Returns: Item
 * Side-Effects: none
 *
 * Description: returns an Item from the list
 *****************************************************************************/

char get_id_lizard (lizard_t *l)
{
  return l->id;
}

//it will return the lizard list head! even if it is not changed!
lizard_t* remove_lizard(lizard_t *lizard_head, char id){

  lizard_t *aux, *aux_to_be_free;

  aux = lizard_head;
  
  if(get_id_lizard(aux) == id){

    lizard_head = aux->next;
    free(aux);
  }
  else{
    while(aux->next!=NULL){
      if(get_id_lizard(aux->next) != id){
        aux = aux->next;
      }
      else{
        aux_to_be_free = aux->next;
        aux->next = aux->next->next;
        //dar free 
        free(aux_to_be_free);
      }
    }
  }

  return lizard_head; 
}

//will search for a specific lizard and return a pointer to its structure!!
lizard_t* search_lizard(lizard_t *lizard_head, char id){

  lizard_t *aux;  /* auxiliar pointers to travel through the list */
  // printf("Vou a procura do id -> %c\n", id);

  for(aux = lizard_head; aux != NULL; aux = aux -> next){
    // printf("Id deste no -> %c\n", aux->id);
    if(aux->id == id){
      break;
    }
  }

  return aux;
}

/******************************************************************************
 * libertaLista ()
 *
 * Arguments: lp - pointer to list
 * Returns:  (void)
 * Side-Effects: frees space occupied by list items
 *
 * Description: free list
 *
 *****************************************************************************/

void libertaLista(lizard_t *lp) {
  lizard_t *aux, *newhead;  /* auxiliar pointers to travel through the list */

  for(aux = lp; aux != NULL; aux = newhead) {
    newhead = aux->next;
    free(aux);
  }
}

void mostra_lista(lizard_t *lp) {
  lizard_t *aux, *newhead;  /* auxiliar pointers to travel through the list */

  for(aux = lp; aux != NULL; aux = newhead) {
    printf("Novo no e este é o meu id -> %c\n", aux->id);
    newhead = aux->next;
  }

  if(lp == NULL){

    printf("jhbvncgvhmbj,nkhvgj\n");
  }
}

//percorrer a lista e mostrar o score
void show_lizard_scores(lizard_t *head, display_message *display){

  lizard_t *aux;  /* auxiliar pointers to travel through the list */
  int i=0;

  for(aux = head; aux != NULL; aux = aux -> next){

    display->information.symbol_array[i] = aux->id;
    display->information.score_array[i] = aux->score;
    i++;

  }

  //fill the rest of the array with default values!
  while(i<26){

    display->information.symbol_array[i] = ' ';
    display->information.score_array[i] = -1;
    i++;
  }
}