#include "menuTree.h"
#include <stdlib.h>
#include <string.h>
#include  <stdbool.h>


bool hasChildWithKey(MenuNode *node, char key);
static void printMenuRecursive(MenuNode *node, int level);



MenuNode* create_node(const char* title, char key, int id) {
    MenuNode *node = (MenuNode*)malloc(sizeof(MenuNode));
    if (node == NULL) {
        return NULL;
    }

    // Inicializa los punteros a NULL
    memset(node->children, 0, sizeof(node->children));

    // Copia el título de manera segura
    strncpy(node->title, title, sizeof(node->title) - 1);
    node->title[sizeof(node->title) - 1] = '\0';  // Asegurar terminación de cadena

    node->key = key;
    node->parent = NULL;
    node->child_count = 0;
    node->id = id;  // Inicializar el ID en 0

    return node;
}

// Agregar hijo con tecla de acceso
void add_child(MenuNode *parent, MenuNode *child) {
    if (parent == NULL || child == NULL) {
        writeSerialComln("Error: Invalid parent or child node");
        return;
    }

    // Verificar si ya existe un hijo con la misma tecla
    if (hasChildWithKey(parent, child->key)) {
        writeSerialComln("Error: Child with this key already exists");
        return;
    }

    // Buscar un espacio disponible en el array de hijos
    if (parent->child_count < MAX_CHILDREN) {
        int i = 0;
        while (i < MAX_CHILDREN && parent->children[i] != NULL) {
            i++;
        }

        // Asegurarse de que no se excede el límite
        if (i < MAX_CHILDREN) {
            parent->children[i] = child;
            child->parent = parent;  // Se asigna el nodo padre
            parent->child_count++;
        } else {
            writeSerialComln("Error: No space available for new child");
        }
    }
}

MenuNode* menuInit() {

    MenuNode* root = create_node("MENU PRINCIPAL", '0', 0);

    

    MenuNode* node_1 = create_node("MENU DEBUG", '1', 1);
    add_child(root, node_1);

        MenuNode* node_11 = create_node("INFO LEVEL 1", '1', 12);
        add_child(node_1,node_11);
        MenuNode* node_12 = create_node("dataStruct", '2', 13);
        add_child(node_1,node_12);


    return root;

    


}



void menuUpdate(char caracter, MenuNode **current) {
    if (current == NULL || *current == NULL) {  
        return;  // Verifica que 'current' y '*current' no sean nulos
    }

    if (caracter == GO_BACK && (*current)->parent) {  // Si es GO_BACK, sube al nodo padre
        *current = (*current)->parent;
    } else {
        // Buscar si el carácter ingresado corresponde a un hijo
        for (int i = 0; i < (*current)->child_count; i++) {
            if ((*current)->children[i] != NULL && (*current)->children[i]->key == caracter) {
                *current = (*current)->children[i];  // Cambia al nodo hijo
                return;
            }
        }
    }
}
void printNode(MenuNode *node) {
    if (node == NULL) {
        return;
    }

    // Imprime el título del nodo padre
    writeSerialComln(node->title);

    // Imprime los hijos con su respectiva clave y título
    for (int i = 0; i < node->child_count; i++) {
        if (node->children[i] != NULL) {
            
            char key_str[2];
            key_str[0] = node->children[i]->key;
            key_str[1] = '\0';
            writeSerialCom("\t");
            writeSerialCom(key_str);
            writeSerialCom(" -> ");
            writeSerialComln(node->children[i]->title);
        }
    }
}



static void printMenuRecursive(MenuNode *node, int level)
{
    if (node == NULL) {
        return;
    }

    // Indentación según nivel
    for (int i = 0; i < level; i++) {
        writeSerialCom("\t");
    }

    // Imprime el nodo
    writeSerialComln(node->title);

    // Recorre hijos
    for (int i = 0; i < node->child_count; i++) {
        printMenuRecursive(node->children[i], level + 1);
    }
}

void printFullMenu(MenuNode *root)
{
    if (root == NULL) {
        return;
    }

    printMenuRecursive(root, 0);
}



bool hasChildWithKey(MenuNode *node, char key) {
    if (node == NULL) {
        return false;
    }

    // Verifica todos los hijos directos del nodo
    for (int i = 0; i < node->child_count; i++) {
        if (node->children[i] != NULL && node->children[i]->key == key) {
            return true;
        }
    }

    return false;
}

void freeMenu(MenuNode *node) {
    if (node == NULL) {
        return;  // Si el nodo es nulo, no hace nada
    }

    // Libera los nodos hijos recursivamente
    for (int i = 0; i < node->child_count; i++) {
        if (node->children[i] != NULL) {
            freeMenu(node->children[i]);  // Llamada recursiva
        }
    }

    // Después de liberar los hijos, libera el nodo actual
    free(node);
}

char* get_title(MenuNode* menu){
    if(menu == NULL){
        return NULL;
    }
    return menu->title;
}