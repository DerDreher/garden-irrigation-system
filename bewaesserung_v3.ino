#include "arduino_secrets.h"
/*  ------ Einstellungen für den Upload/Compiler --------
    Board: Generic ESP8266 Module
    Builtin LED: 0
    Upload Speed: 115200
    CPU Frequency: 80 MHz
    Crystal Frequency: 26 MHz
    Flash Size: 4096kB
    Flash Mode: DOUT
    Flash Frequency: 40 MHz
    Reset Method: dtr (nodemcu)
    Debug Port: Disabled
    Debug Level: Keine
    lwIP Variant: v2 Lower Memory
    VTables: Flash
    Exceptions: Legacy
    Erase Flash: Only Sketch
    Espressif FW: nonos-sdk
    SSL Support: All SSL ciphers
    Port: Entsprechend deinem USB-UART Adapter (siehe Windows Gerätemanager bzw. Linux Befehl "dmesg")
    Programmer: spielt keine Rolle, es wir immer ein USB-UART verwendet


    ToDo
    - Offset Wert beim Erstellen eines neuen Plans einfügen
*/
#include <ArduinoOTA.h>

// ArduinoJson - Version: 5.13.0
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <CircularBuffer.h>
// https://github.com/rlogiacco/CircularBuffer

#include <DallasTemperature.h>

#include <EEPROM.h>

#include <ESP8266WiFi.h>

#include <ESP8266mDNS.h>

#include <NTPClient.h>
// https://github.com/arduino-libraries/NTPClient

#include <OneWire.h>

#include <TimeLib.h>
// https://github.com/PaulStoffregen/Time

#include <UniversalTelegramBot.h>
// https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot

#include <WiFiClientSecure.h>

#include <WiFiUdp.h>
//https://github.com/arduino-libraries/NTPClient

//Beschreibung für die einzelnen Kreise
#define DEF_KREIS1 "Rasen"
#define DEF_KREIS2 "Beet\/Straeucher"
#define DEF_KREIS3 "Acker"
#define DEF_KREIS4 "Hochbeet\/Tomaten"

// Für den Temperatursensor
#define ONE_WIRE_BUS 4                          // GPIO des ESP
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);            // Pass our oneWire reference to Dallas Temperature.

// Einstellung für den Ringpuffer
CircularBuffer < int, 10 > buffer;
unsigned long ringpuffer_startzeit = 0;
int ringpuffer_sperrzeit = 0;
int textint;
int aktuelle_ringbuffer_id = 0;

// Initialize Wifi connection to the router
char ssid[] = SECRET_WLAN_SSID;                      // your network SSID (name)
char pass[] = SECRET_WLAN_PASS;                      // your network key
WiFiClientSecure client;

// Initialize Telegram BOT
#define TELEGRAM_BOT_TOKEN SECRET_TELEGRAM_TOKEN
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, client);
int delayBetweenChecks = 1000;                  //in ms
unsigned long lastTimeChecked;                  //last time messages' scan has been done
int SchrappeChat  = atoi(SECRET_TELEGRAM_FAMILYCHAT); //Familychat
int AdminID       = atoi(SECRET_TELEGRAM_ADMIN1);     //Erwin
int AdminID2      = atoi(SECRET_TELEGRAM_ADMIN2);     //Kathrin

// Definiere die GPIO der Ventile, Regensensor, Spannungsmessung
const int VentilePin1 = 15;
const int VentilePin2 = 14;
const int VentilePin3 = 12;
const int VentilePin4 = 13;
const int RegensensorPin = 5;
const int analogInput = 0;
// #define ONE_WIRE_BUS 4 // siehe Zeile 42

// Alles für die Spannungsmessung
float vout = 0.0;
float vin = 0.0;
float R1 = 180000.0; // Widerstandswert R1 (180K) - siehe Schaltskizze!
float R2 = 13000.0; // Widerstandswert R2 (13K) - siehe Schaltskizze!
int values = 0;

// Einstellungen für die Internetzeit Abfrage
//Zeitverschiebung UTC <-> MEZ (Winterzeit) = 3600 Sekunden (1 Stunde)
//Zeitverschiebung UTC <-> MEZ (Sommerzeit) = 7200 Sekunden (2 Stunden)
const long utcOffsetInSeconds = 3600;
const long updaterateNTCServer = 3600000; // in ms, 3600000 entspricht 1h
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, updaterateNTCServer);

// Einstellungen für den EEPROM
int eeprom_address = 50;
int bewaesserungsdauer[4];
int regensensor_ram;

// Einstellungen für den CronJob
byte cron_letzte_minute = 61; // 61 weil es keine Minute 61 gibt -> erster Durchlauf garantierte Prüfung

// Einstellungen für den DeepSleep
int deepsleep_schlafzeit                = 40;   // in Sekunden, maximal 255 (da Byte definition), Standart = 40
byte deepsleep_schlafzeit_winter        = 240;  // in Sekunden, maximal 255 (da Byte definition)
unsigned long deepsleep_timeoutzeit     = 0;
byte Telegramm_DeepSleep_Stop           = 3;  // in Minuten

// Einstellungen für das 50Hz Rechtecksignal
byte      pwm_ventil  = 0;            // Temporäres zwischenspeichern vom aktiven Ventil

// Einstellung für OTA Updates
bool ota_aktiv                  = false;    // Standartmäßig inaktiv
unsigned long ota_aktive_zeit   = 300;      // in Sekunden

// Einstellungen fürs debug
bool aktiv_debug        = false;  // wird durchs Programm selber gesteuert.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Start VOID'S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void debugmsg(String text) {
  if (aktiv_debug) {
    bot.sendMessage(String(AdminID), text, "Markdown");
  }
}

boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
  if (month < 3 || month > 10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if (month == 3 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7)) || month == 10 && (hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7)))
    return true; // Summertime
  else
    return false;// Wintertime
}

void schlafen(byte tmp_timeout = 0, bool stopnow = false) {
  // tmp_timeout  wird in min angegeben, gibt die Zeit an wie lange der DeepSleep blockiert wird.
  // stopnow    false | true  Wenn true überschreibt tmp_timeout den aktuellen Counterwert.
  // zusätzlich wird eine halbe Minute dazu addiert
  if (tmp_timeout > 0 && (((tmp_timeout + 0.5) * 60000) + millis()) > deepsleep_timeoutzeit) {
    deepsleep_timeoutzeit = ((tmp_timeout + 0.5) * 60000) + millis();
    Serial.print("Neue Timeoutzeit groeßer als die alte, setzte Neue Zeit auf ");
    Serial.print(tmp_timeout);
    Serial.println(" Sekunden.");
  }
  if (tmp_timeout > 0 && stopnow) {
    deepsleep_timeoutzeit = ((tmp_timeout + 0.5) * 60000) + millis();
    Serial.print("Deepsleep Timer gelöscht und neue Zeit auf ");
    Serial.print(tmp_timeout);
    Serial.println(" Minuten gesetzt.");
  }
  if (deepsleep_timeoutzeit > millis()) {
    // mach nix
  }
  else {
    Serial.print("ESP schläft für ");
    Serial.print(deepsleep_schlafzeit);
    Serial.println(" Minuten ein.(DeepSleep)");

    //dirty Debug
    if (aktiv_debug) {
      String debugstring = "ESP schläft für ";
      debugstring += deepsleep_schlafzeit;
      debugstring += " Sekunden ein.(DeepSleep)";
      bot.sendMessage(String(AdminID), debugstring, "Markdown");
    }

    ESP.deepSleep(deepsleep_schlafzeit * 1e6); // DeepSleep
    delay(100);
  }
}

void cronjob_check() {
  // Gleicht die Aktuelle Zeit mit den gespeicherten Plänen aus dem EEPROM ab und löst
  // bei Übereinstimmung einen Job über die Warteschlange aus.

  for (int i = 0; i < 11; i++) {
    byte kreis      = EEPROM.read((eeprom_address + 6 + (i * 5)));   // Kreis
    byte hh         = EEPROM.read((eeprom_address + 7 + (i * 5)));   // hh
    byte mm         = EEPROM.read((eeprom_address + 8 + (i * 5)));   // mm
    byte intervall  = EEPROM.read((eeprom_address + 9 + (i * 5)));   // intervall
    byte offset_d   = EEPROM.read((eeprom_address + 10 + (i * 5)));  // offset_d

    if ( timeClient.getMinutes() == mm && timeClient.getHours() == hh && ((timeClient.getEpochTime() / 86400) + offset_d) % intervall == 0 && kreis != 0) {
      buffer.push(kreis);
      Serial.print("Kreis ");
      Serial.print(kreis);
      Serial.println(" wurde vom Cronjob zur Warteschlange hinzugefügt");
    }
    else {
      // keine Übereinstimmung, nüscht zu tun
    }
  }
}

void cronjob_lade_daten() {
  // lädt gespeicherte Programme aus dem EEPROM strukturiert in den RAM
  regensensor_ram = EEPROM.read(eeprom_address);
  bewaesserungsdauer[0] = EEPROM.read(eeprom_address + 1);
  bewaesserungsdauer[1] = EEPROM.read(eeprom_address + 2);
  bewaesserungsdauer[2] = EEPROM.read(eeprom_address + 3);
  bewaesserungsdauer[3] = EEPROM.read(eeprom_address + 4);
}

int cronjob_speichere_daten(byte plannummer, byte Kreis, byte hh, byte mm, byte intervall, String chat_id) {
  // Speichert Daten/Jobs aus dem Telegram Chat in den EEPROM und lädt zum Schluss die neuen
  // Daten, über "cronjob_lade_daten ()", in den RAM
  byte aktuellerIndex = EEPROM.read(eeprom_address + 5);
  byte offset_d = 0;
  bool fehler = false;

  for (int i = 0; i < 31; i++) {
    if ( timeClient.getMinutes() == mm && timeClient.getHours() == hh && ((timeClient.getEpochTime() / 86400) + i ) % intervall == 0 ) {
      offset_d = i;
      i = 31;
    }
  }
  if (plannummer >= 0 && plannummer < 10) {
    EEPROM.write((eeprom_address + 5), plannummer + 1);
  } else {
    fehler = true;
  }
  if (Kreis >= 0 && Kreis < 5 && !fehler) {
    EEPROM.write((eeprom_address + 6 + (plannummer * 5)), Kreis);
  } else {
    fehler = true;
  }
  if (hh >= 0 && hh < 24 && !fehler) {
    EEPROM.write((eeprom_address + 7 + (plannummer * 5)), hh);
  } else {
    fehler = true;
  }
  if (mm >= 0 && mm < 60 && !fehler) {
    EEPROM.write((eeprom_address + 8 + (plannummer * 5)), mm);
  } else {
    fehler = true;
  }
  if (intervall >= 0 && intervall < 31 && !fehler) {
    EEPROM.write((eeprom_address + 9 + (plannummer * 5)), intervall);
  } else {
    fehler = true;
  }
  EEPROM.write((eeprom_address + 10 + (plannummer * 5)), offset_d);

  if (!fehler) {
    EEPROM.commit();
    bot.sendMessage(chat_id, "Gespeichert!\n\n/bewaesserungsplan\n\n/einstellungen", "Markdown");
  } else {
    bot.sendMessage(chat_id, "Fehler, Versuche es nochmal!\n\n/bewaesserungsplan\n\n/einstellungen\n", "Markdown");
  }
}

int lade_Bewaesserungsdauer(int dauerid) {
  // ausm RAM
  return bewaesserungsdauer[dauerid - 1];
}

float printBuffer() {
  if (buffer.isEmpty()) {
    Serial.println("buffer empty");
  } else {
    Serial.print("[");
    for (decltype(buffer)::index_t i = 0; i < buffer.size() - 1; i++) {
      Serial.print(buffer[i]);
      Serial.print(",");
    }
    Serial.print(buffer[buffer.size() - 1]);
    Serial.print("] (");

    Serial.print(buffer.size());
    Serial.print("/");
    Serial.print(buffer.size() + buffer.available());
    if (buffer.isFull()) {
      Serial.print(" full");
    }

    Serial.println(")");
  }
}
// Funktionen für die NewsMessages (Auslagerung aus der Haupt Funktion)
void msg_regensensor1 (String chat_id) {
  EEPROM.write((eeprom_address), 1);
  EEPROM.commit();
  cronjob_lade_daten();
  Serial.println("Regensensor Aktiviert");
  bot.sendMessage(chat_id, "Regensensor aktiviert");
}
void msg_regensensor0 (String chat_id) {
  EEPROM.write((eeprom_address), 0);
  EEPROM.commit();
  cronjob_lade_daten();
  Serial.println("Regensensor deaktiviert");
  bot.sendMessage(chat_id, "Regensensor deaktiviert.");
}
void msg_otaan (String chat_id) {
  if (!ota_aktiv) { //Aktivierung OTA Funktion wenn noch nicht aktiviert.
    ArduinoOTA.setPassword((const char *)SECRET_OTA_PASS);

    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
  }
  ota_aktiv = true;
  schlafen(5); // Deepsleep für 5min blockieren
  ota_aktive_zeit = millis() + 300000;
  Serial.println("OTA für 5min aktiviert");
  bot.sendMessage(chat_id, "OTA für 5min aktiviert.");
}
void msg_stop (String chat_id) {
  buffer.clear();
  ringpuffer_sperrzeit = 0;
  schlafen(Telegramm_DeepSleep_Stop, true); // DeepSleep Timer auf "Telegramm_DeepSleep_Stop" Zeit einstellen, egal was aktuell läuft
  Serial.println("Alle aktuellen Bewässerungen gestoppt.");
  bot.sendMessage(chat_id, "Alle aktuellen Bewässerungen gestoppt.");
}
void msg_kreisan (String chat_id, String text) {
  text.replace("KREISAN", "");
  textint = text.toInt();
  buffer.push(textint);
  Serial.println(textint);
  Serial.println("Kreis " + text + " wurde in die Warteschlange aufgenommen.");
  bot.sendMessage(chat_id, "Kreis " + text + " wurde in die Warteschlange aufgenommen.");
}
void msg_werkseinstellung (String chat_id) {
  for (int i = 0; i <= 56; i++) {
    EEPROM.write((eeprom_address + i), 0);
    EEPROM.commit();
  }
  bot.sendMessage(chat_id, "Alle Speichereinträge gelöscht!\n\n/status\n\n/einstellungen\n\n/handsteuerung", "Markdown");
  schlafen(Telegramm_DeepSleep_Stop, true);
}
void msg_dauerkreis (String chat_id, String text) { // inputstring  "/dauerkreis5zeit20"
  bot.sendMessage(chat_id, text);
  Serial.println(text);

  char textchar[20];
  text.toCharArray(textchar, 20);
  int addr = getIntFromString(textchar, 1);
  int val = getIntFromString(textchar, 2);
  Serial.println(addr);
  Serial.println(val);
  Serial.println(addr + eeprom_address);

  if (addr > 0 && addr < 5 && val >= 0 && val < 256) {
    EEPROM.write((addr + eeprom_address), val);
    EEPROM.commit();
    cronjob_lade_daten();
    bot.sendMessage(chat_id, "Gespeichert!\n\n/status\n\n/einstellungen\n\n/handsteuerung", "Markdown");
  } else {
    bot.sendMessage(chat_id, "Fehler Versuche es nochmal!\n\n/status\n\n/einstellungen\n\n/handsteuerung", "Markdown");
    //bot.sendMessage(chat_id, addr);
  }
}
void msg_status (String chat_id) {
  //Abfrage Regensensor
  String regensensor_anzeige;
  if (!regensensor_ram) {
    regensensor_anzeige = "\xf0\x9f\x9b\x91\n";
  } else {
    if (digitalRead(RegensensorPin)) {
      regensensor_anzeige = "\xf0\x9f\x8c\x82\n";
    } else {
      regensensor_anzeige = "\xe2\x98\x94\xef\xb8\x8f\n";
    }
  }
  String statusstring = "Aktuelle Werte:\n\n";

  for (int j = 1; j < 5; j++) {
    if (j == aktuelle_ringbuffer_id) {
      statusstring += "\xf0\x9f\x9f\xa2 Kreis ";
      statusstring += j;
      statusstring += " läuft noch ";
      statusstring += (ringpuffer_startzeit + ringpuffer_sperrzeit - millis()) / 60000;
      statusstring += ":";
      statusstring += (ringpuffer_startzeit + ringpuffer_sperrzeit - millis()) % 60000 / 1000;
      statusstring += "min \n";
    } else {
      bool buffer_fund;
      if (buffer.isEmpty()) {
        buffer_fund = 0;
      }
      else {
        for (decltype(buffer)::index_t k = 0; k < buffer.size(); k++) {
          if (j == buffer[k]) {
            buffer_fund = 1;
            k = buffer.size();
          } else {
            buffer_fund = 0;
          }
        }
      }
      if (buffer_fund) {
        statusstring += "\xf0\x9f\x9f\xa1 Kreis ";
        statusstring += j;
        statusstring += " in Warteschlange\n";
      } else {
        statusstring += "\xF0\x9F\x94\xB4 Kreis ";
        statusstring += j;
        statusstring += "\n";
      }
    }
  }
  statusstring += "\n";
  statusstring += "Temperatur:       ";
  statusstring += temperatur_messen();
  statusstring += "°C\n";
  statusstring += "Batteriespannung: ";
  statusstring += spannung_messen();
  statusstring += "V\n";
  statusstring += "Regensensor:      ";
  statusstring += regensensor_anzeige;
  long rssi = WiFi.RSSI();
  statusstring += "WiFi Signalstärke: ";
  statusstring += rssi;
  statusstring += "dBm\n 0 = Perfekt, -85 = sehr schlecht\n ";
  statusstring += "\nzurück zu /start";
  bot.sendMessage(chat_id, statusstring, "Markdown");
}

void msg_einstellungen(String chat_id) {
  bot.sendMessage(chat_id, "Folgende Möglichkeiten:\n\n/bewaesserungsdauer\n\n/bewaesserungsplan\n\n/regensensor\n\n / werkseinstellung\n\n /otaupdate\n\noder zurück zu /start", "Markdown");
}

void msg_handsteuerung(String chat_id) {
  String handsteuerungJson = F("[[{ \"text\" : \"Kreis 1\", \"callback_data\" : \"KREISAN1\" },");
  handsteuerungJson += F("{ \"text\" : \"Kreis 2\", \"callback_data\" : \"KREISAN2\" },");
  handsteuerungJson += F("{ \"text\" : \"Kreis 3\", \"callback_data\" : \"KREISAN3\" },");
  handsteuerungJson += F("{ \"text\" : \"Kreis 4\", \"callback_data\" : \"KREISAN4\" },");
  handsteuerungJson += F("{ \"text\" : \"Stop\", \"callback_data\" : \"STOP\" }]]");
  bot.sendMessageWithInlineKeyboard(chat_id, "Startet ein Bewässerungsprozess von Hand. Diese hält nach der eingestellten Zeit selbstständig an. Zusätzlich gibt es eine Hand-Stop Funktion die alle Kreise sofort stopt(NOTAUS).\n\nKreis 1 = "DEF_KREIS1"\nKreis 2 = "DEF_KREIS2"\nKreis 3 = "DEF_KREIS3"\nKreis 4 = "DEF_KREIS4"\n\n oder zurück zu /start", "", handsteuerungJson);
}

void msg_otaupdate(String chat_id) {
  String otaupdateJson = F("[[{ \"text\" : \"OTA aktivieren\", \"callback_data\" : \"OTAAN\" }]]");
  bot.sendMessageWithInlineKeyboard(chat_id, "Startet die \"Over The Air\" Funktion für 5min bzw. bis zum nächsten Neustart.\n\n oder zurück zu /start", "", otaupdateJson);
}

void msg_bewaesserungsdauer(String chat_id) {
  String bewaesserungsdauerstring = "Aktuelle Einstellungen sind:\n\n";
  bewaesserungsdauerstring += "-Kreis 1 ("DEF_KREIS1") -> ";
  bewaesserungsdauerstring += bewaesserungsdauer[0];
  bewaesserungsdauerstring += "min\n-Kreis 2 ("DEF_KREIS2") -> ";
  bewaesserungsdauerstring += bewaesserungsdauer[1];
  bewaesserungsdauerstring += "min\n-Kreis 3 ("DEF_KREIS3") -> ";
  bewaesserungsdauerstring += bewaesserungsdauer[2];
  bewaesserungsdauerstring += "min\n-Kreis 4 ("DEF_KREIS4") -> ";
  bewaesserungsdauerstring += bewaesserungsdauer[3];
  bewaesserungsdauerstring += "min\n\nWenn du eine Bewässerungsdauer ändern möchtest must du wie im Beispiel folgendes Eingeben. Kreis 5 soll auf 20 min geändert werden, also \n /dauerkreis5zeit20\n\n";
  bewaesserungsdauerstring += "hier geht es zurück zu den \n/einstellungen";
  bot.sendMessage(chat_id, bewaesserungsdauerstring, "Markdown");
}

void msg_bewaesserungsplan(String chat_id) {
  bot.sendMessage(chat_id, "Lade Daten...", "Markdown");
  String bewaesserungsplanstring = "Aktuelle Bewässerungspläne sind:\n\n";

  for (int i = 0; i < 10; i++) {
    byte kreis      = EEPROM.read((eeprom_address + 6 + (i * 5)));   // Kreis
    byte hh         = EEPROM.read((eeprom_address + 7 + (i * 5)));   // hh
    byte mm         = EEPROM.read((eeprom_address + 8 + (i * 5)));   // mm
    byte intervall  = EEPROM.read((eeprom_address + 9 + (i * 5)));   // intervall
    byte offset_d   = EEPROM.read((eeprom_address + 10 + (i * 5)));  // offset_d

    if (kreis != 0) {
      bewaesserungsplanstring += "Plan";
      bewaesserungsplanstring += i;
      bewaesserungsplanstring += ", Kreis";
      bewaesserungsplanstring += kreis;
      bewaesserungsplanstring += ", ";
      bewaesserungsplanstring += hh;
      bewaesserungsplanstring += ":";
      if (mm < 10) {
        bewaesserungsplanstring += "0";
      }
      bewaesserungsplanstring += mm;
      bewaesserungsplanstring += " Uhr jeden ";
      if (intervall == 0 || intervall == 1) {
        //zeige nichts zusätzliches an
      }
      else {
        bewaesserungsplanstring += intervall;
        bewaesserungsplanstring += ".";
      }
      bewaesserungsplanstring += "Tag\n";
    } else {
      bewaesserungsplanstring += "Plan";
      bewaesserungsplanstring += i;
      bewaesserungsplanstring += " - nicht aktiv\n";
    }
  }

  bewaesserungsplanstring += "\nEin Beispiel. Auf Plan 14 soll Kreis1 morgens um 5:00 Uhr jeden 2. Tag ausgeführt werden. Dann Tippe folgenden Befehl:\n";
  bewaesserungsplanstring += "/plan14kreis1start5:00jeden2tag \noder kurz \n/plan14-1-5-0-2\n\n";
  bewaesserungsplanstring += "hier geht es zurück zu den \n/einstellungen";

  bot.sendMessage(chat_id, bewaesserungsplanstring, "Markdown");
}

void msg_plan(String chat_id, String text) {
  // inputstring  "/plan14kreis1start5:00jeden2tag" oder kurz "/plan14-1-5-0-2"
  bot.sendMessage(chat_id, text);
  Serial.println(text);
  char textchar2[40];
  text.toCharArray(textchar2, 40);
  /*
    int plannummer    = getIntFromString(textchar2, 1);
    int plankreis     = getIntFromString(textchar2, 2);
    int plan_hh       = getIntFromString(textchar2, 3);
    int plan_mm       = getIntFromString(textchar2, 4);
    int planintervall = getIntFromString(textchar2, 5);
  */
  cronjob_speichere_daten(getIntFromString(textchar2, 1), getIntFromString(textchar2, 2), getIntFromString(textchar2, 3), getIntFromString(textchar2, 4), getIntFromString(textchar2, 5), chat_id);
}

void msg_regensensor(String chat_id) {
  String regensensorJson = F("[[{ \"text\" : \"AN\", \"callback_data\" : \"REGENSENSOR1\" },{ \"text\" : \"AUS\", \"callback_data\" : \"REGENSENSOR0\" }]]");
  bot.sendMessageWithInlineKeyboard(chat_id, "Regensensor an oder abschalten\n\n hier geht es zurück zu den /einstellungen", "", regensensorJson);
}

void msg_debug(String chat_id) {
  if (aktiv_debug) {
    aktiv_debug = false;
    bot.sendMessage(chat_id, "Debugmodus deaktivert.", "Markdown");
  }
  else {
    bot.sendMessage(chat_id, "Lade Daten...", "Markdown");
    String debugstring = "**Pufferdaten:**\n";
    if (buffer.isEmpty()) {
      debugstring += "Buffer leer";
    } else {
      debugstring += "[";
      for (decltype(buffer)::index_t i = 0; i < buffer.size() - 1; i++) {
        debugstring += buffer[i];
        debugstring += ",";
      }
      debugstring += buffer[buffer.size() - 1];
      debugstring += "] (";
      debugstring += buffer.size();
      debugstring += "/";
      debugstring += buffer.size() + buffer.available();
      if (buffer.isFull()) {
        debugstring += " full";
      }
      debugstring += ")";
    }
    debugstring += "\n\n**Deepsleep Countdown:**\n";
    debugstring += (deepsleep_timeoutzeit - millis()) / 1000;
    debugstring += " Sekunden \n\n**Internetzeit:**\n";
    if (deepsleep_schlafzeit == deepsleep_schlafzeit_winter) {
      debugstring += "Winterzeit\n";
    }
    else {
      debugstring += "Sommerzeit\n";
    }
    debugstring += timeClient.getFormattedTime();
    debugstring += " Uhr\n";
    debugstring += day(timeClient.getEpochTime());
    debugstring += ".";
    debugstring += month(timeClient.getEpochTime());
    debugstring += ".";
    debugstring += year(timeClient.getEpochTime());
    debugstring += "\n\n**OTA Funktion:**\n";
    if (ota_aktiv) {
      debugstring += "Aktiv";
    } else {
      debugstring += "Inaktiv";
    }
    debugstring += "";
    bot.sendMessage(chat_id, debugstring, "Markdown");
    aktiv_debug = true;
  }
}

void msg_restart(String chat_id) {
  debugmsg("Restart ESP");
  //ESP.restart(); //Hier gibts noch irgend einen Fehler ....
}

void handleNewMessages(int numNewMessages) {

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    if (chat_id == String(SchrappeChat) || chat_id == String(AdminID) || chat_id == String(AdminID2)) {

      // If the type is a "callback_query", a inline keyboard button was pressed
      if (bot.messages[i].type == F("callback_query")) {
        String text = bot.messages[i].text;
        String chat_id = String(bot.messages[i].chat_id);
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////    Buttons   /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (text == F("REGENSENSOR1"))        msg_regensensor1(chat_id);
        else if (text == F("REGENSENSOR0"))   msg_regensensor0(chat_id);
        else if (text == F("OTAAN"))          msg_otaan(chat_id);
        else if (text == F("STOP"))           msg_stop(chat_id);
        else if (text.startsWith("KREISAN"))  msg_kreisan(chat_id, text);
      }
      else {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;

        // When a user first uses a bot they will send a "/start" command
        // So this is a good place to let the users know what commands are available
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////     Texte    /////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (text == F("/start")) {

          bot.sendMessage(chat_id, "Willkommen im Hauptmenü!\n\n/status\n\n/einstellungen\n\n/handsteuerung", "Markdown");

        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (text == F("/werkseinstellung"))   msg_werkseinstellung(chat_id);
        if (text.startsWith("/dauerkreis"))   msg_dauerkreis(chat_id, text);
        if (text == F("/status"))             msg_status(chat_id);
        if (text == F("/einstellungen"))      msg_einstellungen(chat_id);
        if (text == F("/handsteuerung"))      msg_handsteuerung(chat_id);
        if (text == F("/otaupdate"))          msg_otaupdate(chat_id);
        if (text == F("/bewaesserungsdauer")) msg_bewaesserungsdauer(chat_id);
        if (text == F("/bewaesserungsplan"))  msg_bewaesserungsplan(chat_id);
        if (text.startsWith("/plan"))         msg_plan(chat_id, text);
        if (text == F("/regensensor"))        msg_regensensor(chat_id);
        if (text == F("/debug"))              msg_debug(chat_id);
        if (text == F("/restart"))            msg_restart(chat_id);
      }
    }
    else {
      bot.sendMessage(chat_id, "Du verfügst nicht über die nötigen Rechte den Bot zu steuern.", "Markdown");
    }
  }
}

int getIntFromString(char * stringWithInt, int num) {
  char * tail;
  while (num > 0) {
    num--;
    while ((!isdigit( * stringWithInt)) && ( * stringWithInt != 0)) stringWithInt++;
    tail = stringWithInt;

    while ((isdigit( * tail)) && ( * tail != 0)) tail++;
    if (num > 0) stringWithInt = tail;
  }

  return atoi(stringWithInt);
}


void ringpuffer_abarbeiten() {
  if ((millis() - ringpuffer_startzeit) > ringpuffer_sperrzeit) {
    noTone(pwm_ventil);
    pwm_ventil  = 0;
    if (buffer.isEmpty()) {
      aktuelle_ringbuffer_id = 0;
    }
    else {
      aktuelle_ringbuffer_id = buffer.shift();
    }
    if (aktuelle_ringbuffer_id != 0) {
      Serial.print("Arbeite Ringbufferaufträge ab...");
      Serial.println(aktuelle_ringbuffer_id);
      Serial.print(millis());
      Serial.print(" - ");
      Serial.print(ringpuffer_startzeit);
      Serial.print(" = ");
      Serial.print(millis() - ringpuffer_startzeit);
      Serial.print(" >= ");
      Serial.println(ringpuffer_sperrzeit);
      Serial.print(ringpuffer_sperrzeit);
      Serial.print(" ms sind ");
      Serial.print(ringpuffer_sperrzeit / 60000);
      Serial.println(" min");
    }

    if (aktuelle_ringbuffer_id > 4) {
      // hier ist platz für Spezial Geschichten
      ringpuffer_startzeit = millis();
      ringpuffer_sperrzeit = 500;
    } else {
      /////
      if (regensensor_ram && !digitalRead(RegensensorPin) && aktuelle_ringbuffer_id > 0) {
        bot.sendMessage(String(AdminID), "Regensensor meldet keine Bewässerung notwendig.", "Markdown");
      }
      else {
        ringpuffer_startzeit = millis();
        ringpuffer_sperrzeit = lade_Bewaesserungsdauer(aktuelle_ringbuffer_id) * 60000;
        schlafen(lade_Bewaesserungsdauer(aktuelle_ringbuffer_id)); // DeepSleep für Bewässerungsdauer blockieren
        // Schalte den Ausgewählten Kreis an
        if (aktuelle_ringbuffer_id == 1) {
          pwm_ventil  = VentilePin1;
          tone(VentilePin1, 50);
          Serial.println("Starte Kreis 1");
          bot.sendMessage(String(AdminID), "Starte Kreis 1", "Markdown");
        } else if (aktuelle_ringbuffer_id == 2) {
          pwm_ventil  = VentilePin2;
          tone(VentilePin2, 50);
          Serial.println("Starte Kreis 2");
          bot.sendMessage(String(AdminID), "Starte Kreis 2", "Markdown");
        } else if (aktuelle_ringbuffer_id == 3) {
          pwm_ventil  = VentilePin3;
          tone(VentilePin3, 50);
          Serial.println("Starte Kreis 3");
          bot.sendMessage(String(AdminID), "Starte Kreis 3", "Markdown");
        } else if (aktuelle_ringbuffer_id == 4) {
          pwm_ventil  = VentilePin4;
          tone(VentilePin4, 50);
          Serial.println("Starte Kreis 4");
          bot.sendMessage(String(AdminID), "Starte Kreis 4", "Markdown");
        } else {}
      }
    }
  } else {
    // Ja nüscht zu tun da wat aktiv ist :)
  }
}

float spannung_messen() {
  // Werte am analogen Pin lesen
  values = analogRead(analogInput); // Messwerte am analogen Pin als "values" definieren
  vout = (values * 1.01) / 1024.0; // Messwerte in Volt umrechnen = Spannung am Ausgang des
  // Spannungsteilers mit Zuleitung zu Pin A0
  vin = vout / (R2 / (R1 + R2)); // Berechnen, welche Spannung am Eingang des Spannungsteilers
  // anliegt. Das entspricht der Spannung der zu untersuchenden Batterie

  //Debug in Serialprint
  Serial.print("Values: ");
  Serial.println(values);
  Serial.print("Vout: ");
  Serial.println(vout);
  Serial.print("Vin: ");
  Serial.println(vin);
  if (vin < 0.09) {
    vin = 0.0; // Unterdrücken unerwünschter Anzeigen
  }
  return vin;
}

double temperatur_messen() {
  // Abfrage Temperatur
  double temperatur;

  sensors.requestTemperatures();

  temperatur = sensors.getTempCByIndex(0);

  return temperatur;
}


void setup() {

  // PinMode der VentilePins + Regensensor festlegen
  pinMode(VentilePin1, OUTPUT);
  pinMode(VentilePin2, OUTPUT);
  pinMode(VentilePin3, OUTPUT);
  pinMode(VentilePin4, OUTPUT);
  pinMode(RegensensorPin, INPUT);

  Serial.begin(115200);
  EEPROM.begin(512);
  while (!Serial) {} //Start running when the serial is open
  delay(100);

  // Wifi Verbindung herstellen
  WiFi.persistent( false ); // avoid writing to flash after every wakeup
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("\nConnecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  delay(1000);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signalstärke: ");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm");

  // Only required on 2.5 Beta
  client.setInsecure();

  // longPoll keeps the request to Telegram open for the given amount of seconds if there are no messages
  // This hugely improves response time of the bot, but is only really suitable for projects
  // where the the initial interaction comes from Telegram as the requests will block the loop for
  // the length of the long poll
  //bot.longPoll = 60;

  // Time Bibo starten
  timeClient.begin();

  // Läd aktuelle Cronjobs in den RAM
  cronjob_lade_daten();

  // Inizialisieren der Dallas Temperature library
  sensors.begin();
}

void loop() {
  if (ota_aktiv) {
    ArduinoOTA.handle();
    if (millis() > ota_aktive_zeit) {
      ota_aktiv = false;
      debugmsg("OTA deaktiviert!");
    }
  }
  timeClient.update(); // Internetzeit update ?!
  yield();             // Abfrage der Zeit dauert manchmal lange

  // Prüfe alle x ms nach einer neues Telegram Nachricht
  if (millis() > lastTimeChecked + delayBetweenChecks) {
    //Serial.print("Prüfe nach neuen Nachrichten...");
    //Serial.print("DeepSleep startet ");
    if (millis() - delayBetweenChecks > deepsleep_timeoutzeit ) {
      //Serial.println("jetzt!");
      debugmsg("DeepSleep startet jetzt!");
    }
    else {
      Serial.print("in ");
      Serial.print((deepsleep_timeoutzeit - millis()) / 1000);
      Serial.println(" Sekunden");
    }
    // getUpdates returns 1 if there is a new message from Telegram
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    if (numNewMessages) {
      Serial.println("Neue Telegram Nachricht erhalten");
      handleNewMessages(numNewMessages);
      schlafen(Telegramm_DeepSleep_Stop); // wenn neue Nachricht, wird der DeepSleep für 3 min blockiert
    }

    lastTimeChecked = millis();
  }

  // Jede Minute ein CronJob Check + Sommer/Winterzeit Check
  if (cron_letzte_minute != timeClient.getMinutes()) {
    Serial.println("Prüfe nach neuen Cronjob...");

    cronjob_check(); // schaut ob geplante Aufräge zur aktuellen Zeit anstehen und schiebt sie in den Ringpuffer

    cron_letzte_minute = timeClient.getMinutes();


    //Prüfen ob Sommerzeit
    if (summertime_EU(year(timeClient.getEpochTime()), month(timeClient.getEpochTime()), timeClient.getDay(), timeClient.getHours(), 1)) {
      timeClient.setTimeOffset(7200);
    }
    else {
      deepsleep_schlafzeit = deepsleep_schlafzeit_winter;
    }
  }

  ringpuffer_abarbeiten(); // wie der Name bereit sagt wird der Ringpuffer nach und nach abgearbeitet

  // DeepSleep
  schlafen();

}
