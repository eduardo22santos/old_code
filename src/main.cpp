#include <Arduino.h>
#include <Wire.h>
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
#include <anemometro.h>
#include <sensores.h>
#include <configuracao.h>
#include <ssd_rtc.h>
#define LED_VERDE 32 //INDICA O ESTADO DA PLACA
#define LED_AMARELO 15 //INDICA O ESTADO DA INTERNET
#define CLOCK_INTERRUPT_PIN 34 //Indica a porta para o pino sqw do rtc

SemaphoreHandle_t semaforoDosIndices;
SemaphoreHandle_t internetStatusConexao;


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
/***************************************************
 * Objetos e variaveis de configuração
 * **************************************************
 */

/**
 * @brief Objeto que armazena as configuracoes fornecidas no arquivo json de configuracao e deixa disponivel para usar no sistema.
 * 
 */
Configuracao config;
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
NTPClient timeClient(ntpUDP, config.servidorNtp, config.timeZone * 3600, 60000); //pode-se alterar o servidor ntp
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

void tarefaSalvarSd(void * pvParameters)
{
    Configuracao config = *(Configuracao*)pvParameters;
    VariaveisTermicas indicesLocais;
    Serial.println("Iniciando a tarefa salvar!");
    if (!SD.begin())
    {
        ESP.restart();
    }
    if (xSemaphoreTake(semaforoDosIndices,portMAX_DELAY) == pdTRUE)
    {
        indicesLocais = atualizaVariaveis(config.tipoAnimal);
        while(xSemaphoreGive(semaforoDosIndices) != pdTRUE);
    }else
    {
        Serial.println("Semaforo indisponivel para a tarefa salvar!");
    }
    

    DateTime tempoAtual = relogio.now();
    if (!tempoAtual.isValid())
    {
        esp_restart();
    }
    char horarioArquivo[9] = "hh:mm:ss";
    char dataArquivo[11] = "DD/MM/YYYY";

        String datalog = String(tempoAtual.toString(dataArquivo)) + "," + 
            String(tempoAtual.toString(horarioArquivo)) + "," +
            String(indicesLocais.umidadeRelativa1) + "," + 
            String(indicesLocais.umidadeRelativa2) + "," + 
            String(indicesLocais.temperaturaDeGlobo) + "," +
            String(indicesLocais.temperaturaDeBulboSeco) + "," +
            String(indicesLocais.temperaturaDeBulboUmido) + "," +
            String(indicesLocais.itu1) + "," +
            String(indicesLocais.itu2) + "," +
            String(indicesLocais.itgu1) + "," +
            String(indicesLocais.itgu2) + "," +
            String(indicesLocais.ibutg1) + "," +
            String(indicesLocais.ibutg2) + "," +
            String(indicesLocais.pontoDeOrvalho1) + "," +
            String(indicesLocais.pontoDeOrvalho2) + "," +
            String(indicesLocais.pressao) + "," +
            String(indicesLocais.velocidadeDoAr);
        //Salva no arquivo datalog
        appendFile(SD, "/datalog.csv", datalog.c_str());
        SD.end();
    vTaskDelete(NULL);
}
void tarefaEnviarMqtt(void * pvParameters)
{

    Configuracao config = *(Configuracao*)pvParameters;
     VariaveisTermicas indicesLocais;

    if (xSemaphoreTake(internetStatusConexao,pdMS_TO_TICKS(100)) == pdTRUE)
    {
        if (xSemaphoreTake(semaforoDosIndices,portMAX_DELAY) == pdTRUE)
        {
            indicesLocais = atualizaVariaveis(config.tipoAnimal);
            while(xSemaphoreGive(semaforoDosIndices) != pdTRUE);
        }else
        {
            Serial.println("Semaforo indisponivel para a tarefa salvar!");
        }

        while(xSemaphoreGive(internetStatusConexao) != pdTRUE);
    }
    enviarMqtt(config, indicesLocais);    
    vTaskDelete(NULL);
}
void reconectarRede(void * pvParameters)
{
    
    reconnect(config);
    permissao = true;
    vTaskDelete(NULL);
}
void setup()
{
    ////Serial para debug
    Serial.begin(9600);
    Wire.begin(21,22);
    pinMode(LED_VERDE, OUTPUT);
    pinMode(LED_AMARELO, OUTPUT);

    //CONFIGURAÇÕES DA PORTA 15 PARA PISCAR O LED
    //POR SER UMA PORTA PWM, É NECESSÁRIO CONFIGURAÇÕES ADICIONAIS
    // configure LED PWM functionalitites
    ledcSetup(0, 5000, 8);
    // attach the channel to the GPIO to be controlled
    ledcAttachPin(LED_AMARELO, 0);


    digitalWrite(LED_VERDE, HIGH);

    vSemaphoreCreateBinary(semaforoDosIndices);
    vSemaphoreCreateBinary(internetStatusConexao);

    //testa o catao de memoria tornando obrigatorio o uso
    if(!SD.begin())
    {
        while (!SD.begin())
        {
            digitalWrite(LED_VERDE, LOW);
            digitalWrite(2,LOW);
            vTaskDelay(250 / portTICK_PERIOD_MS);
            digitalWrite(LED_VERDE, HIGH);
            digitalWrite(2, HIGH);
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
    
    //Inicializa os sensores
    sensoresBegin();

    config = carregarConfiguracao();
    
    uint8_t cont;
    do
    {
        if (SD.begin())
        {
            config.ssd = true;
            //Verifica se existe o arquivo datalog no cartao de memoria, criando-o caso contrario
            if (!SD.exists("/datalog.csv")) //verifica ou cria o arquivo datalog.txt
            {
                File file = SD.open("/datalog.csv", FILE_WRITE);
                file.close();
                appendFile(SD, "/datalog.csv", "data,hora,tgn,tbs,tbu,psi,htu,itu1,itu2,itgu1,itgu2,ibutg1,ibutg2,or1,or2,hpa,v_ar");   
            }
            if (SD.exists("/config.json"))
            {
                SD.end();
                loadConfiguration("/config.json",config);
                
            }
            break;
        }else
        {
            config.ssd = false;
            Serial.println("SSD indisponivel!");
            cont++;
        }
        
    } while (cont < 10);

    //se as variaves internetStatus e mqttStatus estiverem true no aquivo de configuracao, o wifi será ligado no setup
    if (config.mqttStatus && config.internetStatus) internetAtiva = true; 
    
    //Testa o rtc para verificar o horario
    DateTime verificaRtc = relogio.now();
    if (!verificaRtc.isValid() || verificaRtc.year()==2000)
    {
        atualizarRtc(false);
         
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
    zeroHora = DateTime(tempoAtual.year(),tempoAtual.month(),tempoAtual.day(),0,0,0);
    relogio.setAlarm1(zeroHora,DS3231_A1_Hour); 

    //Salva as maximas e minimas no arquivo "/maximas_e_minimas.csv"
    if (salvarHorarioMeiaNoite)
    {
        VariaveisTermicas indicesLocais;
        salvarHorarioMeiaNoite = false;
        
        realizarLeitura();

        if (!SD.begin())
        {
            ESP.restart();
        }

        indicesLocais = atualizaVariaveis(config.tipoAnimal);
        

        tempoAtual = relogio.now();
        if (!tempoAtual.isValid())
        {
            esp_restart();  
        }
        char horarioArquivo[9] = "hh:mm:ss";
        char dataArquivo[11] = "DD/MM/YYYY";

    
        String datalog = String(tempoAtual.toString(dataArquivo)) + "," + 
            String(tempoAtual.toString(horarioArquivo)) + "," +
            String(indicesLocais.umidadeRelativa1) + "," + 
            String(indicesLocais.umidadeRelativa2) + "," + 
            String(indicesLocais.temperaturaDeGlobo) + "," +
            String(indicesLocais.temperaturaDeBulboSeco) + "," +
            String(indicesLocais.temperaturaDeBulboUmido) + "," +
            String(indicesLocais.itu1) + "," +
            String(indicesLocais.itu2) + "," +
            String(indicesLocais.itgu1) + "," +
            String(indicesLocais.itgu2) + "," +
            String(indicesLocais.ibutg1) + "," +
            String(indicesLocais.ibutg2) + "," +
            String(indicesLocais.pontoDeOrvalho1) + "," +
            String(indicesLocais.pontoDeOrvalho2) + "," +
            String(indicesLocais.pressao) + "," +
            String(indicesLocais.velocidadeDoAr);
        //Salva no arquivo datalog
        appendFile(SD, "/datalog.csv", datalog.c_str());
        SD.end();
        
        if (config.internetStatus)
        {
            atualizarRtc(false);
            ESP.restart();
        }else
        {
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
    }

    //Inicia a conexao com internet se o usuario declarar internetStatus e mqttStatus como true no arquivo de configuracao
    if (internetAtiva)
    {   
        WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
        WiFi.mode(WIFI_STA); //init wifi mode

        if(config.eduroamStatus)
        {
            esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)config.eduroanLogin, strlen(config.eduroanLogin)); //provide identity
            esp_wifi_sta_wpa2_ent_set_username((uint8_t *)config.eduroanLogin, strlen(config.eduroanLogin)); //provide username --> identity and username is same
            esp_wifi_sta_wpa2_ent_set_password((uint8_t *)config.eduroanSenha, strlen(config.eduroanSenha)); //provide password
            esp_wifi_sta_wpa2_ent_enable();
            WiFi.begin(config.wifiSsid);
            vTaskDelay(5000/ portTICK_PERIOD_MS);
        }else
        {
            WiFi.begin(config.wifiSsid, config.wifiSenha);
            vTaskDelay(5000/ portTICK_PERIOD_MS);
        }
        
        //INICIA O LOOP DO SERVICO MQTT no core 0
        client.setServer(config.mqttHostname, config.mqttPort);
        client.setCallback(callback);
        if(client.connect(config.mqttName,config.mqttUser, config.mqttSenha))
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
        if (xSemaphoreTake(semaforoDosIndices,portMAX_DELAY) == pdTRUE)
        {
            realizarLeitura();        
            while(xSemaphoreGive(semaforoDosIndices) != pdTRUE);
        }
        
        intervaloLerVariaveis = currentMillis; 
    } 
    /**
     * @brief Salva os dados no cartão de memoria no intervalo de tempo especificado no arquivo de configuração
     * 
     */
    diferenca = currentMillis - intervaloSalvarDados;
    if ( diferenca >= long(config.intervaloSalvar*1000))
    {
        xTaskCreatePinnedToCore(tarefaSalvarSd, "tarefaSalvar", 10000, (void*)&config, 4, &tarefa_salvar,0);
        intervaloSalvarDados = currentMillis;
    }
    /**
     * @brief envia os dados para o servidor no intervalo de tempo especificado no arquivo de configuração
     * 
     */
    diferenca = currentMillis - intervaloEnviarDados;
    if (internetEstado && diferenca >= long(config.intervaloOnline*1000))
    {
        //MQTT
        if (internetEstado)
        {
            xTaskCreatePinnedToCore(tarefaEnviarMqtt, "tarefaEnviarMqtt", 10000, (void*)&config, 2, &tarefa_salvar,0);
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
            client.setServer(config.mqttHostname, config.mqttPort);
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
                        "&field8="+String(indices.umidadeRelativa2)+"&status="+relogio.now().timestamp());
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
    if (config.internetStatus)
    {
        if (!wifiLigado)
        {
            WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
            WiFi.mode(WIFI_STA); //init wifi mode
            if(config.eduroamStatus)
            {
                esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)config.eduroanLogin, strlen(config.eduroanLogin)); //provide identity
                esp_wifi_sta_wpa2_ent_set_username((uint8_t *)config.eduroanLogin, strlen(config.eduroanLogin)); //provide username --> identity and username is same
                esp_wifi_sta_wpa2_ent_set_password((uint8_t *)config.eduroanSenha, strlen(config.eduroanSenha)); //provide password
                esp_wifi_sta_wpa2_ent_enable();
                WiFi.begin(config.wifiSsid);
                vTaskDelay(5000/ portTICK_PERIOD_MS);
            }else
            {
                WiFi.begin(config.wifiSsid, config.wifiSenha);
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
            http.begin(config.hostTest); 
            int httpCode = http.GET();
            if(httpCode == HTTP_CODE_OK)//ok, há internet!
            {
                http.end();
                //Iniciando a atualização do relógio rtc
                timeClient.setTimeOffset(config.timeZone*3600);
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
void callback(char* topic, byte* payload, unsigned int length)
{

}