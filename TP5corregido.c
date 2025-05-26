#include <stdio.h>
#include <stdlib.h>

void procesarImagen(unsigned char *, unsigned char *, unsigned int, unsigned char);
void ingresarBits(unsigned char *, unsigned char, unsigned char, unsigned int); 
void insert_bits(char *, unsigned char, int, int);

int main()
{
    unsigned int numPixeles = 0;
    unsigned int bytesTotales = 0;
    unsigned char k = 0;
    printf("Ingresa el tamanio (num de pixeles) que la imagen tiene:\n");
    scanf("%u",&numPixeles);

    bytesTotales = numPixeles*3;    // Cada pixel tiene 3 colores RGB, entonces esa es la cantidad de bytes que se necesitan.

    while (!(0 < k && k < 8)) // Bucle para que el codigo se mantenga aca hasta que el usuario de una entrada aceptable.
    {
        printf("Ingresa la cantidad de bits menos significativos a recuperar:\n");
        scanf("%hhu",&k);
        if (!(0 < k && k < 8)){
            printf("El numero debe ser un entero 0 < k < 8. Intentar de nuevo\n");
        }
    }

    unsigned char * imagenIn = (unsigned char *) calloc(bytesTotales, 1);    // Hago este vector con calloc, y reservo cantidad bytesTotales de espacio. Cada char es un byte, asi que el segundo parametro es 1.
    unsigned char * mensajeOut = (unsigned char *) calloc(((bytesTotales * k)+7)/8, 1); // Lo mismo que el anterior, pero como este tiene el mensaje tengo que conseguir los bits totales que se conseguiran
                                                                                    // multiplicando cada byte por k (cantidad de bits que se guardan para cada byte entrada), le sumo 7 para que no se pierda
                                                                                    // en el peor caso algun bit al realizar la division entre 8 para llegar a unidad de bytes.

    printf("Ahora ingresa cada byte de los colores de cada pixel de la imagen en hexadecimal. Despues de escribir un byte, oprime enter antes de escribir el proximo (no dejes espacios).\n");
    for (int i = 0; i < bytesTotales; i++)
    {
        scanf("%hhx", &imagenIn[i]); // Lee hexas proporcional al tamaño que el usuario haya dicho al inicio y los guarda en la posicion adecuada en el vector de imagenIn
    }

    procesarImagen(imagenIn, mensajeOut, bytesTotales, k);

    unsigned int tamanioMensaje = (bytesTotales * k + 7) / 8;    // Saca cuantos bits tiene el mensaje oculto y lo convierte a bytes
    printf("El mensaje oculto dentro de esta imagen es:\n");
    for (unsigned int i = 0; i < tamanioMensaje; i++)
    {
        printf("%c", mensajeOut[i]); // Imprime cada char sequencialmente.
    }
    
    free(imagenIn);
    free(mensajeOut);
    
    return 0;
}

void procesarImagen(unsigned char * imagenIn, unsigned char * mensajeOut, unsigned int tamanio, unsigned char k)
{
    __asm {
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
        
        mov ebx, 0 ; se establece un i = 0 en ebx para el forloop
        ; ----- inicio forloop -----
        forloop:
            cmp ebx, [ebp+16]     ; se establece una comparacion entre i (ebx) y tamanio
            jge finLoop ; va a fin del loop si no se cumple la condicion de i < tamanio
            
            mov esi, [ebp+8]  ; Cargar la dirección base de imagenIn en esi
            mov al, [esi + ebx] ; Cargar imagenIn[i] en al
            
            mov cl, [ebp+20]    ; Mover k a cl para la operación de shift
            mov dh, 1           ; Inicializar dh con 1
            shl dh, cl          ; dh = 1 << k
            dec dh              ; dh = (1 << k) - 1 (la máscara)
            
            and al, dh          ; al ahora contiene los k bits menos significativos de imagenIn[i]
                                ; NOTA: Se evita 'and [edi], dl' para no modificar la imagenIn.
                                ; El resultado está en 'al' y se pasará a 'insert_bits'.

            
            push ebx            ; i
            push dword ptr k    ;  k el dword ptr ya que es un entero de 32 bits, asi no perdemos bits.
            push eax            ; el byte extraído, que es al
            push mensajeOut     ; el puntero a mensajeOut
            call insert_bits
            add esp, 16         ; Limpia la pila 
            
            add ebx, 1
            jmp forloop
        ; ----- fin forloop -----
        
        finLoop:
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax
    }
    
}

void insert_bits(char *message, unsigned char bits, int bits_to_recover, int component_num) {
    // Estas variables son para claridad conceptual en C.
    // El bloque de ensamblador a continuación manipula directamente los registros.
    int start_bit;
    unsigned char current_bit;
    int i;
    unsigned char message_byte; // Índice del byte en message
    unsigned char bit_in_byte;  // Posición del bit dentro de ese byte

    __asm {
        // Los argumentos de la función están en la pila:
        // [ebp+8] message (char*)
        // [ebp+12] bits (unsigned char)
        // [ebp+16] bits_to_recover (int)
        // [ebp+20] component_num (int)

        // Calcular start_bit = component_num * bits_to_recover
        mov eax, [ebp+20]   ; Cargar component_num en eax
        imul eax, [ebp+16]  ; eax = component_num * bits_to_recover
        mov start_bit, eax  ; Guardar resultado en la variable C (o mantener en registro si es conveniente)
        
        // Inicializar contador de bucle i = 0
        mov i, 0
        
        for_loop:
            // Comparar i con bits_to_recover
            mov ecx, [ebp+16]   ; Cargar bits_to_recover en ecx
            cmp i, ecx
            jge end_for
            
            // Calcular current_bit = (bits >> (bits_to_recover - 1 - i)) & 1
            mov cl, [ebp+16]    ; Cargar bits_to_recover en cl
            sub cl, 1           ; cl = bits_to_recover - 1
            sub cl, byte ptr i  ; cl = bits_to_recover - 1 - i (cantidad de desplazamiento)
            
            mov bl, [ebp+12]    ; Cargar bits (los k bits extraídos) en bl
            shr bl, cl          ; Desplazar bl a la derecha por cl
            and bl, 1           ; current_bit = (bits_desplazados) & 1 (aislar el LSB)
            mov current_bit, bl ; Guardar resultado
            
            // Calcular message_byte = (start_bit + i) / 8
            mov eax, start_bit
            add eax, i
            cdq                 ; Extender eax a edx:eax para la división
            mov ecx, 8
            idiv ecx            ; eax = (start_bit + i) / 8, edx = (start_bit + i) % 8
            mov message_byte, al ; Guardar el índice del byte
            
            // Calcular bit_in_byte = 7 - ((start_bit + i) % 8)
            ; El resto ya está en edx de la idiv anterior
            mov cl, 7
            sub cl, dl          ; cl = 7 - resto
            mov bit_in_byte, cl ; Guardar la posición del bit dentro del byte
            
            // message[message_byte] |= current_bit << bit_in_byte
            mov eax, 0
            mov al, message_byte ; Cargar el índice del byte en al
            
            mov bl, current_bit ; Cargar el bit individual a insertar
            mov cl, bit_in_byte ; Cargar la posición del bit
            shl bl, cl          ; Desplazar el bit individual a su posición correcta
            
            mov edi, [ebp+8]    ; Cargar la dirección base de message (mensajeOut) en edi
            add edi, eax        ; Añadir el índice del byte para obtener la dirección del byte objetivo
            
            or byte ptr [edi], bl ; Realizar la operación OR en el byte objetivo
            
            // Incrementar contador y bucle
            inc i
            jmp for_loop
            
        end_for:
    }
}