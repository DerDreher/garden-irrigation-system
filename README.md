# garden-irrigation-system
A control software for a 4 circuit irrigation system. #ESP8266 #WIFI #TELEGRAM

### Persönliche Note:
Im Frühling 2020 beschloss ich nach dem Umzug in unserer neuen Wohnung meinen eigenen Bewässerungscomputer für unseren Garten zu bauen. Zu diesem Zeitpunkt gab es wenige, kostengünstige Steuerungen mit WLAN Anschluss die per Internet zu bedienen waren. Also probierte ich mich am großen DIY :) Die Ideen waren schnell gesammelt und dabei kam folgendes zusammen:

### Grundfunktionen der Bewässerungsteuerung
- kann 4 Wasser Kreise steuern
- erzeugt ein 50Hz Rechtecksignal für VDC Magnetventile
- Eingang für Regensensor, Einstellbar ob dieser auch beachtet wird.
- Stromsparfunktion bei Inaktivität(Deepsleep)
- Zur Winterzeit Schläft der Computer länger(mehr Energie Sparen)
- Bewässerungsdauer pro Kreis einstellbar
- 10 Bewässerungspläne programmierbar
- Grundlage ist die Bedienung über den Messenger Telegram
- Integrierter Telegram Bot kann auch im Gruppenchat eingesetzt werden
- OTA Update möglichkeit 

### Das passende Board
Im folgenden Link ist das passende Board zum nachbauen oder abkupfern. Die SMD Bauteile sind noch in gut verarbeitbarer Größe gewählt.
https://oshwlab.com/DerDreher85/bewaesserung

### Installation / Einrichtung
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

In der "arduino_secrets.h" Datei müst ihr nun ein paar wichtige Eingaben tätigen. (Beispielhaft)
```
#define SECRET_WLAN_SSID "MustermannsWLAN"       // Der Name eueres WLAN's
#define SECRET_WLAN_PASS "Musterpasswort"        // WLAN Passwort
#define SECRET_TELEGRAM_TOKEN "lkgdjgr9324676GGHNAsd1!§$!%1vfhsafdjsd7"  // Eurer Telegram Bot Token
#define SECRET_TELEGRAM_FAMILYCHAT "-123456789"  // Wenn mehrere den Bot nutzen wollen lohnt sich ein Chat anzulegen, ID hier rein
#define SECRET_TELEGRAM_ADMIN1 "98765432"        // Telegramm User ID des Hauptadmin (hier werden auch Debugmeldungen ausgegeben)
#define SECRET_TELEGRAM_ADMIN2 ""                // zusätzlicher Zugang für eine Person
#define SECRET_OTA_PASS "OTAmusterPasswort"      // wer mit OTA Arbeitet kann hier ein "Upload Schutz" Passwort festlegen. Drigend empfohlen.
```
Für das erstmalige Uploaden der Firmeware/Sketches braucht ihr einen FTDI-USB Adapter. Beispiellink im Anschluss. Den Mikrokontroller müsst ihr in den Flash Modus booten (FLASH Taste gedrückt halten und dann die RESET Taste drücken) Habt ihr das Teil angesteckt und via USB Kabel mit eurem Rechner verbunden könnt ihr den Script Hochladen. Die notwendigen Einstellungen für den Compiler stehen ebenfalls im Kopfbereich des Sketches. Nach dem Erfolgreichen Upload nochmal die RESET Taste drücken. Nun sollte der ESP laufen!
https://www.amazon.de/AZDelivery-Adapter-FT232RL-Serial-gratis/dp/B01N9RZK6I?th=1

### Der Erste Start
Sucht aus eurer Kontaktliste euren Bot herraus. Als ersten Test schreibe ein `/start` in den Chat. Der Bot sollte nach spätestens 40 Sek antworten. Wenn nicht wirf unten ein Blick in die Problembehandlung. Aber zurück zum "ersten Start". ***Folgender Schritt ist sehr sehr wichtig!*** Tippe `/werkseinstellung` in den Chat. Damit werden alle relevanten Speicherpunkte im EEPROM mit einem gültigen Wert deklariert. Erst jetzt funktioniert der Mikrokontroller so wie er soll!

### Bedienung und Befehle
Die Steuerung wurde so intuitiv wie möglich programmiert. Alle relevanten Befehle können über das Menü erreicht werden. Wenn ihr dennoch verbesserungsvorschläge habt, lasst es mich wissen. Folgend eine Liste aller unterstützter Befehle:
##### Befehle vom Menü
```
/start                    -öffnet das Hauptmenü
|- /status                -zeigt eine allgemeine Übersicht
|- /einstellungen         -öffnet das Menü für verschiedene Einstellmöglichkeiten
   |- /bewaesserungsdauer -öffnet Maske für das Einstellen der Bewässerungsdauer der einzelnen Kreise
   |- /bewaesserungsplan  -öffnet Maske für die Bewässerungsplane
   |- /regensensor        -öffnet Bedienfeld zum de-/aktivieren des Regensensor Eingangs
   |- /werkseinstellung   -Befehl zum reseten (nullen) aller relevanten Speicherpunkte im EEPROM
   |- /otaupdate          -öffnet Bedienfeld zum aktivieren von OTA Funktion für 5min
|- /handsteuerung         -öffnet das Bedienfeld für die Handfunktion der Bewässerung
```
##### weitere Befehle
`/debug`
öffnet und aktivert den Debug Modus. Am Anfang wird ein Feld mit Infos angezeigt. Des weiteren werden verschiedene Debugmeldungen einzelner Funktionen aus dem Code angezeigt. Der Modus kann über erneutes eingeben des `/debug` Befehls abgeschaltet werden. Alternativ schaltet sich der Modus aus wenn der Mikrokontroller in den DeepSleep wechselt.
