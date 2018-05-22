#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#include "arkanoPi.h"


/// TODO: Necesita mas comprobaciones para esquinas, etc.. pero se haria una maquina comn muchas transiciones
/// y para el promela se hace un poco grande.

int ladrillo(fsm_t* this) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;
  return juego.arkanoPi.ladrillos.matriz[nextPosX][nextPosY] == 1;
}

int comprobarY(fsm_t* this) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;

  return nextPosY == 0;
}

int comprobarGolpeConRaquetaCentro(fsm_t* this) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;

  return ((juego.arkanoPi.raqueta.x + 1) == nextPosX && juego.arkanoPi.raqueta.y == nextPosY);
}

int comprobarGolpeConRaquetaDerecha(fsm_t* this) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;

  return ((juego.arkanoPi.raqueta.x + 2) == nextPosX && juego.arkanoPi.raqueta.y == nextPosY);
}

int comprobarGolpeConRaquetaIzquierda(fsm_t* this) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;

  return (juego.arkanoPi.raqueta.x == nextPosX && juego.arkanoPi.raqueta.y == nextPosY);
}

int comprobarX(fsm_t* this) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;
  return nextPosX == 0 || nextPosX == 9;
}

int comprobarFinDeJuego(fsm_t* this) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;
  return nextPosY == 6;
}

int comprobarMovLibre(fsm_t* this) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;

  return !ladrillo(this) && !comprobarX(this) && !comprobarY(this) && !comprobarFinDeJuego(this);
}

void move() {
  juego.arkanoPi.pelota.x = juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv;
  juego.arkanoPi.pelota.y = juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv;
}

void goY() {
  juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;
  move();
}

void goYChangingXToRight() {
  juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;
  juego.arkanoPi.pelota.xv = 1;
  move();
}

void goYChangingXToLeft() {
  juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;
  juego.arkanoPi.pelota.xv = -1;
  move();
}

void goX() {
  juego.arkanoPi.pelota.xv = -juego.arkanoPi.pelota.xv;
  move();
}

void killLadrillo() {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;
  juego.arkanoPi.ladrillos.matriz[nextPosX][nextPosY] = 0;
  goY();
}

void finDeJuego() {
  printf("DEAD\n");
}

// void MovimientoPelota (void): funci�n encargada de actualizar la
// posici�n de la pelota conforme a la trayectoria definida para �sta.
// Para ello deber� identificar los posibles rebotes de la pelota para,
// en ese caso, modificar su correspondiente trayectoria (los rebotes
// detectados contra alguno de los ladrillos implicar�n adicionalmente
// la eliminaci�n del ladrillo). Del mismo modo, deber� tambi�n
// identificar las situaciones en las que se d� por finalizada la partida:
// bien porque el jugador no consiga devolver la pelota, y por tanto �sta
// rebase el l�mite inferior del �rea de juego, bien porque se agoten
// los ladrillos visibles en el �rea de juego.
void MovimientoPelota (fsm_t* fsm) {
  int posX = juego.arkanoPi.pelota.x;
  int posY = juego.arkanoPi.pelota.y;
  int velX = juego.arkanoPi.pelota.xv;
  int velY = juego.arkanoPi.pelota.yv;
  int nextPosX = posX + velX;
  int nextPosY = posY + velY;



  // choca contra ladrillo
  if (juego.arkanoPi.ladrillos.matriz[nextPosX][nextPosY] == 1) {
    juego.arkanoPi.ladrillos.matriz[nextPosX][nextPosY] = 0;
    juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;
  } else if ((juego.arkanoPi.raqueta.x == nextPosX && juego.arkanoPi.raqueta.y == nextPosY) || nextPosY == 6) {
    juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;
  } else if (nextPosX == 0 || nextPosX == 9) {
    juego.arkanoPi.pelota.xv = -juego.arkanoPi.pelota.xv;
  } else if (nextPosY == 0) {
    // dead!
    printf("muerto\n");
  }


  juego.arkanoPi.pelota.x = juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv;
	juego.arkanoPi.pelota.y = juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv;

/*
	if (juego.arkanoPi.ladrillos.matriz[juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv][juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv] == 1){ //choca contra ladrillo
			juego.arkanoPi.ladrillos.matriz[juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv][juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv] = 0;
			juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;
	}

	if (juego.arkanoPi.pelota.y == 0 && juego.arkanoPi.ladrillos.matriz[juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv][juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv] == 0){
		if ((juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv) == 0 && (juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv) == -1){
			juego.arkanoPi.pelota.xv = 1;
		}
		if ((juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv) == 9 && (juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv) == -1){
			juego.arkanoPi.pelota.xv = -1;
		}
		juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;

	}

	if(juego.arkanoPi.pelota.x == 0 || (juego.arkanoPi.pelota.x  == MATRIZ_ANCHO - 1)){ //choca con pared lateral
		juego.arkanoPi.pelota.xv = -juego.arkanoPi.pelota.xv;
		if(juego.arkanoPi.ladrillos.matriz[juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv][juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv] == 1){
			juego.arkanoPi.ladrillos.matriz[juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv][juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv] = 0;
			juego.arkanoPi.pelota.yv = -juego.arkanoPi.pelota.yv;
		}
	}

	if((juego.arkanoPi.pelota.y == (MATRIZ_ALTO - 2))&& juego.arkanoPi.pelota.yv == 1) {  //choca contra raqueta1
		if(juego.arkanoPi.pelota.x == 9 && juego.arkanoPi.pelota.xv == 0){
			juego.arkanoPi.pelota.xv=-1;
			juego.arkanoPi.pelota.yv=-1;

		}
		if ((juego.arkanoPi.raqueta.x + 2) == (juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv)){
			juego.arkanoPi.pelota.yv = -1;
			juego.arkanoPi.pelota.xv = 1;

		}
		else if ((juego.arkanoPi.raqueta.x + 1) == (juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv)){
			juego.arkanoPi.pelota.yv = -1;
			juego.arkanoPi.pelota.xv = 0;
		}
		else if (juego.arkanoPi.raqueta.x  == (juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv)) {
			juego.arkanoPi.pelota.yv = -1;
			if (juego.arkanoPi.pelota.x == 0) {
				juego.arkanoPi.pelota.xv=1;
			}
			else
				juego.arkanoPi.pelota.xv = -1;
		} else {
      // fin del juego en teoria.
		}
	}

	juego.arkanoPi.pelota.x = juego.arkanoPi.pelota.x + juego.arkanoPi.pelota.xv;
	juego.arkanoPi.pelota.y = juego.arkanoPi.pelota.y + juego.arkanoPi.pelota.yv;*/
}

fsm_t*
fsm_new_ball ()
{
  static fsm_trans_t tt[] = {
    {1, ladrillo , 1, killLadrillo},
    {1,  comprobarY, 1, goY},
    {1, comprobarGolpeConRaquetaCentro, 1, goY},
    {1, comprobarGolpeConRaquetaDerecha, 1, goYChangingXToRight},
    {1, comprobarGolpeConRaquetaIzquierda, 1, goYChangingXToLeft},
    {1, comprobarX, 1, goX},
    {1, comprobarFinDeJuego, 1, finDeJuego},
    {1, comprobarMovLibre, 1, move},
    {-1, NULL, -1, NULL},
  };

  return fsm_new (tt);
}
