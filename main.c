#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  char CI[9]; // Clave primaria única
  char Nombre[26];
  int Edad;
  double Prom;
  char Sexo;  // 'M' o 'F'
  int Activo; // Estado (1 = Activo/Inscrito, 0 = Retirado)
  char Grado[20];
  char Fecha[11]; // Fecha de inscripción o retiro (DD/MM/AAAA)
} Estudiante;

/**
 * B. Archivo de Modificaciones (Transacciones)
 */
typedef struct {
  char Accion; // 'I' = Inclusión, 'M' = Modificación, 'R' = Retiro
  Estudiante DatosEstudiante; // Datos a procesar
} Transaccion;

/**
 * D. Estructura de la entrada del índice (Tabla Hash en Memoria RAM)
 */
typedef struct {
  char clave[9]; // CI del estudiante
  long posicion; // Posición (RRN / byte offset) en ESTUDIANTES.dat
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

// Validadores de entrada
bool esCadenaNumerica(const char *str);
void leerCI(char *dest, const char *prompt);
void leerNombre(char *dest, const char *prompt);
int leerEdad(const char *prompt);
double leerPromedio(const char *prompt);
char leerSexo(const char *prompt);
void leerGrado(char *dest, const char *prompt);
bool esFechaValida(const char *fecha);
void leerFecha(char *dest, const char *prompt);

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

int funcionHash(const char *ci) {
  char digitos[10] = "";
  int dCount = 0;

  for (int i = 0; ci[i] != '\0' && dCount < 9; i++) {
    if (isdigit((unsigned char)ci[i])) {
      digitos[dCount++] = ci[i];
    }
  }
  digitos[dCount] = '\0';

  char formateado[7] = "000000";
  if (dCount < 6) {
    int cerosFaltantes = 6 - dCount;
    for (int i = 0; i < dCount; i++) {
      formateado[cerosFaltantes + i] = digitos[i];
    }
  } else {
    strncpy(formateado, digitos, 6);
  }
  formateado[6] = '\0';

  int g1 = (formateado[1] - '0') * 10 + (formateado[0] - '0');
  int g2 = (formateado[2] - '0') * 10 + (formateado[3] - '0');
  int g3 = (formateado[5] - '0') * 10 + (formateado[4] - '0');

  int suma = g1 + g2 + g3;
  return suma % 100;
}

void insertarHash(const char *ci, long pos) {
  int hash_val = funcionHash(ci);
  int pos_inicial = hash_val % 90; // Zona Primaria (0-89)
  int idx = pos_inicial;

  do {
    if (!tablaHash[idx].ocupado || strcmp(tablaHash[idx].clave, ci) == 0) {
      strcpy(tablaHash[idx].clave, ci);
      tablaHash[idx].posicion = pos;
      tablaHash[idx].ocupado = true;
      return;
    }
    idx = (idx + 1) % 90;
  } while (idx != pos_inicial);

  // Zona de Desborde (90-99)
  for (int i = 90; i < MAX_HASH; i++) {
    if (!tablaHash[i].ocupado || strcmp(tablaHash[i].clave, ci) == 0) {
      strcpy(tablaHash[i].clave, ci);
      tablaHash[i].posicion = pos;
      tablaHash[i].ocupado = true;
      return;
    }
  }

  printf("Error crítico: Tabla Hash y Zona de Desborde llenas.\n");
}

long buscarHash(const char *ci) {
  int hash_val = funcionHash(ci);
  int pos_inicial = hash_val % 90; // Zona Primaria (0-89)
  int idx = pos_inicial;

  do {
    if (tablaHash[idx].ocupado && strcmp(tablaHash[idx].clave, ci) == 0) {
      return tablaHash[idx].posicion;
    }
    idx = (idx + 1) % 90;
  } while (idx != pos_inicial);

  // Zona de Desborde (90-99)
  for (int i = 90; i < MAX_HASH; i++) {
    if (tablaHash[i].ocupado && strcmp(tablaHash[i].clave, ci) == 0) {
      return tablaHash[i].posicion;
    }
  }

  return -1;
}

/**
 * Lee ESTUDIANTES.dat al iniciar y carga el índice Hash.
 */
void cargarIndice() {
  inicializarHash();
  FILE *archivo = fopen(DATA_FILE, "rb");
  if (archivo == NULL)
    return;

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
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}

void pausar() {
  printf("\nPresione Enter para continuar...");
  limpiarBuffer();
}

void imprimirEstudiante(Estudiante est) {
  printf("CI: %-8s | Nombre: %-25s | Edad: %2d | Prom: %5.2f | Sexo: %c | "
         "Grado: %-10s | %s: %s | Activo: %d\n",
         est.CI, est.Nombre, est.Edad, est.Prom, est.Sexo, est.Grado,
         (est.Activo == 1) ? "F. Inscr." : "F. Retiro", est.Fecha, est.Activo);
}

// ============================================================================
// --- FUNCIONES DE VALIDACIÓN Y LECTURA DE DATOS ---
// ============================================================================

bool esCadenaNumerica(const char *str) {
  if (str == NULL || *str == '\0')
    return false;
  for (int i = 0; str[i] != '\0'; i++) {
    if (!isdigit((unsigned char)str[i]))
      return false;
  }
  return true;
}

void leerCI(char *dest, const char *prompt) {
  char temp[64];
  while (1) {
    printf("%s", prompt);
    if (scanf("%63s", temp) == 1) {
      limpiarBuffer();
      if (strlen(temp) > 0 && strlen(temp) <= 8 && esCadenaNumerica(temp)) {
        strcpy(dest, temp);
        return;
      }
    } else {
      limpiarBuffer();
    }
    printf("Error: La CI debe tener entre 1 y 8 dígitos numéricos. Intente de "
           "nuevo.\n");
  }
}

void leerNombre(char *dest, const char *prompt) {
  char temp[64];
  while (1) {
    printf("%s", prompt);
    if (fgets(temp, sizeof(temp), stdin) != NULL) {
      temp[strcspn(temp, "\r\n")] = 0;
      if (strlen(temp) > 0 && strlen(temp) <= 25) {
        strcpy(dest, temp);
        return;
      }
    }
    printf("Error: El nombre debe tener entre 1 y 25 caracteres. Intente de "
           "nuevo.\n");
  }
}

int leerEdad(const char *prompt) {
  int edad;
  while (1) {
    printf("%s", prompt);
    if (scanf("%d", &edad) == 1 && edad >= 1 && edad <= 120) {
      limpiarBuffer();
      return edad;
    }
    limpiarBuffer();
    printf(
        "Error: La edad debe ser un número entre 1 y 120. Intente de nuevo.\n");
  }
}

double leerPromedio(const char *prompt) {
  double prom;
  while (1) {
    printf("%s", prompt);
    if (scanf("%lf", &prom) == 1 && prom >= 0.0 && prom <= 20.0) {
      limpiarBuffer();
      return prom;
    }
    limpiarBuffer();
    printf("Error: El promedio debe estar entre 0.00 y 20.00. Intente de "
           "nuevo.\n");
  }
}

char leerSexo(const char *prompt) {
  char c;
  while (1) {
    printf("%s", prompt);
    if (scanf(" %c", &c) == 1) {
      limpiarBuffer();
      c = toupper((unsigned char)c);
      if (c == 'M' || c == 'F') {
        return c;
      }
    } else {
      limpiarBuffer();
    }
    printf("Error: El sexo debe ser 'M' (Masculino) o 'F' (Femenino). Intente "
           "de nuevo.\n");
  }
}

void leerGrado(char *dest, const char *prompt) {
  char temp[64];
  while (1) {
    printf("%s", prompt);
    if (fgets(temp, sizeof(temp), stdin) != NULL) {
      temp[strcspn(temp, "\r\n")] = 0;
      if (strlen(temp) > 0 && strlen(temp) <= 19) {
        strcpy(dest, temp);
        return;
      }
    }
    printf("Error: El grado debe tener entre 1 y 19 caracteres. Intente de "
           "nuevo.\n");
  }
}

bool esFechaValida(const char *fecha) {
  if (strlen(fecha) != 10)
    return false;
  if (fecha[2] != '/' || fecha[5] != '/')
    return false;

  for (int i = 0; i < 10; i++) {
    if (i == 2 || i == 5)
      continue;
    if (!isdigit((unsigned char)fecha[i]))
      return false;
  }

  int dia = (fecha[0] - '0') * 10 + (fecha[1] - '0');
  int mes = (fecha[3] - '0') * 10 + (fecha[4] - '0');
  int anio = (fecha[6] - '0') * 1000 + (fecha[7] - '0') * 100 +
             (fecha[8] - '0') * 10 + (fecha[9] - '0');

  if (dia < 1 || dia > 31)
    return false;
  if (mes < 1 || mes > 12)
    return false;
  if (anio < 1900 || anio > 2100)
    return false;

  return true;
}

void leerFecha(char *dest, const char *prompt) {
  char temp[64];
  while (1) {
    printf("%s", prompt);
    if (scanf("%63s", temp) == 1) {
      limpiarBuffer();
      if (esFechaValida(temp)) {
        strcpy(dest, temp);
        return;
      }
    } else {
      limpiarBuffer();
    }
    printf("Error: Fecha inválida. Formato correcto DD/MM/AAAA (ej. "
           "15/03/2026). Intente de nuevo.\n");
  }
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
  leerCI(ci, "Ingrese la CI (8 dígitos numéricos): ");

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
  memset(&t, 0, sizeof(Transaccion));
  t.Accion = 'I';

  printf("\n--- SOLICITAR INSCRIPCIÓN (Cola de Modificaciones) ---\n");
  leerCI(t.DatosEstudiante.CI, "CI (8 dígitos numéricos): ");
  leerNombre(t.DatosEstudiante.Nombre, "Nombre completo: ");
  t.DatosEstudiante.Edad = leerEdad("Edad (1-120): ");
  t.DatosEstudiante.Prom = leerPromedio("Promedio (0.00 - 20.00): ");
  t.DatosEstudiante.Sexo = leerSexo("Sexo (M/F): ");
  leerGrado(t.DatosEstudiante.Grado, "Grado / Año: ");
  leerFecha(t.DatosEstudiante.Fecha, "Fecha de inscripción (DD/MM/AAAA): ");

  // Se asigna 1 (Verdadero / Inscrito)
  t.DatosEstudiante.Activo = 1;

  FILE *fMod = fopen(MOD_FILE, "ab");
  if (fMod != NULL) {
    fwrite(&t, sizeof(Transaccion), 1, fMod);
    fclose(fMod);
    printf("\n=> Transacción de INCLUSIÓN ('I') agregada a la cola (Activo = 1 "
           "/ Verdadero).\n");
  } else {
    printf("Error al abrir %s\n", MOD_FILE);
  }
}

void solicitarModificacion() {
  Transaccion t;
  memset(&t, 0, sizeof(Transaccion));
  t.Accion = 'M';

  // Inicializar valores centinela para indicar "sin cambio"
  t.DatosEstudiante.Edad = -1;
  t.DatosEstudiante.Prom = -1.0;
  t.DatosEstudiante.Sexo = '\0';
  strcpy(t.DatosEstudiante.Nombre, "");
  strcpy(t.DatosEstudiante.Grado, "");

  printf("\n--- SOLICITAR MODIFICACIÓN (Cola de Modificaciones) ---\n");
  leerCI(t.DatosEstudiante.CI, "CI del estudiante a modificar: ");

  long pos = buscarHash(t.DatosEstudiante.CI);
  Estudiante estActual;
  bool enMaestro = false;

  if (pos != -1) {
    FILE *archivo = fopen(DATA_FILE, "rb");
    if (archivo != NULL) {
      fseek(archivo, pos, SEEK_SET);
      if (fread(&estActual, sizeof(Estudiante), 1, archivo) == 1) {
        enMaestro = true;
        printf("\n=> Estudiante encontrado en Maestro:\n");
        imprimirEstudiante(estActual);
      }
      fclose(archivo);
    }
  }

  int subOpcion;
  bool cambiosRealizados = false;

  do {
    printf("\n--- Menú de Campos a Modificar (CI: %s) ---\n",
           t.DatosEstudiante.CI);

    if (strlen(t.DatosEstudiante.Nombre) > 0) {
      printf(" 1. Nombre    [Nuevo: %s]\n", t.DatosEstudiante.Nombre);
    } else if (enMaestro) {
      printf(" 1. Nombre    [Actual: %s]\n", estActual.Nombre);
    } else {
      printf(" 1. Nombre    [Sin cambio]\n");
    }

    if (t.DatosEstudiante.Edad != -1) {
      printf(" 2. Edad      [Nuevo: %d]\n", t.DatosEstudiante.Edad);
    } else if (enMaestro) {
      printf(" 2. Edad      [Actual: %d]\n", estActual.Edad);
    } else {
      printf(" 2. Edad      [Sin cambio]\n");
    }

    if (t.DatosEstudiante.Prom >= 0) {
      printf(" 3. Promedio  [Nuevo: %.2f]\n", t.DatosEstudiante.Prom);
    } else if (enMaestro) {
      printf(" 3. Promedio  [Actual: %.2f]\n", estActual.Prom);
    } else {
      printf(" 3. Promedio  [Sin cambio]\n");
    }

    if (t.DatosEstudiante.Sexo == 'M' || t.DatosEstudiante.Sexo == 'F') {
      printf(" 4. Sexo      [Nuevo: %c]\n", t.DatosEstudiante.Sexo);
    } else if (enMaestro) {
      printf(" 4. Sexo      [Actual: %c]\n", estActual.Sexo);
    } else {
      printf(" 4. Sexo      [Sin cambio]\n");
    }

    if (strlen(t.DatosEstudiante.Grado) > 0) {
      printf(" 5. Grado     [Nuevo: %s]\n", t.DatosEstudiante.Grado);
    } else if (enMaestro) {
      printf(" 5. Grado     [Actual: %s]\n", estActual.Grado);
    } else {
      printf(" 5. Grado     [Sin cambio]\n");
    }

    printf(" 0. Guardar solicitud de modificación\n");
    printf("Seleccione el campo a modificar: ");

    if (scanf("%d", &subOpcion) != 1) {
      limpiarBuffer();
      subOpcion = -1;
      continue;
    }
    limpiarBuffer();

    switch (subOpcion) {
    case 1:
      leerNombre(t.DatosEstudiante.Nombre, "Ingrese el nuevo Nombre: ");
      cambiosRealizados = true;
      break;
    case 2:
      t.DatosEstudiante.Edad = leerEdad("Ingrese la nueva Edad (1-120): ");
      cambiosRealizados = true;
      break;
    case 3:
      t.DatosEstudiante.Prom =
          leerPromedio("Ingrese el nuevo Promedio (0.00 - 20.00): ");
      cambiosRealizados = true;
      break;
    case 4:
      t.DatosEstudiante.Sexo = leerSexo("Ingrese el nuevo Sexo (M/F): ");
      cambiosRealizados = true;
      break;
    case 5:
      leerGrado(t.DatosEstudiante.Grado, "Ingrese el nuevo Grado: ");
      cambiosRealizados = true;
      break;
    case 0:
      if (!cambiosRealizados) {
        printf("\nNo se seleccionó ningún campo para modificar. Transacción "
               "cancelada.\n");
        return;
      }
      break;
    default:
      printf("Opción inválida.\n");
      break;
    }
  } while (subOpcion != 0);

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
  memset(&t, 0, sizeof(Transaccion));
  t.Accion = 'R';

  printf("\n--- SOLICITAR RETIRO (Cola de Modificaciones) ---\n");
  leerCI(t.DatosEstudiante.CI, "CI del estudiante a retirar: ");
  leerFecha(t.DatosEstudiante.Fecha, "Fecha de Retiro (DD/MM/AAAA): ");

  // Se asigna 0 (Falso / Retirado)
  t.DatosEstudiante.Activo = 0;

  FILE *fMod = fopen(MOD_FILE, "ab");
  if (fMod != NULL) {
    fwrite(&t, sizeof(Transaccion), 1, fMod);
    fclose(fMod);
    printf("\n=> Transacción de RETIRO ('R') agregada a la cola (Activo = 0 / "
           "Falso).\n");
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
    printf("No hay transacciones pendientes (Archivo de modificaciones vacío o "
           "inexistente).\n");
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
        fprintf(fErr,
                "Error: Registro ya existe en Maestro. CI: %s | Accion: %c\n",
                t.DatosEstudiante.CI, t.Accion);
        errores++;
      } else { // IM == 0
        fseek(fMae, 0, SEEK_END);
        long nuevaPos = ftell(fMae);
        t.DatosEstudiante.Activo = 1; // Asegurar Activo = 1

        if (fwrite(&t.DatosEstudiante, sizeof(Estudiante), 1, fMae) == 1) {
          insertarHash(t.DatosEstudiante.CI,
                       nuevaPos); // Actualizar índice Hash RAM
        }
      }
    } else if (t.Accion == 'M') {
      // 4. Si Accion == 'M'
      if (IM == 1) {
        Estudiante estMaestro;
        fseek(fMae, pos, SEEK_SET);
        fread(&estMaestro, sizeof(Estudiante), 1, fMae);

        // Modificar únicamente los campos especificados en la transacción
        if (strlen(t.DatosEstudiante.Nombre) > 0) {
          strcpy(estMaestro.Nombre, t.DatosEstudiante.Nombre);
        }
        if (t.DatosEstudiante.Edad != -1) {
          estMaestro.Edad = t.DatosEstudiante.Edad;
        }
        if (t.DatosEstudiante.Prom >= 0) {
          estMaestro.Prom = t.DatosEstudiante.Prom;
        }
        if (t.DatosEstudiante.Sexo == 'M' || t.DatosEstudiante.Sexo == 'F') {
          estMaestro.Sexo = t.DatosEstudiante.Sexo;
        }
        if (strlen(t.DatosEstudiante.Grado) > 0) {
          strcpy(estMaestro.Grado, t.DatosEstudiante.Grado);
        }

        fseek(fMae, pos, SEEK_SET);
        fwrite(&estMaestro, sizeof(Estudiante), 1, fMae);
      } else { // IM == 0
        fprintf(
            fErr,
            "Error: Registro no existe para modificar. CI: %s | Accion: %c\n",
            t.DatosEstudiante.CI, t.Accion);
        errores++;
      }
    } else if (t.Accion == 'R') {
      // 5. Si Accion == 'R'
      if (IM == 1) {
        Estudiante estMaestro;
        fseek(fMae, pos, SEEK_SET);
        fread(&estMaestro, sizeof(Estudiante), 1, fMae);

        estMaestro.Activo = 0; // Baja Lógica
        strcpy(estMaestro.Fecha,
               t.DatosEstudiante.Fecha); // Actualizar fecha retiro

        fseek(fMae, pos, SEEK_SET);
        fwrite(&estMaestro, sizeof(Estudiante), 1, fMae);
      } else { // IM == 0
        fprintf(fErr,
                "Error: Registro no existe para retirar. CI: %s | Accion: %c\n",
                t.DatosEstudiante.CI, t.Accion);
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
  if (archivo == NULL)
    return;

  Estudiante est;
  bool hay = false;
  while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
    if (est.Activo == 1) {
      imprimirEstudiante(est);
      hay = true;
    }
  }
  if (!hay)
    printf("No hay estudiantes activos.\n");
  fclose(archivo);
}

void cantidadActivos() {
  FILE *archivo = fopen(DATA_FILE, "rb");
  if (archivo == NULL)
    return;

  Estudiante est;
  int c = 0;
  while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
    if (est.Activo == 1)
      c++;
  }
  printf("\n=> Total Activos: %d\n", c);
  fclose(archivo);
}

void listarRetirados() {
  printf("\n--- LISTADO DE ESTUDIANTES RETIRADOS ---\n");
  FILE *archivo = fopen(DATA_FILE, "rb");
  if (archivo == NULL)
    return;

  Estudiante est;
  bool hay = false;
  while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
    if (est.Activo == 0) {
      imprimirEstudiante(est);
      hay = true;
    }
  }
  if (!hay)
    printf("No hay estudiantes retirados.\n");
  fclose(archivo);
}

void cantidadRetirados() {
  FILE *archivo = fopen(DATA_FILE, "rb");
  if (archivo == NULL)
    return;

  Estudiante est;
  int c = 0;
  while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
    if (est.Activo == 0)
      c++;
  }
  printf("\n=> Total Retirados: %d\n", c);
  fclose(archivo);
}

void listarPorFecha(int estadoActivo) {
  char fecha[11];
  printf("\n--- LISTAR %s POR FECHA ---\n",
         estadoActivo ? "INSCRITOS" : "RETIRADOS");
  leerFecha(fecha, "Ingrese Fecha (DD/MM/AAAA): ");

  FILE *archivo = fopen(DATA_FILE, "rb");
  if (archivo == NULL)
    return;

  Estudiante est;
  bool hay = false;
  while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
    if (est.Activo == estadoActivo && strcmp(est.Fecha, fecha) == 0) {
      imprimirEstudiante(est);
      hay = true;
    }
  }
  if (!hay)
    printf("No se encontraron registros en esa fecha.\n");
  fclose(archivo);
}

void cantidadFemeninosActivos() {
  FILE *archivo = fopen(DATA_FILE, "rb");
  if (archivo == NULL)
    return;

  Estudiante est;
  int c = 0;
  while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
    if (est.Activo == 1 && est.Sexo == 'F')
      c++;
  }
  printf("\n=> Total Femeninos Activos: %d\n", c);
  fclose(archivo);
}

void cantidadMasculinosRetirados() {
  FILE *archivo = fopen(DATA_FILE, "rb");
  if (archivo == NULL)
    return;

  Estudiante est;
  int c = 0;
  while (fread(&est, sizeof(Estudiante), 1, archivo) == 1) {
    if (est.Activo == 0 && est.Sexo == 'M')
      c++;
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
    printf(
        "\n===============================================================\n");
    printf("     SISTEMA DE CONTROL DE ESTUDIANTES DEL INSTITUTO EDUCANDO\n");
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
    case 1:
      consultarEstudiante();
      break;
    case 2:
      solicitarInscripcion();
      break;
    case 3:
      solicitarModificacion();
      break;
    case 4:
      solicitarRetiro();
      break;
    case 5:
      procesarLoteModificaciones();
      break;
    case 6:
      listarActivos();
      break;
    case 7:
      cantidadActivos();
      break;
    case 8:
      listarRetirados();
      break;
    case 9:
      cantidadRetirados();
      break;
    case 10:
      listarPorFecha(1);
      break;
    case 11:
      listarPorFecha(0);
      break;
    case 12:
      cantidadFemeninosActivos();
      break;
    case 13:
      cantidadMasculinosRetirados();
      break;
    case 0:
      printf("\nCerrando programa...\n");
      break;
    default:
      printf("Opción inválida.\n");
      break;
    }

    if (opcion != 0)
      pausar();

  } while (opcion != 0);

  return 0;
}