#ifndef ANEMOMETRO_H
#define ANEMOMETRO_H
#include <Arduino.h>
//Pino ao qual a saída de sinal do indutor está conectado
#define anemometroPin 26

struct Anemometro
{
    // Circunferencia do anemômetro
    float circunferencia = 0.7917;
    // Tempo entre um pulso e outro em milissegundos
    int _tempoEntrePulso = 0;
    // Salva o tempo do ultimo pulso em milissegundos
    unsigned long _ultimoPulsoMillis;
};

/**
 * @brief Inicializa o anemômetro
 * 
 */
void anemometroBegin();
/**
 * @brief É chamada na attachInterrupt e calcula o tempo entre os pulsos do 
 * anemômetro
 * 
 */
void contador();
 /**
 * @brief execulta o calculo de velocidade do vento
 * 
 */
void anemometroRun();
/**
* @brief Retorna a velocidade em m/s
* 
* @return float 
*/
float anemometroVelocidade_Ms();

#endif // ANEMOMETRO_H