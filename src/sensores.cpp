#include <sensores.h>
OneWire principal(4);
OneWire secundario(14);
OneWire terciario(27);
DallasTemperature bulboSeco(&principal);
DallasTemperature globoNegro(&terciario);
DallasTemperature bulboUmido(&secundario);
Adafruit_HTU21DF htu21d = Adafruit_HTU21DF();
Adafruit_BMP280 bmp;
VarTemp temporarias;
void VarTemp::zeraVariaveis()
{
    vAr = 0;
    ur2 = 0;
    tgn = 0;
    tbs = 0;
    tbu = 0;
    p = 0;
    contador = 0;
}
void realizarLeitura()
{
        digitalWrite(LED_VERMELHO,HIGH);
        bulboSeco.requestTemperatures();
        bulboSeco.requestTemperatures();
        globoNegro.requestTemperatures();
        bulboUmido.requestTemperatures();
        
        //variaveis temporárias para nao passar leituras erradas
        float pressao1 = bmp.readPressure();
        float tbs,tbu,globo;
        globo = globoNegro.getTempCByIndex(0);
        tbs = bulboSeco.getTempCByIndex(0);
        tbu = bulboUmido.getTempCByIndex(0);
            
        //Verifica se há erros nas leituras dos sensores ds18b20
        if(tbs == -127.0 || tbs == 85)
        {
            temporarias.erroSensorTbs = true;
        }else
        {
            temporarias.tbs += tbs;
            Serial.printf("tbs: %f\n",tbs);
            temporarias.erroSensorTbs = false;
        }

        if(tbu == -127.0 || tbu == 85)
        {
            temporarias.erroSensorUmidade = true;
        }else
        {
            temporarias.tbu += tbu;
            Serial.printf("tbu: %f\n",tbu);
            temporarias.erroSensorUmidade = false;
        }

        if(globo == -127.0 || globo == 85)
        {
            temporarias.erroSensorGlobo = true;
        }else
        {
            temporarias.tgn += globo;
            Serial.printf("tgn: %f\n",globo);
            temporarias.erroSensorGlobo = false;
        }
            
        //Verifica se há erro no sensor de pressão
        if (isnan(pressao1) == 0 && pressao1 > 1)
        {
            temporarias.p += (pressao1 / 100);
            Serial.printf("p: %f\n",(pressao1 / 100));
            temporarias.erroSensorPressao = false;
        }else
        {
            temporarias.erroSensorPressao = true;
        }
            
        //Verifica se há erro na leitura do sensor htu21d
        float umidade = htu21d.readHumidity();
        if(umidade == 998.0)
        {
            temporarias.erroSensorUmidade2 = true;
        }else
        {
            temporarias.ur2 += (umidade > 98)? 98 : umidade;
            temporarias.erroSensorUmidade2 = false;
        }
        temporarias.contador++;
        digitalWrite(LED_VERMELHO,LOW);  
}

float calculaUmidadeRelativa(float tbs,float tbu,float pressao)
{
    /**lembrando que pressaoBmp está em Pa*/
    //pressão de saturação
    float esu = 6.1078 * pow(10, ((7.5*tbu)/(237.3+tbu)));
    float es = 6.1078 * pow(10, ((7.5*tbs)/(237.3+tbs)));
    //pressão real de vapor de água, para psicrômetros não aspirados
    float e = esu - (0.0008 * (pressao*(tbs - tbu)));
    
    return (e/es)*100;
}
float calculaItu(int animal, float umidadeRelativa,float tbs)
{
    if (animal == 0)
    {
        return ((1.8*tbs) + 32) - ((0.55 - (0.55*(umidadeRelativa/100)))*(((1.8 * tbs) + 32) - 58));
    }else 
    {
        return  (0.8 * tbs + ((umidadeRelativa * (tbs - 14.3))/ 100) + 46.3);
    }

}
VariaveisTermicas atualizaVariaveis(int animal)
{          
    
    //Calculos dos indices termicos
    if(!temporarias.erroSensorGlobo && !temporarias.erroSensorTbs && 
        !temporarias.erroSensorUmidade && !temporarias.erroSensorPressao && 
        !temporarias.erroSensorUmidade2)
    {
        digitalWrite(LED_VERMELHO, LOW);

        VariaveisTermicas indices;

        uint8_t n = temporarias.contador;

        indices.velocidadeDoAr = temporarias.vAr;
        indices.umidadeRelativa2 = temporarias.ur2/n;
        indices.temperaturaDeGlobo = temporarias.tgn/n;
        indices.temperaturaDeBulboSeco = temporarias.tbs/n;
        indices.temperaturaDeBulboUmido = temporarias.tbu/n;
        indices.pressao = temporarias.p/n;
        temporarias.zeraVariaveis();

        indices.velocidadeDoAr = anemometroVelocidade_Ms();
                 
        indices.umidadeRelativa1 = calculaUmidadeRelativa(indices.temperaturaDeBulboSeco,indices.temperaturaDeBulboUmido,indices.pressao);
        
        indices.itu1 = calculaItu(animal, indices.umidadeRelativa1,indices.temperaturaDeBulboSeco);
        indices.itu2 = calculaItu(animal, indices.umidadeRelativa2,indices.temperaturaDeBulboSeco);
        indices.pontoDeOrvalho1 = (243.04*(log(indices.umidadeRelativa1/100)+((17.625*indices.temperaturaDeBulboSeco)/(243.04+indices.temperaturaDeBulboSeco))))/(17.625-log(indices.umidadeRelativa1/100)-((17.625*indices.temperaturaDeBulboSeco)/(243.04+indices.temperaturaDeBulboSeco)));
        indices.pontoDeOrvalho2 = (243.04*(log(indices.umidadeRelativa2/100)+((17.625*indices.temperaturaDeBulboSeco)/(243.04+indices.temperaturaDeBulboSeco))))/(17.625-log(indices.umidadeRelativa2/100)-((17.625*indices.temperaturaDeBulboSeco)/(243.04+indices.temperaturaDeBulboSeco)));
        
        indices.itgu1 = indices.temperaturaDeGlobo + (0.36 * indices.pontoDeOrvalho1) + 41.5;
        indices.itgu2 = indices.temperaturaDeGlobo + (0.36 * indices.pontoDeOrvalho2) + 41.5;
        indices.ibutg1 = (0.7*indices.temperaturaDeBulboUmido) + (0.3*indices.temperaturaDeGlobo);
        indices.ibutg2 = (0.7*indices.temperaturaDeBulboUmido) + (0.2*indices.temperaturaDeGlobo) + (0.1*indices.temperaturaDeBulboSeco);

        return indices;
            
    }else
    {
        VariaveisTermicas indices;
        digitalWrite(LED_VERMELHO, HIGH);
        return indices;
    }   
}
bool sensoresBegin()
{
    pinMode(LED_VERMELHO, OUTPUT);
    anemometroBegin();
    //inicializar modulos e sensores
    globoNegro.begin();
    bulboSeco.begin();
    bulboUmido.begin();
    if(!htu21d.begin())
    {
        temporarias.erroSensorUmidade2 = true;
    }
    if (!bmp.begin(0x76))
    {
        /* Default settings from datasheet. sensor de pressão bmp280 */
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
        
        return true;
    }else return false;
}