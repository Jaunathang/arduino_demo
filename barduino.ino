
/*  Getting_BPM_to_Monitor prints the BPM to the Serial Monitor, using the least lines of code and PulseSensor Library.
    Tutorial Webpage: https://pulsesensor.com/pages/getting-advanced

  --------Use This Sketch To------------------------------------------
  1) Displays user's live and changing BPM, Beats Per Minute, in Arduino's native Serial Monitor.
  2) Print: "♥  A HeartBeat Happened !" when a beat is detected, live.
  2) Learn about using a PulseSensor Library "Object".
  4) Blinks LED on PIN 13 with user's Heartbeat.
  --------------------------------------------------------------------*/

#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math.
#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library.   

// Variables capteur cardiaque
const int PulseWire = 0;       // PulseSensor PURPLE WIRE connected to ANALOG PIN 0
const int LED13 = 13;          // The on-board Arduino LED, close to PIN 13.
int Threshold = 550;           // Determine which Signal to "count as a beat" and which to ignore. Leave the default "550" value.
int BPM_final; //bpm de l'utilisateur après enregistrement

PulseSensorPlayground pulseSensor;  // Creates an instance of the PulseSensorPlayground object called "pulseSensor"

// Variables
#define PIN_VITESSE 7
short etat = 0;
String etatString[3] = {"ATTENTE", "LECTURE DE BPM ET CALCUL DES DOSES", "DISTRIBUTION"};
bool changementEtat = false;
int totalTemps;

// Variables timer
#define TEMPS_LECTURE 1000
unsigned long tempsDepart = 0;

// Struct
struct Moteur {
  String nom;
  int BPMmin, BPMmax, pin, ledPin, temps, tempsMin, tempsMax;
  bool working;
};
Moteur m1 = { .nom = "Rhum blanc", .BPMmin = 50, .BPMmax = 100, .pin = 2, .ledPin = 12, .temps = 0, .tempsMin = 20, .tempsMax = 0, .working = false};
Moteur m2 = { .nom = "Limonade", .BPMmin = 75, .BPMmax = 125, .pin = 3, .ledPin = 11, .temps = 0, .tempsMin = 0, .tempsMax = 50, .working = false};
Moteur m3 = { .nom = "Rhum brun", .BPMmin = 100, .BPMmax = 150, .pin = 4, .ledPin = 10, .temps = 0, .tempsMin = 20, .tempsMax = 0, .working = false};
Moteur m4 = { .nom = "Soda", .BPMmin = 125, .BPMmax = 175, .pin = 5, .ledPin = 9, .temps = 0, .tempsMin = 0, .tempsMax = 50, .working = false};
#define NB_MOTEURS 4
struct Moteur arrayMoteurs[NB_MOTEURS] = {m1, m2, m3, m4};

void setup() {
  pinMode(m1.pin , OUTPUT);
  pinMode(m2.pin , OUTPUT);
  pinMode(m3.pin , OUTPUT);
  pinMode(m4.pin , OUTPUT);
  pinMode(PIN_VITESSE, OUTPUT);
  pinMode(m1.ledPin, OUTPUT);
  pinMode(m2.ledPin, OUTPUT);
  pinMode(m3.ledPin, OUTPUT);
  pinMode(m4.ledPin, OUTPUT);

  // Initialisation capteur cardiaque
  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED13);       //auto-magically blink Arduino's LED with heartbeat.
  pulseSensor.setThreshold(Threshold);
  // Double-check the "pulseSensor" object was created and "began" seeing a signal.
  if (pulseSensor.begin()) {
    Serial.println("Capteur de battement initialisé");
  }

  Serial.begin(9600);
}

void loop() {

  affichageEtat(); //s'il y a changement d'état, cette fonction va l'afficher

  switch (etat) {
    case 0:
      // Vérifie si un beat est détecté, mettant fin à l'état d'attente.
      if (pulseSensor.getBeatsPerMinute()) {
        etat = 1;
        changementEtat = true;
        tempsDepart = millis(); //pour timer d'enregistrement
      };
      break;
    case 1:
      checkPulse();
      if (millis() - tempsDepart >= TEMPS_LECTURE) {
        lectureEtCalcul(checkPulse()); //lecture unique. checkPulse() retourne le nombre de BPM
        etat = 2;
        changementEtat = true;
        tempsDepart = millis(); //pour timer la distribution
      }
      break;
    case 2:
      if (distribution()) {
        etat = 0;
        changementEtat = true;
        reinitialisation();
      }
      break;
    default:
      Serial.println("Erreur switch etat");
      break;
  }
}

void affichageEtat() {
  if (changementEtat) {
    changementEtat = false;
    Serial.println(etatString[etat] + "...");
  }
}

int checkPulse() {
  int myBPM = pulseSensor.getBeatsPerMinute();
  if (pulseSensor.sawStartOfBeat()) {            // Constantly test to see if "a beat happened".
    Serial.print("BPM actuel : ");                        // Print phrase "BPM: "
    Serial.println(myBPM);
    return myBPM;
  }
  //EVENTUELLEMENT RAJOUTER UNE PROTECTION SI PLUS DE LECTURE APRES 20 SECONDES
}

void lectureEtCalcul(int myBPM) {
  BPM_final = myBPM;
  Serial.print("Terminé! Votre poul est de ");
  Serial.print(BPM_final);
  Serial.println(" battements par minute ♥");
  calculTemps();
}

void calculTemps() {
  for (int i = 0; i < NB_MOTEURS; i++) {
    struct Moteur mTemp = arrayMoteurs[i];
    int temps;
    if (BPM_final >= mTemp.BPMmin && BPM_final <= mTemp.BPMmax) {
      // *1000 car on veut le temps en millisecondes
      temps = map(BPM_final, mTemp.BPMmin, mTemp.BPMmax, mTemp.tempsMin, mTemp.tempsMax);
    } else {
      //si on est pas dans le range du moteur concernée, on met le temps à directement à 0.
      temps = 0;
    }
    arrayMoteurs[i].temps = temps * 1000;
    totalTemps += temps;
    Serial.print("Le ");
    Serial.print(mTemp.nom);
    Serial.print(" coulera pendant : ");
    Serial.print(temps);
    Serial.println(" seconde(s)");
  }
}

bool distribution() {
  bool distribution_done = false;
  short nbMoteursTermines = 0;
  for (int i = 0; i < NB_MOTEURS; i++) {
    struct Moteur mTemp = arrayMoteurs[i];
    if (millis() - tempsDepart < mTemp.temps) {
      digitalWrite(mTemp.ledPin, HIGH);
      mTemp.working = true;
    } else {
      digitalWrite(mTemp.ledPin, LOW);
      mTemp.working = false;
      nbMoteursTermines++;
    }
  }
  if (nbMoteursTermines == NB_MOTEURS) {
    distribution_done = true;
    Serial.println("La concentration de votre liquide est de ");
    for (int i = 0; i < NB_MOTEURS; i++) {
      struct Moteur mTemp = arrayMoteurs[i];
      Serial.print(mTemp.nom + ": ");
      Serial.print((float)(mTemp.temps / totalTemps));
      Serial.print("%   |   ");
    }
    Serial.println("\n Bonne dégustation! :)");
  }
  return distribution_done;
}

void reinitialisation() {
  totalTemps = 0;
  for (int i = 0; i < NB_MOTEURS; i++) {
    struct Moteur mTemp = arrayMoteurs[i];
    mTemp.temps = 0;
  }
}
