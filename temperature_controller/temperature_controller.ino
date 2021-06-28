/*
#############################################################################################################################
##                                                                                                                         ##
##    ####       ####  ####   ####  ####    ############  ####    #########        ###        ##########   ############    ##     
##    #####     #####  ####   ####  ####    ############  ####   ###########      #####      ############  ############    ##
##    ######   ######  ####   ####  ####        ####      ####  ####     ###     ### ###     ####              ####        ##
##    ###############  ####   ####  ####        ####      ####  ###             ###   ###     #########        ####        ##
##    ###############  ####   ####  ####        ####      ####  ###            ###########          ####       ####        ##
##    #### ##### ####  ##### #####  ####        ####      ####  ####     ###  #####   #####  ####   #####      ####        ##
##    ####  ###  ####   #########   ##########  ####      ####   ###########  ####     ####  ############      ####        ##
##    ####   #   ####    #######    ##########  ####      ####    #########   ####     ####   ##########       ####        ##
##                                                                                                                         ##
#############################################################################################################################
##                                                                                                                         ##  
##                #####   ####  #####  #####   #####  ##  ##   ##  ## ##### ##  ## #####  #####  ## ##   ##                ##
##               ### ### ##  ## ##  ## ##      ##  ## ##  ##   ##  ## ##    ### ## ##  ## ##  ## ##  ## ##                 ##
##               ##      ##  ## ##  ## ###     #####   ####    ###### ###   ###### ##  ## #####  ##   ###                  ##
##               ### ### ##  ## ##  ## ##      ##  ##   ##     ##  ## ##    ## ### ##  ## ## ##  ##  ## ##                 ##
##                #####   ####  #####  #####   #####    #      ##  ## ##### ##  ## #####  ##  ## ## ##   ##                ##
##                                                                                                                         ##
#############################################################################################################################
*/

#include <WiFiClientSecure.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include <HTTPClient.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <ESP_Mail_Client.h>

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The wifi credentials */
#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"

/* The sign in credentials */
#define AUTHOR_EMAIL "EXAMPLE@gmail.com"
#define AUTHOR_PASSWORD "PASSWORD"
#define AUTHOR_NAME "ESP"
#define HTML_MSG "<div style=\"color:#2f4468;\"><h1>A temperatura do freezer está acima do permitido.</h1><p>- Sent from ESP board</p></div>"

/* Recipient's email*/
#define RECIPIENT_EMAIL "EXAMPLE@gmail.com"
#define SUBJECT "ESP Warning Email"
#define RECIPIENT_NAME "Name"

/* Values for sms sender */
#define KEY "http://maker.ifttt.com/trigger/ESP/with/key/*******"
#define HOST "maker.ifttt.com"
#define HTTPS_PORT 443

/* Board constants */
#define PIN_SENSOR_0      4 // pin in which the temperature sensor is connected
#define PIN_SENSOR_1      13 // pin in which the temperature sensor is connected
#define PIN_LED           14
#define PIN_BOARD_BUTTON   0
#define ONE_WIRE_BUS PIN_SENSOR_0

#define MAX_TEMP 25

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

HTTPClient http;
SMTPSession smtp;

void smtpCallback(SMTP_Status status);
void send_sms();
void send_email();

const char* host = HOST;
const int httpsPort = HTTPS_PORT;
int msgSend;

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Starting sensors ...");
  sensors.begin();
  Serial.println("Sensors started");
  Serial.println();
  Serial.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();  
  msgSend = 0;
}

void loop(void)
{ 
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  float tempC = sensors.getTempCByIndex(0);

  if(tempC != DEVICE_DISCONNECTED_C) 
  {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);
    if (tempC >= MAX_TEMP && msgSend == 0)
    {
      send_email(); 
      send_sms();
      msgSend = 1;
    }
    else if (msgSend == 1 && tempC < MAX_TEMP)
      msgSend = 0;
    Serial.println(msgSend);
  } 
  else
    Serial.println("Error: Could not read temperature data");
}

void send_sms()
{
    String key = KEY;
    // Your Domain name with URL path or IP address with path
    http.begin(key);
          // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    // Data to send with HTTP POST
    String httpRequestData = "value1=" + String(random(40)) + "&value2=" + String(random(40))+ "&value3=" + String(random(40));           
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    
    /*
    // If you need an HTTP request with a content type: application/json, use the following:
    http.addHeader("Content-Type", "application/json");
    // JSON data to send with HTTP POST
    String httpRequestData = "{\"value1\":\"" + String(random(40)) + "\",\"value2\":\"" + String(random(40)) + "\",\"value3\":\"" + String(random(40)) + "\"}";
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    */
   
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
}

void send_email()
{
    smtp.debug(1);
  
    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);
  
    /* Declare the session config data */
    ESP_Mail_Session session;
  
    /* Set the session config */
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";
  
    /* Declare the message class */
    SMTP_Message message;
  
    /* Set the message headers */
    message.sender.name = AUTHOR_NAME;
    message.sender.email = AUTHOR_EMAIL;
    message.subject = SUBJECT;
    message.addRecipient(RECIPIENT_NAME, RECIPIENT_EMAIL);
  
    /*Send HTML message*/
    String htmlMsg = HTML_MSG;
    message.html.content = htmlMsg.c_str();
    message.html.content = htmlMsg.c_str();
    message.text.charSet = "us-ascii";
    message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    
    if (!smtp.connect(&session))
      return;
    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
      Serial.println("Error sending Email, " + smtp.errorReason());
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  if (status.success())
  {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}
