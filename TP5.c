#include <stdio.h>
#include <stdlib.h>

void procesarImagen(unsigned char *, unsigned char *, unsigned int, unsigned char);

int main()
{
    unsigned int numPixeles = 0;
    unsigned int bytesTotales = 0;
    unsigned char k = 0;
    printf("Ingresa el tamanio (num de pixeles) que la imagen tiene:\n");
    scanf("%u",&numPixeles);

    bytesTotales = numPixeles*3;	// Cada pixel tiene 3 colores RGB, entonces esa es la cantidad de bytes que se necesitan.

    while (!(0 < k && k < 8)) // Bucle para que el codigo se mantenga aca hasta que el usuario de una entrada aceptable.
    {
        printf("Ingresa la cantidad de bits menos significativos a recuperar:\n");
        scanf("%hhu",&k);
        if (!(0 < k && k < 8)){
            printf("El numero debe ser un entero 0 < k < 8. Intentar de nuevo\n");
        }
    }

    unsigned char * imagenIn = (unsigned char *) calloc(bytesTotales, 1);	// Hago este vector con calloc, y reservo cantidad bytesTotales de espacio. Cada char es un byte, asi que el segundo parametro es 1.
    unsigned char * mensajeOut = (unsigned char *) calloc(((bytesTotales * k)+7)/8, 1); // Lo mismo que el anterior, pero como este tiene el mensaje tengo que conseguir los bits totales que se conseguiran
																						// multiplicando cada byte por k (cantidad de bits que se guardan para cada byte entrada), le sumo 7 para que no se pierda
																						// en el peor caso algun bit al realizar la division entre 8 para llegar a unidad de bytes.

    printf("Ahora ingresa cada byte de los colores de cada pixel de la imagen en hexadecimal. Despues de escribir un byte, oprime enter antes de escribir el proximo (no dejes espacios).\n");
    for (int i = 0; i < bytesTotales; i++)
    {
        scanf("%hhx", &imagenIn[i]); // Lee hexas proporcional al tamaño que el usuario haya dicho al inicio y los guarda en la posicion adecuada en el vector de imagenIn
    }

    procesarImagen(imagenIn, mensajeOut, bytesTotales, k);

    unsigned int tamanioMensaje = (bytesTotales * k + 7) / 8;	// Saca cuantos bits tiene el mensaje oculto y lo convierte a bytes
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
		mov eax, 0 ; i = 0

        forloop:
        cmp eax, [ebp+12];  se compara i (eax)  y tamanio ([ebp+12])
		jge finLoop ; va a fin del loop si no se cumple la condicion i < tamanio
        mov ecx, [ebp+20+eax*4] ; ebp+20 + eax*4= imagenIn[0+i]
        mov edx, 1
        shl edx, [ebp+8]
        ; de acá a arriba edité código. La anterior línea comenzó a hacer esto: imagenIn[i] & ((1 << k) - 1). Pero realmente solo se ha guardado imagenIn[i] en ecx y se ha hecho un 1 << k en edx. Falta hacer el resto de la operación.
		mov bl, [ebx+ebp]
		mov edx,0
		shl ecx,1
		sub ecx,edx
		mov edx, ecx
		and al, edx
		push eax
		push edx
		push ebp
		push eax
		push esi
		call ingresarBits
		add esp, 16
		inc ebp
		jmp loop:
	finLoop:
		ret
	
	}
	
	
}

void ingresarBits(unsigned char * mensajeOut, unsigned char bitsAInsertar, unsigned char k, unsigned int numComponente)
{
    __asm {
        mov edi, numComponente  ; guarda el valor de numComponente en edi
        imul edi, k     ; multiplica edi (numComponente) por k. Esto ahora es currentFullIndex.
        
        mov esi, edi   ; mueve currentFullIndex a esi
        shr esi, 3  ; Como 8 es 2^3, hacer shift right a la derecha 3 posiciones es lo mismo que dividir en 8. Ahora esi es currentByte
    
        mov edx, edi ; mueve currentFullIndex a edx
        and edx, 0x07    ; Si hacemos operador AND con el numero 0x07 que sería en binario 00000111 hacemos una mascara donde solo tenemos los ultimos 3 bits 
                        ; que es equivalente a la operacion mod 8. esto ahora es currentBit, y los unicos valores que quedan estan en dl
    
        mov ch, dl  ; copia dl (currentBit) en ch para preparar para la condicion del if
        add ch, k   ; Se hace currentBit + k para la comparacion
        cmp ch, 8   ; compara ch contra 8
        jg tag_else ; En este if se hace la negacion de lo que teniamos en C para saltarse el if en caso que suceda que currentBit+k es exclusivamente mayor que 8
    
        ; ------------------codigo dentro del if-------------------
        mov cl, 8   ; mueve 8 a cl para preparar la operacion de (8 - currentBit - k)
        sub cl, dl  ; cl = 8 - currentBit (dl)
        sub cl, k   ; cl = cl - k
    
        movzx eax, bitsAInsertar   ; al = bitsAInsertar para preparar la operacion de bitsAInsertar <<= (8 - currentBit - k), es decir, bitsAInsertar <<= cl
        shl eax, cl  ; shift left eax por cl
    
        mov ebx, mensajeOut     ; Sacamos la direccion que esta guardando el puntero de mensajeOut y la guardamos en ebx
        add ebx, esi    ; A esta direccion le agregamos currentByte (esi) para llegar al punto del vector que estamos procesando ahora
        mov ecx, [ebx]      ; Tomamos el valor que esta dentro de la direccion guardada en ebx y la guardamos en ecx
        or ecx, eax      ; Hacemos el operador OR con eax (este teniendo a bitsAInsertar en su registro al. Toca usar eax para consistencia en los tamaños)
        mov [ebx], ecx  ; Ahora movemos el resultado a mensajeOut[currentByte], aca siendo [ebx]
    
        jmp endif   ; salto a endif para no ejecutar el else
        ; ------------------aca se cierra el if -------------------

        tag_else:
        ; ------------------codigo dentro del else-------------------
        mov bl, k
        sub bl, 8     
        sub bl, cl  ; con esto, ahora BL tiene bitsPt2, porque en el codigo original nos dimos cuenta que bitsPt1 sobraba

        movzx ecx, bitsAInsertar   ; movemos bitsAInsertar a ecx
        shr ecx, cl     ; hacemos shift right con bitsPt2. ECX es ahora pt1.
        
        mov eax, mensajeOut     ; Sacamos la direccion que esta guardando el puntero de mensajeOut y la guardamos en eax
        add eax, esi    ; A esta direccion le agregamos currentByte (esi) para llegar al punto del vector que estamos procesando ahora
        mov dl, [eax]      ; Tomamos el valor que esta dentro de la direccion guardada en eax y la guardamos en edx (aqui estaba currentBit pero ya no se necesita)
        or cl, dl      ; Hacemos el operador OR edx (valor de eax) con ecx (este siendo pt1)
        mov [eax], cl  ; Ahora movemos el resultado a mensajeOut[currentByte], aca siendo [eax]

        neg bl  ; Quiero hacer la expresion 8-bitsPt2, que es igual a -bitsPt2+8, entonces aprovecho que ya tengo bitsPt2 en bl y lo hago negativo
        add bl, 8 ; Y aca ya se hace la operacion
        mov cl, bl
        movzx edx, bitsAInsertar   ; cargamos bitsAInsertar
        shl edx, cl              ; hacemos el shift a la izquierda. Ahora EDX es pt2
        
        add eax, 1    ; En eax ya teniamos la direccion a mensajeOut[currentByte], solo falta agregarle 1 a esta direccion de memoria.
        mov [eax], dl      ; Tomamos pt2 y la guardamos en la nueva direccion de eax
        ; ------------------aca se acaba el else-------------------
        
        endif:
        nop ; No quiero hacer nada despues de la etiqueta pero tengo que poner esto o al compilador no le gusta
    }
}  
