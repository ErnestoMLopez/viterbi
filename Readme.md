# Decodificador por algoritmo de Viterbi sin uso de memoria dinámica

Implementación del algoritmo de Viterbi para su uso como decodificador de un 
código convolucional en sistemas embebidos. El proyecto comienza como una 
implementación en C para la decodificación de los datos del mensaje de 
navegación I/NAV del sistema GNSS Galileo. 

Se implementa una variante con parámetros estáticos de acuerdo a la aplicación y
una generalizada que permite la reutilización del código en caso de necesitar 
más de un decodificador.