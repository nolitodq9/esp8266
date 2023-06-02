/* Comunicación MQTT con servo empregando ESP8266

A seguinte práctica consite:

En primeiro lugar grabamos o programa de raspberry nunha tarxeta micro sd para insertar posteriormente na raspberry.
Unha vez dentro descargamos o programa de Arduino abrindo o terminal eseguindo as seguintes instruccións:

  - Escribimos o comando : sudo apt-get update + ENTER
  - A continuación: sudo apt-cache search arduino + ENTER
  - A continuación: sudo sudo apt-get install arduino               // Así xa temos descargado o Arduino na raspberry.
  
  
  
  En segundo lugar creamos unha rede wifi, axudándonos tamen do terminal:
  
  - Escribimos o comando: sudo raspi-config + ENTER.
  - A continuación abre unha ventaniña con distintas opcións na que eleximos :
      Advanced options > Network config > Network manager > OK.  + Reiniciamos
      
   - Agora a través do icono de wifi na parte superior da pantalla pinchamos na opción: 
      Advanced options > Create wireless hotspot
   - A esa rede creada chámolle "Robótica MDQ"
      Agora metémolle o sistema de seguridade "WPA & WPA2 personal" e introducimos un contrasinal de seguridade + Reiniciamos.
      
   - Neste punto xa temos creada a rede wifi dende a que conectarnos coa nosa tarxeta ESP8266.
   
   En terceiro lugar accedemos a unha rede mqtth, que é un protocolo de mensaxería ao que nos conectamos para poder comunicarnos a través
    de mensaxes entre distintos dispositivos conectados a ela.
    
    - Buscaríamos coa tarxeta Esp8266 conectarnos, para así poder enviar consignas dende o ordenador ou noso movil a través dunha aplicación. 
    - Con elo e a librería servo intentaríamos manexar un servo, pero eu non cheguei a ese punto. Aínda así o código está escrito no meu GitHub.
    
    Autor: Manuel Domínguez Queiruga
    Data: 02/06/2023
   
   */
 






#include <ESP8266WiFi.h>
#include <Servo.h>
#include <PubSubClient.h>


//wifi Mobil
#define MAX_INTENTOS 50
#define WIFI_SSID "RoboticaMDQ"
#define WIFI_PASS "pasarMDQ"


// MQTT
// Datos MQTTHQ brocker en mqtthq.com
// URL: public.mqtthq.com  //TCP Port: 1883
//WebSocket Port(porto): 8083
//Websocket Path (camiño): /mqtt
#define MQTT_HOST IPAddress(52, 13, 116, 147)     // Que colla estos numeros como dirección IP
#define MQTT_NOME_PORT 1883                    // Indicamos cal é o porto

// Servo
#define SERVOPIN 0                                // Pin D3(GPIO0), por alguna razón no funciona co nome wemos // declaración dunha macro que o preprocesador cambia en todos os lugares do codigo sen necesidade de itilizar variables que ocupan memoria
Servo servo;                                      // Declaración de variables. (declaración dun obxeto)
#define MQTT_PUB_SERVO "wemos/robotica/servo"     // Se o queremos cambiar no brocker, ir a web e ali cambiamos o nome. ten que existir no que publica e dde te suscribes
#define MQTT_NOME_CLIENTE "Cliente servo"         // É por se utilizamos varios brockers, así distinguilos

WiFiClient espClient;                             // Indicamos o cliente que imos utilizar á wifi (ESP)
PubSubClient mqttClient(espClient);               // Creamos un elemento mqttCliente. Esto foi para poder conectarnos
 
// Pins datos
// GPI014 D5
#define LED 14

bool conectado = false;
int tempo = 500;
int posicion = 0;

void setup () {                                        // setup preparado para: 1- de mensaxes por porto serie 2- se conecte o led pa escintilar 3- se conecte o servo para escribir nomes 4- se conecte wifi 5-primeico conectar wifi e despois ao servidor!! 6- unha vez conectado ao mqtt indicamos a mqttclient cal é a función que imos utilizar pa mandar menaxes
Serial.begin(115200);                                  //HABILITAMOS PORTO SERIE PARA COMUNICARNOS
pinMode(LED, OUTPUT); 
servo.attach(SERVOPIN);                               // Arriba creamos un obxeto servo, agora ese obxeto ten funcións asociadas, que van cun "punto" (servo.attach)". Attach engádelle o pin no que vai estar escoitando. dille a mqtt que se conete a esa wifi
conectado = conectarWiFi();                           //conectamos a wifi
mqttClient.setServer (MQTT_HOST, MQTT_NOME_PORT);          //setserver é unha variable miña, pero estña asociada ao obxeto seguinte        //mqttClient é a variable. conectámonos ao CLIENTE. Obxeto que creamos arriba asociandose a este cliente wifi.   Ahora xa sabe a wifi pola que saír, Agora Indicamos donde escoitar, dicímolle o Host + porto
mqttClient.setCallback (callback);                    //  Como facer pa que mande mensaxes, etc. É unha variable de victor pero é unha funcion asociada co obxecto. esa función está definica abaixo dde está a loxica do que queremos que faga o mqtt


}

void loop () {
  if(conectado) escintila(tempo);
  else escintila(tempo/10);
  // non imos complicar o programa dicindolle que comprobe se escintila
}

void escintila(int espera){
  digitalWrite(LED, HIGH);
  delay (espera);
  digitalWrite(LED, LOW);
  delay(espera);

}

//Funcion que se encarga de xestionar a conexion a rede
bool conectarWiFi() {
  WiFi.mode(WIFI_STA);                                                     // asegura que nos conectamos como modo host conectado a wifi existente
  WiFi.disconnect();                                                      // desconectamos unha posible conexion previa
  WiFi.begin(WIFI_SSID, WIFI_PASS);                                        // inicia una conexion
  Serial.print("\n\nAgardando pola WiFi");    
  int contador = 0;                                                       // COMPOROBA O ESTADO DA CONEXION E FAI VARIAS TENTATIVAS
  while(WiFi.status() != WL_CONNECTED and contador < MAX_INTENTOS){       //comprueba o estado da wifi
    contador++;
    delay (500);
    Serial.print(".");
    
    }
    Serial.println("");
    //INFORMA DO ESTADO DA CONEXION IP E EN CASO DE EXITO
    if(contador < MAX_INTENTOS){
    Serial.print("Conectado a WiFi coa IP: "); Serial.println(WiFi.localIP());
    conectado = true;
  }
  else {
    Serial.print("No se pudo conectar a WiFi");
    conectado = false;
    }

  return (conectado);
  
  }

  //FUNCIÓNS PARA CONEXIÓN SERVIDOR MQTT
 /* ====================================================================================================
 
     -Definición da función callback local
     -É chamada polo método callback do obxeto que descrebe a conexión MQTT, cada
     vez que un dispositivopublica unha mensaxe nova nun canal (topic) ao que está 
     suscrito este ESP8266.

     -Nesta función vai tamén a lóxica que fai accionar os actuadores (servo) conforme
     o mensaxe que reciban no topic ao que estén subscritos.

     -Os sensores publican desde a función ´loop()´ non desde esta función. 

     ================================================================================================
     */

     void callback (String topic, byte* message, unsigned int len) {           //O topic noso era a direccion aquela: wemos../../    as variables que imos recibir van ser bytes e nos imos ter que convertilas a letra, e indicar tame a lonxitude 
      Serial.print("nova mensaxe no topic:  "      ); Serial.print(topic);    // escribe cal é o topic e que se vexa o que está pasando
      Serial.print(". Mensaxe: ");                                             // a continuación que escriba iso
      String mensaxeTmp = "";                                                   // creo 1 variable, un mensaxe sen nada (poñemos string que é mais útil para utilizar)
      for(int i=0; i< len; i++) {                                             // utilixzamos o bucle for(sabemos as veces que se vai repetir): crea variable i, que sea o indice, e mentras o indice sexa menor que len(lonxitude da mensaxe) e vai ir lendo os caracteres, comezando en 0.. e cada vez que remate unha iteracin i++  /asiq que chega a 5 remata este for chega ao i++
        Serial.print((char)message[i]);                                       // imprime i. estamos forzando a que convirta o Byte a un  (caracter) char =  (casting)
        mensaxeTmp += (char)message[i];                                        // sumalle char ao valor que teña e cando remate println, que pase de liña
      }
      Serial.println();

      // lóxica que se executa ao recibir o payload
      accionarServo(mensaxeTmp);                                              // loxica. acciona o servo coa mensaxe, que pode ser centro, medio, esquerda...

     }
     
      /*=============================================================================================================================

      -Definicion da funcion reconnect local ( metodo do obxecto 'mqttClient).
      -Cada vez que se incluia unha nova iteracion do 'loop(), compróbase que existe a
      conexión ao servidor MQTT. Se non é así chámase a esta función.

          -Encárgase de:
                -conectar ao servidor MQTT
                -comunicar por saida serie o estado da conexion MQTT
                -subscribir os sensores/actuadores declarados no topic correspondente
      ==============================================================================================================================
      */
      void reconnect (){
        //Mentres non se reconecta ao servidor MQTT
        while(!espClient.connected()) {
          Serial.print("Tentando conectar ao sevidor MQTT. . .");
          if(mqttClient.connect(MQTT_NOME_CLIENTE)) {
            Serial.print(" Conectado ");
            mqttClient.subscribe(MQTT_PUB_SERVO);

          }
          else {
            Serial.print("Fallo ao conectar ao servidor MQTT, rc=");
            Serial.print(mqttClient.state());
            Serial.println( " nova tentativa en 5 s");
            delay(5000);
          }
        }
      }


     void accionarServo(String orde) {   // declaramos a funcion
          //comprobamos se hai orde no teclado
      orde.toLowerCase();
      if(orde.equals("esquerda")) posicion = 180;
      else if(orde.equals("dereita")) posicion = 0;
      else if(orde.equals("centro")) posicion = 90;
      else {
        int tmp = orde.toInt();
        if(tmp >=0 && tmp <=180) posicion = tmp;
        else posicion = 0 ;

    }
    servo.write(posicion);
    delay(tempo);




     }
