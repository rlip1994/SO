#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

// .......................... constantes .............................................
#define PESOPAQUETE (10*1024*1024)
#define MAXLINEA 1000
#define MAXNOMBRE 256
#define CREAR "-cvf"
#define EXTRAER "-xvf"
#define ELIMINAR "-dvf"
#define ACTUALIZAR "-uvf"
#define AGREGAR "-avf"
#define LISTAR "-l"
#define DUMP "-d"
#define EXTENSION ".star"
#define TRUE 1
#define FALSE 0

// .......................... Funciones el proyecto .............................................

int bloque_disponible(char *pArchivo, int pTamanno);
void borrar_arch(char *pPaquete, int pPosicion);
int calcular_tamano_archivo(char *pArchivo);
void copiar_arch(char *pPaquete, char *pArchivo);
void Crear_paquete(char *pPaquete);
void error(const char *s);
int esDirectorio(char *Pnombre);
int existe_arch(char *pArchivo);
int existe_paq(char *pPaquete);
void extraer_arch(char *pPaquete);
char *getRutaArchivo(char *ruta, struct dirent *ent);
unsigned char getTipoArchivo(char *ruta, struct dirent *ent);
void lectorArchivos(char *ruta, int niv,int argc,int cantidadArg,int op,char *argv[]);
void lista_cont(char *pPaquete);
void lista_paq(char *pPaquete);
void menu_comandosDisponibles();
void paquete_val(char *pNombre);  
int pos_archivo(char *pPaquete, char *pArchivo);
unsigned char stat_tipoArchivo(char *fname);
char* subString(const char* pCadena, size_t pInicio, size_t pFin); 
void switch_op(int argc,int cantidadArg,int op,char *argv[]);

// .......................... Structs necesarios .............................................

typedef struct {
   int id; //id del bloque
   int estado; //libre=0 y ocupado=1
   int dirInicial; //dir de inicio del bloque
   int tam; //tam del bloque
   char ruta[MAXNOMBRE]; //ruta del archivo 
} tabla_bloque;


typedef struct {
  char datos[500*1024];
} Bloque;

// .......................... variable global .............................................

char nombrePaquete[MAXNOMBRE];

// .......................... Implementacion de las funciones .............................................

//............................. funciones extras necesarias

// obtener substring de una cadena 
char* subString(const char* pCadena, size_t pInicio, size_t pFin) 
{ 
    if(pCadena == 0 || strlen(pCadena) == 0 || strlen(pCadena) < pInicio || strlen(pCadena) < (pInicio+pFin)) 
    return 0; 
    return strndup(pCadena + pInicio, pFin); 
}

// Función para devolver un error en caso de que ocurra 
void error(const char *s)
{
  /* perror() devuelve la cadena S y el error (en cadena de caracteres) que tenga errno */
  perror (s);
  exit(EXIT_FAILURE);
}

// parsea el nombre y valida el tipo de paquete
void paquete_val(char *pNombre)
{ 
  int posDot = 0;
  int cont = 0;
  int largo = strlen(pNombre);
  char *temp_paq = pNombre;
  char ext[10];

  while(*temp_paq!= '.') 
  {
    temp_paq++;
    posDot++;
  }

  while(*temp_paq!= '\0') 
  {
    ext[cont] = *temp_paq;
    cont++;
    temp_paq++;
  }

  if(strcmp(ext, EXTENSION) == 0)
  {
    printf("El tipo de paquete es valido\n");
    if(existe_paq(pNombre) == TRUE)
    {
      printf("El paquete ya existe.\n");
    }
    else
    {
      printf("Se crea el paquete.\n");
      Crear_paquete(pNombre);
    }
  }
  else
  {
    printf("El tipo de paquete no es valido\n");
    exit(2);
  }
}


// comprueba que el pquete exista
int existe_paq(char *pPaquete)
{
	FILE *paquete;
	paquete  = fopen (pPaquete, "r");
  if (!paquete) 
  {
    return FALSE;
  }
  else
  {
    fclose(paquete);
    return TRUE;
  }
}


//comprueba el si el archivo existe o no
int existe_arch(char *pArchivo)
{
  FILE *archivo;
  archivo  = fopen (pArchivo, "r");
  if(!archivo) 
  {
    return FALSE;
  }
  else
  {
    fclose(archivo);
    return TRUE;
  }
}

// busca un bloque disponible para escribir
int bloque_disponible(char *pArchivo, int pTamanno)
{
  FILE *paquete;
  paquete = fopen(pArchivo, "rb");
  tabla_bloque tabla;
  int bloque_disponible = -1;
  int i = 0;

  while(i<50){

    fseek(paquete, i*sizeof(tabla_bloque), SEEK_SET);
    fread(&tabla,sizeof(tabla_bloque), 1, paquete);
    if(tabla.estado != 1 && pTamanno <= (500*1024))
    {
      bloque_disponible = i;
      break;
    }

    i++;
  }
 
  fclose(paquete);
  return bloque_disponible;
}

//calcula el tamano del archivo
int calcular_tamano_archivo(char *pArchivo)
  
{
  printf("archivo: %s\n",pArchivo );
  FILE *archivo;
  int tam;
  archivo = fopen(pArchivo,"rb");
  
  if(fseek(archivo, 0, SEEK_END)) 
  {
    printf("Se ha producido un error obteniendo el tamaño de archivo\n");
    return -1;
  }
  printf("...............................\n");
  tam = ftell(archivo);
  if(tam < 0) 
  {
    puts("Se ha producido un error navegando en el archivo\n");
    return -1;
  }
  fclose(archivo);
  printf("tam: %i\n",tam );
  return tam;
}


// busca la posicion del archivo a eliminar

int pos_archivo(char *pPaquete, char *pArchivo)
{
  int posicion = -1;
  tabla_bloque tabla;
  char nombre[strlen(pArchivo)+1]; 
  strcpy(nombre, pArchivo);
  FILE *paquete, *archivo;
  paquete = fopen(pPaquete,"rb");
  int i=0;

  while(i < 50){

    fseek(paquete, i*sizeof(tabla_bloque), SEEK_SET);
    fread(&tabla, sizeof(tabla_bloque), 1, paquete);
    if(strcmp(nombre, tabla.ruta) == 0)
    {
      posicion = i;
      break;
    }
    i++;
  }
  
  fclose(paquete);
  printf("Se ha encontrado el id del archivo a eliminar\n");
  return posicion;
}

// obtener la ruta de archivo
char *getRutaArchivo(char *ruta, struct dirent *ent) 
{
  char *nombrecompleto;
  int tmp;

  tmp=strlen(ruta);
  nombrecompleto=malloc(tmp+strlen(ent->d_name)+2); /* Sumamos 2, por el \0 y la barra de directorios (/) no sabemos si falta */
  if (ruta[tmp-1]=='/')
    sprintf(nombrecompleto,"%s%s", ruta, ent->d_name);
  else
    sprintf(nombrecompleto,"%s/%s", ruta, ent->d_name);
  return nombrecompleto;
}



/* funciones para determinarel tipo de archivo*/

unsigned char getTipoArchivo(char *nombre, struct dirent *ent)
{
  unsigned char tipo;

  tipo=ent->d_type;
  if (tipo==DT_UNKNOWN)
    {
      tipo=stat_tipoArchivo(nombre);
    }

  return tipo;
}

/* stat() es un sistem call en donde se guarda informacion de los archivos 
   que se encuentran en un path determinado*/
// unsigned : unsigned int
unsigned char stat_tipoArchivo(char *fname)
{
  struct stat sdata; // llamar el struct de stat 

  /*
    para este caso solo se usara el st_mode para saber el tipo de archivo
  */
    //stat(path,buf) 
  if (stat(fname, &sdata) == -1)
    {
      return DT_UNKNOWN;
    }

  switch (sdata.st_mode & S_IFMT) // S_IFMT : considerar los bits que indican el tipo del archivo
    {
    case S_IFBLK:  return DT_BLK; // block device
    case S_IFCHR:  return DT_CHR; // character device
    case S_IFDIR:  return DT_DIR; // directorio
    case S_IFIFO:  return DT_FIFO; // FIFO
    case S_IFLNK:  return DT_LNK; // symbolic link
    case S_IFREG:  return DT_REG; // archivo regular
    case S_IFSOCK: return DT_SOCK; // socket
    default:       return DT_UNKNOWN; // en caso que el tipo del archivo sea desconocido
    }
}
 // funcion para saber si el archivo entrada es un directorio o no
int esDirectorio(char *Pnombre){
  unsigned char tipo = stat_tipoArchivo(Pnombre);
  if(tipo == DT_DIR){
    printf("Es un directorio\n");
    return 1;
  } 
  else{
    printf("Es un archivo\n");
    return 0;
  } 
}

// .......................... Funciones principales de tar 

//funcion que crea el paquete
void Crear_paquete(char *pPaquete)
{
  FILE *paquete;
  paquete  = fopen (pPaquete, "wb+");
  int tam = PESOPAQUETE;
  int cant_bloques = tam/sizeof(tabla_bloque);
  printf("%i\n", cant_bloques);
  /*
    Se escribe la tabla con la informacion de los archivos
  */
  tabla_bloque tabla;
  int i;
  for (i = 0; i< cant_bloques; i++)
  {
    tabla.id = i;
    tabla.estado = FALSE; 
    tabla.dirInicial = (i+1)*500*1024;
    tabla.tam = 500*1024;
    sprintf(tabla.ruta, "0");
    fwrite(&tabla, sizeof(tabla_bloque), 1, paquete);
  }
  /*
    Se crean los bloques en el paquete
  */
  fseek(paquete, 0, SEEK_END);
  Bloque segmento;
  sprintf(segmento.datos,"0");
  for (i = 0; i< 50; i++)
  {
    fwrite(&segmento, sizeof(Bloque),1, paquete);
  }
  fclose(paquete);
}

//lista de archivos que contiene un paquete
void lista_cont(char *pPaquete)
{
  tabla_bloque tabla;
  int contenido = FALSE; // contenido = false -> no hay contenido(esta vacio),si es true entonces hay contenido
  FILE *paquete;
  paquete = fopen(pPaquete,"rb");
  printf("Lista de Contenido\n");
  int i;
  for(i = 0; i < 50;i++){
    fseek(paquete, i*sizeof(tabla_bloque), SEEK_SET);
    fread(&tabla, sizeof(tabla_bloque), 1, paquete);
    if(tabla.estado)
    {
      contenido = TRUE;
      printf("Nombre: %s\n", tabla.ruta);
    }
  }
  fclose(paquete);

  if(!contenido)
  {
    printf("El paquete esta vacio.\n");
  }
}

// funcion que imprime la informacion de la tabla del paquete
void lista_paq(char *pPaquete)
{
  tabla_bloque tabla;
  FILE *paquete;
  paquete = fopen(pPaquete,"rb");
  printf("Tabla de Archivos\n");
  int i = 0;

  while(i < 50){
    fseek(paquete, i*sizeof(tabla_bloque), SEEK_SET);
    fread(&tabla, sizeof(tabla_bloque), 1, paquete);
    if(tabla.estado)
    {
      printf("|| Id: %i || Nombre: %s || Direccion Inicial: %i || Tamaño: %i\n", tabla.id, tabla.ruta, tabla.dirInicial, tabla.tam);
    }
    else
    {
      printf("|| Id: %i || Espacio Libre || Direccion Inicial: %i || Tamaño: %i\n", tabla.id, tabla.dirInicial, tabla.tam);
    }

    i++;
  }

  fclose(paquete);
}


// copia un archivo en el paquete

void copiar_arch(char *pPaquete, char *pArchivo)
{
  printf("entra a copiar\n");
  FILE *archivo, *paquete;
  int tam;
  archivo = fopen (pArchivo, "rb+");

  tam = calcular_tamano_archivo(pArchivo);
  printf("..............................\n");
  char datos[tam];
  fread(datos, 1, tam, archivo);
  fclose(archivo);
    
  int posicion = bloque_disponible(pPaquete, tam);
  if (posicion == -1){
    printf("No hay bloques disponibles.\n");
  }

  paquete = fopen(pPaquete,"rb+");
  fseek(paquete, posicion*sizeof(tabla_bloque), SEEK_SET);
  tabla_bloque tabla;
  tabla.id = posicion;
  tabla.estado = 1;
  tabla.dirInicial = (posicion+1)*sizeof(Bloque);
  tabla.tam = tam;
  sprintf(tabla.ruta,"%s",pArchivo);
  printf("ruta del archivo: %s\n",tabla.ruta);
  fwrite(&tabla,sizeof(tabla_bloque), 1, paquete);
  fseek(paquete,(posicion+1)*sizeof(Bloque), SEEK_SET);

  Bloque block;
  sprintf(block.datos,"%s", datos);
  fwrite(&block, sizeof(Bloque), 1, paquete);
  fclose(paquete);
  printf("Se ha agregado al paquete el archivo %s.\n", pArchivo);
}

// funcion para extraer los archivos que contiene el paquete

void extraer_arch(char *pPaquete)
{
  tabla_bloque tabla;
  FILE *paquete, *archivo;
  paquete = fopen(pPaquete,"rb");
  int i = 0;

  while(i <= 49){

    fseek(paquete, i*sizeof(tabla_bloque), SEEK_SET); // da la posicion del archivo del flujo en el dezplasamiento dado
    /*leer un bloque
    fread(ptr:puntero de bloque en memoria,size:tam de cada elemento,count: numero de elementos,stream: puntero al objeto FILE)
    */
    fread(&tabla, sizeof(tabla_bloque), 1, paquete);

    if(tabla.estado)
    {
      archivo = fopen(tabla.ruta,"wb");
      fclose(archivo);
      Bloque block;
      fseek(paquete,(i+1)*sizeof(Bloque), SEEK_SET);
      fread(&block,sizeof(Bloque), 1, paquete);
      archivo = fopen(tabla.ruta,"a+b");
      /* fwrite: 
       *Es capaz de escribir hacia un fichero uno o varios 
       *registros de la misma longitud almacenados a partir de 
       *una dirección de memoria determinada.
       */ 
      fwrite(block.datos, tabla.tam, 1, archivo);
      fclose(archivo);
    }

    i++;    
  }
  
  fclose(paquete);
  printf("Se han extraido todos los archivos\n");
}


//elimina un archivo segun la posicion que se le de

void borrar_arch(char *pPaquete, int pPosicion)
{
  FILE *paquete;
  paquete = fopen(pPaquete,"rb+");
  tabla_bloque tabla;
  tabla.id = pPosicion;
  tabla.estado = FALSE; 
  tabla.dirInicial = (pPosicion+1)*sizeof(Bloque);
  tabla.tam = 500*1024;
  sprintf(tabla.ruta, "0");
  fwrite(&tabla, sizeof(tabla_bloque), 1, paquete);

  fseek(paquete, pPosicion*sizeof(tabla_bloque), SEEK_SET);
  fwrite(&tabla,sizeof(tabla_bloque), 1, paquete);

  Bloque block;
  sprintf(block.datos, "0");
  fseek(paquete, (pPosicion+1)*sizeof(Bloque), SEEK_SET);
  fwrite(&block,sizeof(Bloque), 1, paquete);
  fclose(paquete);
}

 
//............................. funcion para tratar los directorios

void lectorArchivos(char *ruta, int niv,int argc,int cantidadArg,int op,char *argv[])
{
  /* Con un puntero a DIR abriremos el directorio */
  DIR *dir;
  /* en *ent habrá información sobre el archivo que se está "sacando" a cada momento */
  struct dirent *ent;
  unsigned numfiles=0;          /* Ficheros en el directorio actual */
  unsigned char tipo;       /* Tipo: fichero /directorio/enlace/etc */
  char *nombrecompleto;     /* Nombre completo del fichero */
  dir = opendir (ruta);
  int posicion;

  /* Miramos que no haya error */
  if (dir == NULL)
    error("No puedo abrir el directorio");
 
  while ((ent = readdir (dir)) != NULL)

    {
      if ( (strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0) )

    {
      nombrecompleto=getRutaArchivo(ruta, ent);
      tipo=getTipoArchivo(nombrecompleto, ent);
      if (tipo==DT_REG) //si el tipo es un archivo
        {
          ++numfiles;
          printf("nombre del archivo: %s\n",ent->d_name);

          switch(op){
            case 0:
              menu_comandosDisponibles();
              break;
            case 1:
              printf("CREAR\n");
              // validar paquete
              paquete_val(argv[2]);
              // ........
              copiar_arch(argv[2], nombrecompleto);
              printf("Se ha creado el paquete\n");
              break;
            case 2:
              printf("ELIMINAR\n");
                posicion = pos_archivo(argv[2], nombrecompleto);
                borrar_arch(argv[2], posicion);
                printf("Se ha borrado el archivo solicitado\n");
              break;
            case 3:
              printf("EXTRAER\n");
              extraer_arch(argv[2]);
              break;
            case 4:
              printf("ACTUALIZAR\n");
              posicion = pos_archivo(argv[2], nombrecompleto);
              borrar_arch(argv[2], posicion);
              copiar_arch(argv[2], nombrecompleto);
              printf("Se actualizo el archivo en el paquete\n");
            break;

            case 5:
              printf("AGREGAR\n");
              copiar_arch(argv[2],nombrecompleto);
              break;
          }

        }
      else if (tipo==DT_DIR) // si el tipo es un directorio 
        {
          
          printf("Entrando en: %s\n", nombrecompleto);
          lectorArchivos(nombrecompleto,niv+1,argc,cantidadArg,op,argv);         
          printf("\n");
          
        }
      free(nombrecompleto);
    }
     
    }
  closedir (dir); // cerrar el directorio
}



// .......................... Menu principal ............................................./*

/*Funcion que retorna el numero de la opcion escrita por el usuario.
 *Si ingresa una opcion incorrecta retornara 0 sino un numero del 1-7
*/
int num_opcion(char *opcion){
  int op = 0;
  printf("opcion: %s\n",opcion );
  if(strcmp(opcion,CREAR) == 0 ){ // si es igual a 0 entonces son iguales
    op = 1;
    return op; 
  }
  else if (strcmp(opcion,ELIMINAR) == 0 ){
    op = 2;
    return op;
  }
  else if (strcmp(opcion,EXTRAER) == 0 ){
    op = 3;
    return op;
  }
  else if (strcmp(opcion,ACTUALIZAR) == 0 ){
    op = 4;
    return op;
  }
  else if (strcmp(opcion,AGREGAR) == 0 ){
    op = 5;
    return op;
  }
  else if (strcmp(opcion,LISTAR) == 0 ){
    op = 6;
    return op;
  }
  else if (strcmp(opcion,DUMP) == 0 ){
    op = 7;
    return op;
  }
  else{
    return op; // la opcion es incorrecta 
  }


}

// print del menu de opciones que se encuentran disponibles 
void menu_comandosDisponibles(){
  printf("El comando que ingreso es incorrecto\n");
  printf("Los comandos disponibles son: \n");
  printf("               Crear un archivo: -cvf \n");
  printf("               Eliminar un archivo: -dvf \n");
  printf("               Extraer el contenido un archivo: -xvf \n");
  printf("               Actualizar el contenido un archivo: -uvf \n");
  printf("               Agregrar contenido de un archivo: -avf \n");
  printf("               Listar los contenidos de  un archivo: -l \n");
  printf("               Pruebas de correctitud: -d \n");

}

// funcion con la implementacion de las funcionalidad de tar para archivos 
void switch_op(int argc,int cantidadArg,int op,char *argv[]){
  int i;

  switch(op){
        case 0:
          menu_comandosDisponibles();
          exit(2);
          break;

        case 1:
          printf("CREAR\n");

          for(i = 3; i < cantidadArg; i++)
          { 
            if(existe_arch(argv[i]))
            {
              printf("El archivo en la posicion %s si existe.\n", argv[i]);
            }
            else
            {
              printf("El archivo en la posicion %s no existe.\nVerifique el archivo.\n", argv[i]);
            }
          }
          paquete_val(argv[2]);

          i = 3;
          while(i<cantidadArg){

            copiar_arch(argv[2], argv[i]);

            i++;
          }
        

          printf("Se ha creado el paquete\n");
          break;

        case 2:
          printf("ELIMINAR\n");
          i = 3;
          //printf("%d\n",argc-1 ); agregar eso al resto
          if(argc-1 == 2){
            printf("Debe indicar el nombre del archivo que desea eliminar\n");
            exit(2);
            break;
          }
          else{

            while(i < cantidadArg){

              int posicion = pos_archivo(argv[2], argv[i]);
              borrar_arch(argv[2], posicion);
              printf("Se ha borrado el archivo solicitado\n");

              i++;
            }

          }             
          break;

        case 3:
          printf("EXTRAER\n");
          extraer_arch(argv[2]);
          break;

        case 4:
          printf("ACTUALIZAR\n");
          i = 3;
          while(i < cantidadArg){
            int posicion = pos_archivo(argv[2], argv[i]);
            borrar_arch(argv[2], posicion);
            copiar_arch(argv[2], argv[i]);
            printf("Se actualizo el archivo en el paquete\n");

            i++;
          } 
          break;

        case 5:
          printf("AGREGAR\n");  
          i = 3;

          while(i < cantidadArg){

            copiar_arch(argv[2], argv[i]);

            i++;
          }

          printf("Se han agregado los archivos al paquete\n");
          break;

        case 6:
          printf("LISTAR\n");
          lista_cont(argv[2]);
          break;

        case 7:
          printf("DUMP\n");
            lista_paq(argv[2]);
          
          break;

      }
}

// .......................... Funcion principal .............................................
/*Funcion principal*/
int main(int argc, char *argv[]) {
  int i; 
  int cantidadArg = 0;
  int existe_paquete;
  int cant_archivo = 0, cant_directorios = 0; // solo se puede usar un tipo de archivo al mismo tiempo
  int archivo_ = 0 , directorio_ = 0;  // entrada es archivo o directorio
  unsigned char tipo; /* Tipo: fichero /directorio/enlace/etc */

  while (argv[cantidadArg] != '\0')
  {
    cantidadArg++;
  } 

  int opcion = num_opcion(argv[1]); // crear/elimianar/update, etc...
  printf("comando: %d\n",opcion );

//.................................... validaciones .................................
  if(opcion>=2){
    if(!existe_paq(argv[2])){
      printf("Para implementar los comandos de: Eliminar,Extraer,Agregar,Actualizar y Listar , el paquete(archivoSalida) debe existir\n");
      exit(2);
    }
  }

  // ciclo para saber si hay mas de dos tipos de entradas el mismo tiempo 
  for(int i = 3; i < cantidadArg; i++){
    if(esDirectorio(argv[i])){
     cant_directorios++;
    }
    else{
      cant_archivo++;
    }
  }
  // si hay mas de un tipo de entrada dara error 
  printf("cantidad de tipos de archivos %i y cantidad de directorios %i\n",cant_archivo,cant_directorios);
  if(cantidadArg > 4){
    if(cant_archivo != 0 && cant_directorios != 0) { // hay diferentes tipos de archivos
      printf("Las entradas deben corresponder a solo directorios o solo archivos.\n");
      exit(2);
    }
  }
  if(cant_archivo != 0 && cant_directorios == 0){
    archivo_ = 1;
    printf("archivo\n");
  }
  if(cant_archivo == 0 && cant_directorios != 0){
    directorio_ = 2;
    printf("directorio_\n");

  }

 //...................................  

    if(argc-1 >= 2){ // no se puede ingresar con menos de 2 parametros.

      if(directorio_ == 2){ // cuando trata con directorios
        printf("entra a directorio\n");
        //lectorArchivos(char *ruta, int niv,int argc,int cantidadArg,int op,char *argv[])
        printf("nombre del directorio: %s\n", argv[3]);
        lectorArchivos(argv[3],1,argc,cantidadArg,opcion,argv);
      }
      else{ // cuando las entradas son archivos 
        printf("es un archivo\n");
        switch_op(argc,cantidadArg,opcion,argv);

    }
    
  }

  else{
      printf("El numero de parametros son incorrectos\n");
      printf("star <opciones> <archivoSalida> <archivo1> <archivo2> ... <archivoN>\n");
      exit(2);
    }
 
  return EXIT_SUCCESS;
}