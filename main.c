#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_HASH 100 // Tamaño del registro es de 100 (0 a 99)
#define DATA_FILE "ESTUDIANTES.dat"
#define MOD_FILE "MODIFICACIONES.dat"
#define ERR_FILE "ERRORES.dat"

// ============================================================================
// --- 1. ARQUITECTURA DE ARCHIVOS Y ESTRUCTURAS ---
// ============================================================================

/**
 * A. Estructura del registro en el archivo (Archivo Maestro)
 */
typedef struct {
    char CI[9];          // Clave primaria única
    char Nombre[26];
    int Edad;
    double Prom;
    char Sexo;           // 'M' o 'F'
    int Activo;          // Estado (1 = Activo/Inscrito, 0 = Retirado)
    char Grado[20];
    char Fecha[11];      // Fecha de inscripción o retiro (DD/MM/AAAA)
} Estudiante;

/**
 * B. Archivo de Modificaciones (Transacciones)
 */
typedef struct {
    char Accion;         // 'I' = Inclusión, 'M' = Modificación, 'R' = Retiro
    Estudiante DatosEstudiante; // Datos a procesar
} Transaccion;

/**
 * D. Estructura de la entrada del índice (Tabla Hash en Memoria RAM)
 */
typedef struct {
    char clave[9];       // CI del estudiante
    long posicion;       // Posición (RRN / byte offset) en ESTUDIANTES.dat
    bool ocupado;
} IndexEntry;

// --- Variables Globales ---
IndexEntry tablaHash[MAX_HASH];

// --- Prototipos de Funciones ---
void inicializarHash();
int funcionHash(const char *ci);
void insertarHash(const char *ci, long pos);
long buscarHash(const char *ci);
void cargarIndice();
void limpiarBuffer();
void pausar();
void imprimirEstudiante(Estudiante est);

// Funciones del Menú Interactvo (Línea Balanceada)
void consultarEstudiante();
void solicitarInscripcion();
void solicitarModificacion();
void solicitarRetiro();
void procesarLoteModificaciones();
void listarActivos();
void cantidadActivos();
void listarRetirados();
void cantidadRetirados();
void listarPorFecha(int estadoActivo);
void cantidadFemeninosActivos();
void cantidadMasculinosRetirados();

// ============================================================================
// --- IMPLEMENTACIÓN DEL ÍNDICE HASH ---
// ============================================================================

/**
 * Inicializa la tabla Hash marcando todas las posiciones como desocupadas.
 */
void inicializarHash() {
    for (int i = 0; i < MAX_HASH; i++) {
        tablaHash[i].ocupado = false;
        strcpy(tablaHash[i].clave, "");
        tablaHash[i].posicion = -1;
    }
}

/**
 * Función Hash (Método del Módulo).
 */
int funcionHash(const char *ci) {
    int suma = 0;
    for (int i = 0; ci[i] != '\0'; i++) {
        suma += ci[i];
    }
    return suma % MAX_HASH;
}

/**
 * Inserta en la Tabla Hash con Prueba Lineal.
 */
void insertarHash(const char *ci, long pos) {
    int indice = funcionHash(ci);
    int original_indice = indice;

    while (tablaHash[indice].ocupado && strcmp(tablaHash[indice].clave, ci) != 0) {
        indice = (indice + 1) % MAX_HASH;
        if (indice == original_indice) {
            printf("Error crítico: Tabla Hash llena.\n");
            return;
        }
    }
    
    strcpy(tablaHash[indice].clave, ci);
    tablaHash[indice].posicion = pos;
    tablaHash[indice].ocupado = true;
}

/**
 * Busca una CI en la Tabla Hash. Retorna la posición en el archivo maestro o -1.
 */
long buscarHash(const char *ci) {
    int indice = funcionHash(ci);
    int original_indice = indice;

    while (tablaHash[indice].ocupado) {
        if (strcmp(tablaHash[indice].clave, ci) == 0) {
            return tablaHash[indice].posicion;
        }
        indice = (indice + 1) % MAX_HASH;
        if (indice == original_indice) break; 
    }
    return -1;
}

/**
 * Lee ESTUDIANTES.dat al iniciar y carga el índice Hash.
 */
void cargarIndice() {
    inicializarHash();
    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo == NULL) return; 

    Estudiante est;
    long pos = 0;
    while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
        // En esta arquitectura, el Maestro tiene registros únicos y 
        // nunca se eliminan físicamente, por lo que toda clave existente se indexa.
        insertarHash(est.CI, pos);
        pos = ftell(archivo); 
    }
    fclose(archivo);
}

// Utilidades
void limpiarBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void pausar() {
    printf("\nPresione Enter para continuar...");
    limpiarBuffer();
}

void imprimirEstudiante(Estudiante est) {
    printf("CI: %-8s | Nombre: %-25s | Edad: %2d | Prom: %5.2f | Sexo: %c | Grado: %-10s | %s: %s | Activo: %d\n",
           est.CI, est.Nombre, est.Edad, est.Prom, est.Sexo, est.Grado, 
           (est.Activo == 1) ? "F. Inscr." : "F. Retiro", est.Fecha, est.Activo);
}

// ============================================================================
// --- LÓGICA DE PROCESAMIENTO (LÍNEA BALANCEADA) ---
// ============================================================================

/**
 * 1. Consultar Estudiante (Acceso Directo vía Hash)
 */
void consultarEstudiante() {
    char ci[9];
    printf("\n--- CONSULTAR ESTUDIANTE ---\n");
    printf("Ingrese la CI: ");
    scanf("%8s", ci);
    limpiarBuffer();

    long pos = buscarHash(ci);
    if (pos != -1) { 
        FILE *archivo = fopen(DATA_FILE, "rb");
        if (archivo != NULL) {
            Estudiante est;
            fseek(archivo, pos, SEEK_SET); 
            fread(&est, sizeof(Estudiante), 1, archivo);
            printf("\n--- Registro Encontrado ---\n");
            imprimirEstudiante(est);
            fclose(archivo);
        }
    } else {
        printf("Estudiante no encontrado en el Archivo Maestro.\n");
    }
}

/**
 * 2. Solicitar Inscripción / Agregar (Genera Transacción 'I')
 */
void solicitarInscripcion() {
    Transaccion t;
    t.Accion = 'I';
    
    printf("\n--- SOLICITAR INSCRIPCIÓN (Cola de Modificaciones) ---\n");
    printf("CI (8 digitos numéricos): ");
    scanf("%8s", t.DatosEstudiante.CI);
    limpiarBuffer();

    printf("Nombre completo: ");
    fgets(t.DatosEstudiante.Nombre, 26, stdin);
    t.DatosEstudiante.Nombre[strcspn(t.DatosEstudiante.Nombre, "\n")] = 0;

    printf("Edad: ");
    scanf("%d", &t.DatosEstudiante.Edad);

    printf("Promedio: ");
    scanf("%lf", &t.DatosEstudiante.Prom);
    limpiarBuffer();

    printf("Sexo (M/F): ");
    scanf("%c", &t.DatosEstudiante.Sexo);
    t.DatosEstudiante.Sexo = toupper(t.DatosEstudiante.Sexo);
    limpiarBuffer();

    printf("Grado / Año: ");
    fgets(t.DatosEstudiante.Grado, 20, stdin);
    t.DatosEstudiante.Grado[strcspn(t.DatosEstudiante.Grado, "\n")] = 0;

    printf("Fecha de inscripcion (DD/MM/AAAA): ");
    scanf("%10s", t.DatosEstudiante.Fecha);
    limpiarBuffer();

    t.DatosEstudiante.Activo = 1;

    FILE *fMod = fopen(MOD_FILE, "ab");
    if (fMod != NULL) {
        fwrite(&t, sizeof(Transaccion), 1, fMod);
        fclose(fMod);
        printf("\n=> Transacción de INCLUSIÓN ('I') agregada a la cola.\n");
    } else {
        printf("Error al abrir %s\n", MOD_FILE);
    }
}

/**
 * 3. Solicitar Modificación (Genera Transacción 'M')
 */
void solicitarModificacion() {
    Transaccion t;
    t.Accion = 'M';
    
    printf("\n--- SOLICITAR MODIFICACIÓN (Cola de Modificaciones) ---\n");
    printf("CI del estudiante a modificar: ");
    scanf("%8s", t.DatosEstudiante.CI);
    limpiarBuffer();

    printf("Nuevos datos a aplicar:\n");
    printf("Nombre: ");
    fgets(t.DatosEstudiante.Nombre, 26, stdin);
    t.DatosEstudiante.Nombre[strcspn(t.DatosEstudiante.Nombre, "\n")] = 0;

    printf("Edad: ");
    scanf("%d", &t.DatosEstudiante.Edad);

    printf("Promedio: ");
    scanf("%lf", &t.DatosEstudiante.Prom);
    limpiarBuffer();

    printf("Grado: ");
    fgets(t.DatosEstudiante.Grado, 20, stdin);
    t.DatosEstudiante.Grado[strcspn(t.DatosEstudiante.Grado, "\n")] = 0;

    // Campos no alterados por el lote de modificación (se mantendrán los del maestro)
    t.DatosEstudiante.Sexo = 'X'; 
    strcpy(t.DatosEstudiante.Fecha, "");

    FILE *fMod = fopen(MOD_FILE, "ab");
    if (fMod != NULL) {
        fwrite(&t, sizeof(Transaccion), 1, fMod);
        fclose(fMod);
        printf("\n=> Transacción de MODIFICACIÓN ('M') agregada a la cola.\n");
    } else {
        printf("Error al abrir %s\n", MOD_FILE);
    }
}

/**
 * 4. Solicitar Retiro (Genera Transacción 'R')
 */
void solicitarRetiro() {
    Transaccion t;
    t.Accion = 'R';
    
    printf("\n--- SOLICITAR RETIRO (Cola de Modificaciones) ---\n");
    printf("CI del estudiante a retirar: ");
    scanf("%8s", t.DatosEstudiante.CI);
    limpiarBuffer();

    printf("Fecha de Retiro (DD/MM/AAAA): ");
    scanf("%10s", t.DatosEstudiante.Fecha);
    limpiarBuffer();

    FILE *fMod = fopen(MOD_FILE, "ab");
    if (fMod != NULL) {
        fwrite(&t, sizeof(Transaccion), 1, fMod);
        fclose(fMod);
        printf("\n=> Transacción de RETIRO ('R') agregada a la cola.\n");
    } else {
        printf("Error al abrir %s\n", MOD_FILE);
    }
}

/**
 * 5. Procesar Lote de Modificaciones (Línea Balanceada Directa)
 * Lee secuencialmente MODIFICACIONES.dat, valida contra el Maestro (vía Hash) y
 * actualiza el Maestro o genera entradas en ERRORES.dat.
 */
void procesarLoteModificaciones() {
    printf("\n--- PROCESANDO LÍNEA BALANCEADA ---\n");

    FILE *fMod = fopen(MOD_FILE, "rb");
    if (fMod == NULL) {
        printf("No hay transacciones pendientes (Archivo de modificaciones vacío o inexistente).\n");
        return;
    }

    // Abrimos el Maestro en r+ (lectura/escritura) o w+ si no existe.
    FILE *fMae = fopen(DATA_FILE, "rb+");
    if (fMae == NULL) {
        fMae = fopen(DATA_FILE, "wb+"); // Crea si no existe
        if (fMae == NULL) {
            printf("Error crítico: No se puede abrir ni crear el Archivo Maestro.\n");
            fclose(fMod);
            return;
        }
    }

    FILE *fErr = fopen(ERR_FILE, "a"); // Archivo de texto para log de errores
    if (fErr == NULL) {
        printf("Error al abrir %s\n", ERR_FILE);
        fclose(fMod);
        fclose(fMae);
        return;
    }

    Transaccion t;
    int procesados = 0;
    int errores = 0;

    // 1. Se lee una transacción de MODIFICACIONES.dat
    while (fread(&t, sizeof(Transaccion), 1, fMod) == 1) {
        procesados++;
        
        // 2. Se busca la CI en el Maestro (vía Tabla Hash)
        long pos = buscarHash(t.DatosEstudiante.CI);
        int IM = (pos != -1) ? 1 : 0; // Indicador de Maestro

        if (t.Accion == 'I') {
            // 3. Si Accion == 'I'
            if (IM == 1) {
                fprintf(fErr, "Error: Registro ya existe en Maestro. CI: %s | Accion: %c\n", t.DatosEstudiante.CI, t.Accion);
                errores++;
            } else { // IM == 0
                fseek(fMae, 0, SEEK_END);
                long nuevaPos = ftell(fMae);
                t.DatosEstudiante.Activo = 1; // Asegurar Activo = 1
                
                if (fwrite(&t.DatosEstudiante, sizeof(Estudiante), 1, fMae) == 1) {
                    insertarHash(t.DatosEstudiante.CI, nuevaPos); // Actualizar índice Hash RAM
                }
            }
        } 
        else if (t.Accion == 'M') {
            // 4. Si Accion == 'M'
            if (IM == 1) {
                Estudiante estMaestro;
                fseek(fMae, pos, SEEK_SET);
                fread(&estMaestro, sizeof(Estudiante), 1, fMae);
                
                // Aplicar solo los cambios (los que pedimos en solicitarModificacion)
                strcpy(estMaestro.Nombre, t.DatosEstudiante.Nombre);
                estMaestro.Edad = t.DatosEstudiante.Edad;
                estMaestro.Prom = t.DatosEstudiante.Prom;
                strcpy(estMaestro.Grado, t.DatosEstudiante.Grado);
                
                fseek(fMae, pos, SEEK_SET);
                fwrite(&estMaestro, sizeof(Estudiante), 1, fMae);
            } else { // IM == 0
                fprintf(fErr, "Error: Registro no existe para modificar. CI: %s | Accion: %c\n", t.DatosEstudiante.CI, t.Accion);
                errores++;
            }
        } 
        else if (t.Accion == 'R') {
            // 5. Si Accion == 'R'
            if (IM == 1) {
                Estudiante estMaestro;
                fseek(fMae, pos, SEEK_SET);
                fread(&estMaestro, sizeof(Estudiante), 1, fMae);

                estMaestro.Activo = 0; // Baja Lógica
                strcpy(estMaestro.Fecha, t.DatosEstudiante.Fecha); // Actualizar fecha retiro

                fseek(fMae, pos, SEEK_SET);
                fwrite(&estMaestro, sizeof(Estudiante), 1, fMae);
            } else { // IM == 0
                fprintf(fErr, "Error: Registro no existe para retirar. CI: %s | Accion: %c\n", t.DatosEstudiante.CI, t.Accion);
                errores++;
            }
        }
    }

    fclose(fMod);
    fclose(fMae);
    fclose(fErr);

    // Vaciar el archivo de modificaciones para que no se reprocese
    remove(MOD_FILE);

    printf("Procesamiento de Lote Finalizado.\n");
    printf("- Transacciones procesadas: %d\n", procesados);
    printf("- Errores generados: %d (Ver %s)\n", errores, ERR_FILE);
}

// ============================================================================
// --- LISTADOS E INFORMES (Solo Lectura del Maestro) ---
// ============================================================================

void listarActivos() {
    printf("\n--- LISTADO DE ESTUDIANTES ACTIVOS ---\n");
    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo == NULL) return;

    Estudiante est;
    bool hay = false;
    while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
        if (est.Activo == 1) {
            imprimirEstudiante(est);
            hay = true;
        }
    }
    if (!hay) printf("No hay estudiantes activos.\n");
    fclose(archivo);
}

void cantidadActivos() {
    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo == NULL) return;

    Estudiante est;
    int c = 0;
    while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
        if (est.Activo == 1) c++;
    }
    printf("\n=> Total Activos: %d\n", c);
    fclose(archivo);
}

void listarRetirados() {
    printf("\n--- LISTADO DE ESTUDIANTES RETIRADOS ---\n");
    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo == NULL) return;

    Estudiante est;
    bool hay = false;
    while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
        if (est.Activo == 0) {
            imprimirEstudiante(est);
            hay = true;
        }
    }
    if (!hay) printf("No hay estudiantes retirados.\n");
    fclose(archivo);
}

void cantidadRetirados() {
    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo == NULL) return;

    Estudiante est;
    int c = 0;
    while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
        if (est.Activo == 0) c++;
    }
    printf("\n=> Total Retirados: %d\n", c);
    fclose(archivo);
}

void listarPorFecha(int estadoActivo) {
    char fecha[11];
    printf("\n--- LISTAR %s POR FECHA ---\n", estadoActivo ? "INSCRITOS" : "RETIRADOS");
    printf("Ingrese Fecha (DD/MM/AAAA): ");
    scanf("%10s", fecha);
    limpiarBuffer();

    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo == NULL) return;

    Estudiante est;
    bool hay = false;
    while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
        if (est.Activo == estadoActivo && strcmp(est.Fecha, fecha) == 0) {
            imprimirEstudiante(est);
            hay = true;
        }
    }
    if (!hay) printf("No se encontraron registros en esa fecha.\n");
    fclose(archivo);
}

void cantidadFemeninosActivos() {
    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo == NULL) return;

    Estudiante est;
    int c = 0;
    while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
        if (est.Activo == 1 && est.Sexo == 'F') c++;
    }
    printf("\n=> Total Femeninos Activos: %d\n", c);
    fclose(archivo);
}

void cantidadMasculinosRetirados() {
    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo == NULL) return;

    Estudiante est;
    int c = 0;
    while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
        if (est.Activo == 0 && est.Sexo == 'M') c++;
    }
    printf("\n=> Total Masculinos Retirados: %d\n", c);
    fclose(archivo);
}

// ============================================================================
// --- CONTROLADOR PRINCIPAL ---
// ============================================================================

int main() {
    cargarIndice(); // Carga Inicial: Lee Maestro y llena Tabla Hash (0 a 99)

    int opcion;
    do {
        printf("\n===============================================================\n");
        printf(" SISTEMA DE CONTROL - LINEA BALANCEADA (MODO BATCH / LOTES)\n");
        printf("===============================================================\n");
        printf(" 1. Consultar estudiante (vía Hash)\n");
        printf(" 2. Solicitar Inscripción (Cola 'I')\n");
        printf(" 3. Solicitar Modificación (Cola 'M')\n");
        printf(" 4. Solicitar Retiro (Cola 'R')\n");
        printf(" 5. >> PROCESAR LOTE DE MODIFICACIONES <<\n");
        printf(" 6. Listar estudiantes activos\n");
        printf(" 7. Cantidad de estudiantes activos\n");
        printf(" 8. Listar estudiantes retirados\n");
        printf(" 9. Cantidad de estudiantes retirados\n");
        printf("10. Listar inscritos por fecha\n");
        printf("11. Listar retirados por fecha\n");
        printf("12. Cantidad femeninos activos\n");
        printf("13. Cantidad masculinos retirados\n");
        printf(" 0. Salir\n");
        printf("===============================================================\n");
        printf("Seleccione una opción: ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiarBuffer();
            opcion = -1;
        }

        switch (opcion) {
            case 1: consultarEstudiante(); break;
            case 2: solicitarInscripcion(); break;
            case 3: solicitarModificacion(); break;
            case 4: solicitarRetiro(); break;
            case 5: procesarLoteModificaciones(); break;
            case 6: listarActivos(); break;
            case 7: cantidadActivos(); break;
            case 8: listarRetirados(); break;
            case 9: cantidadRetirados(); break;
            case 10: listarPorFecha(1); break;
            case 11: listarPorFecha(0); break;
            case 12: cantidadFemeninosActivos(); break;
            case 13: cantidadMasculinosRetirados(); break;
            case 0: printf("\nCerrando programa...\n"); break;
            default: printf("Opción inválida.\n"); break;
        }
        
        if (opcion != 0) pausar();

    } while (opcion != 0);

    return 0;
}