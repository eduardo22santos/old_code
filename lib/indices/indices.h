/**
 * @file indices.h
 * @author Eduardo José dos Santos (eduardo22santos@hotmail.com)
 * @brief Projeto de Pesquisa: 	PII9027-2020 - Sistema embarcado para determinação remota de índices
 *   de conforto térmico, com globo negro confeccionado em impressão 3D
 *   Orientador: 	WELINGTON GONZAGA DO VALE
 *   Centro: 	FUNDAÇÃO UNIVERSIDADE FEDERAL DE SERGIPE
 *   Departamento: 	DEPARTAMENTO DE ENGENHARIA AGRÍCOLA
 *   Cota: 	PIBITI 2020/2021 (01/08/2020 a 31/07/2021)
 *
 *   DESENVOLVIDO POR EDUARDO JOSÉ DOS SANTOS, TÉCNICO EM AGROPECUÁRIA E GRADUANDO EM ENGENHARIA AGRÍCOLA
 *   NA UNIVERSIDADE FEDERAL DE SERGIPE
 *   LATTES:  http://lattes.cnpq.br/6167567889414237
 * 
 * 
 * @version 0.1
 * @date 2022-01-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef INDICES_h
#define INDICES_h
#include <math.h>
#include <DallasTemperature.h>
#include <SparkFunHTU21D.h>
#include <Adafruit_BMP280.h>
#include <OneWire.h>
#include <RTClib.h>

/**
 * @brief Um enumerador armazenando os tipos de animais com metodologias abordadas pelo sistema
 * 
 */
enum Animal
{
    bovino, geral
};

/**
 * @brief Struct que armazena, atualiza e trata as variáveis climaticas
 * 
 */
struct VariaveisTermicas
{   
    //Armazena o horario da leitura dos sensores
    DateTime horario;
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



    //Armazena a umidade máxima
    float umidadeMax;
    //Armazena a umidade mínima
    float umidadeMin;
    //Armazena a temperatura de bulbo seco máxima
    float temperaturaMax;
    //Armazena a temperatura de bulbo seco mínima
    float temperaturaMin;
    //Armazena a temperatura de bulbo umido máxima
    float temperaturaDeBulboUmidoMax;
    //Armazena a temperatura de bulbo umido mínimo
    float temperaturaDeBulboUmidoMin;
    //Armazena a temperatura de globo negro máxima
    float globoMax;
    //Armazena a temperatura de globo negro Mínima
    float globoMin;
    //Armazena o índice itgu máximo
    float itguMax;
    //Armazena o índice itgu mínimo
    float itguMin;
    //Armazena o índice itu máximo
    float ituMax;
    //Armazena o índice itu mínimo
    float ituMin;
    //Armazena a temperatura do ponto de orvalho máxima
    float orvalhoMax;
    //Armazena a temperatura do ponto de orvalho mínima
    float orvalhoMin;
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
    // reconhece o erro no rtc
    bool erroRtc = false;
    // indica erro em algum sensor
    bool falhaSensores = false;
    
    float globo, tbs, tbu;
    //monitora a temperatura interna da interface com o termômetro instalado no rtc
    float rtcTemperature;
    //altitude recebida pelo sensor mbp280
    float altitude;
    //temperatura obtida via htu21d
    float htu21dTemperatura;
    //temperaturda obtida via bmp280
    float bmpTemperatura;

    /**
     * @brief calcula a umidade relativa a partir do bulbo umido
     * 
     * @param tbs1 temperatura de bulbo seco em °C
     * @param tbu1 temperatura de bulbo Umido em °C
     * @param pressaoBmp Pressão atmosférica em Pa
     * @return float Umidade relativa em %
     */
    float calculaUmidadeRelativa(float tbs1, float tbu1, float pressaoBmp);

    /**
     * @brief atualiza as variaveis dentro do objeto indices
     * 
     * @param globoNegro Temperatura de globo negro em °C
     * @param bulboUmido Temperatura de Bulbo umido em °C
     * @param bulboSeco Temperatura de Bulbo Seco em °C
     * @param htu21d Objeto do sensor HTU21D
     * @param animal Valor numerico inteiro do enum Animal com o tipo de animal escolhido
     * @param LED_VERMELHO Porta digital no qual o led de aviso foi instalado
     * @param bmp Objeto do sensor BMP280
     * @param tipoDeSensor true se for para calcular a umidade com o bulbo umido, false se for calcular a umidade com o sensor HTU21D
     * e o valor da temperatura de bulbo ficará zerado
     * @param relogio objeto do modulo rtc DS3231
     */
    void atualizaVariaveis(Animal animal, uint8_t LED_VERMELHO, Adafruit_BMP280 &bmp, bool tipoDeSensor, RTC_DS3231 &relogio);
    
    /**
     * @brief Calcula o itu de acordo com o animal especificado, se tipoDeSensor = false, índices que precisão da temperatura de bulbo umido serão adaptados para o itu geral
     *      Caso haja erro nos sensores, as variáveis climáticas e os índices não serão atualizados.
     * @param animal Valor numerico inteiro do enum Animal com o tipo de animal escolhido
     * @param tipoDeSensor true se for para calcular a umidade com o bulbo umido, false se for calcular a umidade com o sensor HTU21D
     * e o valor da temperatura de bulbo ficará zerado
     * @return float Valor o itu calculado
     */
    float calculaItu(Animal animal, bool tipoDeSensor, float umidadeRelativa);

    /**
     * @brief iguala os valores de máximas e mínimas para os valores no momento em que a função é chamada
     * 
     */
    void zeraMaxMin();  
 
};


#endif // INDICES_h