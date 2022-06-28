/*
    Autor: Tiago André Lachman
    Biblioteca com o objetivo de otimizar e simplificar o uso do Demultiplexador.
*/


#include <MuxClass.h>

Mux::Mux(//Construct do Demux
    int* pinInhEnables,
    int sizeInhEnables,
    int* pinControls,
    int sizeControls,
    int mode
    ){
    this -> inhEnables = pinInhEnables;
    this -> controls = pinControls;
    this -> numControls = sizeControls;
    this -> numInhEnables = sizeInhEnables;

    this -> potControls = pow(2, sizeControls);
    this -> numSaidas = potControls * sizeInhEnables - 1;

    this -> mode = mode;
    configurarPortas();
}

Mux::Mux(//Construct do Mux
    int* pinInhEnables,
    int sizeInhEnables,
    int* pinControls,
    int sizeControls,
    int pinCom,
    int mode
    ){
    this -> inhEnables = pinInhEnables;
    this -> controls = pinControls;
    this -> numControls = sizeControls;
    this -> numInhEnables = sizeInhEnables;

    this -> potControls = pow(2, sizeControls);
    this -> numSaidas = potControls * sizeInhEnables - 1;

    this -> common = pinCom;

    this -> mode = mode;
    configurarPortas();
}

int Mux::direcionar(int numOut){
    if(numOut < 0 || numOut > numSaidas) return NOT_IN_RANGE;

    bool statusControls[this->numControls];
    bool statusEnables[this->numInhEnables];

    for(int i=0;i<this->numInhEnables;i++){//Desliga todas as saídas
        statusEnables[i] = 1;
        digitalWrite(this->inhEnables[i], 1);
    }

    paraBin(numOut, statusControls, this->numControls);//Transforma o valor da saída requisitada em bits
    
    for(int i=1;i<=this->numInhEnables;i++){//Verifica qual CI do demux deverá ser acionado
        if(numOut < (i * this->potControls) && numOut >= ((i-1) * this->potControls)){
            statusEnables[i-1] = 0;
            break;
        } else if (i==this->numInhEnables){
            return DEMUX_MUX_UNKNOWN;
        }
    }
    for(int i = this->numControls-1; i >=0 ; i--){//Liga as portas de controle correspondente ao valor
        digitalWrite(this->controls[2-i], statusControls[i]);
    }
    for(int i = 0; i < this->numInhEnables; i++){//Manda ligar o CI correspondente ao valor
        digitalWrite(this->inhEnables[i], statusEnables[i]);
    }
    return  DEMUX_MUX_OK;//Função executada sem falhas
}

int Mux::desligarSaidas(){
    //Desliga todas os enables
    for(int i=0;i<this->numInhEnables;i++){
        digitalWrite(this->inhEnables[i], 1);
    }
    //Desliga todos os controls
    for(int i=0;i<this->numControls;i++){
        digitalWrite(this->controls[i], 0);
    }
    return DEMUX_MUX_OK;
}

int Mux::lerEntrada(int numOut){
    if(this->mode != MUX) return WRONG_MODE;
    if(numOut < 0 || numOut > this->numSaidas) return NOT_IN_RANGE;

    this->direcionar(numOut);
    return digitalRead(this->common);
    /*
    int i=1;
    //Verifica qual CI do demux deverá ser acionado
    for(i = 1;i<=this->numInhEnables;i++){
        if(numOut < (i * this->potControls) && numOut >= ((i-1) * this->potControls)){
            i--;
            break;
        } else if (i==this->numInhEnables){
            return DEMUX_MUX_UNKNOWN;
        }
    }
    return digitalRead(this->common[i]);
    */
}

int Mux::maxSaidas(){
    return this->numSaidas;
}

int Mux::showMode(){
    return this->mode;
}

void Mux::configurarPortas(){
    int i=0;

    #if defined (ESP32) || (STM32)
    pinMode(this->common, INPUT_PULLDOWN);
    #else 
    pinMode(this->common, INPUT);
    #endif

    for(i=0;i<this->numControls;i++){
        pinMode(this->controls[i], OUTPUT);
    }
    for(i=0;i<this->numInhEnables;i++){
        pinMode(this->inhEnables[i], OUTPUT);
    }
    
}