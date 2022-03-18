/**
 * @file salvar.h
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


#ifndef SALVAR_h
#define SALVAR_h
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <RTClib.h>
#include <indices.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <esp_sleep.h>

/**
 * @brief Um enumerador contendo as plataformas web
 * thinspeak = 0
 * conftermApi = 1
 * Original = 2
 * Ubidots = 3
 * 
 */
enum PlataformaOnline
{
    thingspeak, conftermApi, Original, ubidots
};

/**
 * @brief Cria um diretório
 * 
 * @param fs 
 * @param path é o nome e caminho do diretório no formato "/diretorio"
 */
void createDir(fs::FS &fs, String path);

/**
 * @brief Adiciona uma linha no arquivo contendo a informações que serão salvas no arquivo
 * 
 * @param fs 
 * @param path é o diretório do arquivo
 * @param message é a informação a ser adicionada no arquivo
 */void appendFile(fs::FS &fs, String path, String message);

/**
 * @brief Struct de configuração, armazena as infomações de configuração do sistema como senhas e endereços
 * 
 */
struct Configuracao {
  //SSid do wifi
  char wifiSsid[64];
  //Senha do wifi
  char wifiSenha[64];
  //eduroan login
  char eduroanLogin[64];
  //eduroan senha
  char eduroanSenha[64];
  //Porta do servidor mqtt
  int mqttPort;
  //endereço do servidor mqtt
  char mqttHostname[64];
  //Senha de acesso do servidor mqtt
  char mqttSenha[64];
  //Usuario de acesso do servidor mqtt
  char mqttUser[64];
  //Nome do tópico publish mqtt
  char mqttTopico[64];
  //Mqtt id
  char mqttName[64];
  //O status do servidor, ativo ou desconectado
  bool mqttStatus;
  //Endereço do servidor NTP, responsável por sincronizar o horario
  char servidorNtp[64];
  //Define o fuso horário do Relógio
  int timeZone;
  //Define o intervalo em que o aparelho irá enviar os dados online
  //O tempo é medido em minutos
  float intervaloOnline;
  //Define o intervalo em que o aparelho irá salvar os dados no cartão de memória
  //O tempo é medido em minutos
  float intervaloSalvar;
  //Indica o status da conexão de internet, conectado ou desconectado
  bool internetStatus;
  //Indica se a conexão wifi é do tipo enterprise
  bool eduroamStatus;
  //Indica o tipo de indice itu que será calculado
  Animal tipoAnimal;
  //endereço da api do servidor
  char httpHostname[64];
  //Nome do cadastro no banco
  int cadastroConfterm;
  //Difine a plataforma que está utilizando
  PlataformaOnline plataforma;
  //Indica se o sensor de bulbo umido será utilizado, caso não, a umidade será calculada com base no sensor resistivo htu21d
  bool sensorBulboUmido = true;
  //Endereço de um servidor para testar conexão de internet, o padrão é usar o google.com
  char hostTest[64];
};

/**
 * @brief carrega o arquivo de configuração salvo na memória SPIFFS para o struct
 * que armazena as informações de configurações em variáveis locais para uso do sistema
 * 
 * @param filename é o cominho do arquivo de configuração
 * @param display objeto do display oled
 * @return Um struct contendo as informações de configuração obtidas do arquivo de configuração
 */
Configuracao loadConfiguration(String &filename, Adafruit_SSD1306 &display);

/**
 * @brief carrega o arquivo de configuração salvo na memória SD para o struct
 * que armazena as informações de configurações em variáveis locais para uso do sistema
 * 
 * @param filename Caminho do arquivo de configuração
 * @param display Objeto do display oled
 * @return Configuracao Um struct contendo as informações de configuração contidas no aquivo json lido
 */
Configuracao loadConfigurationSd(String &filename, Adafruit_SSD1306 &display);

/**
 * @brief Cria um arquivo de configuração na Memoria SPIFFS com informações de configuração do sistema como padrão de fabrica
 * 
 */
void saveConfiguration();

/**
 * @brief Cria um arquivo na memoria SD com informações de configuração do sistema como padrão de fabrica
 * 
 */
void saveConfigurationSd();

/**
 * @brief Atualiza o arquivo de configuração na memoria SPIFFS
 * 
 * @param config Struct de configuração
 */
void exportConfiration(const Configuracao &config);

/**
 * @brief Salva os valores de máximas e mínimas em um arquivo json na memória SD
 * 
 * @param indices struct contendo os valores das variáveis climáticas
 * @param arquivoTemporario diretório do arquivo json na memória sd ("/maximas.json")
 */
void salvarMaxMin(VariaveisTermicas &indices, String &arquivoTemporario);

/**
 * @brief Faz a leitura do arquivo de máximas e mínimas na memória SD
 * 
 * @param indices struct contendo os valores das variáveis climáticas
 * @param arquivoTermporario diretório do arquivo json na memória sd ("/maximas.json")
 */
void lerMaxMin(VariaveisTermicas &indices, String &arquivoTermporario);

#endif // SALVAR_h
