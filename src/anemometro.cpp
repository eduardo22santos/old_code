#include <anemometro.h>

Anemometro anemometro;

void contador()
{
  anemometroRun();
}

void anemometroBegin()
{
    pinMode(anemometroPin,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(anemometroPin),contador,FALLING);
}

float anemometroVelocidade_Ms()
{
    //necessario alterar aqui
    if (anemometro._tempoEntrePulso > 0)
    {
        return anemometro.circunferencia * (1000/anemometro._tempoEntrePulso);
    }else
    {
        return 0;
    }
}

void anemometroRun()
{
    unsigned long time = millis();
    anemometro._tempoEntrePulso = time - anemometro._ultimoPulsoMillis;
    anemometro._ultimoPulsoMillis = time;
}