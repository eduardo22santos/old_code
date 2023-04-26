#ifndef CONFIGURACAO_H
#define CONFIGURACAO_H
#include <Arduino.h>

/**
 * @brief enumerador contendo as palavras chave das variáveis de configuração
 * 
 */
enum config_
{
    //SSid do wifi
    wifiSsid,
    //Senha do wifi
    wifiSenha,
    //eduroan login
    eduroanLogin,
    //eduroan senha
    eduroanSenha,
    //Porta do servidor mqtt
    mqttPort,
    //endereço do servidor mqtt
    mqttHostname,
    //Senha de acesso do servidor mqtt
    mqttSenha,
    //Usuario de acesso do servidor mqtt
    mqttUser,
    //Nome do tópico publish mqtt
    mqttTopico,
    //Mqtt id
    mqttName,
    //O status do servidor, ativo ou desconectado
    mqttStatus,
    //Endereço do servidor NTP, responsável por sincronizar o horario
    servidorNtp,
    //Define o fuso horário do Relógio
    timeZone,
    //Define o intervalo em que o aparelho irá enviar os dados online
    //O tempo é medido em minutos
    intervaloOnline,
    //Define o intervalo em que o aparelho irá salvar os dados no cartão de memória
    //O tempo é medido em minutos
    intervaloSalvar,
    //Indica o status da conexão de internet, conectado ou desconectado
    internetStatus,
    //Indica se a conexão wifi é do tipo enterprise
    eduroamStatus,
    //Indica o tipo de indice itu que será calculado
    tipoAnimal,
    //endereço da api do servidor
    httpHostname,
    //Nome do cadastro no banco
    cadastroConfterm,
    //Difine a plataforma que está utilizando
    plataforma,
    //Endereço de um servidor para testar conexão de internet, o padrão é usar o google.com
    hostTest
};

/**
 * @brief Struct de configuração, armazena as infomações de configuração do sistema como senhas e endereços
 * 
 */
struct Configuracao{
  //SSid do wifi
  char wifiSsid[32];
  //Senha do wifi
  char wifiSenha[32];
  //eduroan login
  char eduroanLogin[32];
  //eduroan senha
  char eduroanSenha[32];
  //Porta do servidor mqtt
  int mqttPort;
  //endereço do servidor mqtt
  char mqttHostname[32];
  //Senha de acesso do servidor mqtt
  char mqttSenha[32];
  //Usuario de acesso do servidor mqtt
  char mqttUser[32];
  //Nome do tópico publish mqtt
  char mqttTopico[32];
  //Mqtt id
  char mqttName[32];
  //O status do servidor, ativo ou desconectado
  bool mqttStatus;
  //Endereço do servidor NTP, responsável por sincronizar o horario
  char servidorNtp[32];
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
  int tipoAnimal;
  //Endereço de um servidor para testar conexão de internet, o padrão é usar o google.com
  char hostTest[32];
  // True para ativar o ssd
  bool ssd;
  // True para ativar o rtc
  bool rtc;
};

#endif // CONFIGURACAO_H