# garden-irrigation-system
A control software for a 4 circuit irrigation system. #ESP8266 #WIFI #TELEGRAM

# Persönliche Note:
Im Frühling 2020 beschloss ich nach dem Umzug in unserer neuen Wohnung meinen eigenen Bewässerungscomputer für unseren Garten zu bauen. Zu diesem Zeitpunkt gab es wenige, kostengünstige Steuerungen mit WLAN Anschluss die per Internet zu bedienen waren. Also probierte ich mich am großen DIY :) Die Ideen waren schnell gesammelt und dabei kam folgendes zusammen:

# Grundfunktionen der Bewässerungsteuerung
- kann 4 Wasser Kreise steuern
- erzeugt ein 50Hz Rechtecksignal für VDC Magnetventile
- Eingang für Regensensor, Einstellbar ob dieser auch beachtet wird.
- Stromsparfunktion bei Inaktivität(Deepsleep)
- Zur Winterzeit Schläft der Computer länger(mehr Energie Sparen)
- Bewässerungsdauer pro Kreis einstellbar
- 10 Bewässerungspläne programmierbar
- Grundlage ist die Bedienung über den Messenger Telegram
- Integrierter Telegram Bot kann auch in Gruppen Chat eingesetzt werden
- OTA Update möglichkeit 

# Das passende Board
Im folgenden Link ist das passende Board zum nachbauen oder abkupfern. Die SMD Bauteile sind noch in gut verarbeitbarer Größe gewählt.
https://oshwlab.com/DerDreher85/bewaesserung

# Installation / Einrichtung
Das ganze wurde mit der Arduino IDE programmiert. Dafür müsst ihr die ESP Boards der IDE hinzufügen. Anleitung im folgenden Link:
https://randomnerdtutorials.com/how-to-install-esp8266-board-arduino-ide/

Des weiteren werden viele Biblioteken verwendet. Im Kopfbereich des Sketches sind alle benötigten aufgelistet. Anleitung im folgenden Link:
https://www.arduino.cc/en/Guide/Libraries?setlang=en

Nun gilt es einen Telegram Bot anzulegen und sich einen Token zu besorgen. Anleitung im folgenden Link:
DE - https://www.heise.de/tipps-tricks/Telegram-Bot-erstellen-so-geht-s-5055172.html
EN - https://social.techjunkie.com/telegram-add-bot/

Wer einen Channel nutzen möchte sollte ihn jetzt ganz normal über z.B. die Smartphone App anlegen. Ausserdem bekommen wir im nächsten Schritt recht einfach benötigte Daten für den Zugang zum Bot. Für die Konfiguration/Zugänge benötigen wir die Telegram UserID's / Channel ID's. Einfach gehts über 
>1. https://web.telegram.org
>2. Anmelden und dem neu Angelegten Channel wechseln
>3. Oben in der Adresszeile können wir dann die Channel ID finden (hinten die NEGATIVE ZAHL) https://web.telegram.org/z/#-123456789
>4. User ID's sind positiv! Diese findest du dann unter den Mitgliedern der Gruppe (es müssen natürlich welche in der Gruppe sein ;) ) Wie bei den Channels auf den Namen klicken und dann aus der Adresszeile ablesen z.B. https://web.telegram.org/z/#98765432
>5. Wäre aus diesem Beispielen also ChannelID -> -123456789 und UserID -> 98765432

In der "arduino_secrets.h" Datei müst ihr nun ein paar wichtige Eingaben tätigen.
>#define SECRET_WLAN_SSID "" <<< Der Name eueres WLAN's<br>
#define SECRET_WLAN_PASS "" <<< WLAN Passwort<br>
#define SECRET_TELEGRAM_TOKEN ""  <<< Eurer Telegram Bot Token<br>
#define SECRET_TELEGRAM_FAMILYCHAT "" <<< Wenn mehrere den Bot nutzen wollen lohnt sich ein Chat anzulegen, ID hier rein<br>
#define SECRET_TELEGRAM_ADMIN1 "" <<< Telegramm User ID des Hauptadmin (hier werden auch Debugmeldungen ausgegeben) <br>
#define SECRET_TELEGRAM_ADMIN2 "" <<< zusätzlicher Zugang für eine Person<br>
#define SECRET_OTA_PASS ""  <<< wer mit OTA Arbeitet kann hier ein "Upload Schutz" Passwort festlegen. Drigend empfohlen.<br>

Für das erstmalige Uploaden der Firmeware/Sketches braucht ihr einen FTDI-Adapter. Beispiellink im Anschluss. Habt ihr das Teil angesteckt und VIA USB Kabel mit eurem Rechner verbunden könnt ihr den Script Hochladen. Die notwendigen Einstellungen für den Compiler stehen ebenfalls im Kopfbereich des Sketches.
