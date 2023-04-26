#ifndef SENSORES_H
#define SENSORES_H
#include <Arduino.h>
#include <math.h>
#include <DallasTemperature.h>
#include "Adafruit_HTU21DF.h"
#include <Adafruit_BMP280.h>
#include <OneWire.h>
#include <anemometro.h>
#define LED_VERMELHO 2 // Led indicador de leitura incorreta

//enum int
//{
//    bovino, geral
//};

/**
 * @brief Struct que armazena, atualiza e trata as variáveis climaticas
 * 
 */
struct VariaveisTermicas
{   
    
        //velocidade do ar medida a partir do anemômetro de copos
        float velocidadeDoAr;
        //Armazena a umidade relativa calculada em % a partir do psicrômetro
        float umidadeRelativa1;
        //Armazena a umidade relativa calculada em % a partir do sensor htu21d
        float umidadeRelativa2;
        //Armazena a temperatura de globo obtida em C
        float temperaturaDeGlobo;
        //Armazena a temperatura de bulbo seco obtida em C
        float temperaturaDeBulboSeco;
        //Armazena a temperatura de bulbo umido obitida em C
        float temperaturaDeBulboUmido;
        //Armazena a temperatura do ponto de orvalho calculada em C a partir do psicrômetro
        float pontoDeOrvalho1;
        //Armazena a temperatura do ponto de orvalho calculada em C a partir do htu21d
        float pontoDeOrvalho2;
        //Armazena a pressão do ambiente em hPa
        float pressao;
        //Índice de temperatura globo e umidade a partir do psicrômetro
        float itgu1;
        //Índice de temperatura globo e umidade a partir do htu21d
        float itgu2;
        //Índice de temperatura e umidade a partir do psicrômetro
        float itu1;
        //Índice de temperatura e umidade a partir do htu21d
        float itu2;
        //Índice de bulbu umido e terperatura de globo sem carga solar
        float ibutg1;
        //Índice de bulbu umido e terperatura de globo com carga solar
        float ibutg2;
};
/**
 * @brief variáveis temporárias para fazer a médias das
 * leituras dos sensores em 1 minuto realizando 10 leituras
 *  
 */
struct VarTemp
{
    float vAr, ur2,tgn,tbs,tbu,p;
    // reconhece o erro no sensor de bulbo umido
    bool erroSensorUmidade = false;
    // reconhece o erro no sensor dhtu21d
    bool erroSensorUmidade2 = false;
     // reconhece o erro no Sensor de bulbo seco
    bool erroSensorTbs = false;
    // reconhece o erro no Sensor de globo
    bool erroSensorGlobo = false;
    // reconhece o erro no Sensor de pressao
    bool erroSensorPressao = false;
    //contador de leituras
    uint8_t contador = 0;

    void zeraVariaveis();
};


/**
 * @brief Faz o calculo de umidade relativa usando o psicrômetro
 * 
 * 
 * @return float 
 */
float calculaUmidadeRelativa(float tbs,float tbu,float pressao);
/**
 * @brief Faz a leitura das variáveis de ambiente
 * 
 * @param int 
 */
VariaveisTermicas atualizaVariaveis(int animal);
/**
 * @brief Faz o cálculo de ITU
 * 
 * @param int 
 * @param umidadeRelativa 
 * @return float 
 */
float calculaItu(int animal, float umidadeRelativa,float tbs);  
/**
 * @brief Inicializa os Sensores
 * 
 * @return true 
 * @return false 
 */
bool sensoresBegin();

void realizarLeitura();
#endif // SENSORES_H