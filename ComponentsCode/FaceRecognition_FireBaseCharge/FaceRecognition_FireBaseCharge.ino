#include "WiFi.h"
#include "FirebaseESP32.h"
#include "esp_camera.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "fr_flash.h"

#define relayPin 2 // pin 12 can also be used
#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7


unsigned long currentMillis = 0;
unsigned long openedMillis = 0;
long interval = 5000;           // open lock for ... milliseconds


////////////////////////////////////////////
////////////////////////////////////////////
// WiFi parameters:                       
////////////////////////////////////////////

const char* ssid = "niv";
const char* password = "204442321";

////////////////////////////////////////////
////////////////////////////////////////////
// Pins Used in the project:;
////////////////////////////////////////////

int CLOCK_PIN = 15;
int ARD_PIN = 13;

int COMM_MP3_PIN = 3;
int COMM_MP3_CLOCK_PIN = 2;
//int OUTPUT_TO_ARD_SPEAKER = 13;
int OUTER_BOX_SENSOR_STATE = 0;
int INSIDE_BOX_TEA_STATE = 0;
int INSIDE_BOX_COFFEE_STATE = 0;
int magnetic_detected = 0;
int coffee_price = 1;
int tea_price = 2;

/////////////////////////////////////////
/////////////////////////////////////////
// Local variables used in the code:
/////////////////////////////////////////
int val = -1;
int state = 0;
int faceRecognitionCounter = 0;
int cost = 0;
int faceID_recognized = -1;
char firebase_credit[256]= "";
char str_id[256] = "";
 
 int transmit_MP3[5] = {0};
/////////////////////////////////////////


void connectWifi() {
  // Let us connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(".......");
  Serial.println("WiFi Connected....IP Address:");
  Serial.println(WiFi.localIP());
}

static inline mtmn_config_t app_mtmn_config()
{
  mtmn_config_t mtmn_config = {0};
  mtmn_config.type = FAST;
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.707;
  mtmn_config.pyramid_times = 4;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.p_threshold.candidate_number = 20;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 10;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.7;
  mtmn_config.o_threshold.candidate_number = 1;
  return mtmn_config;
}
mtmn_config_t mtmn_config = app_mtmn_config();
 
static face_id_list id_list = {0};
dl_matrix3du_t *image_matrix =  NULL;
camera_fb_t * fb = NULL;
 
dl_matrix3du_t *aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);

int rzoCheckForFace() {
  currentMillis = millis();
  int faceID_recognized = run_face_recognition();
  if (faceID_recognized >= 0) { // face recognition function has returned true
    Serial.println("Face recognised");
    digitalWrite(relayPin, HIGH); //close (energise) relay
    openedMillis = millis(); //time relay closed
  }
  if (currentMillis - interval > openedMillis){ // current time - face recognised time > 5 secs
    digitalWrite(relayPin, LOW); //open relay
  }
  return faceID_recognized;
}

int run_face_recognition() {
  bool faceRecognised = false; // default
  int64_t start_time = esp_timer_get_time();
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return -1;
  }
 
  int64_t fb_get_time = esp_timer_get_time();
  Serial.printf("Get one frame in %u ms.\n", (fb_get_time - start_time) / 1000); // this line can be commented out
 
  image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  uint32_t res = fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
  if (!res) {
    Serial.println("to rgb888 failed");
    dl_matrix3du_free(image_matrix);
  }
 
  esp_camera_fb_return(fb);
 
  box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

  int matched_id = -1;
 
  if (net_boxes) {
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK) {
 
      matched_id = recognize_face(&id_list, aligned_face);
      if (matched_id >= 0) {
        Serial.printf("Match Face ID: %u\n", matched_id);
        faceRecognised = true; // function will now return true
      } else {
        Serial.println("No Match Found");
        matched_id = -1;
      }
    } else {
      Serial.println("Face Not Aligned");
    }
 
    free(net_boxes->box);
    free(net_boxes->landmark);
    free(net_boxes);
  }
 
  dl_matrix3du_free(image_matrix);
  return matched_id;
}

bool is_identified = false;
bool is_sound_played = false;
FirebaseData firebaseData;


void transmitState(int* transmit){
    digitalWrite(COMM_MP3_CLOCK_PIN, LOW);
    Serial.print("Strating Transmit");
    Serial.println(LOW);
    delay(10000);
    for (int i=0; i<4; i++){
      digitalWrite(COMM_MP3_PIN, transmit[i]);
       delay(25);
      digitalWrite(COMM_MP3_CLOCK_PIN,HIGH);
      delay(100);
      digitalWrite(COMM_MP3_CLOCK_PIN, LOW);
      Serial.println(transmit[i]);
      delay(500);
    }
    return;
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////



void setup() {
  
  Serial.begin(115200);
  //pinMode(OUTPUT_TO_ARD_SPEAKER,OUTPUT);
  connectWifi();
  Firebase.begin("https://coffeeiot-c846f.firebaseio.com", "ZJqbWyM3KpiLSk7zLaPWC06JEnfsbT5bDov0Zr7K");


  digitalWrite(relayPin, LOW);
  pinMode(relayPin, OUTPUT);
 
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
 
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
 
  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);
 
  face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
  read_face_id_from_flash(&id_list);// Read current face data from on-board flash

  pinMode(COMM_MP3_PIN,OUTPUT);
  digitalWrite(COMM_MP3_CLOCK_PIN,HIGH);
  
}


////////////////////////////////////////////////////
////////////////////////////////////////////////////

char ard_code[5] = {0};

void loop() {

// Reading the sensores state from Arduino:

if (digitalRead(CLOCK_PIN)==LOW){
  for (int i=0; i<4 ; i++){
    if(digitalRead(CLOCK_PIN)==HIGH){
      Serial.print(digitalRead(ARD_PIN));
      Serial.print(' ');
      Serial.println(i);
      ard_code[i] = digitalRead(ARD_PIN);
      delay(200);
    }
    else{
      i--;
    }
  }
}
  OUTER_BOX_SENSOR_STATE = ard_code[0];
  INSIDE_BOX_TEA_STATE = ard_code[1];
  INSIDE_BOX_COFFEE_STATE = ard_code[2];


  
  //Serial.println(atoi(ard_code));
   
  switch(state)
  {
// Case 0: Standby mode, Case is closed, Waiting for a user to open the lid:
    case 0:
      Serial.println("CASE 0");
      if(OUTER_BOX_SENSOR_STATE == 1)
      {
        state = 1;
      }
      break;
      
// Case 1: Outer lid is opened by a user, a voice prompt is sent:
    case 1:
      Serial.println("CASE 1");
      //digitalWrite(OUTPUT_TO_ARD_SPEAKER,HIGH);
      //delay(100);
      //digitalWrite(OUTPUT_TO_ARD_SPEAKER,LOW);
      state = 2;
      //PLAY GREETING SOUND:
      transmit_MP3[0] = 1;
      transmitState(transmit_MP3);
      transmit_MP3[0] = 0;
      
      break;

// Case 2: Outer lid is open & the voice propmt was made, starting voice recognition attempts(Face recognised - move to case #3, 20 failed attempts - back to state #1): 
    case 2:
      Serial.println("CASE 2");
      if (faceRecognitionCounter == 20) {
        //PLAY "FACE_FAIL" SOUND:
        //transmit_MP3[2] = 1;
        //transmitState(transmit_MP3);
        //transmit_MP3[2] = 0;
        //Change state back to begining:
        state = 1;
        faceRecognitionCounter = 0;
        break;
      }
      faceRecognitionCounter += 1;
      faceID_recognized = rzoCheckForFace();
      if(faceID_recognized >= 0) 
      {
        str_id[0] = 0;
        sprintf(str_id, "/%d", faceID_recognized);
        //PLAY "FACE_OK" SOUND:
        transmit_MP3[1] = 1;
        transmitState(transmit_MP3);
        transmit_MP3[1] = 0;
        //change state:
        state = 3;
      }
      break;

// Case 3: The User face was recognised, waiting for coffee/tea box to be opened:  
    case 3:
      Serial.println("CASE 3");
        cost = 0;
        if(INSIDE_BOX_TEA_STATE == 1 || INSIDE_BOX_COFFEE_STATE == 1)
        {
          if (INSIDE_BOX_TEA_STATE == 1)
          {
            cost += tea_price;
            state = 4;
            break;
          }
          if(INSIDE_BOX_COFFEE_STATE == 1) 
          {
            cost += coffee_price;
            state = 5;
            break;
          }
        }
        break;

// Case 4: Recognised that the coffee box was opened, updating the bill for the user:
    case 4:
      Serial.println("CASE 4");
      if(INSIDE_BOX_COFFEE_STATE == 1)
      {
        cost += coffee_price;
        state = 6;
        break;
      }
      else if(OUTER_BOX_SENSOR_STATE == 0)
      {
        state = 6;
        break;
      }
      break;

// Case 5: Recognised that the tea box was opened, updating the bill for the user:      
     case 5:
      Serial.println("CASE 5");
      if(INSIDE_BOX_TEA_STATE == 1)
      {
        cost += tea_price;
        state = 6;
        break;
      }
      else if(OUTER_BOX_SENSOR_STATE == 0)
      {
        state = 6;
        break;
      }
      break;
      
// Case 6: User finished purchase - update the DB:      
    case 6:  //Charged customer in firebase 
      Serial.println("CASE 6");
      if(Firebase.getInt(firebaseData, str_id))
      {
        if(firebaseData.dataType() == "int") 
        {
          val = firebaseData.intData();
          if (val > 0)
          {
            firebase_credit[0] = 0;
            sprintf(firebase_credit,"User %d Has %d" ,faceID_recognized, val);
            Serial.println(firebase_credit);
            firebase_credit[0] = 0;
            sprintf(firebase_credit, "/%d", faceID_recognized);
            Firebase.setInt(firebaseData, str_id, val-cost);
            state = 0;
          }
          faceRecognitionCounter = 0;
        }
      }
      //PLAY GOODBYE SOUND:
      transmit_MP3[3] = 1;
      transmitState(transmit_MP3);
      transmit_MP3[3] = 0;
      break;  
  }
 }
