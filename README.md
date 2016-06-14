# Модуль робота Lynxmotion
-----------------------------

Модуль для работы с роботами производства компании [Lynxmotion](http://www.lynxmotion.com/).<br>

Для работы модуля необходимо, чтобы в одном каталоге с ним находился конфигурационный файл config.ini.<br>
Вариант содержания config.ini:<br>

```ini
  [main]
  servo_count =6
  robots_count =1
  robot_port_1=COM9

  [start_position]
  servo_1 = 90
  servo_2 = 90
  servo_3 = 90
  servo_4 = 90
  servo_5 = 90
  servo_6 = 90

  [safe_position]
  servo_1 = 90
  servo_2 = 170
  servo_3 = 155
  servo_4 = 40
  servo_5 = 90
  servo_6 = 90

  [servo_1]
  min = 0
  max =180
  increment = 1
  low_control_value = -10
  top_control_value = 10
  [servo_2]
  min = 0
  max =170
  increment = 1
  low_control_value = -10
  top_control_value = 10
  [servo_3]
  min = 0
  max =156
  increment = 1
  low_control_value = -10
  top_control_value = 10
  [servo_4]
  min = 0
  max =180
  increment = 1
  low_control_value = -10
  top_control_value = 10
  [servo_5]
  min = 0
  max = 180
  increment = 1
  low_control_value = -1
  top_control_value = 1
  [servo_6]
  min = 0
  max =121
  increment = 1
  low_control_value = -1
  top_control_value = 1
```


####Доступные функции робота:
Наименование  | Кол-во параметров  |  Описание
------------  | -----------------  | ------------------  
move_servo(**servo_index**, **value**)  |  2  |  Установить значение сервы с индексом **servo_index** равным **value**

**servo_index** считается от 1.

####Доступные оси робота
Наименование  | Верхняя граница  | Нижняя граница  | Примечание
------------  | -----------------  | ------------------  | ------------------
locked            | 1    | 0    | Бинарная ось. Если её значение равно 0, то робот считается заблокированным и игнорирует изменение значений по любым другим осям.
servo_%number%    | min  | max  | Количество осей и их граничные значения указываются в конфигурационном файле

Для управления сервой под номером 1 необходимо ввести имя оси servo_1.
В конфигурационном файле в секции настройки осей:
increment = 0 - в ось отправляется максимальное или минимальное значение(min/max).
increment = 1 - в ось отправляется low_control_value или top_control_value.

####Компиляция
Для компиляции модуля потребуется библиотеки:
- [SimpleIni](https://github.com/brofield/simpleini)
