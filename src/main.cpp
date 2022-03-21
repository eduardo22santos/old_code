/**
 * @file main.cpp
 * @author Eduardo José dos Santos (eduardo22santos@hotmail.com)
 * @brief   Projeto de Pesquisa: 	PII9027-2020 - Sistema embarcado para determinação remota de índices
 *          de conforto térmico, com globo negro confeccionado em impressão 3D
 *          Orientador: 	WELINGTON GONZAGA DO VALE
 *          Centro: 	FUNDAÇÃO UNIVERSIDADE FEDERAL DE SERGIPE
 *          Departamento: 	DEPARTAMENTO DE ENGENHARIA AGRÍCOLA
 *          Cota: 	PIBITI 2020/2021 (01/08/2020 a 31/07/2021)
 *
 *          DESENVOLVIDO POR EDUARDO JOSÉ DOS SANTOS, TÉCNICO EM AGROPECUÁRIA E GRADUANDO EM ENGENHARIA AGRÍCOLA
 *          NA UNIVERSIDADE FEDERAL DE SERGIPE
 *          LATTES:  http://lattes.cnpq.br/6167567889414237
 * @version beta
 * @date 2022-03-15
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <Arduino.h>
#include <Wire.h>
#include <indices.h> //biblioteca original do projeto
#include <salvar.h> //biblioteca original do projeto
#include <WiFi.h>
#include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks
#include <esp_wifi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <RTClib.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeMonoOblique9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/Org_01.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_BMP280.h>
#include <Button2.h> //No gitHub ha uma versao mais recente
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <esp_sleep.h>
#define BUTTON_PIN 26 //Indica qual porta esta sendo utilizada para o botao de interacao
#define LED_VERMELHO 32 // Led indicador de leitura incorreta
#define LED_VERDE 2 //INDICA O ESTADO DA PLACA
#define LED_AMARELO 15 //INDICA O ESTADO DA INTERNET
#define CLOCK_INTERRUPT_PIN 34 //Indica a porta para o pino sqw do rtc



/***************************************************
 * INSTANCIANDO OS LOOPS PARA MULTITAREFA
 * **************************************************
 */

/**
 * @brief indentificador taskHandle_d da tarefa loopMqtt
 * 
 */
TaskHandle_t loop_mqtt;
/**
 * @brief indentificador taskHandle_d da terefa loopBotao
 * 
 */
TaskHandle_t loop_botao;
/**
 * @brief indentificador taskHandle_d da terefa loopConfiguracao
 * 
 */
TaskHandle_t loop_configuracao;
/**
 * @brief indentificador taskHandle_d da terefa tarefaHttp
 * 
 */
TaskHandle_t tarefa_http;
/**
 * @brief indentificador taskHandle_d da terefa que salva os dados no cartão de memória
 * 
 */
TaskHandle_t tarefa_salvar;
/**
 * @brief indentificador taskHandle_d da terefa que envia os dados para o servidor
 * 
 */
TaskHandle_t tarefa_enviar_mqtt;
/**
 * @brief indentificador taskHandle_d da terefa que irá fazer a reconexão com o wifi e servidor
 * 
 */
TaskHandle_t reconectar_internet;
/**
 * @brief faz a reconexão com a rede de internet
 * 
 * @param pvParameters 
 */
void reconectarRede(void * pvParameters);
/**
 * @brief tarefa que irá salvar os dados no cartão
 * 
 * @param pvParameters 
 */
void tarefaSalvarSd(void * pvParameters);
/**
 * @brief tarefa que irá enviar os dados no servidor
 * 
 * @param pvParameters 
 */
void tarefaEnviarMqtt(void * pvParameters);
/**
 * @brief Faz a leitura do botão de interação, tornando mais inteligente e possibitando funções multi cliques
 * 
 * @param pvParameters possibilita iniciar a tarefa passando uma variável ou estrutura de dados
 */
void loopBotao(void * pvParameters);
/**
 * @brief Abre uma interface de configuração, possibilitando atualizar o horario no rtc, mudar itu e habilitar o envio de dados na plataforma
 * 
 * @param pvParameters possibilita iniciar a tarefa passando uma variável ou estrutura de dados
 */
void loopConfiguracao(void * pvParameters);
/**
 * @brief Prepara o json para enviar para plataforma online, abrindo uma conexão e fazendo um http post com o json contendo as leituras dos sensores
 * 
 * @param pvParameters possibilita iniciar a tarefa passando uma variável ou estrutura de dados
 */
void tarefaHttp(void * pvParameters);
/**
 * @brief Salva as leituras dos sensores no cartão de memória escrevendo os dados em um arquivo .csv
 * 
 * @param variaveisTermicas objeto contendo os dados ambientais
 */
void tarefaSalvar(VariaveisTermicas variaveisTermicas);

/**********************************************************
 * CONFIGURACOES DOS SENSORES DE TEMPERATURA E PRESSÃO
 * ********************************************************
 */

/**
 * @brief objeto onewire contento o valor do pino de conexão do sensor DS18B20 para o bulboSeco
 * 
 */
OneWire principal(4);
/**
 * @brief objeto onewire contendo o valor do pino de conexão do sensor DS18B20 para o globoNegro
 * 
 */
OneWire secundario(14);
/**
 * @brief objeto onewire contendo o valor do pino de conexão do sensor DS18B20 para o bulboUmido
 * 
 */
OneWire terciario(27);
/**
 * @brief objeto para manipulação e leitura do sensor de bulbo seco
 * 
 * @return DallasTemperature 
 */
DallasTemperature bulboSeco(&principal);
/**
 * @brief objeto para manipulação e leitura do sensor de globo negro
 * 
 * @return DallasTemperature 
 */
DallasTemperature globoNegro(&terciario);
/**
 * @brief objeto para manipulação e leitura do sensor de bulbo umido
 * 
 * @return DallasTemperature 
 */
DallasTemperature bulboUmido(&secundario);
/**
 * @brief Objeto para manipulação e leitura do sensor de umidade e temperatura HTU21D
 * 
 */
HTU21D htu21d;
/**
 * @brief Ojeto para manipulação e leitura do sensor de pressão e temperatura BMP280, conectado via I2C
 * 
 */
Adafruit_BMP280 bmp;
/**
 * @brief Objeto para realizar leituras nos sensores, armazenar e reconhecer falhas nos sensores
 * 
 */
VariaveisTermicas dadosLocais;

/***************************************************
 * Nome de pastas e arquivos no sd card
 * **************************************************
 */

/**
 * @brief nome do arquivo de configuracao no formato json
 * 
 */
String arquivoDeConfiguracao = "/configuracao";
/**
 * @brief nome do diretório que armazena os arquivos diários
 * 
 */
String pasta = "/arquivos";
/**
 * @brief arquivo que armazena todas a leituras do aparelho
 * 
 */
String arquivoDatalog_txt = "/datalog.csv";
/**
 * @brief cabeçalho do arquivo datalog, contendo o identificador de cada coluna
 * 
 */
String datalogCabecalho = "data,horario,umidade1,umidade2,temperaturaDeGlobo,temperaturaBulboSeco,temperaturaBulboUmido,htu21d_Temperatura,bmp_Temperatura,itu1,itu2,itgu1,itgu2,ibutg1,ibutg2,orvalho1,orvalho2,hpa,rtcTemperatura,altitude,falha";
/**
 * @brief Nome do arquivo que salva os valores de máximas e mínimas diários
 * 
 */
String arquivoMaxMin = "/maximas_e_minimas.csv";
/**
 * @brief cabeçalho do arquivo "/maximas_e_minimas.csv" contendo o identificador de cada coluna
 * 
 */
String maxMinCabecalho = "data,horario,tbsMax,tbsMin,tbuMin, TbuMax, umidadeMax,umidadeMin,tgMax,tgMin,ituMax,ituMin,itguMax,itguMin,orvalhoMax,orvalhoMin";
/**
 * @brief String modelo para a geração dos nomes dos arquivos diários, que serão gerados a partir da função toString() da lib RTCLIB
 * 
 */
char dataArquivosDiarios[] = "/arquivos/DD - MM - YYYY.csv";
/**
 * @brief 
 * 
 */
String arquivoMaxMinJson = "/maximas_e_minimas_atual.json";

/***************************************************
 * Objetos e variaveis de configuração
 * **************************************************
 */

/**
 * @brief Objeto que armazena as configuracoes fornecidas no arquivo json de configuracao e deixa disponivel para usar no sistema.
 * 
 */
Configuracao configuracao;
/**
 * @brief Indica se é para iniciar a conexão com o wifi e estabelecer a conexão com a plataforma online
 * 
 */
bool internetAtiva = false;
/**
 * @brief Indica se a conexão com a plataforma está ativa
 * 
 */
bool internetEstado = false;

/***************************************************
 * CONFIGURACOES DO RELOGIO, WIFI, TELA E BOTAO
 * **************************************************
 */

/**
 * @brief função callback para receber mensagens via mqtt e realizar ações
 * 
 * @param topic topico mqtt
 * @param payload mensagem em formato payload
 * @param length tamanho da mensagem
 */
void callback(char* topic, byte* payload, unsigned int length);
/**
 * @brief Objeto de manipulação e leitura do RTC DS3231
 * 
 */
RTC_DS3231 relogio;
/**
 * @brief Objeto DateTime para armazenar o horário para marcar o alarme do RTC DS3231
 * 
 */
DateTime zeroHora;
/**
 * @brief Objeto necessário para o objeto timeClient
 * 
 */
WiFiUDP ntpUDP;
/**
 * @brief Objeto para atualização, manipulação e letura do relógio NTP
 * 
 */
NTPClient timeClient(ntpUDP, configuracao.servidorNtp, configuracao.timeZone * 3600, 60000); //pode-se alterar o servidor ntp
/**
 * @brief Objeto necessário para o objeto client
 * 
 */
WiFiClient espClient;
/**
 * @brief Objeto de manipulação para o uso e implementação do protocolo MQTT
 * 
 * @return PubSubClient 
 */
PubSubClient client(espClient);
/**
 * @brief Objeto de manipulação do display
 * 
 */
Adafruit_SSD1306 display(128, 64, &Wire);
/**
 * @brief Objeto do botão de interação
 * 
 */
Button2 button = Button2(BUTTON_PIN);
//ALTERNAR ENTRE AS TELAS, varivel salva na ram do rtc interno do esp32
/**
 * @brief Variável salva na memória ram do rtc interno, a qual é utilizada para indicar a tela após a reinicialização do esp32
 * 
 */
RTC_DATA_ATTR char alternar = 'a';
/**
 * @brief Variável salva na memória ram do rtc interno, a qual indica se a reinicialização do sistema foi realizada meia noite
 * 
 */
RTC_DATA_ATTR bool salvarHorarioMeiaNoite = false;
/**
 * @brief Variável salva na memória ram do rtc interno, a qual indica para a o sistema entrar no modo de configuração após a reinicialização
 * 
 */
RTC_DATA_ATTR bool configuracoesNoBoot = false;
//É um indicativo para a placa entrar em modo sleep quando está ociosa, quando está em modo offline
/**
 * @brief Variável salva na memória ram do rtc interno, a qual indica para a placa salvar o dado no cartão de memória ainda na função setup()
 * 
 */
RTC_DATA_ATTR bool salvarDado = false;
//funcoes que contao os clicks do botao de interacao
/**
 * @brief fução que reconhece o click rápido no botão de interação
 * 
 * @param btn 
 */
void click(Button2& btn);
/**
 * @brief função que reconhece o click longo no botão de interação
 * 
 * @param btn 
 */
void doubleClick(Button2& btn);
/**
 * @brief Utilizado para indicar a posião da seta na tela
 * 
 */
uint8_t indice = 0; 

//***********************************************************************
//variaveis para o botão de interação no loop de configuração
//***********************************************************************



/**
 * @brief Desativa o loopRelogio com um vtaskSuspend()
 * 
 */
void desativaLoopRelogio();
/**
 * @brief Utilizada para executar a função escolhida na tela
 * 
 */
bool ativarFuncao = false;
/**
 * @brief Utilizada para indicar a rotina a ser executada após a seleção da função na tela
 * 
 */
bool um = false;
/**
 * @brief Utilizada para indicar a rotina a ser executada após a seleção da função na tela
 * 
 */
bool dois = false;
/**
 * @brief Utilizada para indicar a rotina a ser executada após a seleção da função na tela
 * 
 */
bool tres = false;
/**
 * @brief Utilizada para indicar a rotina a ser executada após a seleção da função na tela
 * 
 */
bool quatro = false;
/**
 * @brief Varialvel que armazena o codigo de retorno do post http
 * 
 */
int httpCodigoRetorno;
/**
 * @brief Armazena o a data e horario do ultimo http post realizado
 * 
 */
String httpHorario;
/**
 * @brief redefini as varialves um, dois, tres e quatro para false
 * 
 */
void redefinir();
/**
 * @brief evita o acesso simultaneo de recursos de internet em ambos os núcleos
 * 
 */
bool permissao = true;
/**
 * @brief Função callback para o alarme do rtc
 * 
 */
void onAlarm();
/**
 * @brief Função para atualizar o horario do rtc
 * 
 * @param wifiLigado estado da conexão wifi, true se já estiver ligado, false para a função estabelecer a conexão
 * @return Um String podendo ser "SUCESSO!" se a rotina ocorrer com sucesso, "INTERNET OFF!" caso o código de conexão com o google.com seja diferente de 200, e "WIFI OFF!" caso o wifi esteja desligado por configuração
 */
String atualizarRtc(bool wifiLigado);

/***************************************************
 * Funcoes para tratamento dos dados e configuracoes do mqtt e http
 * **************************************************
 */

/**
 * @brief Reestabelece a conexão com o servidor mqtt
 * 
 * @param config objeto contentndo os as variáveis de configuração do sistema
 */
void reconnect(Configuracao config);
/**
 * @brief Prepara as os valores de leitura das variáveis climaticas em uma string com formato json, e envia no tópico mqtt especificado no arquivo de configuração
 * 
 * @param config objeto contentndo os as variáveis de configuração do sistema
 * @param indices objeto contendo os valores das variáveis de ambiente
 */
void enviarMqtt(Configuracao config, VariaveisTermicas indices);

/***************************************************
 * VARIAVEIS PARA CONTROLAR O TEMPO EM MILISEGUNDOS NAS TAREFAS
 * **************************************************
 */

/**
 * @brief ajusta o intervalo de tela para 1 segundo afim de nao sobrecarregar a interface i2c
 * 
 */
unsigned long intervaloAtualizacaoTela = 5000;
/**
 * @brief ajusta o intervalo para salvar os dados no cartão de memória
 * 
 */
unsigned long intervaloSalvarDados = 0;
/**
 * @brief ajusta o intervalo para enviar os dados na internet
 * 
 */
unsigned long intervaloEnviarDados = 0;
/**
 * @brief ajusto o intervalo para ler os dados dos sensores
 * 
 */
unsigned long intervaloLerVariaveis = 10000;
/**
 * @brief ajusta o intervalo para piscar o led no pino 2, a fim de fornecer um feedback do funcionamento do loopRelogio
 * 
 */
unsigned long ledIndicativo = 250;
/**
 * @brief ajusta o intervalo para piscar o led no pino 15, a fim de fornecer um feedback da conexão com o wi-fi e servidor
 * 
 */
unsigned long ledIndicadorInternet = 0;
/**
 * @brief ajusta o intervalo entre as mudanças das telas
 * 
 */
unsigned long mudarTela = 0;
/***************************************************
 * CONFIGURACOES DO MODO DEEP SLEEP
 * **************************************************
 */

/**
 * @brief fator de conversão de microsegundos para segundos
 * 
 */
#define uS_TO_S_FACTOR 1000000



/***************************************************
 * TAREFAS
 * **************************************************
 */

void tarefaHttp(void * pvParameters)
{
    VariaveisTermicas variaveisTermicas = *(VariaveisTermicas*)pvParameters;

    HTTPClient http;
    //Iniciando conexão com o servidor http  
    http.begin(configuracao.httpHostname);
    // Specify content-type header
    http.addHeader("Content-Type", "application/json");
    //Preparando dados para enviar via https
    String enviar;
    StaticJsonDocument<192> doc;
    doc["producaoAnimalId"] = configuracao.cadastroConfterm;
    char horarioArquivo[9] = "hh:mm:ss";
    char dataArquivo[11] = "DD/MM/YYYY";
    doc["data"] = variaveisTermicas.horario.toString(dataArquivo);
    doc["horario"] = variaveisTermicas.horario.toString(horarioArquivo);
    doc["itu"] = variaveisTermicas.itu1;
    doc["itgu"] = variaveisTermicas.itgu1;
    doc["orvalho"] = variaveisTermicas.pontoDeOrvalho1;
    doc["tbs"] = variaveisTermicas.temperaturaDeBulboSeco;
    doc["bulboUmido"] = variaveisTermicas.temperaturaDeBulboUmido;
    doc["umidade"] = variaveisTermicas.umidadeRelativa1;
    doc["tg"] = variaveisTermicas.temperaturaDeGlobo;
    serializeJson(doc, enviar);
    //enviando o json string via post http
    httpCodigoRetorno = http.POST(enviar.c_str());
    httpHorario = String(String(variaveisTermicas.horario.toString(horarioArquivo))+' '+String(variaveisTermicas.horario.toString(dataArquivo)));
    // httpCodigoRetorno will be negative on error
    http.end();
    vTaskDelete(NULL);
}
void tarefaSalvar(VariaveisTermicas variaveisTermicas)
{ 
    if (!SD.begin())
    {
        ESP.restart();
    }

    char falha = '0';
    if(variaveisTermicas.falhaSensores)
    {
        falha = '1';
    }
    char horarioArquivo[9] = "hh:mm:ss";
    char dataArquivo[11] = "DD/MM/YYYY";

    //""data,horario,umidade1,umidade2,temperaturaDeGlobo,temperaturaBulboSeco,temperaturaBulboUmido,htu21d_Temperatura,bmp_Temperatura,itu1,itu2,itgu1,itgu2,ibutg1,ibutg2,orvalho1,orvalho2,hpa,rtcTemperatura,altitude,falha";

    String datalog = String(variaveisTermicas.horario.toString(dataArquivo)) + "," + 
        String(variaveisTermicas.horario.toString(horarioArquivo)) + "," +
        String(variaveisTermicas.umidadeRelativa1) + "," + 
        String(variaveisTermicas.umidadeRelativa2) + "," + 
        String(variaveisTermicas.temperaturaDeGlobo) + "," +
        String(variaveisTermicas.temperaturaDeBulboSeco) + "," +
        String(variaveisTermicas.temperaturaDeBulboUmido) + "," +
        String(variaveisTermicas.htu21dTemperatura) + "," +
        String(variaveisTermicas.bmpTemperatura) + "," +
        String(variaveisTermicas.itu1) + "," +
        String(variaveisTermicas.itu2) + "," +
        String(variaveisTermicas.itgu1) + "," +
        String(variaveisTermicas.itgu2) + "," +
        String(variaveisTermicas.ibutg1) + "," +
        String(variaveisTermicas.ibutg2) + "," +
        String(variaveisTermicas.pontoDeOrvalho1) + "," +
        String(variaveisTermicas.pontoDeOrvalho2) + "," +
        String(variaveisTermicas.pressao) + "," +
        String(variaveisTermicas.rtcTemperature) + "," +
        String(variaveisTermicas.altitude) + "," +
        String(falha);
    //Salva no arquivo datalog
    appendFile(SD, arquivoDatalog_txt, datalog);
    //Salva no arquivo diário
    appendFile(SD, String(variaveisTermicas.horario.toString(dataArquivosDiarios)), datalog);
    //apaga o arquivo json de maximas e minimas
    if(!SPIFFS.begin()) esp_restart();
    SD.remove(arquivoMaxMinJson);
    salvarMaxMin(variaveisTermicas, arquivoMaxMinJson); //cria um novo arquivo json de maximas e minimas
    SPIFFS.end();
    SD.end();
}
void tarefaSalvarSd(void * pvParameters)
{
    VariaveisTermicas dadosLocais = *(VariaveisTermicas*)pvParameters;
    tarefaSalvar(dadosLocais);
    vTaskDelete(NULL);

}
void tarefaEnviarMqtt(void * pvParameters)
{
    VariaveisTermicas dadosLocais = *(VariaveisTermicas*)pvParameters;
    enviarMqtt(configuracao, dadosLocais);    
    vTaskDelete(NULL);
}
void reconectarRede(void * pvParameters)
{
    
    reconnect(configuracao);
    permissao = true;
    vTaskDelete(NULL);
}
VariaveisTermicas variaveis;
void setup()
{
    ////Serial para debug
    Serial.begin(9600);
    //vTaskDelay(1000/ portTICK_PERIOD_MS);
    Wire.begin(21,22);
    pinMode(LED_VERMELHO, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(LED_AMARELO, OUTPUT);
    digitalWrite(LED_VERDE, HIGH);

    //inicializa a tela
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x64 
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setFont(&FreeMono24pt7b);
    display.clearDisplay();
    display.setCursor(5,30);
    display.print("GMAQ");
    display.setFont(&FreeMonoOblique9pt7b);
    display.setCursor(0,54);
    display.print("Version 1.2");
    display.display();


    //testa o catao de memoria tornando obrigatorio o uso
    if(!SD.begin())
    {
        display.clearDisplay();
        display.setCursor(0,11);
        display.print("SD FALHOU!\nENSIRA O CARTAO SD!");
        display.display();
        while (!SD.begin())
        {
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_VERMELHO,LOW);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(LED_VERMELHO, HIGH);
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }
    }
    // SPIFFS
    if(!SPIFFS.begin())
    {
        display.clearDisplay();
        display.setCursor(0,11);
        display.print("SPIFFS FALHOU!\nPROCURE AJUDA!");
        display.display();
        while (!SPIFFS.begin())
        {
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_VERMELHO,LOW);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(LED_VERMELHO, HIGH);
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }
    }

    //inicia e testa modulo rtc
    if(!relogio.begin())
    {
        display.clearDisplay();
        display.setCursor(0,11);
        display.print("MODULO RELOGIO\n FALHOU!\nOU ESTA\n DESCONECTADO!");
        display.display();
        while(!relogio.begin())
        {
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_VERMELHO,LOW);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(LED_VERMELHO, HIGH);
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }
    }
    
    //inicializar modulos e sensores
    globoNegro.begin();
    bulboSeco.begin();
    bulboUmido.begin();
    //htu21d.begin();

    //if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
    if (!bmp.begin(0x76))
    {
        display.clearDisplay();
        display.setCursor(0,11);
        display.print("SENSOR DE\n PRESSAO FALHOU!\nOU ESTA\n DESCONECTADO!");
        display.display();
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
        while (!bmp.begin(0x76))
        {
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(LED_VERMELHO,LOW);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(LED_VERMELHO, HIGH);
            vTaskDelay(250 / portTICK_PERIOD_MS);
        }   
    }



    /* Default settings from datasheet. sensor de pressão bmp280 */
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
    


    //Atualiza horario do aparelho e define o alarme
    DateTime tempoAtual = relogio.now();
    relogio.disable32K();
    pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), onAlarm, FALLING);
    relogio.clearAlarm(1);
    relogio.clearAlarm(2);
    relogio.writeSqwPinMode(DS3231_OFF);
    relogio.disableAlarm(2);
    zeroHora = DateTime(tempoAtual.year(),tempoAtual.month(),tempoAtual.day(),23,59,58);
    relogio.setAlarm1(zeroHora,DS3231_A1_Hour); 

    //verifica se há arquivo de configuracao no sd card
    if(SD.exists("/config.json"))
    {
        String config = "/config.json";
        if(SPIFFS.exists(arquivoDeConfiguracao)) SPIFFS.remove(arquivoDeConfiguracao);//apaga o arquivo de configuracao na memoria flash
        exportConfiration(loadConfigurationSd(config, display));
        SD.remove("/config.json");
        saveConfigurationSd();
        esp_restart();
    }else
    {
        if (!SD.exists("/exemplo.json"))
        {
            saveConfigurationSd();
            esp_restart();
        }
    }
   
    //Verifica o arquivo de configuracao, criando um modelo exemplo caso não exista e tambem reiniciando o aparelho
    if (!SPIFFS.exists(arquivoDeConfiguracao))
    {
        saveConfiguration();       
    }else
    {
        configuracao = loadConfiguration(arquivoDeConfiguracao, display);//puxa o arquivo de configuracao json no cartao de memoria
    }

    //Verifica se existe o arquivo datalog no cartao de memoria, criando-o caso contrario
    if (!SD.exists(arquivoDatalog_txt)) //verifica ou cria o arquivo datalog.txt
    {
        File file = SD.open(arquivoDatalog_txt, FILE_WRITE);
        file.close();
        appendFile(SD, arquivoDatalog_txt, datalogCabecalho);   
    }

    //Verifica se existe a pasta que armazena os arquivos diarios no sd, criando caso contrario
    if (!SD.exists(pasta)) //Verifica ou cria a pasta com os arquivos diarios
    {
        createDir(SD, pasta);   
    }

    //Salva as maximas e minimas no arquivo "/maximas_e_minimas.csv"
    if (salvarHorarioMeiaNoite)
    {
        salvarHorarioMeiaNoite = false;
        uint8_t n = 0;
        do
        {
            n++;
            bulboSeco.requestTemperatures();
            globoNegro.requestTemperatures();
            bulboUmido.requestTemperatures();
            delay(750);
            dadosLocais.globo = globoNegro.getTempCByIndex(0);
            dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
            dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
            dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, bmp, configuracao.sensorBulboUmido, relogio);
        } while (dadosLocais.falhaSensores && n <=  10);
        

        char horarioArquivo[9] = "hh:mm:ss";
        char dataArquivo[11] = "DD/MM/YYYY";
        String datalog = String(dadosLocais.horario.toString(dataArquivo)) + "," +
            String(dadosLocais.horario.toString(horarioArquivo)) + ","+
            String(dadosLocais.temperaturaMax) + "," + 
            String(dadosLocais.temperaturaMin) + "," +
            String(dadosLocais.temperaturaDeBulboUmidoMax) + "," + 
            String(dadosLocais.temperaturaDeBulboUmidoMin) + "," + 
            String(dadosLocais.umidadeMax) + "," +
            String(dadosLocais.umidadeMin) + "," +
            String(dadosLocais.globoMax) + "," +
            String(dadosLocais.globoMin) + "," +
            String(dadosLocais.ituMax) + "," +
            String(dadosLocais.ituMin) + "," +
            String(dadosLocais.itguMax) + "," +
            String(dadosLocais.itguMin) + "," +
            String(dadosLocais.orvalhoMax) + "," +
            String(dadosLocais.orvalhoMin);
        //Salvando a string no arquivo de maximas e minimas
        appendFile(SD, pasta + arquivoMaxMin, datalog);
        
        if (configuracao.internetStatus)
        {
            display.clearDisplay();
            display.setCursor(0,11);
            display.print("VERIFICANDO RTC...");
            display.display();
            display.setCursor(0,21);
            display.print(atualizarRtc(false));
            display.display();
            vTaskDelay(2000/portTICK_PERIOD_MS); 
            ESP.restart();
        }else
        {
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    }


    //Verifica se ja existe um aquivo diario com a data atualizada pelo modulo rtc,
    //criando um novo arquivo caso nao exista um arquivo do dia corrente, e tambem atualizando o arquivo de maximas e minimasa
    //Tambem cria o arquivo com as data salvas se não exitir e adiciona a data do dia corrente
    if (!SD.exists(tempoAtual.toString(dataArquivosDiarios)))
    {
        SD.remove(arquivoMaxMinJson);
        uint8_t n = 0;
        do
        {
            n++;
            bulboSeco.requestTemperatures();
            globoNegro.requestTemperatures();
            bulboUmido.requestTemperatures();
            delay(750);
            dadosLocais.globo = globoNegro.getTempCByIndex(0);
            dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
            dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
            dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, bmp, configuracao.sensorBulboUmido, relogio);
        } while (dadosLocais.falhaSensores  && n <= 10);
        salvarMaxMin(dadosLocais, arquivoMaxMinJson);

        File file = SD.open( tempoAtual.toString(dataArquivosDiarios), FILE_WRITE);
        file.close();
        appendFile(SD, tempoAtual.toString(dataArquivosDiarios), datalogCabecalho);

    }else
    {  
        // Verifica se existe o arquivo que conten o json com os valores de maximas e minimas no cartão de memória.
        if (!SD.exists(arquivoMaxMinJson))
        {
            uint8_t n = 0;
            do
            {
                n++;
                bulboSeco.requestTemperatures();
                globoNegro.requestTemperatures();
                bulboUmido.requestTemperatures();
                delay(750);
                dadosLocais.globo = globoNegro.getTempCByIndex(0);
                dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
                dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
                dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, bmp, configuracao.sensorBulboUmido, relogio);
            } while (dadosLocais.falhaSensores  && n <= 10);      
            dadosLocais.zeraMaxMin();
            salvarMaxMin(dadosLocais, arquivoMaxMinJson);
        }else
        {
           uint8_t n = 0;
            do
            {
                n++;
                bulboSeco.requestTemperatures();
                globoNegro.requestTemperatures();
                bulboUmido.requestTemperatures();
                delay(750);
                dadosLocais.globo = globoNegro.getTempCByIndex(0);
                dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
                dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
                dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, bmp, configuracao.sensorBulboUmido, relogio);
            } while (dadosLocais.falhaSensores  && n <= 10);
            lerMaxMin(dadosLocais, arquivoMaxMinJson);   
        }
    }

    //cria ou verifica se ja existe o arquivo de maximas e minimas em txt no sd
    if (!SD.exists(pasta + arquivoMaxMin))
    {
        File file = SD.open(pasta + arquivoMaxMin, FILE_WRITE);
        file.close();
        appendFile(SD, pasta + arquivoMaxMin, maxMinCabecalho); 
    }


    //se as variaves internetStatus e mqttStatus estiverem true no aquivo de configuracao, o wifi será ligado no setup
    if (configuracao.mqttStatus && configuracao.internetStatus) internetAtiva = true;

    //Testa o rtc para verificar o horario
    if (dadosLocais.horario.year()==2000 || dadosLocais.erroRtc)
    {
        display.clearDisplay();
        display.setCursor(0,11);
        display.print("SINCRONIZANDO\n HORARIO!");
        display.display();
        display.setCursor(0,21);
        if (!configuracoesNoBoot && internetAtiva)
        {
            display.print(atualizarRtc(false));
            display.display();
            vTaskDelay(2000/portTICK_PERIOD_MS); 
            ESP.restart();
        }
    }

    //Inicia a conexao com internet se o usuario declarar internetStatus e mqttStatus como true no arquivo de configuracao
    if (!configuracoesNoBoot && internetAtiva)
    {   
        display.clearDisplay();
        display.setCursor(0,11);
        display.print("CONECTANDO A REDE\n WI-FI!");
        display.display();
        WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
        WiFi.mode(WIFI_STA); //init wifi mode

        if(configuracao.eduroamStatus)
        {
            esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)configuracao.eduroanLogin, strlen(configuracao.eduroanLogin)); //provide identity
            esp_wifi_sta_wpa2_ent_set_username((uint8_t *)configuracao.eduroanLogin, strlen(configuracao.eduroanLogin)); //provide username --> identity and username is same
            esp_wifi_sta_wpa2_ent_set_password((uint8_t *)configuracao.eduroanSenha, strlen(configuracao.eduroanSenha)); //provide password
            esp_wpa2_config_t wifiMode = WPA2_CONFIG_INIT_DEFAULT();
            esp_wifi_sta_wpa2_ent_enable(&wifiMode);
            WiFi.begin(configuracao.wifiSsid);
            vTaskDelay(5000/ portTICK_PERIOD_MS);
        }else
        {
            WiFi.begin(configuracao.wifiSsid, configuracao.wifiSenha);
            vTaskDelay(5000/ portTICK_PERIOD_MS);
        }
        
        //INICIA O LOOP DO SERVICO MQTT no core 0
        client.setServer(configuracao.mqttHostname, configuracao.mqttPort);
        client.setCallback(callback);
        if(client.connect(configuracao.mqttName,configuracao.mqttUser, configuracao.mqttSenha))
        {
            //Para reduzir o consumo de recursos, a função de se inscrever em tópicos mqtt foi desabilitada
            //client.subscribe(configuracao.mqttTopicoSub);
            internetEstado = true;
            //xTaskCreatePinnedToCore(loopMqtt, "loopMqtt", 10000, (void*)&configuracao, 1, &loop_mqtt, 0);
            //delay(500);
        }          
    }
    SPIFFS.end();
    SD.end();

    //Ativa a função de interrupção quando o botão de interação é prescionado
    if (!configuracoesNoBoot)
    {    
        attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), desativaLoopRelogio, RISING);
    }

    if(configuracoesNoBoot)
    {
        xTaskCreatePinnedToCore(loopBotao, "loopBotao", 4000, NULL, 4, &loop_configuracao,0);
        xTaskCreatePinnedToCore(loopConfiguracao, "loopConfiguracao", 20000, NULL, 5, &loop_configuracao,1);
        configuracoesNoBoot = false;
        delay(240000);
    }

    //Configurações de pwm do pino 15
        //Configura o canal 0 com frequência de 312500 Hz
        sigmaDeltaSetup(0, 312500);
        //Anexa o pino 18 (GPIO18) ao canal 0
        sigmaDeltaAttachPin(15,0);
        //inicializa o canal 0 para desativado
        sigmaDeltaWrite(0, 0);
}
    
void loop()
{
    //Cria uma varial que gerencia o controle de tempo no loop
    unsigned long currentMillis = millis();
    unsigned long diferenca;
    /**
     * @brief Faz a leitura dos sensores e cauculo dos índices a cada 10 segundos
     * 
     */
    diferenca = currentMillis - intervaloLerVariaveis;
    if ( diferenca >= long(10000))
    {   uint8_t n = 0;
        do
        {
            n++;
            bulboSeco.requestTemperatures();
            bulboSeco.requestTemperatures();
            globoNegro.requestTemperatures();
            bulboUmido.requestTemperatures();
            dadosLocais.globo = globoNegro.getTempCByIndex(0);
            dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
            dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
            dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, bmp, configuracao.sensorBulboUmido, relogio);
        } while (dadosLocais.falhaSensores  && n <= 10);
        
        intervaloLerVariaveis = currentMillis;
       
    } 
    /**
     * @brief Salva os dados no cartão de memoria no intervalo de tempo especificado no arquivo de configuração
     * 
     */
    diferenca = currentMillis - intervaloSalvarDados;
    if ( diferenca >= long(configuracao.intervaloSalvar*1000))
    {
        xTaskCreatePinnedToCore(tarefaSalvarSd, "tarefaSalvar", 10000, (void*)&dadosLocais, 4, &tarefa_salvar,0);
        intervaloSalvarDados = currentMillis;
    }
    /**
     * @brief envia os dados para o servidor no intervalo de tempo especificado no arquivo de configuração
     * 
     */
    diferenca = currentMillis - intervaloEnviarDados;
    if (internetEstado && diferenca >= long(configuracao.intervaloOnline*1000))
    {
        //MQTT
        if (internetEstado)
        {
            xTaskCreatePinnedToCore(tarefaEnviarMqtt, "tarefaEnviarMqtt", 10000, (void*)&dadosLocais, 2, &tarefa_salvar,0);
        }
        intervaloEnviarDados = currentMillis;
    }

    /**
     * @brief cria as telas para visualização do feedback do dispositivo
     * sendo duas telas
     *  a --> valores das leituras, status de conexão de wi-fi e servidor, e status do rtc
     *  b --> Feedback das falhas dos sensores
     */
    if(currentMillis - intervaloAtualizacaoTela >= long(1000))
    {           
        DateTime tempoAtual;
         
        tempoAtual = relogio.now();

        if (variaveis.falhaSensores)
        {
            alternar = 'b';
        }else if((currentMillis - mudarTela) >= long(5000))
        {
            switch (alternar)
            {
            case 'a':
                alternar = 'c';
                break;
            case 'c':
                alternar = 'd';
                break;
            case 'd':
                alternar = 'e';
                break;
            case 'e':
                alternar = 'f';
                break;
            default:
                alternar = 'a';
                break;
            }
            mudarTela = currentMillis;
        }

        // TELA A  
        if(alternar == 'a')
        {           
            display.clearDisplay();
            display.setFont(&FreeMono9pt7b);
            display.setCursor(0,11);
            display.printf("BS %.0f BU %.0f \n   GN %.0f"
                    ,dadosLocais.temperaturaDeBulboSeco, dadosLocais.temperaturaDeBulboUmido
                    , dadosLocais.temperaturaDeGlobo);
            display.setCursor(0,50);
            char dataArquivo[12] = " DD:MM:YYYY";
            display.print(tempoAtual.toString(dataArquivo));
            display.setCursor(0,63);
            char horarioArquivo[11] = "  hh:mm:ss";
            display.print(tempoAtual.toString(horarioArquivo));
            display.display();   
        }
        // TELA c  
        else if(alternar == 'c')
        {           
            display.clearDisplay();
            display.setFont(&FreeMono9pt7b);
            display.setCursor(0,11);
            display.printf("  U1 %.0f%%\n  U2 %.0f%%\n  hPa %.0f"
                    ,dadosLocais.umidadeRelativa1, dadosLocais.umidadeRelativa2
                    , dadosLocais.pressao);
            display.setCursor(0,63);
            char horarioArquivo[11] = "  hh:mm:ss";
            display.print(tempoAtual.toString(horarioArquivo));
            display.display();  
        }
        // TELA D  
        else if(alternar == 'd')
        {           
            display.clearDisplay();
            display.setFont(&FreeMono9pt7b);
            display.setCursor(0,11);
            display.printf("  ITU1 %.0f\n  ITU2 %.0f\n  IBUTG1 %.0f"
                    ,dadosLocais.itu1, dadosLocais.itu2
                    , dadosLocais.ibutg1);
            display.setCursor(0,63);
            char horarioArquivo[11] = "  hh:mm:ss";
            display.print(tempoAtual.toString(horarioArquivo));
            display.display();  
        }
        // TELA E  
        else if(alternar == 'e')
        {           
            display.clearDisplay();
            display.setFont(&FreeMono9pt7b);
            display.setCursor(0,11);
            display.printf("  ITGU1 %.0f\n  ITGU2 %.0f\n  IBUTG2 %.0f"
                    ,dadosLocais.itgu1, dadosLocais.itgu2
                    , dadosLocais.ibutg2);
            display.setCursor(0,63);
            char horarioArquivo[11] = "  hh:mm:ss";
            display.print(tempoAtual.toString(horarioArquivo));
            display.display();  
        }
        // TELA F  
        else if(alternar == 'f')
        {           
            display.clearDisplay();
            display.setFont(&FreeMono9pt7b);
            display.setCursor(12,11);
            if (WiFi.status()==WL_CONNECTED)
            {
                display.print(WiFi.localIP());
                display.setCursor(0,41);
                if (internetEstado == true)
                {
                    display.print("SERVIDOR ON!");
                }else
                {    
                    display.print("SERVIDOR OFF");
                }
            }else
            {
                if (internetAtiva)
                {
                    display.print("CONECTANDO\n  NA REDE...");
                }else
                {                
                    display.print("   WI-FI\n DESLIGADO! ");
                }
            }
            display.setCursor(0,63);
            char horarioArquivo[11] = "  hh:mm:ss";
            display.print(tempoAtual.toString(horarioArquivo));
            display.display();    
        }
        //TELA B
        else if(alternar == 'b')
        {   
            display.clearDisplay();
            display.setFont(NULL);
            display.setCursor(0,1);
            display.print("*FALHAS DE SENSORES*");
            display.setCursor(0,11);
            display.print("SENSOR PRESSAO");
            display.setCursor(0,21);
            display.print("Bulbo Seco ");
            display.setCursor(0,31);
            display.print("Bulbo Umido ");
            display.setCursor(0,41);
            display.print("Globo Negro ");
            display.setCursor(0,51);
            display.print("htu21d");
            display.setCursor(84,11);
            if (variaveis.erroSensorPressao) 
            {
                display.printf("FALHOU!");
            }else
            {
                display.printf("- OK");
            }
            display.setCursor(84,21);
            if (variaveis.erroSensorTbs) 
            {
                display.printf("FALHOU!");
            }else
            {
                display.printf("- OK");
            }
            display.setCursor(84,31);
            if (variaveis.erroSensorUmidade) 
            {
                display.printf("FALHOU!");
            }else
            {
                display.printf("- OK");
            }
            display.setCursor(84,41);
            if (variaveis.erroSensorGlobo) 
            {
                display.printf("FALHOU!");
            }else
            {
                display.printf("- OK");
            }
            display.setCursor(84,51);
            if (variaveis.erroSensorUmidade2) 
            {
                display.printf("FALHOU!");
            }else
            {
                display.printf("- OK");
            }
            display.display();
        } 
        intervaloAtualizacaoTela = currentMillis; 
    }

    //FICA PISCANDO O LED PARA MOSTRAR A ATIVIDADE DO LOOP_RELOGIO
    if ((currentMillis - ledIndicativo) >= long(250)) 
    {
        if(digitalRead(LED_VERDE) == HIGH)
        {
            digitalWrite(LED_VERDE, LOW);
        }else
        {
            digitalWrite(LED_VERDE, HIGH);
        }
        ledIndicativo = currentMillis;
    }

    if (internetEstado)
    {
        if ((currentMillis - ledIndicadorInternet) >= long(250)) 
        {
            if(digitalRead(LED_AMARELO) == HIGH)
            {
                sigmaDeltaWrite(0, 0);
            }else
            {
                sigmaDeltaWrite(0, 255);
            }           
            ledIndicadorInternet = currentMillis;
        }
    }

    if (internetAtiva && permissao)
    {
        if (!client.connected())
        {
            internetEstado = false;
            xTaskCreatePinnedToCore(reconectarRede, "reconectarRede", 10000, NULL, 3, &reconectar_internet,0);
            permissao = false;
        }else
        {
            client.loop();
        }
    }
    
    

}
void loopBotao(void * pvParameters)
{
    //INICIANDO CONFIGURACAO DO BOTAO
    button.setClickHandler(click);
    //button.setLongClickHandler(longClick);
    button.setDoubleClickHandler(doubleClick);
    for(;;)
    {
        button.loop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
void loopConfiguracao(void * pvParameters)
{
    for(;;)
    {
        if (millis() >= long(60000))
        {
            esp_restart();
        }
        
        display.clearDisplay();
        display.setFont(NULL);
        display.setCursor(0,1);
        display.setTextSize(2);
        display.print("  SETTINGS  ");
        display.setTextSize(1);
        display.setCursor(0,20);
        if (indice == 0)
        {
            display.print("-> AJUSTE DO RELOGIO");
        }else
        {
            display.print("   AJUSTE DO RELOGIO");
        }
        display.setCursor(0,30);
        if (indice == 1)
        {
            if (configuracao.mqttStatus)
            {
                display.print("-> Desligar Internet!");
            }else
            {
                display.print("-> Ligar Internet!");
            }
        }else
        {
            if (configuracao.mqttStatus)
            {
                display.print("   Desligar Internet!");
            }else
            {
                display.print("   Ligar Internet!");
            }
        }
        display.setCursor(0,40);
        if (indice == 2)
        {
            display.print("-> MUDAR ITU");
        }else
        {
            display.print("   MUDAR ITU");
        }
        display.display();
        //reconhece o duplo clique e altera as variaveis booleanas para ativar funcoes
        if(ativarFuncao)
        {
            switch (indice)
            {
            case 0:
                um = true;
                break;
            case 1:
                dois = true;
                break; 
            case 2:
                tres = true;
                break;
            }
        }
        if(indice > 2) indice = 0;

        if(um)
        {   
            redefinir();
            display.clearDisplay();
            display.setCursor(0,11);
            display.print("AGUARDE...");
            display.display();
            
            display.setCursor(0,31);
            display.print(atualizarRtc(false));
            display.display();
            vTaskDelay(5000/portTICK_PERIOD_MS);
            esp_restart();
        }
        //abre a interface de configuracao criando uma rede wifi
        if (dois)
        {
            display.clearDisplay();
            display.setCursor(0,11);
            display.print("Salvando...");
            display.display();
            SPIFFS.begin();
            if (configuracao.mqttStatus)
            {
                configuracao.mqttStatus = false;
                SPIFFS.remove(arquivoDeConfiguracao);
                exportConfiration(configuracao);
                
            }else
            {
                configuracao.mqttStatus = true;
                SPIFFS.remove(arquivoDeConfiguracao);
                exportConfiration(configuracao);
            }
            display.setCursor(0,21);
            display.print("SUCESSO!\nREINICIANDO...");
            display.display();
            SPIFFS.end();
            vTaskDelay(1000 / portTICK_PERIOD_MS ); 
            esp_restart();
            
        }
        if (tres)
        {
            redefinir();
            while(true)
            {
                if (millis() >= long(60000))
                {
                    esp_restart();
                }
                
                    display.clearDisplay();
                    display.setCursor(0,11);
                    if (indice == 0)
                    {
                        display.print("-> ITU GERAL");
                    }else
                    {
                        display.print("   ITU GERAL");
                    }
                    display.setCursor(0,21);
                    if(indice == 1)
                    {
                        display.print("-> ITU BOVINO");
                    }else
                    {
                        display.print("   ITU BOVINO");
                    }
                    display.setCursor(0,31);
                    if(indice == 2)
                    {
                        display.print("-> ITU PARA FRANGO");
                    }else
                    {
                        display.print("   ITU PARA FRANGO");
                    }
                    display.setCursor(0,41);
                    if(indice == 3)
                    {
                        display.print("-> ITU PARA COELHO");
                    }else
                    {
                        display.print("   ITU PARA COELHO");
                    }
                    display.display();
                    if(indice > 4) indice = 0;
                    if(ativarFuncao)
                    {
                        switch (indice)
                        {
                            case 0:
                                um = true;
                                break;
                            case 1:
                                dois = true;
                                break;
                            case 2:
                                tres = true;
                                break;
                            case 3:
                                quatro = true;
                                break; 
                        }
                    }
                    if(um)
                    {
                        SPIFFS.begin();
                        display.clearDisplay();
                        display.setCursor(0,11);
                        display.print("Salvando...");
                        display.display();
                        
                        configuracao.tipoAnimal = geral;
                        SPIFFS.remove(arquivoDeConfiguracao);
                        exportConfiration(configuracao); 

                        display.setCursor(0,21);
                        display.print("SUCESSO!\nREINICIANDO...");
                        display.display();
                        SPIFFS.end();
                        vTaskDelay(1000 / portTICK_PERIOD_MS ); 
                        esp_restart();
                    }
                    if(dois)
                    {
                        SPIFFS.begin();
                        display.clearDisplay();
                        display.setCursor(0,11);
                        display.print("Salvando...");
                        display.display();
                        
                        configuracao.tipoAnimal = bovino;
                        SPIFFS.remove(arquivoDeConfiguracao);
                        exportConfiration(configuracao); 

                        display.setCursor(0,21);
                        display.print("SUCESSO!\nREINICIANDO...");
                        display.display();
                        SPIFFS.end();
                        vTaskDelay(1000 / portTICK_PERIOD_MS ); 
                        esp_restart();
                    }
                    if(tres)
                    {
                        SPIFFS.begin();
                        display.clearDisplay();
                        display.setCursor(0,11);
                        display.print("INDISPONIVEL");//"Salvando...");
                        display.display();
                        /*
                        configuracao.tipoAnimal = frango;
                        SPIFFS.remove(arquivoDeConfiguracao);
                        exportConfiration(configuracao); 

                        display.setCursor(0,10);
                        display.print("SUCESSO!\nREINICIANDO...");
                        display.display();
                        */
                       SPIFFS.end();
                        vTaskDelay(1000 / portTICK_PERIOD_MS ); 
                        esp_restart();
                    }
                    if(quatro)
                    {
                        SPIFFS.begin();
                        display.clearDisplay();
                        display.setCursor(0,11);
                        display.print("INDISPONIVEL");//"Salvando...");
                        display.display();
                        /*
                        configuracao.tipoAnimal = coelho;
                        SPIFFS.remove(arquivoDeConfiguracao);
                        exportConfiration(configuracao); 

                        display.setCursor(0,10);
                        display.print("SUCESSO!\nREINICIANDO...");
                        display.display();
                        */
                        SPIFFS.end();
                        vTaskDelay(1000 / portTICK_PERIOD_MS ); 
                        esp_restart();
                    }
                vTaskDelay(10/portTICK_PERIOD_MS);
            }
        }
        if (quatro)
        {
            redefinir();
            display.clearDisplay();
            display.setCursor(0,0);
            display.print("Funcao ainda em\ndesenvolvimento!");
            display.display();
            vTaskDelay(1000/ portTICK_PERIOD_MS);
        } 
        vTaskDelay(10/ portTICK_PERIOD_MS);
    }
}

/***************************************************
 * IMPLEMENTACAO DAS FUNCOES DECLARADAS ACIMA
 * **************************************************
 */


void desativaLoopRelogio()
{
    configuracoesNoBoot = true;
    esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR);
    esp_deep_sleep_start(); //força o ESP32 entrar em modo SLEEP por um segundo
}
void reconnect(Configuracao config)
{
    while(WiFi.status() != WL_CONNECTED) 
    {
        WiFi.reconnect();
        sigmaDeltaWrite(0, 255);
        vTaskDelay(10000/ portTICK_PERIOD_MS);
    }
    
    while (!client.connected())
    {
        client.setServer(configuracao.mqttHostname, configuracao.mqttPort);
        client.setCallback(callback);

        if (client.connect(config.mqttName,config.mqttUser, config.mqttSenha))
        {
            internetEstado = true;
        } else
        {
            internetEstado = false;
            vTaskDelay(5000/ portTICK_PERIOD_MS);
        }
    }
}

void enviarMqtt(Configuracao config, VariaveisTermicas indices)
{
    if(config.plataforma == thingspeak)
    {
        String enviar = String("field1="+String(indices.temperaturaDeBulboSeco) +
                        "&field2="+String(indices.temperaturaDeBulboUmido)+
                        "&field3="+String(indices.temperaturaDeGlobo)+
                        "&field4="+String(indices.pressao)+
                        "&field5="+String(indices.umidadeRelativa1)+
                        "&field6="+String(indices.itu1)+
                        "&field7="+String(indices.itgu1)+
                        "&field8="+String(indices.falhaSensores)+"&status=MQTTPUBLISH");
        client.publish(String("channels/"+ String(config.mqttTopico) +"/publish").c_str(), enviar.c_str());
    }else if(config.plataforma == Original)
    {
        String enviar;
        StaticJsonDocument<192> doc;
        doc["temperatura"] = indices.temperaturaDeBulboSeco;
        doc["bulboUmido"] = indices.temperaturaDeBulboUmido;
        doc["umidade"] = indices.umidadeRelativa1;
        doc["globoNegro"] = indices.temperaturaDeGlobo;
        doc["itu"] = indices.itu1;
        doc["itgu"] = indices.itgu1;
        doc["orvalho"] = indices.pontoDeOrvalho1;
        doc["animal"] = config.tipoAnimal;
        doc["equipamento"] = config.mqttName;
        doc["erroBulboSeco"] = indices.erroSensorTbs;
        doc["erroBulboUmido"] = indices.erroSensorUmidade;
        doc["erroGloboNegro"] = indices.erroSensorGlobo;

        serializeJson(doc, enviar);
        client.publish(config.mqttTopico, enviar.c_str());
    }else if(config.plataforma == ubidots)
    {
        String enviar;
        StaticJsonDocument<384> doc;

        doc["timestamp"] = indices.horario.timestamp();
        doc["umidade1"] = indices.umidadeRelativa1;
        doc["umidade2"] = indices.umidadeRelativa2;
        doc["globo"] = indices.temperaturaDeGlobo;
        doc["tbs"] = indices.temperaturaDeBulboSeco;
        doc["tbu"] = indices.temperaturaDeBulboUmido;
        doc["htu21dT"] = indices.htu21dTemperatura;
        doc["bmpT"] = indices.bmpTemperatura;
        doc["itu1"] = indices.itu1;
        doc["itu2"] = indices.itu2;
        doc["itgu1"] = indices.itgu1;
        doc["itgu2"] = indices.itgu2;
        doc["ibutg1"] = indices.ibutg1;
        doc["ibutg2"] = indices.ibutg2;
        doc["orvalho1"] = indices.pontoDeOrvalho1;
        doc["orvalho2"] = indices.pontoDeOrvalho2;
        doc["hpa"] = indices.pressao;
        doc["rtcT"] = indices.rtcTemperature;
        doc["altitude"] = indices.altitude;
        doc["falha"] = indices.falhaSensores;

        serializeJson(doc, enviar);
        client.publish(config.mqttTopico, enviar.c_str());
    }
}
/////////////////////*******lOGICA DO BOTAO********//////////////////////////////

void click(Button2& btn) 
{   
    indice += 1;
}
void doubleClick(Button2& btn) 
{
    ativarFuncao = true;        
}
void redefinir()
{
    indice = 0;
    ativarFuncao = false;
    um = false;
    dois = false;
    tres = false;
    quatro = false;
}
void callback(char* topic, byte* payload, unsigned int length)
{
//Instrução para enviar arquivos via mqtt no tópico subscribe
//mas para economizar recurso de rede, foram descontinuadas
/*
    String c;
    for (int i=0;i<length;i++)
    {
        c += (char)payload[i];
    }
    client.publish(configuracao.mqttTopicoPublishArquivos, c.c_str());

    if (c.operator==("datas"))
    {
        File arquivo = SD.open(arquivodiasSalvos, "r");
        if (arquivo)
        {
            client.publish(configuracao.mqttTopicoPublishArquivos, "ENVIANDO DATAS!");
            while (arquivo.available())
            {
                client.publish(configuracao.mqttTopicoPublishArquivos, arquivo.readStringUntil('\r').c_str());
            }
            client.publish(configuracao.mqttTopicoPublishArquivos, "FIM DO ARQUIVO!");
            arquivo.close();            
        }else
        {
            client.publish(configuracao.mqttTopicoPublishArquivos, "FALHOU AO TENTAR LER O ARQUIVO!");
        }
        
    }else if (c.operator==("datalog"))
    {
        File arquivo;
        arquivo = SD.open(arquivoDatalog_txt, "r");
        if (arquivo)
        {
            client.publish(configuracao.mqttTopicoPublishArquivos, "ENVIANDO DADOS!");
            while (arquivo.available())
            {
                client.publish(configuracao.mqttTopicoPublishArquivos, arquivo.readStringUntil('\r').c_str());
            }
            client.publish(configuracao.mqttTopicoPublishArquivos, "FIM DO ARQUIVO!");
            arquivo.close();            
        }else
        {
            client.publish(configuracao.mqttTopicoPublishArquivos, "FALHOU AO TENTAR LER O ARQUIVO!");
        }
    }else if (c.operator==("maximas"))
    {
        File arquivo;
        arquivo = SD.open("/arquivos/maximas_e_minimas.csv", "r");
        if (arquivo)
        {
            client.publish(configuracao.mqttTopicoPublishArquivos, "ENVIANDO MAXIMAS E MINIMAS!");
            while (arquivo.available())
            {
                client.publish(configuracao.mqttTopicoPublishArquivos, arquivo.readStringUntil('\r').c_str());
            }
            client.publish(configuracao.mqttTopicoPublishArquivos, "FIM DO ARQUIVO!");
            arquivo.close();            
        }else
        {
            client.publish(configuracao.mqttTopicoPublishArquivos, "FALHOU AO TENTAR LER O ARQUIVO!");
        }
    }else
    {
        if(SD.exists(pasta + "/" + c + ".csv"))
        {
            File arquivo;
            arquivo = SD.open(pasta + "/" + c + ".csv", "r");
            if (arquivo)
            {
                client.publish(configuracao.mqttTopicoPublishArquivos, "ENVIANDO ARQUIVO!");
                while (arquivo.available())
                {
                    client.publish(configuracao.mqttTopicoPublishArquivos, arquivo.readStringUntil('\r').c_str());
                }
                client.publish(configuracao.mqttTopicoPublishArquivos, "FIM DO ARQUIVO!");
                arquivo.close();            
            }else
            {
                client.publish(configuracao.mqttTopicoPublishArquivos, "FALHOU AO TENTAR LER O ARQUIVO!");
            }
        }else
        {
            client.publish(configuracao.mqttTopicoPublishArquivos, "O ARQUIVO NÃO EXISTE!");
        }
    }
*/
}
void onAlarm()
{        
    salvarHorarioMeiaNoite = true;
    
    esp_sleep_enable_timer_wakeup(uS_TO_S_FACTOR);
    esp_deep_sleep_start(); //força o ESP32 entrar em modo SLEEP por um segundo
}
String atualizarRtc(bool wifiLigado)
{
    if (configuracao.internetStatus)
    {
        if (!wifiLigado)
        {
            WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
            WiFi.mode(WIFI_STA); //init wifi mode
            if(configuracao.eduroamStatus)
            {
                esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)configuracao.eduroanLogin, strlen(configuracao.eduroanLogin)); //provide identity
                esp_wifi_sta_wpa2_ent_set_username((uint8_t *)configuracao.eduroanLogin, strlen(configuracao.eduroanLogin)); //provide username --> identity and username is same
                esp_wifi_sta_wpa2_ent_set_password((uint8_t *)configuracao.eduroanSenha, strlen(configuracao.eduroanSenha)); //provide password
                esp_wpa2_config_t wifiMode = WPA2_CONFIG_INIT_DEFAULT();
                esp_wifi_sta_wpa2_ent_enable(&wifiMode);
                WiFi.begin(configuracao.wifiSsid);
                vTaskDelay(5000/ portTICK_PERIOD_MS);
            }else
            {
                WiFi.begin(configuracao.wifiSsid, configuracao.wifiSenha);
                vTaskDelay(5000/ portTICK_PERIOD_MS);
            }
            if (WiFi.status()!= WL_CONNECTED)
            {
                do
                {
                    WiFi.reconnect();
                    vTaskDelay(5000/ portTICK_PERIOD_MS);
                } while (WiFi.status()==WL_CONNECTED);   
            }
        }
        if (WiFi.status()==WL_CONNECTED)
        {
            //Ajusta a hora do rtc pelo ntp
            //Verificando se há conexão com a internet para atualizar o relógio
            HTTPClient http;
            http.begin(configuracao.hostTest); 
            int httpCode = http.GET();
            if(httpCode == HTTP_CODE_OK)//ok, há internet!
            {
                http.end();
                //Iniciando a atualização do relógio rtc
                timeClient.setTimeOffset(configuracao.timeZone*3600);
                timeClient.begin();
                vTaskDelay(1000/ portTICK_PERIOD_MS);
                timeClient.update();
                relogio.adjust(DateTime(timeClient.getEpochTime()));
                
                return String("SUCESSO!");
            }else //Abortando a atualização do relógio e reiniciando a placa
            {
                http.end();
                return String("INTERNET OFF!");
            }                    
        }
    }else
    {
        //O aparelho está operando em modo offline, sem internet para atualizar ou enviar dados
        return String("WIFI OFF!");                
    }
}