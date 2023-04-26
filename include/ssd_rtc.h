#ifndef SSD_RTC_H
#define SSD_RTC_H
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <configuracao.h>
#define CLOCK_INTERRUPT_PIN 32

// Salva as variáveis de configuração na memoria interna
void salvarConfiguracao(Configuracao & config);
// Carriga as variáveis de confirução na memoria ram
Configuracao carregarConfiguracao();

/**
 * @brief Adiciona uma linha no arquivo contendo a informações que serão salvas no arquivo
 * 
 * @param fs 
 * @param path é o diretório do arquivo
 * @param message é a informação a ser adicionada no arquivo
 */
void appendFile(fs::FS &fs, const char * path, const char * message);
/**
 * @brief carrega o arquivo de configuração salvo na memória SD para a memória interna
 * que armazena as informações de configurações em variáveis locais para uso do sistema
 * 
 * @param filename Caminho do arquivo de configuração
 * @return true se conseguir fazer a leitura do ssd
 */
bool loadConfiguration(const char * filename,Configuracao & config);

#endif // SSD_RTC_H