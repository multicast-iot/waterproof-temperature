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
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <HTTPClient.h>
//#if defined(ESP32)
#include <WiFi.h>
//#elif defined(ESP8266)
//  #include <ESP8266WiFi.h>
//#endif
#include <ESP_Mail_Client.h>

#define SMTP_HOST "smtp.gmail.com" // Não alterar.
#define SMTP_PORT 465 // Não alterar.

// SSID da rede wifi em que o mote se conectará.
#define WIFI_SSID "NOME-DA-REDE"
// Senha da rede wifi em que o mote se conectará. 
#define WIFI_PASSWORD "SENHA-DA-REDE" 

// Endereço de email de quem vai enviar a mensagem.
#define AUTHOR_EMAIL "EXEMPLO@gmail.com" 
// Senha do email do remetente.
#define AUTHOR_PASSWORD "SENHA-DO-EMAIL" 
// Nome que irá aparecer no email.
#define AUTHOR_NAME "ESP" 
// Corpo da mensagem que será enviada.
#define HTML_MSG "<div style=\"color:#2f4468;\"><h1>A temperatura do freezer está acima do permitido.</h1><p>- Sent from ESP board</p></div>" 

// Endereço de email de quem vai enviar a mensagem.
// ATENÇÃO!!! Para permitir o envio de mensagens usando o ESP32 através de contas GMAIL,
// é necessário liberar o acesso para dispositivos menos seguros.
// Para isso, clique na sua foto de perfil -> 'Gerenciar sua Conta do Google' -> 
// 'Segurança' -> Agora, dentro do campo "Acesso a app menos seguro", clique em 'Ativar Acesso' ->
// e ative a opção 'Permitir aplicativos menos seguros'
#define RECIPIENT_EMAIL "EXEMPLO@gmail.com" 
// Assunto do email.
#define SUBJECT "ESP Warning Email" 
// Nome do destinatário.
#define RECIPIENT_NAME "NOME-DO-DESTINATÁRIO" 

/* Values for sms sender */
#define KEY "http://maker.ifttt.com/trigger/ESP/with/key/pb7CFoVljEzdc-Z8kiOpy" // Key gerada na aplicação IFTTT
#define HOST "maker.ifttt.com" // Link da plataforma (não alterar).
#define HTTPS_PORT 443 // Porta (não alterar)

/* Board constants */
#define PIN_SENSOR_0      0 //4 // pin in which the temperature sensor is connected
#define PIN_SENSOR_1      13 // pin in which the temperature sensor is connected
#define PIN_LED           14
#define PIN_BOARD_BUTTON   0
#define ONE_WIRE_BUS PIN_SENSOR_0

/* Telegram Bot */
#define BOTtoken "1669682539:AAE7ZqDQh2vmhC4InxH4gLFCZS-MHUMd6ew" // Bot do telegram que enviará a mensagem (não precisa alterar).

// ID do Telegram de quem irá receber a mensagem. Para descobrir o seu ID, basta procurar no campo 
// de pesquisa do Telegram o contato 'IDBot' e enviar a mensagem "/getid" (sem aspas).
#define CHAT "*********"

#define MAX_TEMP 25 // Temperatura máxima para emissão de alerta.

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WiFiClientSecure client;
HTTPClient http;
SMTPSession smtp;
UniversalTelegramBot bot(BOTtoken, client);

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
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
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

  if (tempC != DEVICE_DISCONNECTED_C)
  {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);
    if (tempC >= MAX_TEMP && msgSend == 0)
    {
      send_email();
      send_sms();
      bot.sendMessage(CHAT, "A temperatura do freezer está acima do permitido.");
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
  String httpRequestData = "value1=" + String(random(40)) + "&value2=" + String(random(40)) + "&value3=" + String(random(40));
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
