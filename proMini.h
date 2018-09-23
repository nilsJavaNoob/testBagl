001
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
#include "Ucglib.h"    // ВНИМАНИЕ использовать библиотеку не познее 1.01 справка <a href="https://code.google.com/p/ucglib/wiki/" title="https://code.google.com/p/ucglib/wiki/" rel="nofollow">https://code.google.com/p/ucglib/wiki/</a>
010
#include "rusFont.h"   // Русские шрифты
011
#include "nRF24L01.h"  // Беcпроводной модуль
012
#include "RF24.h"      // Беcпроводной модуль
013
 
014
//#define DEBUG                                   // Отладочную  информацию в ком порт посылает 
015
//#define DEMO                                    // Признак демонстрации - данные с датчиков генерятся данные
016
#define BEEP                                    // Использовать пищалку
017
#define RADIO                                   // Признак использования радио модуля
018
 
019
#define dH_OFF          15                      // Гистерезис абсолютной влажности в сотых грамма на куб
020
#define dT_OFF          25                      // Гистерезис температуры в сотых градуса
021
 
022
#ifdef DEMO                                     // Для демо все быстрее и случайным образом
023
    #define NUM_SAMPLES      2                  // Число усреднений измерений датчика
024
    #define TIME_SCAN_SENSOR 2000               // Время опроса датчиков мсек, для демки быстрее
025
    #define TIME_PRINT_CHART 8000               // Время вывода точки графика мсек, для демки быстрее
026
    #define TIME_HOUR        50000              // Число мсек в часе, для демки быстрее  
027
#else  
028
   #define NUM_SAMPLES      10                  // Число усреднений измерений датчика
029
   #define TIME_SCAN_SENSOR 2000                // Время опроса датчиков мсек
030
   #define TIME_PRINT_CHART 300000              // Время вывода точки графика мсек
031
   #define TIME_HOUR        3600000             // Число мсек в часе
032
#endif
033
 
034
#define VERSION "Version: 0.55 23/08/15"        // Текущая версия
035
#define ID             0x21                     // уникально Идентификатор устройства - старшие 4 бита, вторые (младшие) 4 бита серийный номер устройства
036
#define NUM_SETTING    6                        // Число вариантов настроек - 1
037
#define MOTOR_OFF      0                        // Мотор выключен
038
#define MOTOR_ON       1                        // Мотор включен
039
#define LONG_KEY      3000                      // Длительное нажатие кнопки мсек, появляется Экран инфо
040
#define SHORT_KEY     150                       // Короткое нажатие кнопки более мсек
041
#define NRF24_CHANEL  100                       // Номер канала nrf24
042
 
043
// СИСТЕМАТИЧЕСКИЕ ОШИБКИ ДАТЧИКОВ для ID 0x21
044
#define TOUT_ERR      40                         // Ошибка уличного датчика температуры в сотых долях градуса
045
#define TIN_ERR       40                         // Ошибка домового датчика температуры в сотых долях градуса
046
#define HOUT_ERR      -30                        // Ошибка уличного датчика влажности в сотых долях %
047
#define HIN_ERR       +30                        // Ошибка домового датчика влажности в сотых долях %
048
 
049
 
050
//  НОГИ к которым прицеплена переферия (SPI используется для TFT и NRF24 - 11,12,13)
051
#ifdef BEEP
052
    #define PIN_BEEP  15                         // Ножка куда повешена пищалка
053
#endif
054
#define PIN_RELAY     14                         // Ножка на которую повешено реле (SSR) вентилятора - аналоговый вход A0 через резистор 470 ом
055
#define PIN_CS        10                         // TFT дисплей spi
056
#define PIN_CD        9                          // TFT дисплей spi
057
#define PIN_RESET     8                          // TFT дисплей spi
058
#define PIN_CE        7                          // nrf24 ce
059
#define PIN_CSN       6                          // nrf24 cs
060
#define PIN_DHT22a    5                          // Первый датчик DHT22   IN  ДОМ
061
#define PIN_DHT22b    4                          // Второй датчик DHT22   OUT УЛИЦА
062
#define PIN_KEY       3                          // Кнопка, повешена на прерывание, что бы ресурсов не тратить
063
#define PIN_IRQ_NRF24 2                          // Ножка куда заведено прерывание от NRF24 (пока не используется)
064
 
065
 
066
// АЦП ----------------------------------------
067
const long ConstADC=1126400;                    // Калибровка встроенного АЦП (встроенный ИОН) по умолчанию 1126400 дальше измеряем питание и смотрим на дисплей   
068
 
069
Ucglib_ILI9341_18x240x320_HWSPI ucg(/*cd=*/PIN_CD, /*cs=*/PIN_CS, /*reset=*/PIN_RESET); // Аппаратный SPI на дисплей ILI9341
070
leOS myOS;                                      // многозадачность
071
dht DHT;                                        // Датчики
072
bool infoScreen=false;                          // Признак отображениея иформационного экрана  1 - на экран ничего не выводится кроме информационного экрана
073
bool flagKey=false;                             // Флаг нажатия клавиши
074
bool pressKey=false;                            // Флаг необходимости обработки кнопки
075
 
076
unsigned long time_key=0;                       // Время нажатия копки
077
byte last_error=100;                            // Предыдущая ошибка чтения датчиков
078
 
079
 struct type_setting_eeprom                     // Структура для сохранения данных в eeprom
080
 {
081
     byte fStart = 0;                           // Какой набор настроек используется
082
     unsigned long hour_unit;                   // мото часы блок измеряется в интервалах вывода графика TIME_PRINT_CHART
083
     unsigned long hour_motor;                  // мото часы мотор измеряется в интервалах вывода графика TIME_PRINT_CHART
084
 };
085
volatile type_setting_eeprom settingRAM;        // Рабочая копия счетчиков в памяти
086
type_setting_eeprom settingEEPROM EEMEM;        // Копия счетчиков в eeprom - туда пишем
087
 
088
// пакет передаваемый, используется также для хранения результатов.
089
 struct type_packet_NRF24        // Версия 2.0!! адаптация для stm32 Структура передаваемого пакета 32 байта - это максимум
090
    {
091
        byte id=0x22;                               // Идентификатор типа устройства - старшие 4 бита, вторые (младшие) 4 бита серийный номер устройства
092
        byte error;                                 // Ошибка разряды: 0-1 первый датчик (00-ок) 2-3 второй датчик (00-ок) 4 - радиоканал    
093
        int16_t   tOut=-500,tIn=-500;               // Текущие температуры в сотых градуса !!! место экономим
094
        uint16_t  absHOut=123,absHIn=456;           // Абсолютные влажности в сотых грамма на м*3 !!! место экономим
095
        uint16_t  relHOut=555,relHIn=555;           // Относительные влажности сотых процента !!! место экономим
096
        uint8_t   motor=false;                      // битовая маска пока используется один бит Статус вентилятора
097
        uint8_t dH;                                 // Порог включения вентилятора по абсолютной влажностив сотых грамма на м*3
098
        uint16_t  T;                                // Порог выключения вентилятора по температуре в сотых градуса
099
        char note[12] = "ArduDry :-)";              // Примечание не более 11 байт + 0 байт
100
    } packet; 
101
struct type_sensors                                 // структура для усреднения
102
{
103
       int  num;                                    // сколько отсчетов уже сложили не болле NUM_SAMPLES
104
       long  sum_tOut=-5000,sum_tIn=-5000;          // Сумма для усреднения Текущие температуры в сотых градуса !!! место экономим
105
       long  sum_relHOut=55555,sum_relHIn=55555;    // Сумма для усреднения Относительные влажности сотых процента !!! место экономим
106
       int  tOut=-5000,tIn=-5000;                   // Текущие температуры в сотых градуса !!! место экономим
107
       int  absHOut=55555,absHIn=55555;             // Абсолютные влажности в сотых грамма на м*3 !!! место экономим
108
       int  relHOut=55555,relHIn=55555;             // Относительные влажности сотых процента !!! место экономим
109
} sensors;
110
     
111
// Массивы для графиков
112
byte tOutChart[120];
113
byte tInChart[120];
114
byte absHOutChart[120];
115
byte absHInChart[120];
116
byte posChart=0;       // Позиция в массиве графиков - начало вывода от 0 до 120-1
117
byte TimeChart=0;      // Время до вывода очередной точки на график.
118
 
119
#ifdef  RADIO    // Радио модуль NRF42l 
120
   RF24 radio(PIN_CE, PIN_CSN);  //определение управляющих ног
121
   bool send_packet_ok=false;                // признак удачной отправки последнего пакета
122
#endif
123
///////////////////////////////////////////////////////////////////////////////////////////////////////////
124
// ПРОГРАММА
125
///////////////////////////////////////////////////////////////////////////////////////////////////////////
126
void setup(){
127
#ifdef  DEBUG 
128
   Serial.begin(9600);
129
   Serial.println(F("DEBUG MODE"));
130
   #ifdef  BEEP 
131
     Serial.println(F("BEEP ON"));
132
   #else        
133
     Serial.println(F("BEEP OFF"));
134
   #endif
135
   #ifdef  RADIO
136
     Serial.println(F("RADIO ON"));
137
   #else        
138
     Serial.println(F("RADIO OFF"));
139
   #endif
140
#endif
141
 
142
reset_sum();
143
 
144
#ifdef  RADIO    // Радио модуль NRF42l  первичная настройка
145
   radio.begin();
146
   radio.setDataRate(RF24_250KBPS);         //  выбор скорости RF24_250KBPS RF24_1MBPS RF24_2MBPS
147
   radio.setPALevel(RF24_PA_MAX);           // выходная мощность передатчика
148
   radio.setChannel(NRF24_CHANEL);          //тут установка канала
149
   radio.setCRCLength(RF24_CRC_16);         // использовать контрольную сумму в 16 бит
150
// radio.setAutoAck(false);                 // выключить аппаратное потверждение
151
// radio.enableDynamicPayloads();           // разрешить Dynamic Payloads
152
// radio.enableAckPayload();                // разрешить AckPayload
153
  radio.setRetries(10,30);                  // Количество повторов и пауза между повторами
154
  radio.openWritingPipe(0xF0F0F0F0E1LL);    // передатчик
155
  radio.openReadingPipe(1,0xF0F0F0F0D2LL);  // приемник
156
  radio.startListening();
157
#endif
158
  #ifdef BEEP
159
    pinMode(PIN_BEEP, OUTPUT);          //  Настройка ноги для динамика
160
    digitalWrite(PIN_BEEP, LOW);
161
  #endif
162
  pinMode(PIN_KEY, INPUT);              //  Включена кнопка
163
  digitalWrite(PIN_KEY, HIGH); 
164
  pinMode(PIN_RELAY, OUTPUT);           //  Реле
165
  digitalWrite(PIN_RELAY, LOW); 
166
  pinMode(PIN_DHT22a, OUTPUT);          //  Датчик DHT22 #1
167
  digitalWrite(PIN_DHT22a, HIGH); 
168
  pinMode(PIN_DHT22b, OUTPUT);          //  Датчик DHT22 #2
169
  digitalWrite(PIN_DHT22b, HIGH); 
170
   
171
  reset_ili9341();                      // сброс дисплея
172
  readEeprom();                          // Прочитать настройки
173
  if (digitalRead(PIN_KEY)==0)           // Если при включении нажата кнопка то стираем все
174
  {   int i;
175
      settingRAM.fStart=0;
176
      settingRAM.hour_unit=0;
177
      settingRAM.hour_motor=0;
178
      ucg.setColor(255, 255, 255);
179
      print_StrXY(10,50,F("Сброс настроек и счетчиков"));
180
      for(i=0;i<3;i++)
181
        {
182
         delay(1000);
183
         ucg.print(F(" ."));
184
        }
185
      writeEeprom();       // Запись в EEPROM 
186
      delay(1000);
187
      ucg.clearScreen();
188
  }
189
  
190
  wdt_enable(WDTO_8S); // Сторожевой таймер Для тестов не рекомендуется устанавливать значение менее 8 сек.
191
  // Запуск задач по таймеру
192
  myOS.begin();
193
  myOS.addTask(measurement,TIME_SCAN_SENSOR);    // Измерение
194
  attachInterrupt(1, scanKey, CHANGE);           // КНОПКА Прерывания по обоим фронтам
195
  print_static();                                // распечатать таблицу
196
  Setting();                                     // Применить настройки
197
  measurement();                                 // Считать данные
198
  #ifdef BEEP
199
     beep(100);
200
  #endif
201
resetKey();
202
}
203
 
204
void loop()
205
{
206
 wdt_reset();                        // Сбросить сторожевой таймер
207
// Обработка нажатия кнопки
208
if (pressKey==true) // Кнопка
209
{
210
 myOS.pauseTask(measurement);                                 // Остановить задачи
211
 if (flagKey!=false)
212
 if (time_key > LONG_KEY)                                     // Длительное нажатие кнопки
213
   printInfo();                                               // Вывод информационного экрана
214
 else
215
   if (time_key > SHORT_KEY)                                  // Короткое нажатие кнопки
216
     {
217
        #ifdef BEEP
218
          beep(30);                                            // Звук нажатия на клавишу
219
        #endif
220
       if (infoScreen==true)  clearInfo();                    // если информационный экран то стереть его
221
       else {  if (settingRAM.fStart >= NUM_SETTING) settingRAM.fStart=0; //  Кольцевой счетчик настроек
222
               else settingRAM.fStart++;                      // В противном случае следующая настройка
223
               Setting(); } 
224
     }
225
  resetKey();                                                //  Сброс состояния кнопки
226
  myOS.restartTask(measurement);                             // Запустить задачи
227
  }
228
}
229
 
230
void resetKey(void)  // Сброс состяния кнопки
231
{
232
  flagKey=false;                                            
233
  time_key=millis();                                               
234
  pressKey=false;
235
} 
236
void print_static()  // Печать статической картинки
237
{
238
   cli();
239
  // Заголовок
240
  ucg.setColor(0, 0, 180);  //
241
  ucg.drawBox(0, 0, 320-1, 23);
242
  ucg.setColor(250, 250, 250);
243
  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
244
  print_StrXY(2,19,F("ОСУШИТЕЛЬ ID: 0x"));
245
  ucg.print( hex(packet.id >> 4));
246
  ucg.print( hex(packet.id&0x0f));
247
    
248
  #ifdef DEMO
249
    ucg.print(F(" demo"));
250
  #endif
251
   
252
  // Таблица для данных
253
  ucg.setColor(0, 200, 0);
254
  ucg.drawHLine(0,25,320-1);
255
  ucg.drawHLine(0,25+23*1,320-1);
256
  ucg.drawHLine(0,25+23*2,320-1);
257
  ucg.drawHLine(0,25+23*3,320-1);
258
  ucg.drawHLine(0,25+23*4,320-1);
259
  ucg.drawVLine(200-4,25,24+23*3);
260
  ucg.drawVLine(260,25,24+23*3);
261
 
262
  // Заголовки таблиц
263
  ucg.setColor(255, 255, 0);
264
  print_StrXY(180+30,25+0+18,F("Дом"));
265
  print_StrXY(250+20,25+0+18,F("Улица"));
266
  print_StrXY(0,25+23*1+18,F("Температура градусы C"));
267
  print_StrXY(0,25+23*2+18,F("Относительная влаж. %"));
268
  print_StrXY(0,25+23*3+18,F("Абсолют. влаж. г/м*3"));
269
 
270
  // Графики
271
  ucg.setColor(200, 200, 200);
272
  ucg.drawHLine(1,240-1,130);
273
  ucg.drawVLine(1,135,105);
274
  ucg.drawHLine(10+150,240-1,130);
275
  ucg.drawVLine(10+150,135,105);
276
  print_StrXY(10,135+0,F("Температура"));
277
  print_StrXY(20+150,135+0,F("Абс. влажность"));
278
   
279
  // надписи на графиках
280
  print_StrXY(128,154,F("+20"));
281
  print_StrXY(135,194,F("0"));
282
  print_StrXY(128,233,F("-20"));
283
   
284
  print_StrXY(296,164,F("15"));
285
  print_StrXY(296,194,F("10"));
286
  print_StrXY(296,223,F("5"));
287
   sei();
288
}
289
 
290
void print_status() // Печать панели статуса Значки на статус панели
291
{
292
  if (infoScreen==true) return;        // если отображен информационный экран то ничего не выводим 
293
   cli();
294
 // 1. печать ошибки чтения датчиков
295
   print_error_DHT();
296
 // 2. Признак включения мотора
297
  if (packet.motor==true)  ucg.setColor(0, 240, 0);
298
  else                     ucg.setColor(0, 40, 0);
299
  ucg.drawBox(290-32, 5, 14, 14);
300
   
301
  #ifdef  RADIO
302
  // 3. Признак удачной передачи информации по радиоканалу
303
      if (send_packet_ok==true)  ucg.setColor(0, 240, 0);
304
      else                       ucg.setColor(0, 40, 0);
305
      ucg.setPrintDir(3);
306
      ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
307
      print_StrXY(290-40,20,F(">>"));
308
      ucg.setPrintDir(0);
309
      ucg.setFontMode(UCG_FONT_MODE_SOLID);
310
  #endif
311
  sei();
312
} 
313
 
314
void print_error_DHT() // Печать ошибки чтения датчиков выводится при каждом чтении датчика
315
{
316
  if (infoScreen==true) return;        // если отображен информационный экран то ничего не выводим 
317
 // 1. печать ошибки чтения датчиков
318
  if (packet.error!=last_error)        // если статус ошибки поменялся то надо вывести если нет то не выводим - экономия время и нет мерцания
319
  {
320
      cli();
321
      last_error=packet.error;
322
      ucg.setColor(0, 0, 180);         // Сначала стереть
323
      ucg.drawBox(290, 0, 26, 18);
324
      ucg.setPrintPos(290,18);
325
      ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
326
      if (packet.error>0)
327
        {
328
        ucg.setColor(255, 100, 100);
329
        print_StrXY(280,19,F("0x"));
330
        ucg.print( hex(packet.error >> 4));
331
        ucg.print( hex(packet.error & 0x0f));
332
        }
333
      else  { ucg.setColor(200, 240, 0);   ucg.print(F("ok")); }
334
     sei();
335
   }  
336
}
337
//  вывод на экран данных (то что меняется)
338
void print_data()
339
{
340
 if (infoScreen==true) return;                  // если отображен информационный экран то ничего не выводим 
341
  cli();
342
 // Печать значений для дома
343
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
344
  ucg.setColor(250, 0, 100);  // Цвет ДОМА
345
  print_floatXY(200+0,25+23*1+18,((float)packet.tIn)/100);
346
  print_floatXY(200+0,25+23*2+18,((float)packet.relHIn)/100);
347
  print_floatXY(200+0,25+23*3+18,((float)packet.absHIn)/100);
348
  ucg.setColor(0, 250, 100);  // Цвет УЛИЦЫ
349
  print_floatXY(260+4,25+23*1+18,((float)packet.tOut)/100);
350
  print_floatXY(260+6,25+23*2+18,((float)packet.relHOut)/100);
351
  print_floatXY(260+6,25+23*3+18,((float)packet.absHOut)/100);
352
  sei();
353
} 
354
 
355
// Печать графика на экране, добавляется одна точка и график сдвигается
356
void printChart()
357
{
358
byte i,x=0;
359
byte tInLast,absHInLast,tOutLast,absHOutLast;
360
 
361
// Статистика по моточасам, время ведется в тиках графика а потом пересчитывается в часы при выводе.
362
settingRAM.hour_unit++;
363
if (packet.motor==true) settingRAM.hour_motor++;
364
 
365
// Работаем через кольцевой буфер
366
// Добавить новую точку в кольцевой буфер
367
     // Температура в доме. диапазон -25 . . . +25 растягиваем на 100 точек
368
     tInLast=tInChart[posChart];                       // Сохранить точку для стирания на графике
369
     if (packet.tIn<=-2500) tInChart[posChart]=0;      // Если температура меньше -25 то округляем до -25
370
     if (packet.tIn>=2500)  tInChart[posChart]=100;    // Если температура больше 25  то округляем до 25
371
     if ((packet.tIn>-2500)&&(packet.tIn<2500)) tInChart[posChart]=((2*packet.tIn)+5000)/100;  // внутри -25...+25 растягиваем в два раза
372
     // Температура на улице. диапазон -25 . . . +25 растягиваем на 100 точек
373
     tOutLast=tOutChart[posChart];                       // Сохранить точку для стирания на графике
374
     if (packet.tOut<=-2500) tOutChart[posChart]=0;      // Если температура меньше -25 то округляем до -25
375
     if (packet.tOut>=2500)  tOutChart[posChart]=100;    // Если температура больше 25  то округляем до 25
376
     if ((packet.tOut>-2500)&&(packet.tOut<2500)) tOutChart[posChart]=((2*packet.tOut)+5000)/100;  // внутри -25...+25 растягиваем в два раза
377
     // Абсолютная влажность в доме диапазон от 0 до 20 грамм на кубометр, растягиваем на 100 точек
378
     absHInLast=absHInChart[posChart];                   // Сохранить точку для стирания на графике
379
     if (packet.absHIn>=2000) absHInChart[posChart]=100;
380
     else absHInChart[posChart]=(5*packet.absHIn)/100;   // внутри 0...20 растягиваем в пять  раз
381
     // Абсолютная влажность на улицу диапазон от 0 до 20 грамм на кубометр, растягиваем на 100 точек
382
     absHOutLast=absHOutChart[posChart];                   // Сохранить точку для стирания на графике
383
     if (packet.absHOut>=2000) absHOutChart[posChart]=100;
384
     else absHOutChart[posChart]=(5*packet.absHOut)/100;   // внутри 0...20 растягиваем в пять раз
385
      
386
  if (infoScreen==false)                 // если отображен информационный экран то ничего не выводим
387
   {
388
   cli(); 
389
   for(i=0;i<120;i++)    // График слева на право
390
     {
391
     // Вычислить координаты текущей точки x в кольцевом буфере. Изменяются от 0 до 120-1
392
     if (posChart<i) x=120+posChart-i; else x=posChart-i;
393
 
394
     ucg.setColor(0, 0, 0); // Стереть предыдущую точку
395
     if (x-1<0) ucg.drawPixel(5+120-i,237-tInLast);
396
     else       ucg.drawPixel(5+120-i,237-tInChart[x-1]);
397
     if (x-1<0) ucg.drawPixel(5+120-i,237-tOutLast);
398
     else       ucg.drawPixel(5+120-i,237-tOutChart[x-1]);
399
     if (x-1<0) ucg.drawPixel(6+120-i+158,237-absHInLast);
400
     else       ucg.drawPixel(6+120-i+158,237-absHInChart[x-1]);
401
     if (x-1<0) ucg.drawPixel(6+120-i+158,237-absHOutLast);
402
     else       ucg.drawPixel(6+120-i+158,237-absHOutChart[x-1]);
403
 
404
     // Вывести новую точку
405
     if ((tInChart[x]==0)||(tInChart[x]==100))   ucg.setColor(255, 255, 255); else ucg.setColor(250, 0, 100);
406
     ucg.drawPixel(5+120-i,237-tInChart[x]);
407
      
408
     if (absHInChart[x]==100) ucg.setColor(255, 255, 255); else ucg.setColor(250, 0, 100);
409
     ucg.drawPixel(6+120-i+158,237-absHInChart[x]);
410
      
411
     if ((tOutChart[x]==0) || (tOutChart[x]==100)) ucg.setColor(255, 255, 255); else ucg.setColor(0, 250, 100);
412
     ucg.drawPixel(5+120-i,237-tOutChart[x]);
413
      
414
     if (absHOutChart[x]==100) ucg.setColor(255, 255, 255); else ucg.setColor(0, 250, 100);
415
     ucg.drawPixel(6+120-i+158,237-absHOutChart[x]);
416
      }
417
   
418
     // Пунктирные линии графика
419
    ucg.setColor(100, 100, 100); 
420
    for(i=1;i<=120;i=i+5)
421
    {
422
       ucg.drawPixel(5+120-i,237-10);
423
       ucg.drawPixel(5+120-i,237-50);
424
       ucg.drawPixel(5+120-i,237-90);
425
        
426
       ucg.drawPixel(6+120-i+158,237-25);
427
       ucg.drawPixel(6+120-i+158,237-50);
428
       ucg.drawPixel(6+120-i+158,237-75);
429
     } 
430
    sei();
431
   }
432
 if (posChart<120-1) posChart++; else posChart=0;            // Изменили положение в буфере и Замкнули буфер
433
}
434
 
435
// ---ПЕРЕДАЧА ДАННЫХ ЧЕРЕЗ РАДИОМОДУЛЬ -----------------------------
436
#ifdef  RADIO    // Радио модуль NRF42l
437
void send_packet()
438
{
439
        radio.stopListening();     // Остановить приемник
440
        send_packet_ok = radio.write(&packet,sizeof(packet));
441
         #ifdef BEEP
442
           if (send_packet_ok==true) beep(40);           // Пакет передан успешно
443
         #endif
444
         #ifdef  DEBUG 
445
           if (send_packet_ok==true)  Serial.println(F("Packet sending ++++++++++"));
446
           else                       Serial.println(F("Packet NOT sending -----------"));
447
         #endif  
448
        radio.startListening();    // Включить приемник
449
 } 
450
#endif
451
// Чтение датчика возвращает код ошибки:
452
// DHTLIB_OK                   0
453
// DHTLIB_ERROR_CHECKSUM       1
454
// DHTLIB_ERROR_TIMEOUT        2
455
// DHTLIB_ERROR_CONNECT        3
456
// DHTLIB_ERROR_ACK_L          4
457
// DHTLIB_ERROR_ACK_H          5
458
byte readDHT(byte pin)
459
{
460
delay(5);
461
cli();
462
  byte err=-1*DHT.read22(pin); // Чтение датчика
463
sei(); 
464
return err;
465
}
466
 
467
// Измерение и обработка данных чтение датчиков --------------------------------------------------------------------------------
468
void measurement()
469
{
470
 myOS.pauseTask(measurement);        // Обязательно здесь, а то датчики плохо читаются мешает leos
471
// wdt_reset();                        // Сбросить сторожевой таймер
472
  
473
 packet.error=readDHT(PIN_DHT22a);   // ПЕРВЫЙ ДАТЧИК ДОМ  Новый пакет, сбросить все ошибки и прочитать первый датчик
474
  
475
 #ifdef  DEMO
476
   DHT.temperature=packet.tIn/100+random(-20,25)/10.0;
477
   if (DHT.temperature>20) DHT.temperature=19;
478
   if (DHT.temperature<-10) DHT.temperature=-9;
479
   DHT.humidity=packet.relHIn/100+(float)random(-5,7);
480
   if (DHT.humidity>96) DHT.humidity=90;
481
   if (DHT.humidity<1) DHT.humidity=10;
482
   packet.error=0; // в Демо режиме
483
//   DHT.temperature=3.0;
484
//   DHT.humidity=21.0;
485
 #endif 
486
     sensors.tIn=(int)(DHT.temperature*100.0)+TIN_ERR;  // Запомнить результаты для суммирования
487
     sensors.relHIn=(int)(DHT.humidity*100.0)+HOUT_ERR; 
488
      
489
    #ifdef  DEBUG 
490
       Serial.print(F("Sensor read samples:")); Serial.println(sensors.num);
491
       Serial.print(F("IN T="));Serial.print(sensors.tIn);Serial.print(F(" H=")); Serial.print(sensors.relHIn); Serial.print(F(" error=")); Serial.println(packet.error);
492
    #endif  
493
  
494
 packet.error=packet.error+16*readDHT(PIN_DHT22b);// ВТОРОЙ ДАТЧИК УЛИЦА  ошибки в старшие четыре бита
495
 #ifdef  DEMO
496
   DHT.temperature=packet.tOut/100+random(-40,45)/10.0;
497
   if (DHT.temperature>30) DHT.temperature=29;
498
   if (DHT.temperature<-30) DHT.temperature=-29;
499
   DHT.humidity=packet.relHOut/100+random(-10,12);
500
   if (DHT.humidity>96) DHT.humidity=90;
501
   if (DHT.humidity<1)  DHT.humidity=10;
502
   packet.error=0;      // в Демо режиме
503
 //  DHT.temperature=-10.0;
504
 //  DHT.humidity=40.0;
505
 #endif 
506
     sensors.tOut=(int)(DHT.temperature*100.0)+TOUT_ERR;  // Запомнить результаты для суммирования
507
     sensors.relHOut=(int)(DHT.humidity*100.0)+HOUT_ERR;
508
  
509
    #ifdef  DEBUG 
510
       Serial.print(F("OUT T="));Serial.print(sensors.tOut);Serial.print(F(" H=")); Serial.print(sensors.relHOut); Serial.print(F(" error=")); Serial.println(packet.error);
511
    #endif  
512
  
513
 print_error_DHT();    // Вывод ошибки чтения датчика при каждом чтении контроль за качеством связи с датчиком
514
  
515
 if (packet.error==0)// Если чтение без ошибок у ДВУХ датчиков  копим сумму для усреднения
516
  {
517
     sensors.sum_tIn=sensors.sum_tIn+sensors.tIn;
518
     sensors.sum_relHIn=sensors.sum_relHIn+sensors.relHIn;
519
     sensors.sum_tOut=sensors.sum_tOut+sensors.tOut;
520
     sensors.sum_relHOut=sensors.sum_relHOut+sensors.relHOut;
521
     sensors.num++;
522
   }
523
  
524
 // набрали в сумме нужное число отсчетов рассчитываем усреднение и выводим
525
 if (sensors.num>=NUM_SAMPLES)  // Пора усреднять и выводить значения
526
 {
527
         resetKey();          // сброс кнопки почему то часто выскакивает длительное нажатие
528
        // вычисление средних значений
529
         packet.tIn=sensors.sum_tIn/NUM_SAMPLES;
530
         packet.relHIn=sensors.sum_relHIn/NUM_SAMPLES;
531
         packet.tOut=sensors.sum_tOut/NUM_SAMPLES;
532
         packet.relHOut=sensors.sum_relHOut/NUM_SAMPLES;
533
         reset_sum();       // Сброс счетчиков и сумм
534
         // вычисление абсолютной влажности
535
         packet.absHIn=(int)(calculationAbsH((float)(packet.tIn/100.0),(float)(packet.relHIn/100.0))*100.0);
536
         packet.absHOut=(int)(calculationAbsH((float)(packet.tOut/100.0),(float)(packet.relHOut/100.0))*100.0);
537
          
538
     #ifdef  DEBUG 
539
       Serial.println(F("Average value >>>>>>>>>>"));
540
       Serial.print(F("IN T="));Serial.print(packet.tIn);Serial.print(F(" H=")); Serial.print(packet.relHIn); Serial.print(F(" abs H=")); Serial.println(packet.absHIn);
541
       Serial.print(F("OUT T="));Serial.print(packet.tOut);Serial.print(F(" H=")); Serial.print(packet.relHOut); Serial.print(F(" abs H=")); Serial.println(packet.absHOut);
542
     #endif  
543
                      
544
         #ifdef  RADIO     // Радио модуль NRF42l
545
            send_packet();   // Послать данные
546
         #endif
547
         CheckON();         // Проверка статуса вентилятора
548
         print_status();    // панель состояния
549
         print_data();       // вывод усредненных значений
550
         if ((long)((long)TimeChart*TIME_SCAN_SENSOR*NUM_SAMPLES)>=(long)TIME_PRINT_CHART) // проврека не пора ли выводить график
551
            { printChart(); TimeChart=0; // Сдвиг графика и вывод новой точки
552
              #ifdef  DEBUG 
553
                 Serial.println(F("Point add chart ++++++++++++++++++++"));
554
              #endif 
555
              #ifdef BEEP
556
 //               beep(50);
557
              #endif
558
             }
559
         else TimeChart++;
560
    }
561
    myOS.restartTask(measurement);     // Пустить задачи
562
}
563
 
564
// Функция переводит относительную влажность в абсолютную
565
// t-температура в градусах Цельсия h-относительная влажность в процентах
566
float calculationAbsH(float t, float h)
567
{
568
 float temp;
569
 temp=pow(2.718281828,(17.67*t)/(t+243.5));
570
 return (6.112*temp*h*2.1674)/(273.15+t);
571
}
572
 
573
// Сканирование клавиш ------------------------------------------
574
void scanKey()
575
{ 
576
    byte key; 
577
    cli();
578
    key=digitalRead(PIN_KEY);                                         // Прочитать кнопку 0 - нажата
579
    if ((key==0)&&(flagKey==false))                                   // Если кнопка была нажата запомнить время и поставить флаг нажатия
580
    {
581
        flagKey=true;                                                 // Кнопка нажата  ждем обратного фронта
582
        time_key=millis();                                            // Время нажатия запомнили
583
     }
584
       
585
    if ((key==1)&&(flagKey==true))                                    // Если кнопка была отжата
586
    {
587
         time_key=millis()-time_key;                                  // Рассчитать время нажатия
588
         pressKey=true;
589
    }
590
   sei();
591
 }
592
 
593
// Проверка статуса вытяжки, не пора ли переключится
594
void CheckON()
595
{
596
cli();
597
if (packet.motor==false) // Вентилятор выключен
598
  {
599
  if (settingRAM.fStart!=MOTOR_OFF)
600
  if (settingRAM.fStart==MOTOR_ON) {packet.motor=true; digitalWrite(PIN_RELAY, HIGH);}  // Все время включен 
601
  else  if (( packet.tIn>=packet.T )&& ((packet.absHIn-packet.dH)> packet.absHOut)) {packet.motor=true; digitalWrite(PIN_RELAY, HIGH);} // ВКЛЮЧЕНИЕ
602
  }
603
else // Вентилятор включен 
604
  {
605
  if (settingRAM.fStart!=MOTOR_ON)
606
  if (settingRAM.fStart==MOTOR_OFF) {packet.motor=false; digitalWrite(PIN_RELAY, LOW);}  // Все время выключен
607
  else  if((packet.tIn<=packet.T-dT_OFF)||(packet.absHIn-packet.absHOut<packet.dH-dT_OFF)) {packet.motor=false; digitalWrite(PIN_RELAY, LOW);} // ВЫКЛЮЧЕНИЕ 
608
  }
609
sei();
610
}
611
 
612
// Вывод информации о настройках и сохрание индекса настроек в eeprom ---------------------------------
613
void Setting()
614
{
615
 // Настройка
616
  cli();
617
  ucg.setColor(0, 100, 255);
618
  ucg.setFontMode(UCG_FONT_MODE_SOLID);
619
  ucg.setPrintPos(0,25+0+18);
620
  switch (settingRAM.fStart)
621
        {
622
        case  MOTOR_OFF: ucg.print(F("Выключено              ")); packet.dH=255;packet.T=255; break;
623
        case  MOTOR_ON:  ucg.print(F("Режим вытяжки         "));  packet.dH=0;  packet.T=0;   break;
624
        case  2:         ucg.print(F("Включение T>+2 dH>0.2"));   packet.dH=20; packet.T=200; break;
625
        case  3:         ucg.print(F("Включение T>+2 dH>0.4"));   packet.dH=40; packet.T=200; break;
626
        case  4:         ucg.print(F("Включение T>+3 dH>0.3"));   packet.dH=30; packet.T=300; break;
627
        case  5:         ucg.print(F("Включение T>+3 dH>0.5"));   packet.dH=50; packet.T=300; break;
628
        case  6:         ucg.print(F("Включение T>+4 dH>0.6"));   packet.dH=30; packet.T=600; break;
629
        }
630
 resetKey();
631
 writeEeprom();       // Запись в EEPROM 
632
 CheckON();           // Возможно надо включить мотор
633
 print_status();      // панель состояния
634
 sei();  
635
}
636
 
637
// Вывод float  с одним десятичным знаком в координаты x y // для экономии места
638
void print_floatXY(int x,int y, float v)
639
{
640
 ucg.setPrintPos(x,y);
641
 ucg.print(v,2);
642
 ucg.print(F("  ")); // Стереть хвост от предыдущего числа
643
}
644
 
645
// Вывод строки константы в координаты x y // для экономии места
646
void print_StrXY(int x,int y, const __FlashStringHelper* b)
647
{
648
 ucg.setPrintPos(x,y);
649
 ucg.print(b);
650
}
651
 
652
void printInfo() // Окно с информацией о блоке, появляется при длительном нажатии на кнопку
653
{
654
  infoScreen=true;
655
  resetKey();
656
  cli();
657
  ucg.setColor(250, 250, 250);  //
658
  ucg.drawBox(10, 10, 320-1-20, 240-1-20);
659
  ucg.setColor(0, 50, 250);
660
  ucg.drawFrame(10+5, 10+5, 320-1-20-10, 240-1-20-10);
661
   
662
  ucg.setFontMode(UCG_FONT_MODE_TRANSPARENT);
663
  ucg.setColor(0, 150, 10);
664
  print_StrXY(35,18+15,F("ОСУШИТЕЛЬ на Arduino Pro Mini"));
665
   
666
  ucg.setColor(0, 50, 50);
667
  print_StrXY(10+10,15+17*2,F("1 Напряжение питания В."));
668
  print_floatXY(10+230,15+17*2,readVcc()/1000.0);
669
  
670
  print_StrXY(10+10,15+17*3,F("2 Температура блока гр."));
671
  print_floatXY(10+230,15+17*3,GetTemp());
672
  
673
  print_StrXY(10+10,15+17*4,F("3 Свободная память байт"));
674
  ucg.setPrintPos(10+230,15+17*4);
675
  ucg.print(freeRam());
676
  
677
  print_StrXY(10+10,15+17*5,F("4 Мото часы блока"));
678
  ucg.setPrintPos(10+230,15+17*5);
679
  ucg.print(settingRAM.hour_unit/(TIME_HOUR/TIME_PRINT_CHART));
680
   
681
  print_StrXY(10+10,15+17*6,F("5 Мото часы вентилятора"));
682
  ucg.setPrintPos(10+230,15+17*6);
683
  ucg.print(settingRAM.hour_motor/(TIME_HOUR/TIME_PRINT_CHART));
684
   
685
  print_StrXY(10+10,15+17*7,F("6 Канал NRF24l01+"));
686
  ucg.setPrintPos(10+230,15+17*7);
687
  ucg.print(NRF24_CHANEL);
688
  
689
  print_StrXY(10+10,15+17*8,F("7 Гистерезис абс. влажности"));
690
  print_floatXY(10+255,15+17*8,(float)dH_OFF/100.0);
691
   
692
  print_StrXY(10+10,15+17*9,F("8 Гистерезис температуры"));
693
  print_floatXY(10+255,15+17*9,(float)dT_OFF/100.0);
694
   
695
  print_StrXY(10+10,15+17*10,F("9 ERR Т/Н in:"));
696
  ucg.print(TIN_ERR);
697
  ucg.print(F("/"));
698
  ucg.print(HIN_ERR);
699
  ucg.print(F(" out:"));
700
  ucg.print(TOUT_ERR);
701
  ucg.print(F("/"));
702
  ucg.print(HOUT_ERR);
703
  
704
  ucg.setColor(0, 0, 150);
705
  print_StrXY(10+10,16+17*11,F("СБРОС - Вкл. при нажатой кнопке."));
706
   
707
  ucg.setColor(250,80,80);
708
  print_StrXY(10+10,20+21+18*10,F(VERSION));
709
  resetKey();
710
  sei();
711
  #ifdef BEEP
712
   beep(40);
713
  #endif
714
}
715
 
716
void clearInfo()  // Стереть информационный экран
717
{
718
      infoScreen=false;
719
      resetKey();
720
      last_error=100;         // Признак обновления ошибки
721
      cli();
722
      ucg.setColor(0, 0, 0);  // залить черным
723
      ucg.drawBox(10, 10, 320-1-20, 240-1-20);
724
      print_static();
725
      Setting(); 
726
      printChart();
727
      sei();
728
}
729
// Чтение свободной памяти --------------------------------------------------------------------
730
int freeRam () {
731
  extern int __heap_start, *__brkval;
732
  int v;
733
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
734
}
735
// Чтение внутреннего датчика температуры ---------------------------------------
736
double GetTemp(void)
737
{
738
  unsigned int wADC;
739
  double t;
740
  sei();  // Должны быть разрешены прерывания
741
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
742
  ADCSRA |= _BV(ADEN); 
743
  delay(20);          
744
  ADCSRA |= _BV(ADSC); 
745
  while (bit_is_set(ADCSRA,ADSC));
746
  wADC = ADCW;
747
  t = (wADC - 324.31 ) / 1.22;
748
  return (t);
749
}
750
// Чтение напряжения питания ----------------------------------------------
751
long readVcc() {
752
  long result;
753
  // Read 1.1V reference against AVcc
754
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
755
  delay(2); // Wait for Vref to settle
756
  ADCSRA |= _BV(ADSC); // Convert
757
  while (bit_is_set(ADCSRA,ADSC));
758
  result = ADCL;
759
  result |= ADCH<<8;
760
  result = ConstADC / result; // Back-calculate AVcc in mV
761
  return result;
762
}
763
// Запись счетчиков в Eeprom --------------------------------------------------
764
void writeEeprom()
765
{
766
cli();
767
  eeprom_write_block((const void*)&settingRAM, (void*) &settingEEPROM, sizeof(settingRAM));
768
sei();
769
}
770
// Чтение счетчиков из Eeprom --------------------------------------------------
771
void readEeprom()
772
{
773
cli();
774
   eeprom_read_block((void*)&settingRAM, (const void*) &settingEEPROM, sizeof(settingRAM));
775
sei();
776
}
777
 
778
void reset_sum()  // Сброс счетчиков накоплений
779
{
780
sensors.num=0;  // Рассчитать величину усреднения
781
sensors.sum_tOut=0;
782
sensors.sum_tIn=0;
783
sensors.sum_relHOut=0;
784
sensors.sum_relHIn=0;
785
}
786
 
787
char hex(byte x)  // Функция для вывода в hex
788
{
789
   if(x >= 0 && x <= 9 ) return (char)(x + '0');
790
   else      return (char)('a'+x-10);
791
}
792
 
793
bool reset_ili9341(void)
794
{
795
  pinMode(PIN_RESET, OUTPUT);                    // Сброс дисплея сигнал активным является LOW
796
  digitalWrite(PIN_RESET, LOW); 
797
  delay(100);
798
  digitalWrite(PIN_RESET, HIGH); 
799
  // Дисплей
800
  ucg.begin(UCG_FONT_MODE_TRANSPARENT);
801
  ucg.setFont(my14x10rus); 
802
 //  ucg.setRotate90();
803
  ucg.setRotate270();
804
  ucg.clearScreen();
805
}
806
 
807
#ifdef BEEP 
808
  void beep(int x)  // Пищать х мсек
809
  {
810
    digitalWrite(PIN_BEEP, HIGH);
811
    delay(x);
812
    digitalWrite(PIN_BEEP, LOW);
813
  }
814
#endif
