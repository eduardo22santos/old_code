/**
 * @file salvar.cpp
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

#include <salvar.h>

/**
 * @brief Cria um diretório
 * 
 * @param fs 
 * @param path é o nome e caminho do diretório no formato "/diretorio"
 */
void createDir(fs::FS &fs, String path){
    fs.mkdir(path);
}

/**
 * @brief Adiciona uma linha no arquivo contendo a informações que serão salvas no arquivo
 * 
 * @param fs 
 * @param path é o diretório do arquivo
 * @param message é a informação a ser adicionada no arquivo
 */
void appendFile(fs::FS &fs, String path, String message){
    File file = fs.open(path, FILE_APPEND);
    if(file.println(message)){
    } else {
        SD.end();
        delay(500);
        SD.begin();
    }
    file.close();
}

/**
 * @brief carrega o arquivo de configuração salvo na memória SPIFFS para o struct
 * que armazena as informações de configurações em variáveis locais para uso do sistema
 * 
 * @param filename é o cominho do arquivo de configuração
 * @param display objeto do display oled
 * @return Um struct contendo as informações de configuração obtidas do arquivo de configuração
 */
Configuracao loadConfiguration(String &filename, Adafruit_SSD1306 &display) {
    Configuracao config;
    // Open file for reading
    File file = SPIFFS.open(filename, "r");
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<1536> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        display.clearDisplay();
        display.setCursor(0,0);
        display.print("Falha ao ler arquivo!\nAPLICANDO CONFIGURACAO\nPADRAO");
        display.display();
        delay(1000);
        SPIFFS.remove(filename);
        esp_restart();
    }else
    {
        strlcpy(config.mqttHostname, doc["mqtt_host"], sizeof(config.mqttHostname));
        config.mqttPort = doc["mqtt_porta"];
        strlcpy(config.wifiSsid, doc["wifi_ssid"], sizeof(config.wifiSsid));
        strlcpy(config.wifiSenha, doc["wifi_senha"], sizeof(config.wifiSenha));
        strlcpy(config.eduroanLogin, doc["eduroam_login"], sizeof(config.eduroanLogin));
        strlcpy(config.eduroanSenha, doc["eduroam_senha"], sizeof(config.eduroanSenha));
        strlcpy(config.mqttSenha, doc["mqtt_senha"], sizeof(config.mqttSenha));
        strlcpy(config.mqttUser, doc["mqtt_usuario"], sizeof(config.mqttUser));
        config.timeZone = doc["fuso_horario"];
        config.intervaloOnline = doc["intervalo_internet"];
        config.intervaloSalvar = doc["intervalo_sd"];
        config.internetStatus = doc["wifi_status"];
        config.eduroamStatus = doc["eduroam_status"];
        config.mqttStatus = doc["plataforma_status"];
        strlcpy(config.mqttTopico, doc["mqtt_topico"], sizeof(config.mqttTopico));
        strlcpy(config.servidorNtp, doc["ntp_host"], sizeof(config.servidorNtp));
        strlcpy(config.mqttName, doc["mqtt_device_id"], sizeof(config.mqttName));
        config.tipoAnimal = doc["itu_animal"] | geral;
        strlcpy(config.httpHostname, doc["http_host"], sizeof(config.httpHostname));
        config.cadastroConfterm = doc["producao_animal_id"];
        config.plataforma = doc["plataforma"];
        config.sensorBulboUmido = doc["bulbo_umido_status"];
        strlcpy(config.hostTest, doc["host_teste_conexao"], sizeof(config.hostTest));
    }
    file.close();
    return config;
}

/**
 * @brief carrega o arquivo de configuração salvo na memória SD para o struct
 * que armazena as informações de configurações em variáveis locais para uso do sistema
 * 
 * @param filename Caminho do arquivo de configuração
 * @param display Objeto do display oled
 * @return Configuracao Um struct contendo as informações de configuração contidas no aquivo json lido
 */
Configuracao loadConfigurationSd(String &filename, Adafruit_SSD1306 &display) {
    Configuracao config;
    // Open file for reading
    File file = SD.open(filename, "r");
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<1536> doc;

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        display.clearDisplay();
        display.setCursor(0,0);
        display.print("Falha ao ler arquivo!\nAPLICANDO CONFIGURACAO\nPADRAO");
        display.display();
        delay(1000);
        SPIFFS.remove(filename);
        saveConfiguration();
    }else
    {
        strlcpy(config.mqttHostname, doc["mqtt_host"], sizeof(config.mqttHostname));
        config.mqttPort = doc["mqtt_porta"];
        strlcpy(config.wifiSsid, doc["wifi_ssid"], sizeof(config.wifiSsid));
        strlcpy(config.wifiSenha, doc["wifi_senha"], sizeof(config.wifiSenha));
        strlcpy(config.eduroanLogin, doc["eduroam_login"], sizeof(config.eduroanLogin));
        strlcpy(config.eduroanSenha, doc["eduroam_senha"], sizeof(config.eduroanSenha));
        strlcpy(config.mqttSenha, doc["mqtt_senha"], sizeof(config.mqttSenha));
        strlcpy(config.mqttUser, doc["mqtt_usuario"], sizeof(config.mqttUser));
        config.timeZone = doc["fuso_horario"];
        config.intervaloOnline = doc["intervalo_internet"];
        config.intervaloSalvar = doc["intervalo_sd"];
        config.internetStatus = doc["wifi_status"];
        config.eduroamStatus = doc["eduroam_status"];
        config.mqttStatus = doc["plataforma_status"];
        strlcpy(config.mqttTopico, doc["mqtt_topico"], sizeof(config.mqttTopico));
        strlcpy(config.servidorNtp, doc["ntp_host"], sizeof(config.servidorNtp));
        strlcpy(config.mqttName, doc["mqtt_device_id"], sizeof(config.mqttName));
        config.tipoAnimal = doc["itu_animal"] | geral;
        strlcpy(config.httpHostname, doc["http_host"], sizeof(config.httpHostname));
        config.cadastroConfterm = doc["producao_animal_id"];
        config.plataforma = doc["plataforma"];
        config.sensorBulboUmido = doc["bulbo_umido_status"];
        strlcpy(config.hostTest, doc["host_teste_conexao"], sizeof(config.hostTest));
    }
    file.close();
    return config;
}

/**
 * @brief Cria um arquivo de configuração na Memoria SPIFFS com informações de configuração do sistema como padrão de fabrica
 * 
 */
void saveConfiguration() {
  // Delete existing file, otherwise the configuration is appended to the file
    //createDir(SD, "/configuracaoPadrao");
    File file = SPIFFS.open("/configuracao", FILE_WRITE);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<768> doc;

    // Set the values in the document
    doc["wifi_ssid"] = "EDUARDO";
    doc["wifi_senha"] = "16r04o18b";
    doc["wifi_status"] = true;
    doc["eduroam_login"] = "login da rede eduroam";
    doc["eduroam_senha"] = "senha da rede eduroam";
    doc["eduroam_status"] = false;
    doc["mqtt_host"] = "exemplo.com";
    doc["mqtt_porta"] = 1883;
    doc["mqtt_senha"] = "senha_broker_mqtt";
    doc["mqtt_usuario"] = "usuario_broker_mqtt";
    doc["mqtt_topico"] = "topico_broker_mqtt/";
    doc["mqtt_device_id"] = "deviceID_mqtt_broker";
    doc["plataforma_status"] = false;
    doc["intervalo_internet"] = 300;
    doc["intervalo_sd"] = 60;
    doc["fuso_horario"] = -3;
    doc["ntp_host"] = "0.br.pool.ntp.org";
    doc["itu_animal"] = 0;
    doc["http_host"] = "https://www.exemplo.com.br/api/exemplo";
    doc["producao_animal_id"] = 1;
    doc["plataforma"] = Original;
    doc["bulbo_umido_status"] = true;
    doc["host_teste_conexao"] = "https://www.google.com/";

    serializeJson(doc, file);

    file.close();
}

/**
 * @brief Cria um arquivo na memoria SD com informações de configuração do sistema como padrão de fabrica
 * 
 */
void saveConfigurationSd() {
  // Delete existing file, otherwise the configuration is appended to the file
    //createDir(SD, "/configuracaoPadrao");
    File file = SD.open("/exemplo.json", FILE_WRITE);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<768> doc;

    // Set the values in the document
    doc["wifi_ssid"] = "EDUARDO";
    doc["wifi_senha"] = "16r04o18b";
    doc["wifi_status"] = true;
    doc["eduroam_login"] = "login da rede eduroam";
    doc["eduroam_senha"] = "senha da rede eduroam";
    doc["eduroam_status"] = false;
    doc["mqtt_host"] = "mqtt://mqtt3.thingspeak.com";
    doc["mqtt_porta"] = 1883;
    doc["mqtt_senha"] = "WTGuR8JVoxPDu8FJTHZx5k0p";
    doc["mqtt_usuario"] = "LSQ2DiIANjcNAjc0NgsRFTo";
    doc["mqtt_topico"] = "1451468";
    doc["mqtt_device_id"] = "LSQ2DiIANjcNAjc0NgsRFTo";
    doc["plataforma_status"] = false;
    doc["intervalo_internet"] = 60;
    doc["intervalo_sd"] = 60;
    doc["fuso_horario"] = -3;
    doc["ntp_host"] = "a.st1.ntp.br";
    doc["itu_animal"] = 0;
    doc["http_host"] = "https://www.exemplo.com.br/api/exemplo";
    doc["producao_animal_id"] = 1;
    doc["plataforma"] = thingspeak;
    doc["bulbo_umido_status"] = true;
    doc["host_teste_conexao"] = "https://www.google.com/";

    serializeJsonPretty(doc, file);

    file.close();
}

/**
 * @brief Atualiza o arquivo de configuração na memoria SPIFFS
 * 
 * @param config Struct de configuração
 */
void exportConfiration(const Configuracao &config) {
  // Delete existing file, otherwise the configuration is appended to the file
    //createDir(SD, "/configuracaoPadrao");
    File file = SPIFFS.open("/configuracao", FILE_WRITE);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<768> doc;

    // Set the values in the document
    doc["wifi_ssid"] = config.wifiSsid;
    doc["wifi_senha"] = config.wifiSenha;
    doc["eduroam_login"] = config.eduroanLogin;
    doc["eduroam_senha"] = config.eduroanSenha;
    doc["eduroam_status"] = config.eduroamStatus;
    doc["wifi_status"] = config.internetStatus;
    doc["plataforma_status"] = config.mqttStatus;
    doc["intervalo_internet"] = config.intervaloOnline;
    doc["intervalo_sd"] = config.intervaloSalvar;
    doc["mqtt_host"] = config.mqttHostname;
    doc["mqtt_porta"] = config.mqttPort;
    doc["mqtt_senha"] = config.mqttSenha;
    doc["mqtt_usuario"] = config.mqttUser;
    doc["mqtt_topico"] = config.mqttTopico;
    doc["mqtt_device_id"] = config.mqttName;
    doc["ntp_host"] = config.servidorNtp;
    doc["fuso_horario"] = config.timeZone;
    doc["itu_animal"] = config.tipoAnimal;
    doc["http_host"] = config.httpHostname;
    doc["producao_animal_id"] = config.cadastroConfterm;
    doc["plataforma"] = config.plataforma;
    doc["bulbo_umido_status"] = config.sensorBulboUmido;
    doc["host_teste_conexao"] = config.hostTest;


    serializeJson(doc, file);


    file.close();
}

/**
 * @brief Salva os valores de máximas e mínimas em um arquivo json na memória SD
 * 
 * @param indices struct contendo os valores das variáveis climáticas
 * @param arquivoTemporario diretório do arquivo json na memória sd ("/maximas.json")
 */
void salvarMaxMin(VariaveisTermicas &indices, String &arquivoTemporario)
{
    File file = SD.open(arquivoTemporario, FILE_WRITE);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

    // Set the values in the document

    doc["umidadeMax"] = indices.umidadeMax;
    doc["umidadeMin"] = indices.umidadeMin;
    doc["temperaturaMax"] = indices.temperaturaMax;
    doc["temperaturaMin"] = indices.temperaturaMin;
    doc["bulboUmidoMax"] = indices.temperaturaDeBulboUmidoMax;
    doc["bulboUmidoMin"] = indices.temperaturaDeBulboUmidoMin;
    doc["globaMax"] = indices.globoMax;
    doc["globoMin"] = indices.globoMin;
    doc["itguMax"] = indices.itguMax;
    doc["itguMin"] = indices.itguMin;
    doc["ituMax"] = indices.ituMax;
    doc["ituMin"] = indices.ituMin;
    doc["orvalhoMax"] = indices.orvalhoMax;
    doc["orvalhoMin"] = indices.orvalhoMin;
    
    serializeJsonPretty(doc, file);


    file.close();
}

/**
 * @brief Faz a leitura do arquivo de máximas e mínimas na memória SD
 * 
 * @param indices struct contendo os valores das variáveis climáticas
 * @param arquivoTermporario diretório do arquivo json na memória sd ("/maximas.json")
 */
void lerMaxMin(VariaveisTermicas &indices, String &arquivoTermporario)
{
    File file = SD.open(arquivoTermporario, "r");

    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.
    StaticJsonDocument<512> doc;

    // Deserialize the JSON document
    deserializeJson(doc, file);

    indices.umidadeMax = doc["umidadeMax"];
    indices.umidadeMin = doc["umidadeMin"];
    indices.temperaturaMax = doc["temperaturaMax"];
    indices.temperaturaMin =  doc["temperaturaMin"];
    indices.temperaturaDeBulboUmidoMax = doc["bulboUmidoMax"];
    indices.temperaturaDeBulboUmidoMin = doc["bulboUmidoMin"];
    indices.globoMax = doc["globaMax"];
    indices.globoMin = doc["globoMin"];
    indices.itguMax = doc["itguMax"];
    indices.itguMin = doc["itguMin"];
    indices.ituMax = doc["ituMax"];
    indices.ituMin = doc["ituMin"];
    indices.orvalhoMax = doc["orvalhoMax"];
    indices.orvalhoMin = doc["orvalhoMin"];

    file.close();
}