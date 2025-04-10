#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MEM_SIZE 1024      // Tamaño total de la memoria simulada (KB)
#define MIN_BLOCK_SIZE 4   // Tamaño mínimo de un bloque (KB)
#define MAX_BLOCK_SIZE 128 // Tamaño máximo de un bloque (KB)

// Estructura para bloque de memoria
typedef struct MemoryBlock {
    int id;                 // Identificador del proceso
    int size;               // Tamaño en KB
    int address;            // Dirección de inicio (offset desde el inicio de la memoria)
    int is_allocated;       // 1 si está asignado, 0 si está libre
    struct MemoryBlock *next;
} MemoryBlock;


// Variables globales (memoria estática)
MemoryBlock *memory_list = NULL;  // Lista enlazada para administrar bloques de memoria
int total_memory = MEM_SIZE;      // Memoria total disponible
int used_memory = 0;              // Memoria actualmente en uso
int num_blocks = 0;               // Número de bloques asignados
int next_process_id = 1;          // ID para el siguiente proceso

// Prototipos de funciones
void inicializar_memoria();
void mostrar_estado_memoria();
void asignar_memoria_estatica();
MemoryBlock* asignar_memoria_dinamica(int proceso_id, int tamano);
void liberar_memoria(int proceso_id);
void desfragmentar_memoria();
void visualizar_memoria();
void menu();

int main() {
    srand(time(NULL));
    
    // Inicialización de la memoria simulada
    inicializar_memoria();
    
    printf("Bienvenido a la simulación de administración de memoria\n");
    printf("------------------------------------------------------\n\n");
    
    menu();
    
    // Liberar toda la memoria al salir
    MemoryBlock *current = memory_list;
    while (current != NULL) {
        MemoryBlock *temp = current;
        current = current->next;
        free(temp);
    }
    
    return 0;
}

void inicializar_memoria() {
    // Crear un único bloque libre que representa toda la memoria disponible
    memory_list = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    if (!memory_list) {
        perror("Error al inicializar la memoria");
        exit(1);
    }
    
    memory_list->id = 0;
    memory_list->size = total_memory;
    memory_list->address = 0;
    memory_list->is_allocated = 0;
    memory_list->next = NULL;
    
    printf("Memoria inicializada: %d KB disponibles\n", total_memory);
}

void mostrar_estado_memoria() {
    printf("\nEstado actual de la memoria:\n");
    printf("---------------------------\n");
    printf("Memoria total: %d KB\n", total_memory);
    printf("Memoria usada: %d KB (%.2f%%)\n", used_memory, (float)used_memory / total_memory * 100);
    printf("Memoria libre: %d KB (%.2f%%)\n", total_memory - used_memory, 
           (float)(total_memory - used_memory) / total_memory * 100);
    printf("Número de bloques asignados: %d\n", num_blocks);
}

void asignar_memoria_estatica() {
    // Simula la asignación de memoria estática (variables globales, código, etc.)
    int tamano_estatico = 64; // 64 KB fijos para código, variables globales, etc.
    
    printf("\n--- Asignación de Memoria Estática ---\n");
    printf("Asignando %d KB para código y variables globales...\n", tamano_estatico);
    
    // Usamos nuestra función de asignación dinámica pero marcamos este bloque como "estático"
    MemoryBlock *bloque = asignar_memoria_dinamica(0, tamano_estatico);
    
    if (bloque) {
        printf("Memoria estática asignada con éxito en la dirección %d\n", bloque->address);
        printf("Esta memoria permanecerá asignada durante toda la vida del programa\n");
    } else {
        printf("No se pudo asignar la memoria estática\n");
    }
}

MemoryBlock* asignar_memoria_dinamica(int proceso_id, int tamano) {
    MemoryBlock *current = memory_list;
    MemoryBlock *best_fit = NULL;
    int min_extra_space = total_memory + 1;
    
    // Buscar el bloque libre que mejor se ajuste (best-fit)
    while (current != NULL) {
        if (!current->is_allocated && current->size >= tamano) {
            int extra_space = current->size - tamano;
            if (extra_space < min_extra_space) {
                min_extra_space = extra_space;
                best_fit = current;
            }
        }
        current = current->next;
    }
    
    // Si no se encontró un bloque adecuado
    if (best_fit == NULL) {
        printf("Error: No hay suficiente memoria contigua disponible para %d KB\n", tamano);
        return NULL;
    }
    
    // Si el bloque encontrado es más grande que lo necesario, dividirlo
    if (best_fit->size > tamano && (best_fit->size - tamano) >= MIN_BLOCK_SIZE) {
        MemoryBlock *new_block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
        if (!new_block) {
            perror("Error al crear un nuevo bloque de memoria; es más grande que lo necesario");
            return NULL;
        }
        
        // Configurar el nuevo bloque (la parte libre restante)
        new_block->id = 0;
        new_block->size = best_fit->size - tamano;
        new_block->address = best_fit->address + tamano;
        new_block->is_allocated = 0;
        new_block->next = best_fit->next;
        
        // Actualizar el bloque original (la parte asignada)
        best_fit->size = tamano;
        best_fit->next = new_block;
    }
    
    // Marcar el bloque como asignado
    best_fit->id = proceso_id;
    best_fit->is_allocated = 1;
    
    // Actualizar estadísticas
    used_memory += tamano;
    if (proceso_id > 0) { // No contamos el bloque estático como un "proceso"
        num_blocks++;
    }
    
    return best_fit;
}

void liberar_memoria(int proceso_id) {
    if (proceso_id <= 0) {
        printf("Error: ID de proceso inválido\n");
        return;
    }
    
    MemoryBlock *current = memory_list;
    int memoria_liberada = 0;
    int bloques_liberados = 0;
    
    // Buscar y liberar todos los bloques del proceso
    while (current != NULL) {
        if (current->is_allocated && current->id == proceso_id) {
            current->is_allocated = 0;
            current->id = 0;
            memoria_liberada += current->size;
            bloques_liberados++;
        }
        current = current->next;
    }
    
    if (memoria_liberada > 0) {
        // Actualizar estadísticas
        used_memory -= memoria_liberada;
        num_blocks -= bloques_liberados;
        
        printf("Proceso %d: %d KB liberados (%d bloque(s))\n", 
               proceso_id, memoria_liberada, bloques_liberados);
        
        // Fusionar bloques libres adyacentes (coalescing)
        desfragmentar_memoria();
    } else {
        printf("No se encontraron bloques asignados al proceso %d\n", proceso_id);
    }
}

void desfragmentar_memoria() {
    int fusiones = 0;
    MemoryBlock *current = memory_list;
    
    while (current != NULL && current->next != NULL) {
        // Si el bloque actual y el siguiente están libres, fusionarlos
        if (!current->is_allocated && !current->next->is_allocated) {
            MemoryBlock *to_remove = current->next;
            
            // Aumentar el tamaño del bloque actual para absorber el siguiente
            current->size += to_remove->size;
            
            // Actualizar el puntero next para omitir el bloque fusionado
            current->next = to_remove->next;
            
            // Liberar la estructura del bloque fusionado
            free(to_remove);
            
            fusiones++;
            // No avanzar current aquí, ya que podríamos necesitar fusionar más bloques
        } else {
            // Solo avanzar si no se realizó una fusión
            current = current->next;
        }
    }
    
    if (fusiones > 0) {
        printf("Desfragmentación completada: %d bloques fusionados\n", fusiones);
    }
}

void visualizar_memoria() {
    printf("\nRepresentación visual de la memoria (%d KB total):\n", total_memory);
    printf("[");
    
    MemoryBlock *current = memory_list;
    int pos = 0;
    
    while (current != NULL) {
        // Calcular el ancho proporcional de este bloque para la visualización
        int block_width = (current->size * 50) / total_memory;
        if (block_width < 1) block_width = 1;
        
        // Imprimir el bloque
        for (int i = 0; i < block_width; i++) {
            if (current->is_allocated) {
                if (current->id == 0) {
                    printf("S"); // Memoria estática
                } else {
                    printf("%d", current->id % 10); // ID del proceso (último dígito)
                }
            } else {
                printf("."); // Memoria libre
            }
        }
        
        pos += current->size;
        current = current->next;
    }
    
    printf("]\n");
    printf("Leyenda: '.' = Libre, 'S' = Estática, '0-9' = ID de proceso (módulo 10)\n");
}

void menu() {
    int opcion = 0;
    int proceso_id, tamano;
    
    do {
        printf("\n=== MENÚ DE ADMINISTRACIÓN DE MEMORIA ===\n");
        printf("1. Inicializar memoria estática del sistema\n");
        printf("2. Asignar memoria dinámica para un proceso\n");
        printf("3. Liberar memoria de un proceso\n");
        printf("4. Mostrar estado de la memoria\n");
        printf("5. Visualizar mapa de memoria\n");
        printf("6. Salir\n");
        printf("Seleccione una opción: ");
        scanf("%d", &opcion);
        
        switch (opcion) {
            case 1:
                asignar_memoria_estatica();
                break;
            case 2:
                printf("Introduzca el tamaño de memoria a asignar (KB): ");
                scanf("%d", &tamano);
                
                if (tamano < MIN_BLOCK_SIZE || tamano > MAX_BLOCK_SIZE) {
                    printf("El tamaño debe estar entre %d y %d KB\n", MIN_BLOCK_SIZE, MAX_BLOCK_SIZE);
                } else {
                    MemoryBlock *bloque = asignar_memoria_dinamica(next_process_id, tamano);
                    if (bloque) {
                        printf("Memoria asignada para proceso %d: %d KB en dirección %d\n", 
                               next_process_id, tamano, bloque->address);
                        next_process_id++;
                    }
                }
                break;
            case 3:
                printf("Introduzca el ID del proceso a liberar: ");
                scanf("%d", &proceso_id);
                liberar_memoria(proceso_id);
                break;
            case 4:
                mostrar_estado_memoria();
                break;
            case 5:
                visualizar_memoria();
                break;
            case 6:
                printf("Saliendo del simulador...\n");
                break;
            default:
                printf("Opción inválida\n");
                break;
        }
    } while (opcion != 6);
}