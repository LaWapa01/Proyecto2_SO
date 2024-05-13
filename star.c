#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_FILE_NAME 200

// Estructura para el encabezado de archivo en el archivo de salida
typedef struct FileHeader
{
    char name[MAX_FILE_NAME];
    unsigned int size;
    unsigned int jump;
    bool deleted;
} f_h;

const int HEADER_SIZE = sizeof(f_h);

// Función para crear un archivo tar
void create_tar(const char *outputFile, int numFiles, char *inputFiles[])
{
    printf("Nombre del archivo de salida: %s\n", outputFile);

    FILE *outFile = fopen(outputFile, "wb");
    if (!outFile)
    {
        perror("Error al abrir el archivo de salida\n");
        exit(1);
    }

    printf("Abriendo los archivos de entrada...\n");

    for (int i = 0; i < numFiles; i++)
    {
        printf("Leyendo %s en memoria...\n", inputFiles[i]);
        FILE *inputFile = fopen(inputFiles[i], "rb");
        if (!inputFile)
        {
            perror("Error al abrir el archivo de entrada\n");
            fclose(outFile);
            exit(1);
        }

        printf("Calculando tamaño de archivo en paquete...\n");
        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        rewind(inputFile);

        printf("Creando cabecera de metadatos...\n");
        struct FileHeader header;
        strncpy(header.name, inputFiles[i], sizeof(header.name));
        header.size = (unsigned int)fileSize;
        header.jump = (unsigned int)fileSize;
        header.deleted = false;
        fwrite(&header, sizeof(struct FileHeader), 1, outFile);

        printf("Copiando datos al paquete %s...\n", outputFile);
        char *buffer;
        buffer = (char *)calloc(fileSize, sizeof(char));
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, fileSize, inputFile)) > 0)
        {
            fwrite(buffer, 1, bytesRead, outFile);
        }

        printf("Liberando buffers de memoria...\n");
        free(buffer);
        fclose(inputFile);
        printf("Se agregó el archivo %s al paquete %s\n", inputFiles[i], outputFile);
    }

    fclose(outFile);
    printf("Se creó el archivo star: %s\n", outputFile);
}

void extract_tar(const char *archiveFile, int numFiles, char *filesToExtract[]) {
    FILE *tarFile = fopen(archiveFile, "rb");
    if (!tarFile) {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    if (numFiles == 0) {
        // No se especificaron archivos específicos para extraer,
        // por lo que extraeremos todos los archivos del .tar.
        struct FileHeader header;
        while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
            if (!header.deleted) {
                FILE *outputFile = fopen(header.name, "wb");
                if (!outputFile) {
                    perror("Error al crear el archivo de salida");
                    fclose(tarFile);
                    exit(1);
                }

                char *buffer;
                buffer = (char *)calloc(header.size, sizeof(char));
                size_t bytesRead;

                while (header.size > 0) {
                    size_t readSize = (header.size > sizeof(buffer)) ? sizeof(buffer) : header.size;
                    bytesRead = fread(buffer, 1, readSize, tarFile);
                    fwrite(buffer, 1, bytesRead, outputFile);
                    header.size -= bytesRead;
                }

                free(buffer);
                fclose(outputFile);
            } else {
                fseek(tarFile, header.size, SEEK_CUR); // Saltar archivos marcados como eliminados
            }
        }
    } else {
        // Se especificaron archivos específicos para extraer.
        for (int i = 0; i < numFiles; i++) {
            struct FileHeader header;
            bool found = false;

            while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
                if (!header.deleted) {
                    if (strcmp(header.name, filesToExtract[i]) == 0) {
                        FILE *outputFile = fopen(header.name, "wb");
                        if (!outputFile) {
                            perror("Error al crear el archivo de salida");
                            fclose(tarFile);
                            exit(1);
                        }

                        char *buffer;
                        buffer = (char *)calloc(header.size, sizeof(char));
                        size_t bytesRead;

                        while (header.size > 0) {
                            size_t readSize = (header.size > sizeof(buffer)) ? sizeof(buffer) : header.size;
                            bytesRead = fread(buffer, 1, readSize, tarFile);
                            fwrite(buffer, 1, bytesRead, outputFile);
                            header.size -= bytesRead;
                        }

                        free(buffer);
                        fclose(outputFile);
                        found = true;
                        break;
                    } else {
                        fseek(tarFile, header.size, SEEK_CUR); // Saltar otros archivos
                    }
                } else {
                    fseek(tarFile, header.size, SEEK_CUR); // Saltar archivos marcados como eliminados
                }
            }

            if (!found) {
                printf("Archivo no encontrado: %s\n", filesToExtract[i]);
            }
        }
    }

    fclose(tarFile);
}

void list_tar(const char *archiveFile) {
    FILE *tarFile = fopen(archiveFile, "rb");
    if (!tarFile) {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    struct FileHeader header;
    while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
        if (!header.deleted) {
            printf("Nombre: %s, Tamaño: %u bytes\n", header.name, header.size);
            fseek(tarFile, header.jump, SEEK_CUR);
        } else {
            fseek(tarFile, header.jump, SEEK_CUR); // Saltar archivos marcados como eliminados
        }
    }

    fclose(tarFile);
}

void delete_from_tar(const char *archiveFile, int numFiles, char *filesToDelete[]) {
    printf("Archivo TAR: %s\n", archiveFile);
    printf("Archivos a borrar:\n");
    for (int i = 0; i < numFiles; i++) {
        printf("- %s\n", filesToDelete[i]);
    }

    FILE *tarFile = fopen(archiveFile, "rb+");
    if (!tarFile) {
        perror("Error al abrir el archivo tar");
        exit(1);
    }

    struct FileHeader header;
    long fileOffset = 0;
    bool found = false;

    // Iterar sobre cada entrada del tar
    while (fread(&header, sizeof(struct FileHeader), 1, tarFile) == 1) {
        // Verificar si la entrada no está marcada como eliminada
        if (!header.deleted) {
            printf("Nombre del archivo en el tar: %s\n", header.name);
            // Iterar sobre los archivos a eliminar
            for (int i = 0; i < numFiles; i++) {
                // Comparar el nombre del archivo actual con los nombres de los archivos a eliminar
                printf("Comparando con: %s\n", filesToDelete[i]);
                if (strcmp(header.name, filesToDelete[i]) == 0) {
                    // Marcar la entrada como eliminada
                    printf("Encontrado, marcando como eliminado\n");
                    header.deleted = true;
                    header.size = header.jump;
                    fseek(tarFile, fileOffset, SEEK_SET);
                    fwrite(&header, sizeof(struct FileHeader), 1, tarFile);
                    found = true;
                    break; // Salir del bucle una vez encontrado y eliminado el archivo
                }
            }
            if (found) {
                break; // Salir del bucle principal si se encontró y eliminó el archivo
            }
        }
        // Actualizar la posición del archivo para leer la siguiente entrada
        fileOffset = ftell(tarFile);
        fileOffset += header.jump;
        fseek(tarFile, fileOffset, SEEK_SET);
    }

    fclose(tarFile);

    // Si no se encontró ningún archivo para eliminar, mostrar un mensaje
    if (!found) {
        printf("Los archivos especificados no se encontraron en el archivo tar.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <comando> <archivo_tar> [<archivos>]\n", argv[0]);
        return 1;
    }

    const char *command = argv[1];
    const char *tar_file = argv[2];

    if (strcmp(command, "-cvf") == 0) {
        if (argc < 4) {
            printf("Uso: %s -c <archivo_tar> <archivos>\n", argv[0]);
            return 1;
        }
        create_tar(tar_file, argc - 3, argv + 3);
    } else if (strcmp(command, "-xvf") == 0) {
        if (argc < 4) {
            printf("Uso: %s -x <archivo_tar> <archivos>\n", argv[0]);
            return 1;
        }
        extract_tar(tar_file, argc - 3, argv + 3);
    } else if (strcmp(command, "-tvf") == 0) {
        list_tar(tar_file);
    } else if (strcmp(command, "--delete") == 0) {
        if (argc < 4) {
            printf("Uso: %s --delete <archivo_tar> <archivo_a_borrar>\n", argv[0]);
            return 1;
        }
        delete_from_tar(tar_file, argc - 3, argv + 3);
    } else {
        printf("Comando no válido\n");
        return 1;
    }

    return 0;
}