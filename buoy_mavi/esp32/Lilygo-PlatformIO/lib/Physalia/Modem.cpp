#include "Modem.h"

Modem::Modem(){

    // Initialisation de modem dans la liste d'initialisation
    this->modem = new TinyGsm(SerialAT);
    this->modem->begin();
    // Initialisez ntripClient avec la méthode begin() après la création de l'objet modem
    ntripClient = TinyGsmClient(*this->modem, 2);}

Modem::~Modem() {
    delete this->modem;
}

void Modem::setup() {
    // Ajoutez le code d'initialisation supplémentaire si nécessaire
}

void Modem::process() {
    // Ajoutez le code de traitement supplémentaire si nécessaire
}

bool Modem::connected() {
    return this->ntripClient.connected();
}

// Correction de la méthode print
void Modem::print(const char *nmeaData) {
    ntripClient.print(nmeaData);
}
