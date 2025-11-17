#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <termios.h>
#endif

using namespace std;


class FuenteDatos {
public:
    virtual int obtenerSiguiente() = 0;
    virtual bool tieneMasDatos() = 0;
    virtual ~FuenteDatos() {}
};


class FuenteSerial : public FuenteDatos {
private:
    #ifdef _WIN32
        HANDLE puerto;
    #else
        int puerto;
    #endif
    bool conectado;
    char buffer[256];
    int posicion;
    int longitud;

public:
    FuenteSerial(const char* nombrePuerto) {
        conectado = false;
        posicion = 0;
        longitud = 0;
        
        #ifdef _WIN32
            puerto = CreateFileA(nombrePuerto, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (puerto != INVALID_HANDLE_VALUE) {
                DCB parametros = {0};
                parametros.DCBlength = sizeof(parametros);
                GetCommState(puerto, &parametros);
                parametros.BaudRate = CBR_9600;
                parametros.ByteSize = 8;
                parametros.StopBits = ONESTOPBIT;
                parametros.Parity = NOPARITY;
                SetCommState(puerto, &parametros);
                
                COMMTIMEOUTS timeouts = {0};
                timeouts.ReadIntervalTimeout = 50;
                timeouts.ReadTotalTimeoutConstant = 50;
                timeouts.ReadTotalTimeoutMultiplier = 10;
                SetCommTimeouts(puerto, &timeouts);
                
                conectado = true;
                cout << "Conectado al puerto " << nombrePuerto << endl;
            }
        #else
            puerto = open(nombrePuerto, O_RDWR | O_NOCTTY);
            if (puerto != -1) {
                struct termios opciones;
                tcgetattr(puerto, &opciones);
                cfsetispeed(&opciones, B9600);
                cfsetospeed(&opciones, B9600);
                opciones.c_cflag |= (CLOCAL | CREAD);
                tcsetattr(puerto, TCSANOW, &opciones);
                conectado = true;
                cout << "Conectado al puerto " << nombrePuerto << endl;
            }
        #endif
    }

    int obtenerSiguiente() {
        char linea[128];
        int indice = 0;
        
        while (true) {
            if (posicion >= longitud) {
                #ifdef _WIN32
                    DWORD leidos;
                    if (!ReadFile(puerto, buffer, 255, &leidos, NULL) || leidos == 0) {
                        conectado = false;
                        return -1;
                    }
                    longitud = leidos;
                #else
                    longitud = read(puerto, buffer, 255);
                    if (longitud <= 0) {
                        conectado = false;
                        return -1;
                    }
                #endif
                posicion = 0;
            }
            
            char c = buffer[posicion++];
            if (c == '\n' || c == '\r') {
                if (indice > 0) {
                    linea[indice] = '\0';
                    return atoi(linea);
                }
            } else if (c >= '0' && c <= '9' || c == '-') {
                linea[indice++] = c;
                if (indice >= 127) {
                    linea[indice] = '\0';
                    return atoi(linea);
                }
            }
        }
    }

    bool tieneMasDatos() {
        return conectado;
    }

    ~FuenteSerial() {
        #ifdef _WIN32
            if (puerto != INVALID_HANDLE_VALUE) CloseHandle(puerto);
        #else
            if (puerto != -1) close(puerto);
        #endif
    }
};


class FuenteArchivo : public FuenteDatos {
private:
    ifstream archivo;
    bool hayDatos;

public:
    FuenteArchivo(const char* nombreArchivo) {
        archivo.open(nombreArchivo);
        hayDatos = archivo.is_open() && !archivo.eof();
    }

    int obtenerSiguiente() {
        int valor;
        if (archivo >> valor) {
            hayDatos = !archivo.eof();
            return valor;
        }
        hayDatos = false;
        return -1;
    }

    bool tieneMasDatos() {
        return hayDatos;
    }

    ~FuenteArchivo() {
        if (archivo.is_open()) {
            archivo.close();
        }
    }
};


struct NodoCircular {
    int dato;
    NodoCircular* siguiente;
    NodoCircular* anterior;
};


class BufferCircular {
private:
    NodoCircular* cabeza;
    int capacidad;
    int tamanoActual;

public:
    BufferCircular(int cap) {
        capacidad = cap;
        tamanoActual = 0;
        cabeza = NULL;
    }

    bool estaLleno() {
        return tamanoActual >= capacidad;
    }

    void insertar(int valor) {
        NodoCircular* nuevo = new NodoCircular;
        nuevo->dato = valor;

        if (cabeza == NULL) {
            nuevo->siguiente = nuevo;
            nuevo->anterior = nuevo;
            cabeza = nuevo;
        } else {
            NodoCircular* cola = cabeza->anterior;
            nuevo->siguiente = cabeza;
            nuevo->anterior = cola;
            cola->siguiente = nuevo;
            cabeza->anterior = nuevo;
        }
        tamanoActual++;
    }

    void ordenarInternamente() {
        if (tamanoActual <= 1) return;

        
        NodoCircular* actual = cabeza->siguiente;
        for (int i = 1; i < tamanoActual; i++) {
            int valorClave = actual->dato;
            NodoCircular* posicion = actual->anterior;
            NodoCircular* nodoActual = actual;
            
            int j = i - 1;
            while (j >= 0 && posicion->dato > valorClave) {
                nodoActual->dato = posicion->dato;
                nodoActual = posicion;
                posicion = posicion->anterior;
                j--;
            }
            nodoActual->dato = valorClave;
            actual = actual->siguiente;
        }
    }

    void escribirAArchivo(const char* nombreArchivo) {
        ofstream archivo(nombreArchivo);
        NodoCircular* actual = cabeza;
        for (int i = 0; i < tamanoActual; i++) {
            archivo << actual->dato << "\n";
            actual = actual->siguiente;
        }
        archivo.close();
    }

    void mostrarContenido() {
        cout << "[";
        NodoCircular* actual = cabeza;
        for (int i = 0; i < tamanoActual; i++) {
            cout << actual->dato;
            if (i < tamanoActual - 1) cout << ", ";
            actual = actual->siguiente;
        }
        cout << "]" << endl;
    }

    void limpiar() {
        while (cabeza != NULL && tamanoActual > 0) {
            NodoCircular* temp = cabeza;
            if (tamanoActual == 1) {
                cabeza = NULL;
            } else {
                cabeza->anterior->siguiente = cabeza->siguiente;
                cabeza->siguiente->anterior = cabeza->anterior;
                cabeza = cabeza->siguiente;
            }
            delete temp;
            tamanoActual--;
        }
        cabeza = NULL;
        tamanoActual = 0;
    }

    ~BufferCircular() {
        limpiar();
    }
};


void crearNombreChunk(int numero, char* nombre) {
    char buffer[20];
    int i = 0;
    int n = numero;
    
    if (n == 0) {
        buffer[i++] = '0';
    } else {
        while (n > 0) {
            buffer[i++] = '0' + (n % 10);
            n /= 10;
        }
    }
    
    int j = 0;
    nombre[j++] = 'c'; nombre[j++] = 'h'; nombre[j++] = 'u'; 
    nombre[j++] = 'n'; nombre[j++] = 'k'; nombre[j++] = '_';
    
    for (int k = i - 1; k >= 0; k--) {
        nombre[j++] = buffer[k];
    }
    
    nombre[j++] = '.'; nombre[j++] = 't'; nombre[j++] = 'm'; nombre[j++] = 'p';
    nombre[j] = '\0';
}


int fase1Adquisicion(const char* puertoSerial, int tamanoBuffer) {
    cout << "\n=== Iniciando Fase 1: Adquisicion de datos ===" << endl;
    
    FuenteSerial* fuente = new FuenteSerial(puertoSerial);
    BufferCircular buffer(tamanoBuffer);
    
    int numeroChunk = 0;
    int totalDatos = 0;
    
    while (fuente->tieneMasDatos()) {
        int valor = fuente->obtenerSiguiente();
        if (valor == -1) break;
        
        cout << "Leyendo -> " << valor << endl;
        buffer.insertar(valor);
        totalDatos++;
        
        if (buffer.estaLleno()) {
            cout << "Buffer lleno. Ordenando internamente..." << endl;
            buffer.ordenarInternamente();
            cout << "Buffer ordenado: ";
            buffer.mostrarContenido();
            
            char nombreArchivo[50];
            crearNombreChunk(numeroChunk, nombreArchivo);
            cout << "Escribiendo " << nombreArchivo << "... ";
            buffer.escribirAArchivo(nombreArchivo);
            cout << "OK." << endl;
            
            buffer.limpiar();
            cout << "Buffer limpiado.\n" << endl;
            numeroChunk++;
        }
    }
    
    
    if (!buffer.estaLleno() && totalDatos % tamanoBuffer != 0) {
        cout << "Procesando datos restantes en buffer..." << endl;
        buffer.ordenarInternamente();
        cout << "Buffer ordenado: ";
        buffer.mostrarContenido();
        
        char nombreArchivo[50];
        crearNombreChunk(numeroChunk, nombreArchivo);
        cout << "Escribiendo " << nombreArchivo << "... ";
        buffer.escribirAArchivo(nombreArchivo);
        cout << "OK." << endl;
        numeroChunk++;
    }
    
    delete fuente;
    cout << "Fase 1 completada. " << numeroChunk << " chunks generados." << endl;
    return numeroChunk;
}


void fase2Fusion(int cantidadChunks) {
    cout << "\n=== Iniciando Fase 2: Fusion Externa (K-Way Merge) ===" << endl;
    cout << "Abriendo " << cantidadChunks << " archivos fuente..." << endl;
    
    
    FuenteDatos** fuentes = new FuenteDatos*[cantidadChunks];
    int* valoresActuales = new int[cantidadChunks];
    bool* activos = new bool[cantidadChunks];
    
    
    for (int i = 0; i < cantidadChunks; i++) {
        char nombreArchivo[50];
        crearNombreChunk(i, nombreArchivo);
        fuentes[i] = new FuenteArchivo(nombreArchivo);
        
        if (fuentes[i]->tieneMasDatos()) {
            valoresActuales[i] = fuentes[i]->obtenerSiguiente();
            activos[i] = true;
        } else {
            activos[i] = false;
        }
    }
    
    ofstream salida("output.sorted.txt");
    
    cout << "K=" << cantidadChunks << ". Fusion en progreso..." << endl;
    
    
    while (true) {
    
        int indiceMenor = -1;
        int valorMenor = 2147483647; // INT_MAX
        
        for (int i = 0; i < cantidadChunks; i++) {
            if (activos[i] && valoresActuales[i] < valorMenor) {
                valorMenor = valoresActuales[i];
                indiceMenor = i;
            }
        }
        
        
        if (indiceMenor == -1) break;
        
        
        salida << valorMenor << "\n";
        cout << " - Min(";
        for (int i = 0; i < cantidadChunks; i++) {
            if (activos[i]) {
                cout << "chunk_" << i << "[" << valoresActuales[i] << "]";
                if (i < cantidadChunks - 1) cout << ", ";
            }
        }
        cout << ") -> " << valorMenor << ". Escribiendo " << valorMenor << "." << endl;
        
        
        if (fuentes[indiceMenor]->tieneMasDatos()) {
            valoresActuales[indiceMenor] = fuentes[indiceMenor]->obtenerSiguiente();
        } else {
            activos[indiceMenor] = false;
        }
    }
    
    salida.close();
    
    
    for (int i = 0; i < cantidadChunks; i++) {
        delete fuentes[i];
    }
    delete[] fuentes;
    delete[] valoresActuales;
    delete[] activos;
    
    cout << "\nFusion completada. Archivo final: output.sorted.txt" << endl;
}

int main() {
    cout << "==================================================" << endl;
    cout << "Sistema de Ordenamiento Externo E-Sort" << endl;
    cout << "==================================================" << endl;
    
    
    const char* puertoSerial = "COM3";  
    int tamanoBuffer = 1000;  
    
    
    int chunksGenerados = fase1Adquisicion(puertoSerial, tamanoBuffer);
    
    
    if (chunksGenerados > 0) {
        fase2Fusion(chunksGenerados);
    }
    
    cout << "\nLiberando memoria... Sistema apagado." << endl;
    return 0;
}