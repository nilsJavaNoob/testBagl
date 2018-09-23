#pragma GCC optimize ("-Os")
002
//#pragma pack(push, 1)     // выравнивание по одному байту ????????
003
 
004
#include <SPI.h>
005
#include <EEPROM.h>
006
#include <avr/wdt.h>
007
#include <leOS.h>      // Шедуллер задач
008
#include <dht.h>       // Дачик влажности и температуры
009
#include "Ucglib.h"    // ВНИМАНИЕ использовать библиотеку не познее 1.01 справка <a href="https://code.google.com/p/ucglib/wiki/" rel="nofollow">https://code.google.com/p/ucglib/wiki/</a>
010
#include "rusFont.h"   // Русские шрифты
011
#include "nRF24L01.h"  // Беcпроводной модуль
012
#include "RF24.h"      // Беcпроводной модуль
013
 
014
// - ОПЦИИ -------------------------------
015
//#define DEBUG                                   // Отладочную  информацию в ком порт посылает 
016
//#define DEMO                                    // Признак демонстрации - данные с датчиков генерятся рандом
017
#define BEEP                                    // Использовать пищалку
018
#define RADIO                                   // Признак использования радио модуля
019
#define VERSION "Version: 0.58 28/08/15"        // Текущая версия
020
#define ID             0x21                     // уникально Идентификатор устройства - старшие 4 бита, вторые (младшие) 4 бита серийный номер устройства
021
 
022
 
023
// Макросы для работы с портами  скорость и место
024
#define SetOutput(port,bit)       DDR ## port |= _BV(bit)
025
#define SetInput(port,bit)        DDR ## port &= ~_BV(bit)
026
#define SetBit(port,bit)          PORT ## port |= _BV(bit)
027
#define ClearBit(port,bit)        PORT ## port &= ~_BV(bit)
028
#define WritePort(port,bit,value) PORT ## port = (PORT ## port & ~_BV(bit)) | ((value & 1) << bit)
029
#define ReadPort(port,bit)        (PIN ## port >> bit) & 1
030
#define PullUp(port,bit)          { SetInput(port,bit); SetBit(port,bit); }
031
#define Release(port,bit)         { SetInput(port,bit); ClearBit(port,bit); }
032
// Мои макросы
033
#define MOTOR_BIT                 0            // бит мотора в packet.flags
034
#define HEAT_BIT                  1            // бит калорифера в packet.flags
035
 
036
#define FLAG_MOTOR_ON             packet.flags |= (1<<MOTOR_BIT)   // бит мотора установить в 1
037
#define FLAG_MOTOR_OFF            packet.flags &= ~(1<<MOTOR_BIT)  // бит мотора установить в 0
038
#define FLAG_MOTOR_CHECK          packet.flags & (1<<MOTOR_BIT)    // бит мотора проверить на 1
039
#define MOTOR_ON                  { WritePort(C,0,HIGH); FLAG_MOTOR_ON;  }   // включить мотор
040
#define MOTOR_OFF                 { WritePort(C,0,LOW) ; FLAG_MOTOR_OFF; }   // выключить мотор
041
 
042
#define FLAG_HEAT_ON              packet.flags |= (1<<HEAT_BIT)   // бит калорифера установить в 1
043
#define FLAG_HEAT_OFF             packet.flags &= ~(1<<HEAT_BIT)  // бит калорифера установить в 0
044
#define FLAG_HEAT_CHECK           packet.flags & (1<<HEAT_BIT)    // бит калорифера проверить на 1
045
#define HEAT_ON                   { WritePort(C,2,HIGH); FLAG_HEAT_ON;  }   // включить калорифер
046
#define HEAT_OFF                  { WritePort(C,2,LOW); FLAG_HEAT_OFF; }   // выключить калорифер
047
 
048
 
049
// - КОНСТАНТЫ --------------------------------------
050
#define dH_OFF          15                      // Гистерезис абсолютной влажности в сотых грамма на куб
051
#define dT_OFF          25                      // Гистерезис температуры в сотых градуса
052
#define TEMP_LOW       200                      // Температура подвала критическая (в сотых градуса) - система выключается и включается калорифер
053
// СИСТЕМАТИЧЕСКИЕ ОШИБКИ ДАТЧИКОВ для ID 0x21  ОШИБКИ ДОБАВЛЯЮТСЯ!!
054
#define TOUT_ERR      -40                        // Ошибка уличного датчика температуры в сотых долях градуса
055
#define TIN_ERR       -40                        // Ошибка домового датчика температуры в сотых долях градуса
056
#define HOUT_ERR      -30                        // Ошибка уличного датчика влажности в сотых долях %
057
#define HIN_ERR       +30                        // Ошибка домового датчика влажности в сотых долях %
058
 
059
// - ВРЕМЕНА ---------------------------------------
060
#ifdef DEMO                                     // Для демо все быстрее и случайным образом
061
    #define NUM_SAMPLES      2                  // Число усреднений измерений датчика
062
    #define TIME_SCAN_SENSOR 1000               // Время опроса датчиков мсек, для демки быстрее
063
    #define TIME_PRINT_CHART 4000               // Время вывода точки графика мсек, для демки быстрее
064
    #define TIME_HOUR        50000              // Число мсек в часе, для демки быстрее  
065
#else  
066
   #define NUM_SAMPLES      10                  // Число усреднений измерений датчика
067
   #define TIME_SCAN_SENSOR 2000                // Время опроса датчиков мсек
068
   #define TIME_PRINT_CHART 300000              // Время вывода точки графика мсек
069
   #define TIME_HOUR        3600000             // Число мсек в часе
070
#endif
071
 
072
#define LONG_KEY      3000                      // Длительное нажатие кнопки мсек, появляется Экран инфо
073
#define SHORT_KEY     150                       // Короткое нажатие кнопки более мсек
074
#define NRF24_CHANEL  100                       // Номер канала nrf24
075
 
076
//  НОГИ к которым прицеплена переферия (SPI используется для TFT и NRF24 - 11,12,13)
077
#define PIN_HEAT      16                         // Ножка куда повешен калорифер A2 (port C2)
078
#ifdef BEEP
079
    #define PIN_BEEP  15                         // Ножка куда повешена пищалка A1 (port C1)
080
#endif
081
#define PIN_RELAY     14                         // Ножка на которую повешено реле (SSR) вентилятора - аналоговый вход A0 через резистор 470 ом (port C0)
082
#define PIN_CS        10                         // TFT дисплей spi
083
#define PIN_CD        9                          // TFT дисплей spi
084
#define PIN_RESET     8                          // TFT дисплей spi
085
#define PIN_CE        7                          // nrf24 ce
086
#define PIN_CSN       6                          // nrf24 cs
087
#define PIN_DHT22a    5                          // Первый датчик DHT22   IN  ДОМ
088
#define PIN_DHT22b    4                          // Второй датчик DHT22   OUT УЛИЦА
089
#define PIN_KEY       3                          // Кнопка, повешена на прерывание, что бы ресурсов не тратить (port D3)
090
#define PIN_IRQ_NRF24 2                          // Ножка куда заведено прерывание от NRF24 (пока не используется)
091
 
092
// Настройки
093
#define NUM_SETTING    7                        // Число вариантов настроек
094
#define BLOCK_OFF      0                        // Выключено (вариант настроек)
095
#define HOOD_ON        1                        // Режим вытяжки (вариант настроек)
096
#define COOLING        2                        // Режим хлаждение (вариант настроек)
097
 
098
// АЦП ----------------------------------------
099
const long ConstADC=1126400;                    // Калибровка встроенного АЦП (встроенный ИОН) по умолчанию 1126400 дальше измеряем питание и смотрим на дисплей   
100
 
101
Ucglib_ILI9341_18x240x320_HWSPI ucg(/*cd=*/PIN_CD, /*cs=*/PIN_CS, /*reset=*/PIN_RESET); // Аппаратный SPI на дисплей ILI9341
102
leOS myOS;                                      // многозадачность
103
dht DHT;                                        // Датчики
104
bool infoScreen=false;                          // Признак отображениея иформационного экрана  1 - на экран ничего не выводится кроме информационного экрана
105
bool flagKey=false;                             // Флаг нажатия клавиши
106
bool pressKey=false;                            // Флаг необходимости обработки кнопки
107
 
108
unsigned long time_key=0;                       // Время нажатия копки
109
byte last_error=100;                            // Предыдущая ошибка чтения датчиков
110
 
111
 struct type_setting_eeprom                     // Структура для сохранения данных в eeprom
112
 {
113
     byte fStart = 0;                           // Какой набор настроек используется
114
     unsigned long hour_unit;                   // мото часы блок измеряется в интервалах вывода графика TIME_PRINT_CHART
115
     unsigned long hour_motor;                  // мото часы мотор измеряется в интервалах вывода графика TIME_PRINT_CHART
116
 };
117
volatile type_setting_eeprom settingRAM;        // Рабочая копия счетчиков в памяти
118
type_setting_eeprom settingEEPROM EEMEM;        // Копия счетчиков в eeprom - туда пишем
119
 
120
// Пакет передаваемый, используется также для хранения результатов.
121
 struct type_packet_NRF24   // Версия 2.3!! адаптация для stm32 Структура передаваемого пакета 30 байта - 32 максимум
122
    {
123
        byte id=ID;                                 // Идентификатор типа устройства - старшие 4 бита, вторые (младшие) 4 бита серийный номер устройства
124
        byte error;                                 // Ошибка разряды: 0-1 первый датчик (00-ок) 2-3 второй датчик (00-ок) 4 - радиоканал    
125
        int16_t   tOut=-500,tIn=-500;               // Текущие температуры в сотых градуса !!! место экономим
126
        uint16_t  absHOut=456,absHIn=123;           // Абсолютные влажности в сотых грамма на м*3 !!! место экономим
127
        uint16_t  relHOut=456,relHIn=123;           // Относительные влажности сотых процента !!! место экономим
128
        uint8_t   flags=0x00;                       // байт флагов  0 бит - мотор включен/выключен 1 бит - калорифер включен/выключен
129
        uint8_t   dH_min;                           // Порог включения вентилятора по РАЗНИЦЕ абсолютной влажности в сотых грамма на м*3
130
        int16_t   T_min;                            // Порог выключения вентилятора по температуре в сотых градуса
131
        char note[12] = "ArduDry :-)";              // Примечание не более 13 байт + 0 байт
132
    } packet;
133
     
134
struct type_sensors                                 // структура для усреднения
135
{
136
       int  num;                                    // сколько отсчетов уже сложили не болле NUM_SAMPLES
137
       long  sum_tOut=-5000,sum_tIn=-5000;          // Сумма для усреднения Текущие температуры в сотых градуса !!! место экономим
138
       long  sum_relHOut=55555,sum_relHIn=55555;    // Сумма для усреднения Относительные влажности сотых процента !!! место экономим
139
       int  tOut=-5000,tIn=-5000;                   // Текущие температуры в сотых градуса !!! место экономим
140
       int  absHOut=55555,absHIn=55555;             // Абсолютные влажности в сотых грамма на м*3 !!! место экономим
141
       int  relHOut=55555,relHIn=55555;             // Относительные влажности сотых процента !!! место экономим
142
} sensors;
143
     
144
// Массивы для графиков
145
uint8_t tOutChart[120];
146
uint8_t tInChart[120];
147
uint8_t absHOutChart[120];
148
uint8_t absHInChart[120];
149
uint8_t posChart=0;       // Позиция в массиве графиков - начало вывода от 0 до 120-1
150
uint8_t TimeChart=0;      // Время до вывода очередной точки на график.
151
bool ChartMotor=false;    // Признак работы мотора во время интервала графика если мотор был включен на любое время то на графике фон меняется в этом месте
152
 
153
 
154
#ifdef  RADIO    // Радио модуль NRF42l 
155
   RF24 radio(PIN_CE, PIN_CSN);  // определение управляющих ног
156
   bool send_packet_ok=false;    // признак удачной отправки последнего пакета
157
#endif
158
///////////////////////////////////////////////////////////////////////////////////////////////////////////
159
// ПРОГРАММА
160
///////////////////////////////////////////////////////////////////////////////////////////////////////////
161
void setup(){
162
#ifdef  DEBUG 
163
   Serial.begin(9600);
164
   Serial.println(F("DEBUG MODE"));
165
   #ifdef  BEEP 
166
     Serial.println(F("BEEP ON"));
167
   #else        
168
     Serial.println(F("BEEP OFF"));
169
   #endif
170
   #ifdef  RADIO
171
     Serial.println(F("RADIO ON"));
172
   #else        
173
     Serial.println(F("RADIO OFF"));
174
   #endif
175
#endif
176
 
177
reset_sum();
178
 
179
#ifdef  RADIO    // Радио модуль NRF42l  первичная настройка
180
   radio.begin();
181
   radio.setDataRate(RF24_250KBPS);         //  выбор скорости RF24_250KBPS RF24_1MBPS RF24_2MBPS
182
   radio.setPALevel(RF24_PA_MAX);           // выходная мощность передатчика
183
   radio.setChannel(NRF24_CHANEL);          //тут установка канала
184
   radio.setCRCLength(RF24_CRC_16);         // использовать контрольную сумму в 16 бит
185
   radio.setAutoAck(1);                 // выключить аппаратное потверждение
186
// radio.enableDynamicPayloads();           // разрешить Dynamic Payloads
187
// radio.enableAckPayload();                // разрешить AckPayload
188
  radio.setRetries(10,50);                  // Количество повторов и пауза между повторами
189
  radio.openWritingPipe(0xF0F0F0F0E1LL);    // передатчик
190
  radio.openReadingPipe(1,0xF0F0F0F0D2LL);  // приемник
191
  radio.startListening();
192
#endif
193
  #ifdef BEEP
194
     SetOutput(C,1);                      //  Настройка ноги для динамика
195
     WritePort(C,1,LOW);
196
  #endif
197
  SetInput(D,3);                        //  Включена кнопка
198
  WritePort(D,3,HIGH);
199
  SetOutput(C,0);                       //  Подключить Реле
200
  WritePort(C,0,LOW);
201
  SetOutput(C,2);                       //  Подключить Калорифер
202
  WritePort(C,2,LOW);
203
   
204
  pinMode(PIN_DHT22a, OUTPUT);          //  Датчик DHT22 #1
205
  digitalWrite(PIN_DHT22a, HIGH); 
206
  pinMode(PIN_DHT22b, OUTPUT);          //  Датчик DHT22 #2
207
  digitalWrite(PIN_DHT22b, HIGH); 
208
   
209
  reset_ili9341();   // сброс дисплея
  ========================
  readEeprom();       // Прочитать настройки
2
byte i=ReadPort(D,3);
3
 if (i==0)         // Если при включении нажата кнопка то стираем Eeprom
4
 {   settingRAM.fStart=0;

  ========================
210
  readEeprom();                         // Прочитать настройки
211byte i=ReadPort(D,3);        // Если при включении нажата кнопка то стираем Eeprom
212 if(i==0){
213     settingRAM.fStart=0;
214     settingRAM.hour_unit=0;
215     settingRAM.hour_motor=0;
216     ucg.setColor(255, 255, 255);
217     print_StrXY(10,50,F("Сброс настроек и счетчиков"));
218     for(i=0;i<3;i++)
219     {
220     delay(1000);
221     ucg.print(F(" ."));
222     }
223
      writeEeprom();       // Запись в EEPROM 
224
      delay(1000);
225
      ucg.clearScreen();
226
    }
227
    
228 wdt_enable(WDTO_8S); // Сторожевой таймер Для тестов не рекомендуется устанавливать значение менее 8 сек.
229
  // Запуск задач по таймеру
230
  myOS.begin();
231
  myOS.addTask(measurement,TIME_SCAN_SENSOR);    // Измерение
232
  attachInterrupt(1, scanKey, CHANGE);           // КНОПКА Прерывания по обоим фронтам
233
  print_static();                                // распечатать таблицу
234
  Setting();                                     // Применить настройки
235
  measurement();                                 // Считать данные
236
  #ifdef BEEP
237
     beep(100);
238
  #endif
239
resetKey();
240
}
241
 
242
void loop()
243
{
244
 wdt_reset();                        // Сбросить сторожевой таймер
245
// Обработка нажатия кнопки
246
if (pressKey==true) // Кнопка была нажата
247
{
248
 myOS.pauseTask(measurement);                                 // Остановить задачи
249
 if (flagKey!=false)                                          // Обработчик кнопки
250
 if (time_key > LONG_KEY)                                     // Длительное нажатие кнопки
251
   printInfo();                                               // Вывод информационного экрана
252
 else
253
   if (time_key > SHORT_KEY)                                  // Короткое нажатие кнопки
254
     {
255
        #ifdef BEEP
256
          beep(30);                                            // Звук нажатия на клавишу
257
        #endif
258
       if (infoScreen==true)  clearInfo();                    // если информационный экран то стереть его
259
       else {if (settingRAM.fStart >= NUM_SETTING) settingRAM.fStart=0; //  Кольцевой счетчик настроек
260
               else settingRAM.fStart++;                      // В противном случае следующая настройка
261
               Setting(); } 
262
     }
263
  resetKey();                                                //  Сброс состояния кнопки
264
  myOS.restartTask(measurement);                             // Запустить задачи
265
  }
266
}
267
 
268
void resetKey(void)  // Сброс состяния кнопки
269
{
270
  flagKey=false;                                            
271
  time_key=millis();                                               
272
  pressKey=false;
273
} 
274
void print_static()  // Печать статической картинки
275
{
276
   cli();
277
  // Заголовок
278
  ucg.setColor(0, 0, 180);  //
279
  ucg.drawBox(0, 0, 320-1, 23);
280
  ucg.setColor(250, 250, 250);
281
  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
282
  print_StrXY(2,19,F("ОСУШИТЕЛЬ ID: 0x"));
283
  ucg.print( hex(packet.id >> 4));
284
  ucg.print( hex(packet.id&0x0f));
285
    
286
  #ifdef DEMO
287
    ucg.print(F(" demo"));
288
  #endif
289
   
290
  // Таблица для данных
291
  ucg.setColor(0, 200, 0);
292
  ucg.drawHLine(0,25,320-1);
293
  ucg.drawHLine(0,25+23*1,320-1);
294
  ucg.drawHLine(0,25+23*2,320-1);
295
  ucg.drawHLine(0,25+23*3,320-1);
296
  ucg.drawHLine(0,25+23*4,320-1);
297
  ucg.drawVLine(200-4,25,24+23*3);
298
  ucg.drawVLine(260,25,24+23*3);
299
 
300
  // Заголовки таблиц
301
  ucg.setColor(255, 255, 0);
302
  print_StrXY(180+30,25+0+18,F("Дом"));
303
  print_StrXY(250+20,25+0+18,F("Улица"));
304
  print_StrXY(0,25+23*1+18,F("Температура градусы C"));
305
  print_StrXY(0,25+23*2+18,F("Относительная влаж. %"));
306
  print_StrXY(0,25+23*3+18,F("Абсолют. влаж. г/м*3"));
307
 
308
  // Графики
309
  ucg.setColor(220, 220, 200);
310
  ucg.drawHLine(1,240-1,130);
311
  ucg.drawVLine(1,135,105);
312
  ucg.drawHLine(10+150,240-1,130);
313
  ucg.drawVLine(10+150,135,105);
314
  print_StrXY(10,135+0,F("Температура"));
315
  print_StrXY(20+150,135+0,F("Абс. влажность"));
316
   
317
  // надписи на графиках
318
  print_StrXY(128,154,F("+20"));
319
  print_StrXY(135,194,F("0"));
320
  print_StrXY(128,233,F("-20"));
321
   
322
  print_StrXY(296,164,F("15"));
323
  print_StrXY(296,194,F("10"));
324
  print_StrXY(296,223,F("5"));
325
   sei();
326
}
327
 
328
void print_status(void) // Печать панели статуса Значки на статус панели
329
{
330
  if (infoScreen==true) return;        // если отображен информационный экран то ничего не выводим 
331
   cli();
332
 // 1. печать ошибки чтения датчиков
333
   print_error_DHT();
334
 // 2. Признак включения мотора
335
  if (FLAG_MOTOR_CHECK)    ucg.setColor(0, 240, 0);
336
  else                     ucg.setColor(0, 40, 0);
337
  ucg.drawBox(290-32, 5, 14, 14);
338
   
339
  #ifdef  RADIO
340
  // 3. Признак удачной передачи информации по радиоканалу
341
      if (send_packet_ok==true)  ucg.setColor(0, 240, 0);
342
      else                       ucg.setColor(0, 40, 0);
343
      ucg.setPrintDir(3);
344
      ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
345
      print_StrXY(290-40,20,F(">>"));
346
      ucg.setPrintDir(0);
347
      ucg.setFontMode(UCG_FONT_MODE_SOLID);
348
  #endif
349
  sei();
350
} 
351
 
352
void print_error_DHT(void) // Печать ошибки чтения датчиков выводится при каждом чтении датчика
353
{
354
  if (infoScreen==true) return;        // если отображен информационный экран то ничего не выводим 
355
 // 1. печать ошибки чтения датчиков
356
  if (packet.error!=last_error)        // если статус ошибки поменялся то надо вывести если нет то не выводим - экономия время и нет мерцания
357
  {
358
      cli();
359
      last_error=packet.error;
360
      ucg.setColor(0, 0, 180);         // Сначала стереть
361
      ucg.drawBox(290, 0, 26, 18);
362
      ucg.setPrintPos(290,18);
363
      ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
364
      if (packet.error>0)
365
        {
366
        ucg.setColor(255, 100, 100);
367
        print_StrXY(280,19,F("0x"));
368
        ucg.print( hex(packet.error >> 4));
369
        ucg.print( hex(packet.error & 0x0f));
370
        }
371
      else  { ucg.setColor(200, 240, 0);   ucg.print(F("ok")); }
372
     sei();
373
   }  
374
}
375
//  вывод на экран данных (то что меняется)
376
void print_data()
377
{
378
 if (infoScreen==true) return;                  // если отображен информационный экран то ничего не выводим 
379
  cli();
380
 // Печать значений для дома
381
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
382
  ucg.setColor(250, 0, 100);  // Цвет ДОМА
383
  print_floatXY(200+0,25+23*1+18,((float)packet.tIn)/100);
384
  print_floatXY(200+0,25+23*2+18,((float)packet.relHIn)/100);
385
  print_floatXY(200+0,25+23*3+18,((float)packet.absHIn)/100);
386
  ucg.setColor(0, 250, 100);  // Цвет УЛИЦЫ
387
  print_floatXY(260+4,25+23*1+18,((float)packet.tOut)/100);
388
  print_floatXY(260+6,25+23*2+18,((float)packet.relHOut)/100);
389
  print_floatXY(260+6,25+23*3+18,((float)packet.absHOut)/100);
390
  sei();
391
} 
392
 
393
// Печать графика на экране, добавляется одна точка и график сдвигается
394
void printChart()
395
{
396
byte i,x=0;
397
uint8_t tmp;
398
byte tInLast,absHInLast,tOutLast,absHOutLast;
399
 
400
// Статистика по моточасам, время ведется в тиках графика а потом пересчитывается в часы при выводе.
401
settingRAM.hour_unit++;
402
if (FLAG_MOTOR_CHECK) settingRAM.hour_motor++;
403
 
404
// Работаем через кольцевой буфер
405
// Добавить новую точку в кольцевой буфер
406
     // Температура в доме. диапазон -25 . . . +25 растягиваем на 100 точек
407
     tInLast=tInChart[posChart];                       // Сохранить точку для стирания на графике
408
     if (packet.tIn<=-2500) tInChart[posChart]=0;      // Если температура меньше -25 то округляем до -25
409
     if (packet.tIn>=2500)  tInChart[posChart]=100;    // Если температура больше 25  то округляем до 25
410
     if ((packet.tIn>-2500)&&(packet.tIn<2500)) tInChart[posChart]=((2*packet.tIn)+5000)/100;  // внутри -25...+25 растягиваем в два раза
411
     if (ChartMotor==true) tInChart[posChart]=tInChart[posChart]+0x80;                       // Признак включения мотора- старший бит в 1 - цвет фона на графике меняется
412
     ChartMotor=false;
413
     // Температура на улице. диапазон -25 . . . +25 растягиваем на 100 точек
414
     tOutLast=tOutChart[posChart];                       // Сохранить точку для стирания на графике
415
     if (packet.tOut<=-2500) tOutChart[posChart]=0;      // Если температура меньше -25 то округляем до -25
416
     if (packet.tOut>=2500)  tOutChart[posChart]=100;    // Если температура больше 25  то округляем до 25
417
     if ((packet.tOut>-2500)&&(packet.tOut<2500)) tOutChart[posChart]=((2*packet.tOut)+5000)/100;  // внутри -25...+25 растягиваем в два раза
418
     // Абсолютная влажность в доме диапазон от 0 до 20 грамм на кубометр, растягиваем на 100 точек
419
     absHInLast=absHInChart[posChart];                   // Сохранить точку для стирания на графике
420
     if (packet.absHIn>=2000) absHInChart[posChart]=100;
421
     else absHInChart[posChart]=(5*packet.absHIn)/100;   // внутри 0...20 растягиваем в пять  раз
422
     // Абсолютная влажность на улицу диапазон от 0 до 20 грамм на кубометр, растягиваем на 100 точек
423
     absHOutLast=absHOutChart[posChart];                   // Сохранить точку для стирания на графике
424
     if (packet.absHOut>=2000) absHOutChart[posChart]=100;
425
     else absHOutChart[posChart]=(5*packet.absHOut)/100;   // внутри 0...20 растягиваем в пять раз
426
      
427
  if (infoScreen==false)                 // если отображен информационный экран то ничего не выводим
428
   {
429
   cli(); 
430
   for(i=0;i<120;i++)    // График слева на право
431
     {
432
     // Вычислить координаты текущей точки x в кольцевом буфере. Изменяются от 0 до 120-1
433
     if (posChart<i) x=120+posChart-i; else x=posChart-i;
434
     
435
     // нарисовать фон в зависимости от статуса мотора
436
     if  (tInChart[x]>0x80)  ucg.setColor(80, 80, 0);  // Мотор быв ключен
437
     else                    ucg.setColor(0, 0, 0);    // Мотор был выключен
438
     ucg.drawVLine(5+120-i,238-101,101);
439
     ucg.drawVLine(5+120-i+158+1,238-101,101); 
440
      
441
     ucg.setColor(180, 180, 180); 
442
     if (x%5==0) // Пунктирные линии графика
443
     {
444
       ucg.drawPixel(5+120-i,237-10);
445
       ucg.drawPixel(5+120-i,237-50);
446
       ucg.drawPixel(5+120-i,237-90);
447
        
448
       ucg.drawPixel(6+120-i+158,237-25);
449
       ucg.drawPixel(6+120-i+158,237-50);
450
       ucg.drawPixel(6+120-i+158,237-75); 
451
     }
452
      
453
     // Вывести новую точку
454
     tmp=tInChart[x]&0x7f;   // Отбросить старший разряд - признак включения мотора
455
     if ((tmp==0)||(tmp==100))   ucg.setColor(255, 255, 255); else ucg.setColor(250, 0, 100);
456
     ucg.drawPixel(5+120-i,237-tmp);
457
      
458
     if (absHInChart[x]==100) ucg.setColor(255, 255, 255); else ucg.setColor(250, 0, 100);
459
     ucg.drawPixel(6+120-i+158,237-absHInChart[x]);
460
      
461
     if ((tOutChart[x]==0) || (tOutChart[x]==100)) ucg.setColor(255, 255, 255); else ucg.setColor(0, 250, 100);
462
     ucg.drawPixel(5+120-i,237-tOutChart[x]);
463
      
464
     if (absHOutChart[x]==100) ucg.setColor(255, 255, 255); else ucg.setColor(0, 250, 100);
465
     ucg.drawPixel(6+120-i+158,237-absHOutChart[x]);
466
      }
467
     sei();
468
   }
469
 if (posChart<120-1) posChart++; else posChart=0;            // Изменили положение в буфере и Замкнули буфер
470
}
471
 
472
// ---ПЕРЕДАЧА ДАННЫХ ЧЕРЕЗ РАДИОМОДУЛЬ -----------------------------
473
#ifdef  RADIO    // Радио модуль NRF42l
474
void send_packet()
475
{      
476
        radio.stopListening();     // Остановить приемник
477
        delay(2);
478
        radio.openWritingPipe(0xF0F0F0F0E1LL);    // передатчик
479
        cli();
480
            send_packet_ok = radio.write(&packet,sizeof(packet));
481
        sei();
482
         #ifdef BEEP
483
           if (send_packet_ok==true) beep(40);           // Пакет передан успешно
484
         #endif
485
         #ifdef  DEBUG 
486
           if (send_packet_ok==true)  Serial.println(F("Packet sending ++++++++++"));
487
           else                       Serial.println(F("Packet NOT sending -----------"));
488
         #endif  
489
        radio.startListening();    // Включить приемник
490
 } 
491
#endif
492
// Чтение датчика возвращает код ошибки:
493
// DHTLIB_OK                   0
494
// DHTLIB_ERROR_CHECKSUM       1
495
// DHTLIB_ERROR_TIMEOUT        2
496
// DHTLIB_ERROR_CONNECT        3
497
// DHTLIB_ERROR_ACK_L          4
498
// DHTLIB_ERROR_ACK_H          5
499
byte readDHT(byte pin)
500
{
501
delay(5);
502
cli();
503
  byte err=-1*DHT.read22(pin); // Чтение датчика
504
sei(); 
505
return err;
506
}
507
 
508
// Измерение и обработка данных чтение датчиков --------------------------------------------------------------------------------
509
void measurement()
510
{
511
 myOS.pauseTask(measurement);        // Обязательно здесь, а то датчики плохо читаются мешает leos
512
 wdt_reset();                        // Сбросить сторожевой таймер
513
  
514
 packet.error=readDHT(PIN_DHT22a);   // ПЕРВЫЙ ДАТЧИК ДОМ  Новый пакет, сбросить все ошибки и прочитать первый датчик
515
  
516
 #ifdef  DEMO
517
   DHT.temperature=packet.tIn/100+random(-20,30)/10.0;
518
   if (DHT.temperature>20) DHT.temperature=19;
519
   if (DHT.temperature<-10) DHT.temperature=-9;
520
   DHT.humidity=packet.relHIn/100+(float)random(-5,8);
521
   if (DHT.humidity>96) DHT.humidity=90;
522
   if (DHT.humidity<1) DHT.humidity=10;
523
   packet.error=0; // в Демо режиме
524
//   DHT.temperature=3.0;
525
//   DHT.humidity=21.0;
526
  #endif 
527
     sensors.tIn=(int)(DHT.temperature*100.0)+TIN_ERR;  // Запомнить результаты для суммирования с учетом ошибок
528
     sensors.relHIn=(int)(DHT.humidity*100.0)+HIN_ERR; 
529
      
530
    #ifdef  DEBUG 
531
       Serial.print(F("Sensor read samples:")); Serial.println(sensors.num);
532
       Serial.print(F("IN T="));Serial.print(sensors.tIn);Serial.print(F(" H=")); Serial.print(sensors.relHIn); Serial.print(F(" error=")); Serial.println(packet.error);
533
    #endif
534
     
535
 
536
  
537
 packet.error=packet.error+16*readDHT(PIN_DHT22b);// ВТОРОЙ ДАТЧИК УЛИЦА  ошибки в старшие четыре бита
538
 #ifdef  DEMO
539
   DHT.temperature=packet.tOut/100+random(-40,45)/10.0;
540
   if (DHT.temperature>30) DHT.temperature=29;
541
   if (DHT.temperature<-30) DHT.temperature=-29;
542
   DHT.humidity=packet.relHOut/100+random(-10,12);
543
   if (DHT.humidity>96) DHT.humidity=90;
544
   if (DHT.humidity<1)  DHT.humidity=10;
545
   packet.error=0;      // в Демо режиме
546
// DHT.temperature=-10.0;
547
// DHT.humidity=40.0;
548
 #endif 
549
     sensors.tOut=(int)(DHT.temperature*100.0)+TOUT_ERR;  // Запомнить результаты для суммирования с учетом ошибок
550
     sensors.relHOut=(int)(DHT.humidity*100.0)+HOUT_ERR;
551
  
552
    #ifdef  DEBUG 
553
       Serial.print(F("OUT T="));Serial.print(sensors.tOut);Serial.print(F(" H=")); Serial.print(sensors.relHOut); Serial.print(F(" error=")); Serial.println(packet.error);
554
    #endif  
555
  
556
 print_error_DHT();    // Вывод ошибки чтения датчика при каждом чтении контроль за качеством связи с датчиком
557
  
558
 if (packet.error==0)// Если чтение без ошибок у ДВУХ датчиков  копим сумму для усреднения
559
  {
560
     sensors.sum_tIn=sensors.sum_tIn+sensors.tIn;
561
     sensors.sum_relHIn=sensors.sum_relHIn+sensors.relHIn;
562
     sensors.sum_tOut=sensors.sum_tOut+sensors.tOut;
563
     sensors.sum_relHOut=sensors.sum_relHOut+sensors.relHOut;
564
     sensors.num++;
565
   }
566
  
567
 // набрали в сумме нужное число отсчетов рассчитываем усреднение и выводим
568
 if (sensors.num>=NUM_SAMPLES)  // Пора усреднять и выводить значения
569
 {
570
         resetKey();          // сброс кнопки почему то часто выскакивает длительное нажатие
571
        // вычисление средних значений
572
         packet.tIn=sensors.sum_tIn/NUM_SAMPLES;
573
         packet.relHIn=sensors.sum_relHIn/NUM_SAMPLES;
574
         packet.tOut=sensors.sum_tOut/NUM_SAMPLES;
575
         packet.relHOut=sensors.sum_relHOut/NUM_SAMPLES;
576
         reset_sum();       // Сброс счетчиков и сумм
577
          
578
         // вычисление абсолютной влажности
579
         packet.absHIn=(int)(calculationAbsH((float)(packet.tIn/100.0),(float)(packet.relHIn/100.0))*100.0);
580
         packet.absHOut=(int)(calculationAbsH((float)(packet.tOut/100.0),(float)(packet.relHOut/100.0))*100.0);
581
          
582
     #ifdef  DEBUG 
583
       Serial.println(F("Average value >>>>>>>>>>"));
584
       Serial.print(F("IN T="));Serial.print(packet.tIn);Serial.print(F(" H=")); Serial.print(packet.relHIn); Serial.print(F(" abs H=")); Serial.println(packet.absHIn);
585
       Serial.print(F("OUT T="));Serial.print(packet.tOut);Serial.print(F(" H=")); Serial.print(packet.relHOut); Serial.print(F(" abs H=")); Serial.println(packet.absHOut);
586
     #endif  
587
                      
588
         #ifdef  RADIO     // Радио модуль NRF42l
589
            send_packet();   // Послать данные
590
         #endif
591
         CheckON();                                // Проверка статуса вентилятора
592
         print_data();                             // вывод усредненных значений
593
         print_status();                           // панель состояния
594
         if (FLAG_MOTOR_CHECK) ChartMotor=true;  // Признак того что надо показывать включение мотора на графике
595
          
596
         if ((long)((long)TimeChart*TIME_SCAN_SENSOR*NUM_SAMPLES)>=(long)TIME_PRINT_CHART) // проврека не пора ли выводить график
597
            { printChart(); TimeChart=0; // Сдвиг графика и вывод новой точки
598
              #ifdef  DEBUG 
599
                 Serial.println(F("Point add chart ++++++++++++++++++++"));
600
              #endif 
601
              #ifdef BEEP
602
 //               beep(50);
603
              #endif
604
             }
605
         else TimeChart++;
606
    }
607
    myOS.restartTask(measurement);     // Пустить задачи
608
}
609
 
610
// Функция переводит относительную влажность в абсолютную
611
// t-температура в градусах Цельсия h-относительная влажность в процентах
612
float calculationAbsH(float t, float h)
613
{
614
 float temp;
615
 temp=pow(2.718281828,(17.67*t)/(t+243.5));
616
 return (6.112*temp*h*2.1674)/(273.15+t);
617
}
618
 
619
// Сканирование клавиш ------------------------------------------
620
void scanKey()
621
{ 
622
    byte key; 
623
    cli();
624
    key=ReadPort(D,3);                                                // Прочитать кнопку 0 - нажата
625
    if ((key==0)&&(flagKey==false))                                   // Если кнопка была нажата запомнить время и поставить флаг нажатия
626
    {
627
        flagKey=true;                                                 // Кнопка нажата  ждем обратного фронта
628
        time_key=millis();                                            // Время нажатия запомнили
629
     }
630
       
631
    if ((key==1)&&(flagKey==true))                                    // Если кнопка была отжата
632
    {
633
         time_key=millis()-time_key;                                  // Рассчитать время нажатия
634
         pressKey=true;
635
    }
636
   sei();
637
 }
638
 
639
// Проверка статуса вытяжки, не пора ли переключится
640
void CheckON(void)
641
{
642
//cli();
643
// 0.  Проверить замораживание подвала КАЛОРИФЕР
644
if (packet.tIn<=TEMP_LOW) { MOTOR_OFF; HEAT_ON; return;}   // Контроль от промораживания подвала по идеи здесь надо включать калорифер
645
if ((FLAG_HEAT_CHECK)&&(packet.tIn>TEMP_LOW+dT_OFF)) HEAT_OFF;    // Выключить калорифер
646
 
647
// 1. Режимы не зависящие от влажности и температуры ВЫСШИЙ приоритет
648
if ((settingRAM.fStart==BLOCK_OFF)&&(~FLAG_MOTOR_CHECK))  return;
649
if ((settingRAM.fStart==BLOCK_OFF)&&(FLAG_MOTOR_CHECK))  { MOTOR_OFF ; return;}
650
if ((settingRAM.fStart==HOOD_ON )&&(FLAG_MOTOR_CHECK))   return;
651
if ((settingRAM.fStart==HOOD_ON )&&(~FLAG_MOTOR_CHECK))  { MOTOR_ON  ; return;}
652
 
653
// 2. Режим охлаждения (второй приоритет) температура внутри больше 10 градусов темература снаружи меньше на 2 градуса чем внутри, на влажность не смотрим
654
if (settingRAM.fStart==COOLING)          // Режим охлаждение
655
  {
656
    if ((~FLAG_MOTOR_CHECK)&&(packet.tIn>packet.T_min)&&((packet.tIn-packet.tOut)>packet.dH_min)) // dH_min используется не штатно для температуры
657
       {MOTOR_ON; return;}            // мотор выключен, температура выше установленной и снаружи температура ниже на 2 градуса  то ВКЛЮЧЕНИЕ мотора
658
    if ((FLAG_MOTOR_CHECK)&&(packet.tIn<=packet.tOut))  
659
       {MOTOR_OFF; return;}        // мотор включен и темература внутри ниже наружней то ВЫКЛЮЧЕННИЕ мотора
660
   return;                             // изменений нет выходим   
661
  }
662
// 3. В режиме осушения - проверка на достижение минимальной температуры помещения в режиме осушения - СРОЧНО ВЫКЛЮЧИТЬ  третий приоритет
663
if (packet.tIn<=packet.T_min)
664
   {
665
     if (~FLAG_MOTOR_CHECK)   return;      // Мотор уже выключен, выходим
666
     else  { MOTOR_OFF; return;}         // выключить и выйти
667
   }
668
    
669
// 4. Режимы зависящие от температуры и влажности низший приоритет (что осталось)
670
if ((~FLAG_MOTOR_CHECK)&&(packet.tIn>packet.T_min)&&((packet.absHIn-packet.dH_min)>packet.absHOut))
671
        {MOTOR_ON; return;}        // мотор выключен, темература выше критической, абс влажность с наружи меньше  то ВКЛЮЧЕНИЕ мотора
672
if ((FLAG_MOTOR_CHECK)&&((packet.tIn<=(packet.T_min-dT_OFF))||(packet.absHIn<(packet.absHOut-dH_OFF))))
673
        {MOTOR_OFF; return;}       // мотор включен и темература ниже критической или абс влажность внутри ниже  то ВЫКЛЮЧЕННИЕ мотора
674
          
675
//sei();
676
}
677
  
678
 
679
// Вывод информации о настройках и сохрание индекса настроек в eeprom ---------------------------------
680
void Setting()
681
{
682
 // Настройка
683
  cli();
684
  ucg.setColor(0, 100, 255);
685
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
686
  ucg.setPrintPos(0,25+0+18);
687
  switch (settingRAM.fStart)
688
        {
689
        case  BLOCK_OFF: ucg.print(F("Выключено              ")); packet.dH_min=255;packet.T_min=255; break;
690
        case  HOOD_ON:   ucg.print(F("Режим вытяжки         "));  packet.dH_min=0;  packet.T_min=0;   break;
691
        case  COOLING:   ucg.print(F("Охлаждение T>10 dT>2"));    packet.dH_min=200;packet.T_min=1000;break;   // dH_min используется не штатно для температуры    
692
        case  3:         ucg.print(F("Осушение T>+3 dH>0.2 "));   packet.dH_min=20; packet.T_min=300; break;
693
        case  4:         ucg.print(F("Осушение T>+3 dH>0.4 "));   packet.dH_min=40; packet.T_min=300; break;
694
        case  5:         ucg.print(F("Осушение T>+4 dH>0.3 "));   packet.dH_min=30; packet.T_min=400; break;
695
        case  6:         ucg.print(F("Осушение T>+4 dH>0.5 "));   packet.dH_min=50; packet.T_min=400; break;
696
        case  7:         ucg.print(F("Осушение T>+5 dH>0.6 "));   packet.dH_min=30; packet.T_min=500; break;
697
        }
698
 resetKey();
699
 writeEeprom();       // Запись в EEPROM  новых настроек
700
 MOTOR_OFF;           // Поменяли настройки - отключить мотор, пусть заново настройки сработают если потребуется
701
 CheckON();           // Возможно надо включить мотор
702
 print_status();      // Показать панель состояния (смена настроек)
703
 sei();  
704
}
705
 
706
// Вывод float  с двумя десятичными знаком в координаты x y // для экономии места
707
void print_floatXY(int x,int y, float v)
708
{
709
 ucg.setPrintPos(x,y);
710
 ucg.print(v,2);
711
 ucg.print(F("  ")); // Стереть хвост от предыдущего числа
712
}
713
 
714
// Вывод строки константы в координаты x y // для экономии места
715
void print_StrXY(int x,int y, const __FlashStringHelper* b)
716
{
717
 ucg.setPrintPos(x,y);
718
 ucg.print(b);
719
}
720
 
721
void printInfo() // Окно с информацией о блоке, появляется при длительном нажатии на кнопку
722
{
723
  infoScreen=true;
724
  resetKey();
725
  cli();
726
  ucg.setColor(250, 250, 250);  //
727
  ucg.drawBox(10, 10, 320-1-20, 240-1-20);
728
  ucg.setColor(0, 50, 250);
729
  ucg.drawFrame(10+5, 10+5, 320-1-20-10, 240-1-20-10);
730
   
731
  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
732
  ucg.setColor(0, 150, 10);
733
  print_StrXY(35,18+15,F("ОСУШИТЕЛЬ на Arduino Pro Mini"));
734
   
735
  ucg.setColor(0, 50, 50);
736
  print_StrXY(10+10,15+17*2,F("1 Напряжение питания В."));
737
  print_floatXY(10+230,15+17*2,readVcc()/1000.0);
738
  
739
  print_StrXY(10+10,15+17*3,F("2 Температура блока гр."));
740
  print_floatXY(10+230,15+17*3,GetTemp());
741
  
742
  print_StrXY(10+10,15+17*4,F("3 Свободная память байт"));
743
  ucg.setPrintPos(10+230,15+17*4);
744
  ucg.print(freeRam());
745
  
746
  print_StrXY(10+10,15+17*5,F("4 Мото часы блока"));
747
  ucg.setPrintPos(10+230,15+17*5);
748
  ucg.print(settingRAM.hour_unit/(TIME_HOUR/TIME_PRINT_CHART));
749
   
750
  print_StrXY(10+10,15+17*6,F("5 Мото часы вентилятора"));
751
  ucg.setPrintPos(10+230,15+17*6);
752
  ucg.print(settingRAM.hour_motor/(TIME_HOUR/TIME_PRINT_CHART));
753
   
754
  print_StrXY(10+10,15+17*7,F("6 Канал NRF24l01+"));
755
  ucg.setPrintPos(10+230,15+17*7);
756
  ucg.print(NRF24_CHANEL);
757
  
758
  print_StrXY(10+10,15+17*8,F("7 Гистерезис абс. влажности"));
759
  print_floatXY(10+255,15+17*8,(float)dH_OFF/100.0);
760
   
761
  print_StrXY(10+10,15+17*9,F("8 Гистерезис температуры"));
762
  print_floatXY(10+255,15+17*9,(float)dT_OFF/100.0);
763
   
764
  print_StrXY(10+10,15+17*10,F("9 ERR Т/Н in:"));
765
  ucg.print(TIN_ERR);
766
  ucg.print(F("/"));
767
  ucg.print(HIN_ERR);
768
  ucg.print(F(" out:"));
769
  ucg.print(TOUT_ERR);
770
  ucg.print(F("/"));
771
  ucg.print(HOUT_ERR);
772
  
773
  ucg.setColor(0, 0, 150);
774
  print_StrXY(10+10,16+17*11,F("СБРОС - Вкл. при нажатой кнопке."));
775
   
776
  ucg.setColor(250,80,80);
777
  print_StrXY(10+10,20+21+18*10,F(VERSION));
778
  resetKey();
779
  sei();
780
  #ifdef BEEP
781
   beep(40);
782
  #endif
783
}
784
 
785
void clearInfo()  // Стереть информационный экран
786
{
787
      infoScreen=false;
788
      resetKey();
789
      last_error=100;         // Признак обновления ошибки
790
      cli();
791
      ucg.setColor(0, 0, 0);  // залить черным
792
      ucg.drawBox(10, 10, 320-1-20, 240-1-20);
793
      print_static();
794
      Setting(); 
795
      printChart();
796
      sei();
797
}
798
// Чтение свободной памяти --------------------------------------------------------------------
799
int freeRam () {
800
  extern int __heap_start, *__brkval;
801
  int v;
802
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
803
}
804
// Чтение внутреннего датчика температуры ---------------------------------------
805
double GetTemp(void)
806
{
807
  unsigned int wADC;
808
  double t;
809
  sei();  // Должны быть разрешены прерывания
810
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
811
  ADCSRA |= _BV(ADEN); 
812
  delay(20);          
813
  ADCSRA |= _BV(ADSC); 
814
  while (bit_is_set(ADCSRA,ADSC));
815
  wADC = ADCW;
816
  t = (wADC - 324.31 ) / 1.22;
817
  return (t);
818
}
819
// Чтение напряжения питания ----------------------------------------------
820
long readVcc() {
821
  long result;
822
  // Read 1.1V reference against AVcc
823
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
824
  delay(2); // Wait for Vref to settle
825
  ADCSRA |= _BV(ADSC); // Convert
826
  while (bit_is_set(ADCSRA,ADSC));
827
  result = ADCL;
828
  result |= ADCH<<8;
829
  result = ConstADC / result; // Back-calculate AVcc in mV
830
  return result;
831
}
832
// Запись счетчиков в Eeprom --------------------------------------------------
833
void writeEeprom()
834
{
835
cli();
836
  eeprom_write_block((const void*)&settingRAM, (void*) &settingEEPROM, sizeof(settingRAM));
837
sei();
838
}
839
// Чтение счетчиков из Eeprom --------------------------------------------------
840
void readEeprom()
841
{
842
cli();
843
   eeprom_read_block((void*)&settingRAM, (const void*) &settingEEPROM, sizeof(settingRAM));
844
   if ((settingRAM.fStart>(NUM_SETTING-1))||(settingRAM.fStart<0)) settingRAM.fStart=0;        // гарантированно попадаем в диапазон
845
sei();
846
}
847
 
848
void reset_sum()  // Сброс счетчиков накоплений
849
{
850
sensors.num=0;  // Рассчитать величину усреднения
851
sensors.sum_tOut=0;
852
sensors.sum_tIn=0;
853
sensors.sum_relHOut=0;
854
sensors.sum_relHIn=0;
855
}
856
 
857
char hex(byte x)  // Функция для вывода в hex
858
{
859
   if(x >= 0 && x <= 9 ) return (char)(x + '0');
860
   else      return (char)('a'+x-10);
861
}
862
 
863
bool reset_ili9341(void)
864
{
865
  pinMode(PIN_RESET, OUTPUT);                    // Сброс дисплея сигнал активным является LOW
866
  digitalWrite(PIN_RESET, LOW); 
867
  delay(100);
868
  digitalWrite(PIN_RESET, HIGH); 
869
  // Дисплей
870
  ucg.begin(UCG_FONT_MODE_TRANSPARENT);
871
  ucg.setFont(my14x10rus); 
872
//   ucg.setRotate90();
873
  ucg.setRotate270();
874
  ucg.clearScreen();
875
}
876
 
877
#ifdef BEEP 
878
  void beep(int x)  // Пищать х мсек
879
  {
880
    WritePort(C,1,HIGH);
881
    delay(x);
882
    WritePort(C,1,LOW);
883
  }
884
#endif
 