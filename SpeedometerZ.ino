/*
 Test the tft.print() viz embedded tft.write() function

 This sketch used font 2, 4, 7

 Make sure all the display driver and pin connections are correct by
 editing the User_Setup.h file in the TFT_eSPI library folder.

 Note that yield() or delay(0) must be called in long duration for/while
 loops to stop the ESP8266 watchdog triggering.

 #########################################################################
 ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
 #########################################################################
 */

#include <TFT_eSPI.h>  // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <time.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

#define TFT_GREY 0x0333  // New colour

// Custom screen objects starts here
#define MARGIN_X 5
#define MARGIN_Y 10

#define TOTAL_BOXES 4

#define BOX_H 25
#define BOX_W 50
#define BOX_RADIUS 5

#define TOTAL_SPEED_BOXES 17
#define SPEED_BAR_MARGIN_X 3
#define SPEED_BAR_MARGIN_Y 110
#define SPEED_BAR_W 12
#define SPEED_BAR_H 20

#define SPEED_STEP 5  // speed step in km/h
#define WAIT_TIME 1   // time to wait between speed changes in seconds

#define BUTTON1PIN 35
#define BUTTON2PIN 0

static const int RXPin = 25, TXPin = 26;
static const uint32_t GPSBaud = 9600;

// The TinyGPSPlus object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

int speed = 0;
int prev_speed = 0;


void IRAM_ATTR toggleButton1() {
  prev_speed = speed;
  update_speed_label(++speed);
}

void IRAM_ATTR toggleButton2() {
  prev_speed = speed;

  update_speed_label(--speed);
}

struct Infobox {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
  int32_t radius;
  uint32_t color;
  int32_t type;
  int32_t bg_color;
};

struct Infobox *info_boxes[TOTAL_BOXES] = { NULL };
struct Infobox *speed_boxes[TOTAL_SPEED_BOXES] = { NULL };
char *info_box_types[3] = { "gps", "ble", "net" };


int calculate_info_box_position() {
  int offset = 10;
  if (info_boxes[0] == NULL) {
    return MARGIN_X + offset;
  }

  int enabled_boxes = 0;
  for (int i = 0; i < TOTAL_BOXES; i++) {
    if (info_boxes[i] != NULL) {
      info_boxes[i]->x = MARGIN_X + (BOX_W + offset) * enabled_boxes;
      enabled_boxes++;
    }
  }

  int x = MARGIN_X + (BOX_W + offset) * enabled_boxes;

  return x;
}

int calculate_speed_box_position() {
  int offset = 2;
  if (speed_boxes[0] == NULL) {
    return SPEED_BAR_MARGIN_X + offset;
  }

  int enabled_boxes = 0;
  for (int i = 0; i < TOTAL_SPEED_BOXES; i++) {
    if (speed_boxes[i] != NULL) {
      speed_boxes[i]->x = SPEED_BAR_MARGIN_X + (SPEED_BAR_W + offset) * enabled_boxes;
      enabled_boxes++;
    }
  }

  int x = SPEED_BAR_MARGIN_X + (SPEED_BAR_W + offset) * enabled_boxes;

  return x;
}

int get_color(char *types) {
  if (types == "gps") {
    return TFT_ORANGE;
  }

  else if (types == "ble") {
    return TFT_BLUE;
  }

  else if (types == "net") {
    return TFT_GREEN;
  }

  return TFT_DARKGREY;
}


int get_speed_color(int speed) {
  if (speed < 70) {
    return TFT_GREEN;
  }

  if (speed > 69 && speed < 90) {
    return TFT_ORANGE;
  }

  else if (speed > 90 && speed < 110) {
    return TFT_RED;
  }

  else if (speed > 110) {
    return TFT_PURPLE;
  }

  return TFT_DARKGREY;
}

void create_info_boxes(void) {
  int count = 0;
  for (int i = 0; i < TOTAL_BOXES; i++) {
    int32_t y = calculate_info_box_position();
    struct Infobox box = {
      calculate_info_box_position(),
      MARGIN_Y,
      BOX_W,
      BOX_H,
      BOX_RADIUS,
      TFT_DARKGREY,
      0,
      get_color(info_box_types[i])
    };
    info_boxes[i] = new Infobox(box);
  }
}


void create_speed_boxes(void) {
  int count = 0;
  for (int i = 0; i < TOTAL_SPEED_BOXES; i++) {
    struct Infobox box = {
      calculate_speed_box_position(),
      SPEED_BAR_MARGIN_Y,
      SPEED_BAR_W,
      SPEED_BAR_H,
      BOX_RADIUS,
      TFT_DARKGREY,
      0,
      TFT_DARKGREY
    };
    speed_boxes[i] = new Infobox(box);
  }
}

void draw_info_boxes(void) {
  for (int i = 0; i < TOTAL_BOXES; i++) {
    if (info_boxes[i] == NULL) {
      continue;
    }
    struct Infobox *curr_rect = info_boxes[i];
    Serial.println(curr_rect->bg_color);
    tft.fillSmoothRoundRect(curr_rect->x, curr_rect->y, curr_rect->w, curr_rect->h, curr_rect->radius, curr_rect->bg_color);
    tft.setCursor(curr_rect->x + 5, curr_rect->y + 5);

    if (i < 3) {
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE);
      tft.println(info_box_types[i]);
    }
  }
}

void clear_speed_boxes(void) {

  tft.fillRect(SPEED_BAR_MARGIN_X, SPEED_BAR_MARGIN_Y, 230, 40, TFT_BLACK);
}

void draw_speed_boxes(void) {
  for (int i = 0; i < TOTAL_SPEED_BOXES; i++) {
    if (speed_boxes[i] == NULL) {
      continue;
    }
    struct Infobox *curr_rect = speed_boxes[i];
    tft.drawRect(curr_rect->x, curr_rect->y, curr_rect->w, curr_rect->h, curr_rect->bg_color);
    tft.setCursor(curr_rect->x + 5, curr_rect->y + 5);
  }
}


void draw_speed_boxes_filled(int speed) {
  for (int i = 0; i < TOTAL_SPEED_BOXES; i++) {
    int lower_bound = i * 10 + 1;
    int upper_bound = (i + 1) * 10;

    if (prev_speed > lower_bound && prev_speed < upper_bound) {
      struct Infobox *curr_rect = speed_boxes[i];
      tft.fillRect(curr_rect->x, curr_rect->y, curr_rect->w, curr_rect->h, TFT_BLACK);
      tft.drawRect(curr_rect->x, curr_rect->y, curr_rect->w, curr_rect->h, curr_rect->bg_color);
    }

    if (speed > lower_bound && speed < upper_bound) {
      for (int j = 0; j <= i; j++) {
        struct Infobox *curr_rect = speed_boxes[j];
        tft.fillRect(curr_rect->x, curr_rect->y, curr_rect->w, curr_rect->h, TFT_BLACK);
        tft.fillRect(curr_rect->x, curr_rect->y, curr_rect->w, curr_rect->h, get_speed_color(speed));
        tft.setCursor(curr_rect->x + 5, curr_rect->y + 5);
      }
    }
  }
}

void update_speed_label(int speed2) {
  speed = speed2;
  tft.fillRect(25, 65, 230, 30, TFT_BLACK);
  tft.setCursor(25, 65);
  tft.setTextSize(4);
  tft.setTextColor(TFT_WHITE);
  tft.setTextPadding(100);
  tft.printf("%i Km/h", speed);

  draw_speed_boxes();
  draw_speed_boxes_filled(speed);
}

void setup(void) {
  Serial.begin(115200);

  ss.begin(GPSBaud);

  tft.init();
  tft.setRotation(1);
  tft.setCursor(0, 0);
  tft.fillScreen(TFT_BLACK);

  create_info_boxes();
  draw_info_boxes();

  create_speed_boxes();
  draw_speed_boxes();

  tft.drawLine(5, 45, 233, 45, TFT_DARKGREY);

  pinMode(BUTTON1PIN, INPUT);
  pinMode(BUTTON2PIN, INPUT);
  attachInterrupt(BUTTON1PIN, toggleButton1, FALLING);
  attachInterrupt(BUTTON2PIN, toggleButton2, FALLING);

  update_speed_label(speed);
}

void loop() {
  // This sketch displays information every time a new sentence is correctly encoded.
  while (ss.available() > 0)
    if (gps.encode(ss.read())){
      speed = gps.speed.kmph();
      update_speed_label(speed);
      prev_speed = speed;
    }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while (true)
      ;
  }
}
