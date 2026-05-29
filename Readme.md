# Decodificador por algoritmo de Viterbi sin uso de memoria dinámica

Implementación del algoritmo de Viterbi para su uso como decodificador de un 
código convolucional en sistemas embebidos. El proyecto comienza como una 
implementación en C para la decodificación de los datos del mensaje de 
navegación I/NAV del sistema GNSS Galileo. 

Se implementa una forma generalizada que permite la reutilización del código en caso de necesitar más de un decodificador. Por ahora solo se soporta decodificación en bloques, por lo que no es óptimo para esquemas de codificación de streams continuos de datos.
