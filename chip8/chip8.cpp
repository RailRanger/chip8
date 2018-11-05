// chip8.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <SDL.h>
#include <fstream>
#include <string>
#include <ctime>
#include <windows.h>
#pragma warning(disable:4996)


uint8_t key[16];

class chip8 {
public:
	unsigned short opcode = 0;		//instruction from ram/rom
	unsigned char memory[4096];		//chip8 has 4096kB of RAM
	unsigned char V[16];			//chip8 has 16 registers
	unsigned short I = 0;			//instruction pointer		
	unsigned short pc = 0x200;		//program counter
	unsigned short gfx[64][32];		//vram, chip8 has a 64x32 display, with a 1 or 0 deciding if a pixel is on or off
	unsigned char delay_timer = 0;	
	unsigned char sound_timer = 0;
	void initialize();
	void emulateCycle();
	unsigned __int64 now();
	unsigned __int64 startTime;
	unsigned __int64 currentTime;
	unsigned __int64 lastTime;
	unsigned short stack[16];
	unsigned short sp = 0;
	bool drawFlag = false;
	double timerFrequency = 0;
	unsigned __int64 freq;

};

unsigned __int64 chip8::now() {
		QueryPerformanceCounter((LARGE_INTEGER *)&startTime);
		return startTime;
	}



unsigned char chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void chip8::initialize()
{
	//Clear memory
	for (int i = 0; i < 4096; i++) {
		memory[i] = 0;
	}

	//Clear registers

	for (int i = 0; i < 16; i++) {
		V[i] = 0;
	}

	for (int i = 0; i < 80; ++i) {
		memory[i] = chip8_fontset[i];
	}


	for (unsigned short i = 0; i < 16; i++) {
		stack[i] = 0;
	}


	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 64; x++) {
			gfx[x][y] = 0x0;
		}
	}

	//	0x000 - 0x1FF - Chip 8 interpreter(contains font set in emu)
	//	0x050 - 0x0A0 - Used for the built in 4x5 pixel font set(0 - F)
	//	0x200 - 0xFFF - Program ROM and work RAM
	size_t lSize;
	FILE * pFile;
	lSize = 0;
	pFile = fopen("framed.ch8", "rb");


	if (pFile == NULL) {
		perror("Error opening file");
		Sleep(2000);
		exit(EXIT_FAILURE);
	}
	else
	{
		char * buffer;
		fseek(pFile, 0, SEEK_END);   // non-portable
		lSize = ftell(pFile);
		printf("Size of rom: %ld bytes.\n", lSize);
		rewind(pFile);



		if ((4095 - 512) > lSize) {
			buffer = (char*)malloc(sizeof(char)*lSize);

			size_t result = fread(buffer, 1, lSize, pFile);
			if (buffer == NULL) { fputs("Memory error\n", stderr); exit(2); }
			if (result != lSize) { fputs("Reading error\n", stderr); exit(3); }

			for (size_t i = 0; i < lSize; i++) {
				memory[i + 512] = buffer[i];
			}

			free(buffer);
			fclose(pFile);
		}
		else {
			std::cout << "ROM is too large" << std::endl;
		}
	}

}

void chip8::emulateCycle() {
	unsigned short vx = 0;
	unsigned short vy = 0;
	unsigned short nn = 0;


	//decide opcode
	opcode = memory[pc] << 8 | memory[pc + 1];

	//printf("The opcode undecoded is: %x\n", opcode);

	switch (opcode & 0xF000) {

	case 0x0000:
		switch (opcode & 0x000F)
		{
		case 0x0000: // 0x00E0: Clears the screen
			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 64; x++) {
					gfx[x][y] = 0x0;
				}
			}
			drawFlag = true;
			pc += 2;
			goto EXITSWITCH;

		case 0x000E: // 0x00EE: Returns from subroutine
			sp--;			// 16 levels of stack, decrease stack pointer to prevent overwrite
			pc = stack[sp];	// Put the stored return address from the stack back into the program counter					
			pc += 2;		
			goto EXITSWITCH;

		default:
			printf("Unknown opcode [0x0000] TEST0: 0x%X\n", opcode);
		}



	case 0xA000: //Sets I to the address NNN.
		I = opcode & 0x0FFF;
		pc += 2;
		goto EXITSWITCH;


	case 0xB000:
		pc = (opcode & 0x0FFF) + V[0];
		goto EXITSWITCH;

	case 0xC000:
		vx = opcode & 0x0F00;
		vx = vx >> 8;
		nn = opcode & 0x00FF;
		std::srand(time(0));
		V[vx] = (std::rand() % 255) & nn;
		pc += 2;
		goto EXITSWITCH;

	case 0xD000: {
		vx = opcode & 0x0F00;
		vx = vx >> 8;
		vy = opcode & 0x00F0;
		vy = vy >> 4;
		vx = V[vx];		//X Position offset
		vy = V[vy];		//Y Position offset

		unsigned short height = opcode & 0x000F;
		unsigned char pixel;
		V[0XF] = 0;

		//std::cout << vx << std::endl;
		//std::cout << vy << std::endl;

		for (unsigned short y = 0; y < height; y++) {
			pixel = memory[I + y];
			for (unsigned short x = 0; x < 8; x++) {

				if (gfx[vx + x][vy + y] == 1 && ((0x80 >> x) & pixel) >> (7 - x) == 1) {
					V[0xF] = 1;
				}

				gfx[vx + x][vy + y] ^= ((((0x80 >> x) & pixel)) >> (7 - x));


			}

		}

		drawFlag = true;
		pc += 2;
		goto EXITSWITCH;
	}


	case 0xE000:
		switch (opcode & 0x00F0) {	//Nested switch case for EX9E and EXA1
		case 0x0090:

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			if (key[V[vx]] == 1) {
				pc += 4;

			}
			else {
				pc += 2;


			}
			goto EXITSWITCH;

		case 0x00A0:
			vx = opcode & 0x0F00;
			vx = vx >> 8;
			if (key[V[vx]] == 0) {
				pc += 4;
			}
			else {
				pc += 2;
			}
			goto EXITSWITCH;

		default:
			printf("Unknown opcode TEST: 0x%X\n", opcode);

		}

	case 0xF000:		//Nest of opcodes starting with F


		switch (opcode & 0x00FF) {

		case 0x0007: //Sets VX to the value of the delay timer.
			vx = opcode & 0x0F00;
			vx = vx >> 8;
			V[vx] = delay_timer;
			pc += 2;
			goto EXITSWITCH;

		case 0x000A: {		//Double Check this //A key press is awaited, and then stored in VX. (Blocking Operation. All instruction halted until next key event)

			unsigned short i = 0;
			vx = opcode & 0x0F00;
			vx = vx >> 8;
			while (true) {
				if (key[i] == 1) {
					V[vx] = i;
					key[i] = 0;
					i++;
					goto EXITSWITCH;
				}
			}
			pc += 2;
			goto EXITSWITCH;
		}

					 //test

		case 0x0015: //Sets the delay timer to VX.

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			delay_timer = V[vx];
			pc += 2;
			goto EXITSWITCH;

		case 0x0018:	//Sets the sound timer to VX.

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			sound_timer = V[vx];
			pc += 2;
			goto EXITSWITCH;

		case 0x001E: // FX1E: Adds VX to I. VF is set to 1 when there is a range overflow (I+VX>0xFFF), and to 0 when there isn't. This is an undocumented feature of the CHIP-8 and used by the Spacefight 2091! game.

			vx = opcode & 0x0F00;
			vx = vx >> 8;

			if ((I + V[(vx)]) > 0xFFF) {// VF is set to 1 when range overflow (I+VX>0xFFF), and 0 when there isn't.
				V[0xF] = 1;
			}
			else {
				V[0xF] = 0;
			}
			I = I + V[vx];
			pc += 2;
			goto EXITSWITCH;

		case 0x0029:	//Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			I = V[vx] * 0x5;
			pc += 2;
			goto EXITSWITCH;

		case 0x0033:
			memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
			memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
			memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
			pc += 2;
			goto EXITSWITCH;

		case 0x0055: {
			vx = opcode & 0x0F00;
			vx = vx >> 8;
			
			for (int i = 0; i <= vx; i++) {
				memory[I + i] = V[i];
			}
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			
			goto EXITSWITCH;

		}

		case 0x0065: { // FX65: Fills V0 to VX with values from memory starting at address I		
			vx = opcode & 0x0F00;
			vx = vx >> 8;
			
			for (int i = 0; i <= vx; i++) {
				V[i] = memory[I + i];
			}
			I += ((opcode & 0x0F00) >> 8) + 1;
			pc += 2;
			goto EXITSWITCH;
		}

		default:
			printf("Unknown opcode TEST2: 0x%X\n", opcode);
		}
		//End of opcodes starting with F


	case 0x1000: //Jumps to address NNN.
		pc = opcode & 0x0FFF;
		goto EXITSWITCH;

	case 0x2000: //Calls subroutine at NNN.
		stack[sp] = pc;
		sp++;
		pc = 0x0FFF & opcode;
		goto EXITSWITCH;

	case 0x3000: //Skips the next instruction if VX equals NN. (Usually the next instruction is a jump to skip a code block)
		vx = opcode & 0x0F00;
		vx = vx >> 8;

		if (V[vx] == (opcode & 0x00FF)) {
			pc += 4;
		}
		else {
			pc += 2;
		}
		goto EXITSWITCH;

	case 0x4000: //Skips the next instruction if VX doesn't equal NN. (Usually the next instruction is a jump to skip a code block)
		vx = opcode & 0x0F00;
		vx = vx >> 8;

		if (V[vx] != (opcode & 0x00FF)) {
			pc += 4;
		}
		else {
			pc += 2;
		}
		goto EXITSWITCH;

	case 0x5000: //Skips the next instruction if VX equals VY. (Usually the next instruction is a jump to skip a code block)
		vx = opcode & 0x0F00;
		vx = vx >> 8;

		vy = opcode & 0x00F0;
		vy = vy >> 4;

		if (V[vx] == V[vy]) {
			pc += 4;
		}
		else {
			pc += 2;
		}
		goto EXITSWITCH;

	case 0x6000: //Sets VX to NN.
		vx = opcode & 0x0F00;
		vx = vx >> 8;
		V[vx] = opcode & 0x00FF;
		pc += 2;
		goto EXITSWITCH;

	case 0x7000: //Adds NN to VX. (Carry flag is not changed)
		vx = opcode & 0x0F00;
		vx = vx >> 8;
		V[vx] = V[vx] + (opcode & 0x00FF);
		pc += 2;
		goto EXITSWITCH;


	case 0x9000: //	Skips the next instruction if VX doesn't equal VY. (Usually the next instruction is a jump to skip a code block)

		vx = opcode & 0x0F00;
		vx = vx >> 8;
		vy = opcode & 0x00F0;
		vy = vy >> 4;
		if (V[vx] != V[vy]) {
			pc += 4;
		}
		else {
			pc += 2;
		}
		goto EXITSWITCH;





	case 0x8000:	//Opcodes starting with 8
		switch (opcode & 0x000F) {
		case 0x0000: //Sets VX to the value of VY.

			vx = opcode & 0x0F00;
			vy = opcode & 0x00F0;
			vx = vx >> 8;
			vy = vy >> 4;
			V[vx] = V[vy];
			pc += 2;
			goto EXITSWITCH;

		case 0x0001: //Sets VX to VX or VY. (Bitwise OR operation)

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			vy = opcode & 0x00F0;
			vy = vy >> 4;
			V[vx] = V[vx] | V[vy];
			pc += 2;
			goto EXITSWITCH;

		case 0x0002: //Sets VX to VX and VY. (Bitwise AND operation)

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			vy = opcode & 0x00F0;
			vy = vy >> 4;
			V[vx] = V[vx] & V[vy];
			pc += 2;
			goto EXITSWITCH;

		case 0x0003: //Sets VX to VX xor VY.

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			vy = opcode & 0x00F0;
			vy = vy >> 4;
			V[vx] = V[vx] ^ V[vy];
			pc += 2;
			goto EXITSWITCH;

		case 0x0004: //Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there isn't.

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			vy = opcode & 0x00F0;
			vy = vy >> 4;

			if(V[vx] + V[vx] > 0xFF) {
				V[0XF] = 1;
			}
			else {
				V[0xF] = 0;
			}

			V[vx] = V[vx] + V[vy];
			pc += 2;
			goto EXITSWITCH;

		case 0x0005: //VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there isn't.


			vx = opcode & 0x0F00;
			vx = vx >> 8;
			vy = opcode & 0x00F0;
			vy = vy >> 4;

			if (V[vy] > V[vx]) {
				V[0xF] = 0;
			}
			else {
				V[0xF] = 1;
			}

			V[vx] = V[vx] - V[vy];
			pc += 2;
			goto EXITSWITCH;


		case 0x0006: //Stores the least significant bit of VX in VF and then shifts VX to the right by 1.[2]

		
			vx = opcode & 0x0F00;
			vx = vx >> 8;
			V[0xF] = (V[vx] & 0x01);
			V[vx] = V[vx] >> 1;

			pc += 2;
			goto EXITSWITCH;


		case 0x0007: //Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there isn't.


			vx = opcode & 0x0F00;
			vx = vx >> 8;
			vy = opcode & 0x00F0;
			vy = vy >> 4;
			if (V[vx] > V[vy]) {
				V[0xF] = 0;
			}
			else {
				V[0xF] = 1;
			}
			V[vx] = V[vy] - V[vx];
			pc += 2;
			goto EXITSWITCH;

		case 0x000E:	//Stores the most significant bit of VX in VF and then shifts VX to the left by 1.[3]

			vx = opcode & 0x0F00;
			vx = vx >> 8;
			vy = opcode & 0x00F0;
			vy = vy >> 4;

			V[0xF] = (V[vx] >> 7);
			V[vx] = V[vx] << 1;
			pc += 2;
			goto EXITSWITCH;

		default:
			printf("Unknown opcode: TEST3 0x%X\n", opcode);

		}


	default:
		printf("Unknown opcode TEST4: 0x%X\n", opcode);

	}
EXITSWITCH:;

	if (delay_timer > 0) {
		--delay_timer;
	}
	if (sound_timer > 0) {
		if (sound_timer == 1) {
			std::cout << "BEEP" << std::endl;
			--sound_timer;
		}
	}

}








int main(int argc, char *argv[])
{
	chip8 myChip8;
	myChip8.initialize();

	/*while (true) {	//debug vram
		myChip8.emulateCycle();
		if (myChip8.drawFlag == 1) {


			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 64; x++) {
					if (myChip8.gfx[x][y] == 1) {
						printf("x");
					}
					else if (myChip8.gfx[x][y] == 0) {
						printf(" ");
					}
					else {
						std::cout << '?';
					}

					if (x == 63) {
						std::cout << std::endl;
					}
				}
			}
		}
		myChip8.drawFlag = 0;
		//Sleep(100);
	}
	*/
	QueryPerformanceFrequency((LARGE_INTEGER*)&myChip8.freq);
	myChip8.timerFrequency = (1.0 / myChip8.freq);





	int scaler = 10;
	const int SCREEN_WIDTH = 64 * scaler;
	const int SCREEN_HEIGHT = 32 * scaler;


	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window = NULL;
	SDL_Surface* screenSurface = NULL;

	window = SDL_CreateWindow("Chippy8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	screenSurface = SDL_GetWindowSurface(window);
	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
	SDL_UpdateWindowSurface(window);
	SDL_Event event;

	SDL_Renderer* renderer = NULL;
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_GL_SwapWindow;
	
	// Set render color to red ( background will be rendered in this color )


	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);


	int quit = 0;

	while (SDL_PollEvent(&event) || quit!=1) {
		if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
			case SDLK_1: key[0x1] = 1; break;
			case SDLK_2: key[0x2] = 1; break;
			case SDLK_3: key[0x3] = 1; break;
			case SDLK_4: key[0xC] = 1; break;
			case SDLK_q: key[0x4] = 1; break;
			case SDLK_w: key[0x5] = 1; break;
			case SDLK_e: key[0x6] = 1; break;
			case SDLK_r: key[0xD] = 1; break;
			case SDLK_a: key[0x7] = 1; break;
			case SDLK_s: key[0x8] = 1; break;
			case SDLK_d: key[0x9] = 1; break;
			case SDLK_f: key[0xE] = 1; break;
			case SDLK_z: key[0xA] = 1; break;
			case SDLK_x: key[0x0] = 1; break;
			case SDLK_c: key[0xB] = 1; break;
			case SDLK_v: key[0xF] = 1; break;
			case SDLK_ESCAPE: exit(1); break;
			}
		}
		else if (event.type == SDL_KEYUP) {
			switch (event.key.keysym.sym) {
			case SDLK_1: key[0x1] = 0; break;
			case SDLK_2: key[0x2] = 0; break;
			case SDLK_3: key[0x3] = 0; break;
			case SDLK_4: key[0xC] = 0; break;
			case SDLK_q: key[0x4] = 0; break;
			case SDLK_w: key[0x5] = 0; break;
			case SDLK_e: key[0x6] = 0; break;
			case SDLK_r: key[0xD] = 0; break;
			case SDLK_a: key[0x7] = 0; break;
			case SDLK_s: key[0x8] = 0; break;
			case SDLK_d: key[0x9] = 0; break;
			case SDLK_f: key[0xE] = 0; break;
			case SDLK_z: key[0xA] = 0; break;
			case SDLK_x: key[0x0] = 0; break;
			case SDLK_c: key[0xB] = 0; break;
			case SDLK_v: key[0xF] = 0; break;
			}
		}
		else if (event.type == SDL_QUIT) {
			quit = 1;
		}


		myChip8.emulateCycle();


		/*if (myChip8.drawFlag == 1) {
			for (unsigned short y = 0; y < 32; y++) {
				for (unsigned short x = 0; x < 64; x++) {
					for (unsigned short yy = 0; yy < scaler; yy++) {
						for (unsigned short xx = 0; xx < scaler; xx++) {
							if (myChip8.gfx[x][y] == 1) {
								SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
								SDL_RenderDrawPoint(renderer, x*scaler + xx, y*scaler + yy);

							}
							else {
								SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
								SDL_RenderDrawPoint(renderer, x*scaler+xx, y*scaler+yy);

							}
						}

					}

				}
			}
		}
		*/
		
	


		if (myChip8.drawFlag == 1) {
			SDL_RenderClear;
			SDL_RendererFlip();
			for (int y = 0; y < 32; y++) {
				for (int x = 0; x < 64; x++) {
					
					if (myChip8.gfx[x][y] == 1) {
						SDL_Rect pixel = { x*scaler, y*scaler, scaler, scaler };
						SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
						SDL_RenderFillRect(renderer, &pixel);
						

					}
					else {
						SDL_Rect pixel = { x*scaler, y*scaler, scaler, scaler };
						SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
						SDL_RenderFillRect(renderer, &pixel);
						

					}
				}
			}
			myChip8.currentTime = myChip8.now();
			if (((myChip8.currentTime - myChip8.lastTime)*myChip8.timerFrequency) < 16.67) {
				SDL_Delay(16.67 - (myChip8.currentTime - myChip8.lastTime)*myChip8.timerFrequency);
			}
			SDL_RenderPresent(renderer);
			myChip8.drawFlag = 0;
			myChip8.lastTime = myChip8.startTime;
		}
		

		
		



	
	}




	return 0;
}

bool init()
{
	return false;
}
