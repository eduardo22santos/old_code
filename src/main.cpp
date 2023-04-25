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
#include <Adafruit_I2CDevice.h>
#include <Adafruit_BMP280.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <esp_sleep.h>
#define ANEMOMETRO_PIN 26 //Indica qual porta esta sendo utilizada para o botao de interacao
#define LED_VERMELHO 2 // Led indicador de leitura incorreta
#define LED_VERDE 32 //INDICA O ESTADO DA PLACA
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
 * @brief Variável salva na memória ram do rtc interno, a qual indica se a reinicialização do sistema foi realizada meia noite
 * 
 */
RTC_DATA_ATTR bool salvarHorarioMeiaNoite = false;
//É um indicativo para a placa entrar em modo sleep quando está ociosa, quando está em modo offline
/**
 * @brief Variável salva na memória ram do rtc interno, a qual indica para a placa salvar o dado no cartão de memória ainda na função setup()
 * 
 */
RTC_DATA_ATTR bool salvarDado = false;
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
void atualizarRtc(bool wifiLigado);

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
/***************************************************
 * TAREFAS
 * **************************************************
 */
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

String logErro(VariaveisTermicas variaveis)
{   
    String status;
    if (variaveis.erroSensorGlobo)
    {
        status = "erro globo";
        return status;
    }else if (variaveis.erroSensorPressao)
    {
        status = "erro pressao";
        return status;
    }else if (variaveis.erroSensorTbs)
    {
        status = "erro tbs";
        return status;
    }else if (variaveis.erroSensorUmidade)
    {
        status = "erro tbu";
        return status;
    }else if (variaveis.erroRtc)
    {
        status = "erro rtc";
        return status;
    }else
    {
        status = "tudo funcionando";
        return status;
    }
    
    
    
}
void setup()
{
    ////Serial para debug
    Serial.begin(9600);
    Wire.begin(21,22);
    pinMode(LED_VERMELHO, OUTPUT);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_AMARELO, OUTPUT);

    //CONFIGURAÇÕES DA PORTA 15 PARA PISCAR O LED
    //POR SER UMA PORTA PWM, É NECESSÁRIO CONFIGURAÇÕES ADICIONAIS
    // configure LED PWM functionalitites
    ledcSetup(0, 5000, 8);
    // attach the channel to the GPIO to be controlled
    ledcAttachPin(LED_AMARELO, 0);


    digitalWrite(LED_VERDE, HIGH);

    //testa o catao de memoria tornando obrigatorio o uso
    if(!SD.begin())
    {
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

    //inicia e testa modulo rtc
    if(!relogio.begin())
    {
        
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
    htu21d.begin();
    if (!bmp.begin(0x76))
    {
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

    //se as variaves internetStatus e mqttStatus estiverem true no aquivo de configuracao, o wifi será ligado no setup
    if (configuracao.mqttStatus && configuracao.internetStatus) internetAtiva = true; 
    
    //Testa o rtc para verificar o horario
    DateTime verificaRtc = relogio.now();
    if (!verificaRtc.isValid() || verificaRtc.year()==2000)
    {
        atualizarRtc(false).c_str();
         
        ESP.restart();
    }

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
        for (size_t i = 0; i < 5; i++)
        {
            bulboSeco.requestTemperatures();
            globoNegro.requestTemperatures();
            bulboUmido.requestTemperatures();
            delay(750);
            dadosLocais.globo = globoNegro.getTempCByIndex(0);
            dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
            dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
            dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, htu21d, bmp, configuracao.sensorBulboUmido, relogio);
        }

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
            atualizarRtc(false);
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
        if (SD.exists(arquivoMaxMinJson))
        {
            SD.remove(arquivoMaxMinJson);
        }
        for (size_t i = 0; i < 5; i++)
        {
            bulboSeco.requestTemperatures();
            globoNegro.requestTemperatures();
            bulboUmido.requestTemperatures();
            delay(750);
            dadosLocais.globo = globoNegro.getTempCByIndex(0);
            dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
            dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
            dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, htu21d, bmp, configuracao.sensorBulboUmido, relogio);
        }
        salvarMaxMin(dadosLocais, arquivoMaxMinJson);

        File file = SD.open( tempoAtual.toString(dataArquivosDiarios), FILE_WRITE);
        file.close();
        appendFile(SD, tempoAtual.toString(dataArquivosDiarios), datalogCabecalho);

    }else
    {  
        // Verifica se existe o arquivo que conten o json com os valores de maximas e minimas no cartão de memória.
        if (!SD.exists(arquivoMaxMinJson))
        {
            for (size_t i = 0; i < 5; i++)
            {
                bulboSeco.requestTemperatures();
                globoNegro.requestTemperatures();
                bulboUmido.requestTemperatures();
                delay(750);
                dadosLocais.globo = globoNegro.getTempCByIndex(0);
                dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
                dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
                dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, htu21d, bmp, configuracao.sensorBulboUmido, relogio);
            }  
            dadosLocais.zeraMaxMin();
            salvarMaxMin(dadosLocais, arquivoMaxMinJson);
        }else
        {
          for (size_t i = 0; i < 5; i++)
        {
            bulboSeco.requestTemperatures();
            globoNegro.requestTemperatures();
            bulboUmido.requestTemperatures();
            delay(750);
            dadosLocais.globo = globoNegro.getTempCByIndex(0);
            dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
            dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
            dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, htu21d, bmp, configuracao.sensorBulboUmido, relogio);
        }
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

    //Inicia a conexao com internet se o usuario declarar internetStatus e mqttStatus como true no arquivo de configuracao
    if (internetAtiva)
    {   
        WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
        WiFi.mode(WIFI_STA); //init wifi mode

        if(configuracao.eduroamStatus)
        {
            esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)configuracao.eduroanLogin, strlen(configuracao.eduroanLogin)); //provide identity
            esp_wifi_sta_wpa2_ent_set_username((uint8_t *)configuracao.eduroanLogin, strlen(configuracao.eduroanLogin)); //provide username --> identity and username is same
            esp_wifi_sta_wpa2_ent_set_password((uint8_t *)configuracao.eduroanSenha, strlen(configuracao.eduroanSenha)); //provide password
            //esp_wpa2_config_t wifiMode = WPA2_CONFIG_INIT_DEFAULT();
            //esp_wifi_sta_wpa2_ent_enable(&wifiMode);
            esp_wifi_sta_wpa2_ent_enable();
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
    SD.end();
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
    {   
            bulboSeco.requestTemperatures();
            bulboSeco.requestTemperatures();
            globoNegro.requestTemperatures();
            bulboUmido.requestTemperatures();
            dadosLocais.globo = globoNegro.getTempCByIndex(0);
            dadosLocais.tbs = bulboSeco.getTempCByIndex(0);
            dadosLocais.tbu = bulboUmido.getTempCByIndex(0);
            dadosLocais.atualizaVariaveis(configuracao.tipoAnimal, LED_VERMELHO, htu21d, bmp, configuracao.sensorBulboUmido, relogio);
        
        
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
                ledcWrite(0, 0);
            }else
            {
                ledcWrite(0, 255);
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


/***************************************************
 * IMPLEMENTACAO DAS FUNCOES DECLARADAS ACIMA
 * **************************************************
 */
void reconnect(Configuracao config)
{
    unsigned long reiniciarWifi = millis();
    
    while(true) 
    {
        //testa a conexão wifi e com o servidor mqtt
        if (WiFi.status() != WL_CONNECTED)
        {
            WiFi.reconnect();
            sigmaDeltaWrite(0, 255);
            vTaskDelay(10000/ portTICK_PERIOD_MS);
        }else if (!client.connected())
        {
            client.setServer(configuracao.mqttHostname, configuracao.mqttPort);
            client.setCallback(callback);

            if (client.connect(config.mqttName,config.mqttUser, config.mqttSenha))
            {
                internetEstado = true;
                break;
            } else
            {
                internetEstado = false;
                vTaskDelay(5000/ portTICK_PERIOD_MS);
            }
        }

        //reinicia a placa após 10 minutos tentando se reconectar
        if ((millis() - reiniciarWifi)>= long(600000))
        {
            esp_restart();
        }
    }
}

void enviarMqtt(Configuracao config, VariaveisTermicas indices)
{
        String enviar = String("field1="+String(indices.temperaturaDeBulboSeco) +
                        "&field2="+String(indices.temperaturaDeBulboUmido)+
                        "&field3="+String(indices.temperaturaDeGlobo)+
                        "&field4="+String(indices.pressao)+
                        "&field5="+String(indices.umidadeRelativa1)+
                        "&field6="+String(indices.itu1)+
                        "&field7="+String(indices.itgu1)+
                        "&field8="+String(indices.umidadeRelativa2)+"&status="+indices.horario.timestamp());
        client.publish(String("channels/"+ String(config.mqttTopico) +"/publish").c_str(), enviar.c_str());
}
void onAlarm()
{        
    salvarHorarioMeiaNoite = true;
    
    esp_sleep_enable_timer_wakeup(1000000);
    esp_deep_sleep_start(); //força o ESP32 entrar em modo SLEEP por um segundo
}
void atualizarRtc(bool wifiLigado)
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
               //esp_wpa2_config_t wifiMode = WPA2_CONFIG_INIT_DEFAULT();
                //esp_wifi_sta_wpa2_ent_enable(&wifiMode);
                esp_wifi_sta_wpa2_ent_enable();
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
                } while (WiFi.status()!=WL_CONNECTED);   
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
            }else //Abortando a atualização do relógio e reiniciando a placa
            {
                http.end();
            }                    
        }
    }
}