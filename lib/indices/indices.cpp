/*
Projeto de Pesquisa: 	PII9027-2020 - Sistema embarcado para determinação remota de índices
de conforto térmico, com globo negro confeccionado em impressão 3D
Orientador: 	WELINGTON GONZAGA DO VALE
Centro: 	FUNDAÇÃO UNIVERSIDADE FEDERAL DE SERGIPE
Departamento: 	DEPARTAMENTO DE ENGENHARIA AGRÍCOLA
Cota: 	PIBITI 2020/2021 (01/08/2020 a 31/07/2021)

DESENVOLVIDO POR EDUARDO JOSÉ DOS SANTOS, TÉCNICO EM AGROPECUÁRIA E GRADUANDO EM ENGENHARIA AGRÍCOLA
NA UNIVERSIDADE FEDERAL DE SERGIPE
LATTES:  http://lattes.cnpq.br/6167567889414237
*/
#include <indices.h>
float VariaveisTermicas::calculaUmidadeRelativa(float tbs1, float tbu1, float pressaoBmp)
{
    /**lembrando que pressaoBmp está em Pa*/
    //pressão de saturação
    float esu = 6.1078 * pow(10, ((7.5*tbu1)/(237.3+tbu1)));
    float es = 6.1078 * pow(10, ((7.5*tbs1)/(237.3+tbs1)));
    //pressão real de vapor de água, para psicrômetros não aspirados
    float e = esu - (0.0008 * ((pressaoBmp)*(tbs1 - tbu1)));
    
    return (e/es)*100;
}
void VariaveisTermicas::zeraMaxMin()
{
    umidadeMax = umidadeMin = umidadeRelativa2;
    temperaturaMax = temperaturaMin = temperaturaDeBulboSeco;
    temperaturaDeBulboUmidoMax = temperaturaDeBulboUmidoMin = temperaturaDeBulboUmido;
    globoMax = globoMin = temperaturaDeGlobo;
    ituMax = ituMin = itu2;
    itguMax = itguMin = itgu2;
    orvalhoMax = orvalhoMin = pontoDeOrvalho2;
}
float VariaveisTermicas::calculaItu(Animal animal, bool tipoDeSensor, float umidadeRelativa)
{
    if (animal == geral)
    {
        return  (0.8 * temperaturaDeBulboSeco + ((umidadeRelativa * (temperaturaDeBulboSeco - 14.3))/ 100) + 46.3);
    }else if (animal == bovino)
    {
        return ((1.8*temperaturaDeBulboSeco) + 32) - ((0.55 - (0.55*(umidadeRelativa/100)))*(((1.8 * temperaturaDeBulboSeco) + 32) - 58));
    }else 
    {
        return  (0.8 * temperaturaDeBulboSeco + ((umidadeRelativa * (temperaturaDeBulboSeco - 14.3))/ 100) + 46.3);
    }

}
void VariaveisTermicas::atualizaVariaveis(DallasTemperature &globoNegro, DallasTemperature &bulboUmido, DallasTemperature &bulboSeco, HTU21D &htu21d, Animal animal, uint8_t LED_VERMELHO, Adafruit_BMP280 &bmp, bool tipoDeSensor, RTC_DS3231 &relogio)
{
        //Atualiza o horario
        horario = relogio.now();
        if (!horario.isValid())
        {
            erroRtc = true;
        }else
        {
            erroRtc = false;
        }      
        //atualiza os sensores de temperatura
        globoNegro.requestTemperatures();
        globoNegro.requestTemperatures();
        bulboSeco.requestTemperatures();
        bulboUmido.requestTemperatures();
        delay(750);
        //variaveis temporárias para nao passar leituras erradas
        float globo = globoNegro.getTempCByIndex(0);
        float tbs = bulboSeco.getTempCByIndex(0);
        float tbu = bulboUmido.getTempCByIndex(0);
        float pressao1 = bmp.readPressure();
        
        //Verifica se há erros nas leituras dos sensores ds18b20
        if(tbs == -127.0 || tbs == 85)
        {
            erroSensorTbs = true;
        }else
        {
            temperaturaDeBulboSeco = tbs;
            erroSensorTbs = false;
        }

        if(tbu == -127.0 || tbu == 85)
        {
            erroSensorUmidade = true;
        }else
        {
            temperaturaDeBulboUmido = tbu;
            erroSensorUmidade = false;
        }

        if(globo == -127.0 || globo == 85)
        {
            erroSensorGlobo = true;
        }else
        {
            temperaturaDeGlobo = globo;
            erroSensorGlobo = false;
        }
        
        //Verifica se há erro no sensor de pressão
        if (isnan(pressao1) == 0 && pressao1 > 1)
        {
            pressao = (pressao1 / 100);
            erroSensorPressao = false;
        }else
        {
            erroSensorPressao = true;
        }
        
        //Verifica se há erro na leitura do sensor htu21d
        float umidade = htu21d.readHumidity();
        if(umidade == 998.0)
        {
            erroSensorUmidade = true;
        }else
        {
            umidadeRelativa2 = (umidade > 98)? 98 : umidade;
            erroSensorUmidade = false;
        }
        
        
    
        //Calculos dos indices termicos
        if(!erroSensorGlobo && !erroSensorTbs && !erroSensorUmidade && !erroSensorPressao && !erroRtc)
        {
            digitalWrite(LED_VERMELHO, LOW);

                     
            umidadeRelativa1 = calculaUmidadeRelativa(tbs, tbu, pressao1);
            

            itu1 = calculaItu(animal, tipoDeSensor, umidadeRelativa1);
            itu2 = calculaItu(animal, tipoDeSensor, umidadeRelativa2);

            pontoDeOrvalho1 = (243.04*(log(umidadeRelativa1/100)+((17.625*temperaturaDeBulboSeco)/(243.04+temperaturaDeBulboSeco))))/(17.625-log(umidadeRelativa1/100)-((17.625*temperaturaDeBulboSeco)/(243.04+temperaturaDeBulboSeco)));
            pontoDeOrvalho2 = (243.04*(log(umidadeRelativa2/100)+((17.625*temperaturaDeBulboSeco)/(243.04+temperaturaDeBulboSeco))))/(17.625-log(umidadeRelativa2/100)-((17.625*temperaturaDeBulboSeco)/(243.04+temperaturaDeBulboSeco)));
            
            itgu1 = temperaturaDeGlobo + (0.36 * pontoDeOrvalho1) + 41.5;
            itgu2 = temperaturaDeGlobo + (0.36 * pontoDeOrvalho2) + 41.5;

            ibutg1 = (0.7*temperaturaDeBulboUmido) + (0.3*temperaturaDeGlobo);
            ibutg2 = (0.7*temperaturaDeBulboUmido) + (0.2*temperaturaDeGlobo) + (0.1*temperaturaDeBulboSeco);

            //Compara as maximas e minimas
            if (temperaturaDeBulboSeco >  temperaturaMax)
            {
                temperaturaMax = temperaturaDeBulboSeco;
            }else if (temperaturaDeBulboSeco < temperaturaMin)
            {
                temperaturaMin = temperaturaDeBulboSeco;
            }
            if (temperaturaDeBulboUmido >  temperaturaDeBulboUmidoMax)
            {
                temperaturaDeBulboUmidoMax = temperaturaDeBulboUmido;
            }else if (temperaturaDeBulboUmido < temperaturaDeBulboUmidoMin)
            {
                temperaturaDeBulboUmidoMin = temperaturaDeBulboUmido;
            }
            if (umidadeRelativa2 > umidadeMax)
            {
                umidadeMax = umidadeRelativa2;
            }else if (umidadeRelativa2 < umidadeMin)
            {
                umidadeMin = umidadeRelativa2;
            }
            if (itgu2 > itguMax)
            {
                itguMax = itgu2;
            }else if (itgu2 < itguMin)
            {
                itguMin = itgu2;
            }
            if (itu2 > ituMax)
            {
                ituMax = itu2;
            }else if (itu2 < ituMin)
            {
                ituMin = itu2;
            }
            if (temperaturaDeGlobo > globoMax)
            {
                globoMax = temperaturaDeGlobo;
            }else if (temperaturaDeGlobo < globoMin)
            {
                globoMin = temperaturaDeGlobo;
            }
            if (pontoDeOrvalho2 > orvalhoMax)
            {
                orvalhoMax = pontoDeOrvalho2;
            }else if (pontoDeOrvalho2 < orvalhoMin)
            {
                orvalhoMin = pontoDeOrvalho2;
            } 
        }else
        {
            digitalWrite(LED_VERMELHO, HIGH);
        }   
}