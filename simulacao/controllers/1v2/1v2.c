/*
 * Copyright 1996-2020 Cyberbotics Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Description:  A really simple controller which moves the MyBot robot
 *               and avoids the walls
 */

#include <stdio.h>
#include <webots/distance_sensor.h>
#include <webots/motor.h>
#include <webots/robot.h>

#define SPEED 6
#define TIME_STEP 64
enum direcao { LEFT, RIGHT };
enum STATE { ANDAR, RECUAR, BUSCAR, ATACAR, PARAR, FORCAR };
int state = ANDAR;
int last_state;
void recuar(int dir, WbDeviceTag left_motor, WbDeviceTag right_motor,
            float tempo_inicio_tarefa, float tempo, double *speed_l,
            double *speed_r);
int verifica_linha(double right_ground_ir_value, double left_ground_ir_value,
                   int dir);
void verifica_oponente(double *lidar_value, int *menor_lidar);
double speed_converter(int speed, int real_speed);

int main() {
  wb_robot_init(); /* necessary to initialize webots stuff */
  printf("Robo inicializado\n");
  /* Get and enable the distance sensors. */
  // Vetores para os sensores Lidar
  char lidar_tag[7][8] = {"lidar 1", "lidar 2", "lidar 3", "lidar 4",
                          "lidar 5", "lidar 6", "lidar 7"};
  WbDeviceTag lidar[7];
  for (int i = 0; i < 7; i++) {
    lidar[i] = wb_robot_get_device(lidar_tag[i]);
    wb_distance_sensor_enable(lidar[i], TIME_STEP);
  }

  WbDeviceTag right_ground_ir = wb_robot_get_device("right_ground_infrared");
  WbDeviceTag left_ground_ir = wb_robot_get_device("left_ground_infrared");
  wb_distance_sensor_enable(right_ground_ir, TIME_STEP);
  wb_distance_sensor_enable(left_ground_ir, TIME_STEP);

  /* get a handler to the motors and set target position to infinity (speed
   * control). */
  WbDeviceTag left_motor = wb_robot_get_device("left wheel motor");
  WbDeviceTag right_motor = wb_robot_get_device("right wheel motor");
  wb_motor_set_position(left_motor, INFINITY);
  wb_motor_set_position(right_motor, INFINITY);
  wb_motor_set_velocity(left_motor, 0.0);
  wb_motor_set_velocity(right_motor, 0.0);
  double lidar_value[7];
  double right_ground_ir_value = 0, left_ground_ir_value = 0, speed_l = 0,
         speed_r = 0, real_speed_l = 0, real_speed_r = 0;
  double initial_velocity = 1;
  float tempo, tempo_inicio_tarefa;
  int dir;
  int menor_lidar;
  while (wb_robot_step(TIME_STEP) != -1) {
    tempo = wb_robot_get_time();
    for (int i = 0; i < 7; i++) {
      lidar_value[i] = wb_distance_sensor_get_value(lidar[i]);
      // printf("Lidar %d: %f\n", i+1, lidar_value[i]);
    }
    // sensor de linha ( 952 para preto e 1024 para branco )
    right_ground_ir_value = wb_distance_sensor_get_value(right_ground_ir);
    left_ground_ir_value = wb_distance_sensor_get_value(left_ground_ir);
    last_state = state;
    dir = verifica_linha(right_ground_ir_value, left_ground_ir_value, dir);
    verifica_oponente(lidar_value, &menor_lidar);
    if (state == RECUAR && last_state != RECUAR)
      tempo_inicio_tarefa = wb_robot_get_time();
    // printf("right_ground_ir_value = %f left_ground_ir_value = %f\n",
    // right_ground_ir_value, left_ground_ir_value);
    printf("state: %d   tempo: %f   speed_l: %f speed_r: %f menor_lidar: %d\n",
           state, tempo, speed_l, speed_r, menor_lidar);
    switch (state) {
      case ANDAR:
        speed_l = 10;
        speed_r = 10;
        break;
      case RECUAR:
        recuar(dir, left_motor, right_motor, tempo_inicio_tarefa, tempo,
               &speed_l, &speed_r);
        break;
      case ATACAR:
        // 7 sensores, 3 para esquerda em diferentes angulos, 3 para direita, um
        // centralizado. Se for o sensor central que capta a menor distancia,
        // faz um movimento retilineo, caso não Faz o movimento mais curvo ou
        // menos curvo em direção ao oponente quanto mais ou menos angulado for
        // o sensor.
        speed_l = 20 * (1 + ((menor_lidar - 3) / 1.5));
        speed_r = 20 * (1 - ((menor_lidar - 3) / 1.5));
        break;
      case PARAR:
        printf("Parando!\n");
        speed_l = 0;
        speed_r = 0;
        break;
      case BUSCAR:
        if (dir == LEFT) {
          speed_l = 10;
          speed_r = 15;
        } else {
          speed_l = 15;
          speed_r = 10;
        }
        break;
      case FORCAR:
        speed_l = 40;
        speed_r = 40;
        break;
    }
    wb_motor_set_velocity(left_motor, real_speed_l);
    wb_motor_set_velocity(right_motor, real_speed_r);
  }
  return 0;
}