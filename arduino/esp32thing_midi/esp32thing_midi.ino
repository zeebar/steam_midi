

// These need to be included when using standard Ethernet

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>

#define REALLY_SIMPLE_MIDI

#ifdef REALLY_SIMPLE_MIDI

#define SSMIDI_UDP_DATA_PORT 15014
#define SSMIDI_UDP_STATE_PORT (SSMIDI_UDP_DATA_PORT + 1)

#define SSMIDI_SERVER_ADDRESS "192.168.4.10"

#define SSMIDI_ACTIVE_SENSE      254
#define SSMIDI_NOTE_ON           129
#define SSMIDI_NOTE_OFF          128
#define SSMIDI_SYNC_INTERVAL_MS  500

const char *ssMidiUDPAddress = SSMIDI_SERVER_ADDRESS;

const int ssMidiUDPDataPort  = SSMIDI_UDP_DATA_PORT;
const int ssMidiUDPStatePort = SSMIDI_UDP_STATE_PORT;


WiFiUDP ssMidiUDP;

static char ssMidiPacketBuffer[255];


#else

#include "AppleMidi.h"

IPAddress midiServerIP(192, 168, 4, 10);

APPLEMIDI_CREATE_INSTANCE(WiFiUDP, AppleMIDI); // see definition in AppleMidi_Defs.h

#endif

#include "steam_private.h"

#define MY_HOST_NAME "steamdemo"

#define MIDI_SESSION_NAME "STEAMDemo"

#define MIDI_LAPTOP_DEMO_SESSION_NAME "STEAMLaptopDemo"

unsigned long tDemoStart = 0L;
unsigned long tCurrent = millis();
unsigned long tLastMidiSent = millis();
unsigned long tLastBrowseSent = millis();
unsigned long tPrev = 0;

bool midi_connected = false;

unsigned long tLastSSMidiInviteSent = 0L;

static volatile bool wifi_connected = false;


uint64_t chipid;  

int ssMidiStartHead = 0;
int ssMidiStartTail = 0;


// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0     0

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT  13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

// magnets
#define NMAGS 16

int magPins[NMAGS] = { 
       16, 17,  4,  2,  
       15,  5, 18, 23,  
       19, 22, 21, 13,  
       12, 14, 27, 26 
};

uint32_t magPower[NMAGS];
uint32_t magMin[NMAGS];
uint32_t magMax[NMAGS];
uint32_t magAttack[NMAGS];
uint32_t magDecay[NMAGS];
uint32_t magSustain[NMAGS];
uint32_t magHold[NMAGS];
uint32_t magRelease[NMAGS];
unsigned long magStart[NMAGS];



#define MAX_MAGD_PER_MS     16

#define EXER_MAG_GO_MS     200
#define EXER_MAG_GAP_MS     50
#define EXER_MS_PAUSE_MS  2000
#define EXER_MS           (EXER_MS_PAUSE_MS + 16 * (EXER_MAG_GO_MS + EXER_MAG_GAP_MS) + EXER_MS_PAUSE_MS)


int protoBase[6]   = {  0,  2,  4,  7,  9 };
int protoDeltaP[6] = {  1,  0,  1,  0,  0 };
int protoDeltaQ[6] = { -1, -1,  0,  1,  1 };

int protoDeltaScaleP = 1;
int protoDeltaScaleQ = 1;




#define PROTO_BARS              4
#define PROTO_NOTE_MS         250
#define PROTO_NOTE_GO_MS      140
#define PROTO_NOTE_TRIGGER_MS 100
#define PROTO_NOTES_PER_BAR     8
#define PROTO_BAR_REPS          2
#define PROTO_BAR_MS     (PROTO_NOTES_PER_BAR * PROTO_NOTE_MS)
#define PROTO_TUNE_GAP_MS    4000
#define PROTO_TUNE_MS     (PROTO_BAR_MS * PROTO_BAR_REPS * PROTO_BARS + PROTO_TUNE_GAP_MS)

int buttonPin = 25;

// Arduino like analogWrite
// value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  
  if ( value < 0 ) {
    value = 0;
  }
  
  if ( value > valueMax ) {
    value = valueMax;
  }
  
  uint32_t duty = (8191 / valueMax) * value;

  // write duty to LEDC
  ledcWrite(channel, duty);
}


#define MAG_MAX     250
#define MAG_ATTACK   20
#define MAG_DECAY   120
#define MAG_SUSTAIN 190
#define MAG_HOLD     20
#define MAG_RELEASE  80


#define BASE_MAG 3

int bxPrev = -1;
int rxPrev = -1;
int nxPrev = -1;
int nmxPrev = -1;

void setMagnetPower( uint8_t mx, uint32_t newMagPower ) {
  if ( newMagPower != magPower[mx] ) {
     // Serial.printf("%ld mag %x: %d => %d\n", tCurrent, mx, (int) magPower[mx], (int) newMagPower);
     ledcAnalogWrite(((uint8_t) LEDC_CHANNEL_0 + mx), newMagPower);
     magPower[mx] = newMagPower;
  }
}


void setupButtons() {
    pinMode(buttonPin, INPUT_PULLUP); 
}

int prevButtonState = LOW;

void runButtons() {
  
  int buttonState = digitalRead(buttonPin);

  unsigned long tButtonTime = millis();
  
  if (buttonState == LOW && prevButtonState != LOW && tDemoStart != tButtonTime) {
    tDemoStart = tButtonTime;
    Serial.printf("%ld demo go!\n", tDemoStart);
  }

   prevButtonState = buttonState;
}


void setupMagnets() {
  
  tDemoStart = 0L;
  
  for (int mx = 0; mx < NMAGS; mx++) {
    
    magMin[mx] = (mx >= 8) ? 1 : 0;
    magMax[mx] = MAG_MAX;
    magAttack[mx] = MAG_ATTACK;
    magDecay[mx] = MAG_DECAY;
    magHold[mx] = MAG_HOLD;
    magSustain[mx] = MAG_SUSTAIN;
    magRelease[mx] = MAG_RELEASE;
    magStart[mx] = 0;
    
    ledcSetup(LEDC_CHANNEL_0 + mx, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
    ledcAttachPin(magPins[mx], LEDC_CHANNEL_0 + mx);
    ledcAnalogWrite(LEDC_CHANNEL_0 + mx, magPower[mx] = magMin[mx]);
  }

  tPrev = 0;
}


void runMagnets() {

  tCurrent = millis();
  int dt = tCurrent - tPrev;

  if ( tCurrent < EXER_MS ) {
    
     int et = tCurrent - EXER_MS_PAUSE_MS;
     int emx = et / (EXER_MAG_GO_MS + EXER_MAG_GAP_MS);
     int emt = et - emx * (EXER_MAG_GO_MS + EXER_MAG_GAP_MS);

     for (int mx = 0; mx < NMAGS; mx++) {
       setMagnetPower(mx, (mx == emx && emt > 0 && emt < EXER_MAG_GO_MS) ? magMax[mx] : magMin[mx]);
     }
      
  }
  else if ( tDemoStart > 0L && tDemoStart <= tCurrent ) {

    int t = (int) (tCurrent - tDemoStart);
    int sx = t / PROTO_TUNE_MS;
    int st = t - sx * PROTO_TUNE_MS;
    int bx = st / (PROTO_BAR_MS * PROTO_BAR_REPS);
    int bt = st - bx * (PROTO_BAR_MS * PROTO_BAR_REPS);
    int rx = bt / PROTO_BAR_MS;
    int rt = bt - rx * PROTO_BAR_MS;
    int nx = rt / PROTO_NOTE_MS;
    int nt = rt - nx * PROTO_NOTE_MS;
    
    if ( nx >= 5 ) {
      nx -= 3;
    }
    
    int r = bx % 4;
    int p = (bx == 1);
    int q = p || (bx == 2);
    int nmx = BASE_MAG + protoBase[nx] 
        + p * protoDeltaP[nx] * protoDeltaScaleP
        + q * protoDeltaQ[nx]  * protoDeltaScaleQ;


    if ( nmx >= NMAGS || nmx < 0 ) {
      nmx = -1;
    }
    else if ( sx >= 1 || bx >= PROTO_BARS || nt >= PROTO_NOTE_TRIGGER_MS ) {
      nmx = -1;
    }

    if ( nmx != nmxPrev ) {
      if ( nmx != -1 ) {
        Serial.printf("%d bar %d rep %d note %d\n", t, bx, rx, nmx);
        magStart[nmx] = tCurrent;
      } 
      bxPrev = bx;
      rxPrev = rx;
      nxPrev = nx;
      nmxPrev = nmx;
    }
  }

  if ( true ) {

    for (int mx = 0; mx < NMAGS; mx++) {
      
      int magPowerTarget = magMin[mx];
      int magPowerChangeRate = magSustain[mx] * 1000 / magRelease[mx];
      int newMagPower = magPower[mx];

      if ( magStart[mx] != 0 || magPower[mx] > magMin[mx] ) {

        int nmt = tCurrent - magStart[mx];
        int tHeld = nmt - (magAttack[mx] + magDecay[mx]);

        if ( nmt < 0 ) {
          // BEFORE
        }
        else if ( tHeld < 0 ) {
          if ( nmt < magAttack[mx] ) {
            // ATTACK
            magPowerTarget = magMax[mx];
            magPowerChangeRate = magMax[mx] * 1000 / magAttack[mx];
          }
          else {
            // DECAY
            magPowerTarget = magSustain[mx];
            magPowerChangeRate = (magMax[mx] - magSustain[mx]) * 1000 / magDecay[mx];          
          }
        }
        else {
          if ( tHeld < magHold[mx] ) {
            // SUSTAIN
            magPowerTarget = magSustain[mx];
            magPowerChangeRate = magMax[mx] * 1000 / magAttack[mx];
          }
          else {
            // RELEASE
          }
        }
        newMagPower = magPower[mx];
        if ( magPowerChangeRate < 0 ) {
          magPowerChangeRate = 0 - magPowerChangeRate;
        }
        if ( newMagPower < magPowerTarget ) { 
          newMagPower += magPowerChangeRate * dt / 1000;
          if ( newMagPower > magPowerTarget ) {
            newMagPower = magPowerTarget;
          }
        }
        else if ( newMagPower > magPowerTarget ) { 
          newMagPower -= magPowerChangeRate * dt / 1000;
          if ( newMagPower < magPowerTarget ) {
            newMagPower = magPowerTarget;
          }
        }  
        if ( newMagPower != magPower[mx] ) {
          //Serial.printf("%ld mag %x: %d target %d speed %d\n", tCurrent, mx, (int) magPower[mx], (int) magPowerTarget, (int) magPowerChangeRate, (int) dt);          
        }

      } 
      setMagnetPower(mx, newMagPower);

    }

  }

  tPrev = tCurrent;
}


// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// rtpMIDI session. Device connected
// -----------------------------------------------------------------------------
void OnAppleMidiConnected(uint32_t ssrc, char* name) {
  midi_connected  = true;
  Serial.print("Connected to MIDI session ");
  Serial.println(name);
}

// -----------------------------------------------------------------------------
// rtpMIDI session. Device disconnected
// -----------------------------------------------------------------------------
void OnAppleMidiDisconnected(uint32_t ssrc) {
  midi_connected  = false;
  Serial.println("MIDI session Disconnected");
}


// -----------------------------------------------------------------------------
//

#define PROTO_BASE_NOTE 55

int midiNoteMagMap[] = {      0, -1,  1, -1,  2, 
  3, -1,  4, -1,  5,  6, -1,  7, -1,  8, -1,  9, 
 10, -1, 11, -1, 12, 13, -1, 14
};

// -----------------------------------------------------------------------------
void OnAppleMidiNoteOn(byte channel, byte note, byte velocity) {

  int nx = note - PROTO_BASE_NOTE;
  int mx = -1;

  if ( nx >= 0 && nx < sizeof(midiNoteMagMap) / sizeof(int) ) {
    mx = midiNoteMagMap[nx];
  } 

  if ( mx >= 0 && mx < NMAGS ) {
    magStart[mx] = millis();
  }
  
  Serial.print("Incoming NoteOn from channel:");
  Serial.print(channel);
  Serial.print(" note:");
  Serial.print(note);
  Serial.print(" magnet:");
  Serial.print(mx);
  
  Serial.print(" velocity:");
  Serial.print(velocity);
  Serial.println();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOff(byte channel, byte note, byte velocity) {
  Serial.print("Incoming NoteOff from channel:");
  Serial.print(channel);
  Serial.print(" note:");
  Serial.print(note);
  Serial.print(" velocity:");
  Serial.print(velocity);
  Serial.println();
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------

#ifdef REALLY_SIMPLE_MIDI


int SSMidi_send_invite() {
    char msgBuf[20];
    ssMidiUDP.beginPacket(ssMidiUDPAddress,ssMidiUDPStatePort);
    ssMidiUDP.printf("*esp32_%08x", (uint32_t)chipid);
    return ssMidiUDP.endPacket();
}


void SSMidi_run() {

   int packetSize = 0;

   if ( wifi_connected && ssMidiStartHead > 0 ) {
    
     if ( ssMidiStartHead > ssMidiStartTail ) {
       ssMidiStartTail = ssMidiStartHead;
       ssMidiUDP.begin(WiFi.localIP(), ssMidiUDPDataPort);
     }
     
     if ((millis() - tLastSSMidiInviteSent) > SSMIDI_SYNC_INTERVAL_MS) {
       if (SSMidi_send_invite()) {
         tLastSSMidiInviteSent = millis();
         Serial.print("-");
       }
       else {
        Serial.println("could not send rsmidi invite");
       }
     }

     packetSize = ssMidiUDP.parsePacket();
     
     if (packetSize) {
       Serial.print("Received packet of size ");
       Serial.println(packetSize);
       Serial.print("From ");
       IPAddress remoteIp = ssMidiUDP.remoteIP();
       Serial.print(remoteIp);
       Serial.print(", port ");
       Serial.println(ssMidiUDP.remotePort());

       // read the packet into packetBufffer
       int len = ssMidiUDP.read(ssMidiPacketBuffer, 255);

       if (ssMidiPacketBuffer[0] == 144 && len == 3 && ssMidiPacketBuffer[2] > 0) {
         byte note = ssMidiPacketBuffer[1];
         Serial.printf("Contents: NoteOn %d", note);
         OnAppleMidiNoteOn((byte) 0, note, (byte) ssMidiPacketBuffer[2]);
       }
       else if (len > 0) {
         Serial.printf("Contents: 0x%02x ...\n", ssMidiPacketBuffer[0]);
       }
     }
   }
}

void SSMidi_start() {
    ssMidiStartHead ++;
}

#endif

void startMidi() {

#ifdef REALLY_SIMPLE_MIDI

  Serial.println("starting really simple Midi session");

  // Create a session and wait for a remote host to connect to us
  //Serial.print("AppleMIDI Session ");
  //Serial.print(AppleMIDI.getSessionName());
  //Serial.print(" with SSRC 0x");
  //Serial.println(AppleMIDI.getSynchronizationSource(), HEX);

  SSMidi_start();
  
#else

  Serial.println("starting Midi session");

  AppleMIDI.begin(MIDI_LAPTOP_DEMO_SESSION_NAME);
  AppleMIDI.invite(midiServerIP);
  
  Serial.println("sent MiDi invite");

  AppleMIDI.OnConnected(OnAppleMidiConnected);
  AppleMIDI.OnDisconnected(OnAppleMidiDisconnected);

  AppleMIDI.OnReceiveNoteOn(OnAppleMidiNoteOn);
  AppleMIDI.OnReceiveNoteOff(OnAppleMidiNoteOff);

  Serial.println("midi callbacks set");
#endif
}



void wifiOnConnect(){
    Serial.println("STA Connected");
    Serial.print("STA IPv4: ");
    Serial.println(WiFi.localIP());
    startMidi();
}

void wifiOnDisconnect(){
    Serial.println("STA Disconnected");
    delay(1000);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}


void WiFiEvent(WiFiEvent_t event){

    // Serial.println("WiFi event type " + event);
  
    switch(event) {

        case SYSTEM_EVENT_AP_START:
            Serial.println("WiFi AP start");
            //WiFi.softAPsetHostname(MY_HOSTTNAME);
            //WiFi.softAPenableIpV6();
            break;

        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi STA start");
            WiFi.setHostname(MY_HOST_NAME);
            break;
            
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("WiFi STA connected");
            // WiFi.enableIpV6();
            break;
            
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            //both interfaces get the same event
            Serial.print("STA IPv6: ");
            Serial.println(WiFi.localIPv6());
            Serial.print("AP IPv6: ");
            Serial.println(WiFi.softAPIPv6());
            break;
            
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.println("got WiFi IP");
            wifiOnConnect();
            wifi_connected = true;
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            wifi_connected = false;
            wifiOnDisconnect();
            break;
            
        default:
            break;
    }
}

void setupNetworking() {
    WiFi.disconnect(true);
    WiFi.onEvent(WiFiEvent);
}



// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------

void setup()
{
  int wifi_count_down = 0;
   
  // Serial communications and wait for port to open:
  Serial.begin(115200);

  setupMagnets();
  setupButtons();
  setupNetworking();

  delay(500);
  
  wifi_connected = false;

  while ( wifi_connected == false ) {
    Serial.print("?");
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    wifi_count_down = 10; 
    
    while ( wifi_connected == false && wifi_count_down > 0 ) {
      delay(500);
      Serial.print("-");
    }
  }
  
  Serial.println("! WiFi connected");
  
  if (!MDNS.begin(MY_HOST_NAME)) {
    Serial.println("Error setting up MDNS responder!");
  }
  else {
    Serial.println("mDNS responder started");
  }
  
  //Serial.println("OK, now make sure you an rtpMIDI session that is Enabled");
  //Serial.print("Add device named Arduino with Host/Port ");
  //Serial.print(WiFi.localIP());
  //Serial.println(":5004");
  //Serial.println("Then press the Connect button");
  //Serial.println("Then open a MIDI listener (eg MIDI-OX) and monitor incoming notes");

  delay(1000);
  Serial.println("started MiDi");
  
  // startMidi();
  
  chipid = ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  
  Serial.printf("ESP32 Chip ID = %04X",(uint16_t)(chipid>>32));//print High 2 bytes
  Serial.printf("%08X\n",(uint32_t)chipid);//print Low 4bytes.

  Serial.println("setup done");
}



void browseService(const char * service, const char * proto){
    Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    int n = MDNS.queryService(service, proto);
    if (n == 0) {
        Serial.println("no services found");
    } else {
        Serial.print(n);
        Serial.println(" service(s) found");
        for (int i = 0; i < n; ++i) {
            // Print details for each service found
            Serial.print("  ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(MDNS.hostname(i));
            Serial.print(" (");
            Serial.print(MDNS.IP(i));
            Serial.print(":");
            Serial.print(MDNS.port(i));
            Serial.println(")");
        }
    }
    Serial.println();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop()
{
  runMagnets();
  runButtons();

#ifdef REALLY_SIMPLE_MIDI
  SSMidi_run();
#else
 AppleMIDI.run();
#endif

  // send a note every second
  // (dont cáll delay(1000) as it will stall the pipeline)
  //if (isConnected && (millis() - tLastMidiSent) > 1000) {
  //  tLastMidiSent = millis();
    //   Serial.print(".");

  //  int note = 45;
  //  int velocity = 55;
  //  int channel = 1;
  //
  //  AppleMIDI.noteOn(note, velocity, channel);
  //  AppleMIDI.noteOff(note, velocity, channel);
  //}

//  if ((WiFi.status() == WL_CONNECTED) && (millis() - tLastBrowseSent) > 4000) {
//    tLastBrowseSent = millis();
//    browseService("apple-midi", "udp");
//  }
  
  delay(10);
}

