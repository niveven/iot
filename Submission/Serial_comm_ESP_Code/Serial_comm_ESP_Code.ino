#include "WiFi.h"
#include "FirebaseESP32.h"
#include "esp_camera.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "fr_flash.h"

#define relayPin 2 // pin 12 can also be used
#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7

/* Code from webserver*/

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

void stop_webserver();
int startCameraServer();
/*End of Code from webserver*/


unsigned long currentMillis = 0;
unsigned long openedMillis = 0;
long interval = 5000; // open lock for ... milliseconds

////////////////////////////////////////////
// WiFi parameters:
////////////////////////////////////////////

//const char *ssid = "niv";
//const char *password = "204442321";

const char *ssid = "Oded2.4";
const char *password = "67321daoa";


////////////////////////////////////////////
// Pins Used in the ESP:
////////////////////////////////////////////

///////////////////////////////////////////
// Initial state of machine sensors:
///////////////////////////////////////////

int OUTER_BOX_STATE = 0;
int TEA_BOX_STATE = 0;
int COFFEE_BOX_STATE = 0;

//////////////////////////////////////////
// Local variables used in the code:
/////////////////////////////////////////

// Communicaiton variables:
String sensors_state;
  
// Sensors and states' variables:
const int  buttonPin = 2;    // the pin that the "mode" button is attached to
int buttonState = 1;         // current state of the button
int lastButtonState = 0;     // previous state of the button
int state = 0;
int prev_state = -1;
// Prices for drinks:
int coffee_price = 1;
int tea_price = 2;
int cost = 0;
int used_coffee = 0;
int used_tea = 0;

// Face recognition and DB variables:
bool is_identified = false;
bool is_sound_played = false;
FirebaseData firebaseData;
int faceRecognitionCounter = 0;
int faceID_recognized = -1;
char firebase_credit[256] = ""; // TODO:CHECK
String str_id;
String slash;
int val = -1;
/////////////////////////////////////////
/////////////////////////////////////////

void connectWifi()
{
  // Let us connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(".......");
  Serial.println("WiFi Connected....IP Address:");
  Serial.println(WiFi.localIP());
}

//////////////////////////////////////////

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

//////////////////////////////////////////////

static face_id_list id_list = {0};
dl_matrix3du_t *image_matrix = NULL;
camera_fb_t *fb = NULL;

dl_matrix3du_t *aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);

int rzoCheckForFace()
{
  currentMillis = millis();
  int faceID_recognized = run_face_recognition();
  if (faceID_recognized >= 0)
  { // face recognition function has returned true
    //Serial.println("Face recognised");
    digitalWrite(relayPin, HIGH); //close (energise) relay
    openedMillis = millis();      //time relay closed
  }
  if (currentMillis - interval > openedMillis)
  {                              // current time - face recognised time > 5 secs
    digitalWrite(relayPin, LOW); //open relay
  }
  return faceID_recognized;
}

///////////////////////////////////////////////////////

int run_face_recognition()
{
  bool faceRecognised = false; // default
  int64_t start_time = esp_timer_get_time();
  fb = esp_camera_fb_get();
  if (!fb)
  {
    //Serial.println("Camera capture failed");
    return -1;
  }

  int64_t fb_get_time = esp_timer_get_time();
  //Serial.printf("Get one frame in %u ms.\n", (fb_get_time - start_time) / 1000); // this line can be commented out

  image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  uint32_t res = fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
  if (!res)
  {
    //Serial.println("to rgb888 failed");
    dl_matrix3du_free(image_matrix);
  }

  esp_camera_fb_return(fb);

  box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

  int matched_id = -1;

  if (net_boxes)
  {
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK)
    {

      matched_id = recognize_face(&id_list, aligned_face);
      if (matched_id >= 0)
      {
      //  Serial.printf("Match Face ID: %u\n", matched_id);
        faceRecognised = true; // function will now return true
      }
      else
      {
        //Serial.println("No Match Found");
        matched_id = -1;
      }
    }
    else
    {
      //Serial.println("Face Not Aligned");
    }

    free(net_boxes->box);
    free(net_boxes->landmark);
    free(net_boxes);
  }

  dl_matrix3du_free(image_matrix);
  return matched_id;
}

////////////////////////////////////////////////////
/////////////////////////////////////////////////////
int is_server = -1;
void setup()
{
  
  Serial.begin(9600, SERIAL_8N1);
  connectWifi();
 //Firebase.begin("https://coffeeiot-c846f.firebaseio.com", "ZJqbWyM3KpiLSk7zLaPWC06JEnfsbT5bDov0Zr7K");
  str_id = String();
  slash = String("/");
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
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //drop down frame size for higher initial frame rate
  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);

  face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
  read_face_id_from_flash(&id_list); // Read current face data from on-board flash

  //Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());

  //Firebase.begin("https://coffeeiot-c846f.firebaseio.com", "ZJqbWyM3KpiLSk7zLaPWC06JEnfsbT5bDov0Zr7K");

    pinMode(buttonPin, INPUT);

/* End of Code from webserver*/
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

void loop()
{

// read the pushbutton input pin:
  buttonState = digitalRead(buttonPin);
// compare the buttonState to its previous state,  "1" means registration mode:
  if ((buttonState != lastButtonState) && (buttonState == 1)) {
          startCameraServer();
          Serial.println("start server");
      }
    else {
      if (buttonState != lastButtonState &&(buttonState == 0) ){
        Serial.println("stopping server");
        stop_webserver();
        Firebase.begin("https://coffeeiot-c846f.firebaseio.com", "ZJqbWyM3KpiLSk7zLaPWC06JEnfsbT5bDov0Zr7K");
        Serial.println("stopping done");
        delay(20);
        }
        if (buttonState == 0){
    // Reading the sensores state from Arduino:
        if(Serial.available() > 0){
          sensors_state = Serial.readString();
          
          //Debug prints:
          //Serial.print(sensors_state);
         // Serial.println();
          //End of debug prints
          
          //Setting Sensor variables to zeros:
          if (sensors_state == "1" || sensors_state == "11" || sensors_state == "10" || sensors_state == "0"){ 
          //  Serial.println("inside IF 1");
            OUTER_BOX_STATE = 0;}
          if (sensors_state == "101" || sensors_state == "100" || sensors_state == "1" || sensors_state ==  "0"){
            //Serial.println("inside IF 2");
            COFFEE_BOX_STATE = 0;} 
          if (sensors_state == "100"|| sensors_state == "110" || sensors_state == "10" || sensors_state == "0"){
            //Serial.println("inside IF 3");
            TEA_BOX_STATE = 0;}
            
          //Setting Sensor variables to ones:        
          if (sensors_state == "1" || sensors_state == "11" || sensors_state == "101" || sensors_state == "111"){ 
          //  Serial.println("inside IF 1");
            TEA_BOX_STATE = 1;}
          if (sensors_state == "10" || sensors_state == "11" || sensors_state == "110" || sensors_state ==  "111"){
            //Serial.println("inside IF 2");
            COFFEE_BOX_STATE = 1;} 
          if (sensors_state == "100"|| sensors_state == "101" || sensors_state == "110" || sensors_state == "111"){
            //Serial.println("inside IF 3");
            OUTER_BOX_STATE = 1;}
        //Serial.println();
        }
      // Machine logic according to states:
      switch (state)
      {
        // Case 0: Standby mode, Case is closed, Waiting for a user to open the lid:
      case 0:
      //  Serial.println("CASE 0");
        if (state != prev_state){
          Serial.print(1);
          delay(2000);
          prev_state = state;
        }
        if (OUTER_BOX_STATE == 1)
        {
          state = 1;
        }
        break;
    
        // Case 1: Outer lid is opened by a user, a voice prompt is sent:
      case 1:
        if (state != prev_state){
          Serial.print(2);
          delay(2500);
          prev_state = state;
        }
        state = 2;
        //PLAY GREETING SOUND:
        Serial.print(10);
        delay(2500);
        break;
    
        // Case 2: Outer lid is open & the voice propmt was made, starting face recognition attempts(Face recognised - move to case #3, 20 failed attempts - back to state #1):
      case 2:
        if (state != prev_state){
          Serial.print(3);
          delay(2000);
          prev_state = state;
        }
        if ((faceRecognitionCounter%25 == 0) && (faceRecognitionCounter!=0))
        {
          //PLAY "FACE_FAIL" SOUND:
          Serial.print(12);
          delay(2000);
          //Change state back to begining (state still = 2):
          if (faceRecognitionCounter = 125){
            faceRecognitionCounter = 0;
            state = 0;
            delay(2500);
            //TODO add a sound
            }
          break;
        }
        faceRecognitionCounter += 1;
        faceID_recognized = rzoCheckForFace();
        //faceID_recognized = 1;
        if (faceID_recognized >= 0)
        {
          //itoa(faceID_recognized,str_id,10);
          //str_id[0] = faceID_recognized;
          //Serial.println(faceID_recognized); Debug print
          // Play "Face OK":
          str_id = slash + faceID_recognized;
          Serial.print(11);
          delay(2000);
          //change state:
          state = 3;
        }
        break;
    
        // Case 3: The User face was recognised, waiting for coffee/tea box to be opened:
      case 3:
        if (state != prev_state){
          Serial.print(4);
          delay(2000);
          prev_state = state;
        }
        cost = 0;
        if (TEA_BOX_STATE == 1 || COFFEE_BOX_STATE == 1)
        {
          if (TEA_BOX_STATE == 1)
          {
            cost += tea_price;
            used_tea = 1;
            state = 4;
            break;
          }
          if (COFFEE_BOX_STATE == 1)
          {
            cost += coffee_price;
            used_coffee = 1;
            state = 5;
            break;
          }
        }
        else if (OUTER_BOX_STATE == 0 && TEA_BOX_STATE == 0 && COFFEE_BOX_STATE == 0)
        {
          state = 6;
          delay(2000);
          break;
        }
        break;
    
        // Case 4: Recognised that the coffee box was opened, updating the bill for the user:
      case 4:
        if (state != prev_state){
          Serial.print(5);
          delay(2000);
          prev_state = state;
        }
      //  Serial.println("CASE 4");
        if ((COFFEE_BOX_STATE == 1) && (used_coffee == 0))
        {
          cost += coffee_price;
          used_coffee = 1;
          //state = 6;
          delay(2000);
          break;
        }
        else if (OUTER_BOX_STATE == 0)
        {
          state = 6;
          delay(2000);
          break;
        }
        break;
    
        // Case 5: Recognised that the tea box was opened, updating the bill for the user:
      case 5:
        if (state != prev_state){
          Serial.print(6);
          delay(2000);
          prev_state = state;
        }
        if ((TEA_BOX_STATE == 1) && (used_tea == 0))
        {
          cost += tea_price;
          used_tea = 1;
          //state = 6;
          delay(2000);
          break;
        }
        else if (OUTER_BOX_STATE == 0)
        {
          state = 6;
          delay(2000);
          break;
        }
        break;
    
        // Case 6: User finished purchase - update the DB:
      case 6: //Charged customer in firebase
        if (state != prev_state){
          Serial.println(7);
          delay(2000);
          prev_state = state;
        }

        if(used_coffee == 0 && used_tea == 0) {
          Serial.print(14);
          delay(5000);
          state=0;
          break;
        }
        if (Firebase.getInt(firebaseData, str_id))
        {
           if (firebaseData.dataType() == "int")
          {
            val = firebaseData.intData();
            firebase_credit[0] = 0;
            firebase_credit[0] = 0;
            Firebase.setInt(firebaseData, str_id, val - cost);
    
            //Updating quota of coffee/tea:
        if (used_coffee == 1){
          if (Firebase.getInt(firebaseData, slash + "-10")){
            val = firebaseData.intData();
            Firebase.setInt(firebaseData,slash + "-10", val - 1);
            }
          used_coffee = 0;  
        }
    
        if (used_tea == 1){
          if (Firebase.getInt(firebaseData, slash + "-20")){
            val = firebaseData.intData();
            Firebase.setInt(firebaseData,slash + "-20", val - 1);
            }
          used_tea = 0;  
        }
        
        state = 0;
        faceRecognitionCounter = 0;
          }
        }
        //PLAY GOODBYE SOUND:
        Serial.print(13);
        delay(5000);
        state=0;
        break;
        }
      }
    }
    delay(50);
    lastButtonState = buttonState;
}
