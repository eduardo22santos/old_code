#include <ssd_rtc.h>

Preferences preferences;

void salvarConfiguracao(Configuracao & config)
{
    preferences.begin("my-app", false);
    preferences.clear();
    preferences.putString(String(wifiSsid).c_str(),config.wifiSsid);
    preferences.putString(String(wifiSenha).c_str(),config.wifiSenha);
    preferences.putString(String(eduroanLogin).c_str(),config.eduroanLogin);
    preferences.putString(String(eduroanSenha).c_str(),config.eduroanSenha);
    preferences.putString(String(mqttHostname).c_str(),config.mqttHostname);
    preferences.putString(String(mqttSenha).c_str(),config.mqttSenha);
    preferences.putString(String(mqttUser).c_str(),config.mqttUser);
    preferences.putString(String(mqttTopico).c_str(),config.mqttTopico);
    preferences.putString(String(mqttName).c_str(),config.mqttName);
    preferences.putString(String(servidorNtp).c_str(),config.servidorNtp);
    preferences.putString(String(hostTest).c_str(),config.hostTest);
    preferences.putInt(String(mqttPort).c_str(),config.mqttPort);
    preferences.putInt(String(timeZone).c_str(),config.timeZone);
    preferences.putInt(String(intervaloOnline).c_str(),config.intervaloOnline);
    preferences.putInt(String(intervaloSalvar).c_str(),config.intervaloSalvar);
    preferences.putInt(String(tipoAnimal).c_str(),config.tipoAnimal);
    preferences.putBool(String(mqttStatus).c_str(),config.mqttStatus);
    preferences.putBool(String(internetStatus).c_str(),config.internetStatus);
    preferences.putBool(String(eduroamStatus).c_str(),config.eduroamStatus);
    preferences.end();
    esp_restart();
}

Configuracao carregarConfiguracao()
{
    Configuracao config;

    if(preferences.begin("my-app",true))
    {
        Serial.println("Abrindo configuracao...");
        strlcpy(config.wifiSsid, preferences.getString(String(wifiSsid).c_str()).c_str(), sizeof(config.wifiSsid));
        strlcpy(config.wifiSenha, preferences.getString(String(wifiSenha).c_str()).c_str(), sizeof(config.wifiSenha));
        strlcpy(config.eduroanLogin, preferences.getString(String(eduroanLogin).c_str()).c_str(), sizeof(config.eduroanLogin));
        strlcpy(config.eduroanSenha, preferences.getString(String(eduroanSenha).c_str()).c_str(), sizeof(config.eduroanSenha));
        strlcpy(config.mqttHostname, preferences.getString(String(mqttHostname).c_str()).c_str(), sizeof(config.mqttHostname));
        strlcpy(config.mqttSenha, preferences.getString(String(mqttSenha).c_str()).c_str(), sizeof(config.mqttSenha));
        strlcpy(config.mqttUser, preferences.getString(String(mqttUser).c_str()).c_str(), sizeof(config.mqttUser));
        strlcpy(config.mqttTopico, preferences.getString(String(mqttTopico).c_str()).c_str(), sizeof(config.mqttTopico));
        strlcpy(config.mqttName, preferences.getString(String(mqttName).c_str()).c_str(), sizeof(config.mqttName));
        strlcpy(config.servidorNtp, preferences.getString(String(servidorNtp).c_str()).c_str(), sizeof(config.servidorNtp));
        strlcpy(config.hostTest, preferences.getString(String(hostTest).c_str()).c_str(), sizeof(config.hostTest));
        config.mqttPort = preferences.getInt(String(mqttPort).c_str());
        config.timeZone = preferences.getInt(String(timeZone).c_str());
        config.intervaloOnline = preferences.getInt(String(intervaloOnline).c_str());
        config.intervaloSalvar = preferences.getInt(String(intervaloSalvar).c_str());
        config.tipoAnimal = preferences.getInt(String(tipoAnimal).c_str());
        config.mqttStatus = preferences.getBool(String(mqttStatus).c_str());
        config.internetStatus = preferences.getBool(String(internetStatus).c_str());
        config.eduroamStatus = preferences.getBool(String(eduroamStatus).c_str());
        preferences.end();
        Serial.println("Configuracao aplicada!");
        
        return config;
    }else
    {

        Serial.println("Aplicando configuracao padrao!");
        preferences.begin("my-app", false);
        preferences.clear();
        preferences.putString(String(wifiSsid).c_str(),String("esp32_test"));
        preferences.putString(String(wifiSenha).c_str(),String("12345678"));
        preferences.putString(String(eduroanLogin).c_str(),"loginEduroam");
        preferences.putString(String(eduroanSenha).c_str(),"senhaEduroam");
        preferences.putString(String(mqttHostname).c_str(),"hostMqtt");
        preferences.putString(String(mqttSenha).c_str(),"senhaMqtt");
        preferences.putString(String(mqttUser).c_str(),"usuarioMqtt");
        preferences.putString(String(mqttTopico).c_str(),"topicoMqtt");
        preferences.putString(String(mqttName).c_str(),"NameMqtt");
        preferences.putString(String(servidorNtp).c_str(),String("a.st1.ntp.br"));
        preferences.putString(String(httpHostname).c_str(),"hostHttp");
        preferences.putString(String(hostTest).c_str(),String("https://ntp.br/"));
        preferences.putInt(String(mqttPort).c_str(),1883);
        preferences.putInt(String(timeZone).c_str(),-3);
        preferences.putInt(String(intervaloOnline).c_str(),1);
        preferences.putInt(String(intervaloSalvar).c_str(),1);
        preferences.putInt(String(tipoAnimal).c_str(),0);
        preferences.putBool(String(mqttStatus).c_str(),false);
        preferences.putBool(String(internetStatus).c_str(),true);
        preferences.putBool(String(eduroamStatus).c_str(),false);
        preferences.end();
        Serial.println("Configuracao padrao aplicada!");

        return carregarConfiguracao();
    }
    
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    File file = fs.open(path, FILE_APPEND);
    if(file.println(message)){
    } else {
        SD.end();
        delay(500);
        SD.begin();
    }
    file.close();
}

bool loadConfiguration(const char * filename,Configuracao & config) {

    if (SD.begin())
    {
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
            Serial.println("Erro al ler o arquivo de configuração no cartão SD...");
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
            config.tipoAnimal = doc["itu_animal"] | 1;
            strlcpy(config.hostTest, doc["host_teste_conexao"], sizeof(config.hostTest));
        }
        file.close();
        SD.remove("/config.json");
        SD.end();
        salvarConfiguracao(config);
        return true;
    }else
    {
        return false;
    }
}