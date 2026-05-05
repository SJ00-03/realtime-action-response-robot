//************************************//
// 2026 비전 응용 프로젝트
// 팀 : 구성최광임

#include "stm32f4xx.h"


#define basic_speed  90  // 수동 제어 시 기본 모터 PWM 90%

// --- 함수 선언부 ---
void _GPIO_Init(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void TIMER4_PWM_Init(void);	// Timer4 PWM mode 초기화 

void BLINK_LED_Init(void); // GPIOE LED 초기화 (현재 미사용)
void TIMER3_Interrupt_Init(void);  // 500ms 주기로 인터럽트를 발생시키는 TIM3 초기화
void TIM3_IRQHandler(void);  // TIM3 인터럽트 발생 시 실행되는 핸들러 함수 

void UART1_Bluetooth_Init(void);  // 블루투스 앱 통신을 위한 UART1 초기화
int UART1_Read_Byte(void);  // 블루투스 데이터 1바이트 읽기

// --- 로봇 주행 제어 함수 ---
void STOP_ROBOT(void);  // 로봇 정지
void FRONT(int speed_front1);  // 로봇 전진
void FRONT_SLOW(int speed_front2);  //로봇 전진(느리게)
void BACK(int speed_BACK);  // 로봇 후진

void LEFT_TURN(int speed_left_turn);	// 로봇 좌회전(제자리)
void RIGHT_TURN(int speed_right_turn);	// 로봇 우회전(제자리)
void FRONT_LEFT(int speed1);	// 로봇 좌회전(전진하며)
void FRONT_RIGHT(int speed2);	// 로봇 우회전(전진하며)

int cmd;  // 블루투스로 수신한 현재 명령 저장 변수
int manual;  // 수동 조작 모드 ON/OFF 변수 (1: 수동, 0: 자동)
int auto_move;  // 자동 순찰 모드 ON/OFF 변수 (1: 자동, 0: 정지)
int prev_cmd; // 이전에 실행한 명령을 기억하여 부드러운 전환을 돕는 변수
int end_line; // 라인트레이싱 종료 여부 (1: 종료, 0: 진행중)
int temp_cmd; // 블루투스 통신 시 데이터 유효성 검증을 위한 임시 변수 (전 명령 기억)

int SLOW_speed = 80;  // 자동 주행 기본 속도 PWM 80%
int TURN_speed = 70;  // 회전 기본 속도 PWM 70%

int right_offset = 0; //모터 속도보정: 우측 바퀴가 빠를 때 깎아줄 PWM 값
int left_offset = 0;  //모터 속도보정: 좌측 바퀴가 빠를 때 깎아줄 PWM 값
int turn_pulse_ms = 200; // 자동 주행 시 회전하는 시간(ms) 초기값

volatile uint16_t RPI_data, situation_data, drive_data, enable_run;  // 라즈베리파이에서 받은 데이터들
volatile int blink_flag = 0;  // 1이면 TIM3 인터럽트에서 LED를 깜빡임, 0이면 깜빡임 중지
int prev_situation_data;  // 이전 상황을 기억해 상태가 바뀔 때만 LED/부저를 리셋
volatile int m_em;  // 수동 비상 모드 플래그

void MANUAL_CON(void); // 수동제어 함수
void Read_RPI_Data(void); // 라즈베리파이 통신함수
void LINE_TRACING(void); // 라인트레이싱 함수

int main(void)
{
	_GPIO_Init();		// 모든 GPIO(LED, 모터, 센서) 핀 초기화
	GPIOD->ODR &= ~0xFF00;	// 초기값: 보드에 내장된 LED(PD8~PD15) 모두 끄기
	TIMER4_PWM_Init();		// TIM4 Init (PWM mode) 모터 PWM (PB8, 9) 초기화
	UART1_Bluetooth_Init();	// 스마트폰 앱 조종을 위한 블루투스 통신 초기화
	
    TIMER3_Interrupt_Init();  //점멸 동작을 위한 타이머 인터럽트
	
	// 상태 변수들 초기화
	cmd = -1; // -1은 데이터가 없는 초기 상태
	manual = 0;  // 처음엔 수동 조작 OFF
	prev_cmd = 1;
	
	RPI_data = 0; //변수 초기화
	situation_data = 0;   //변수 초기화
	drive_data = 0;   //변수 초기화
	enable_run = 0;   //변수 초기화
	end_line = 0; // 라인트레이싱 변수 초기화
	blink_flag = 0;
	m_em = 0;
	prev_situation_data = -1;
	
	DelayMS(500);	// 시스템 안정화를 위한 짧은 대기
	GPIOD->ODR |= 0x8000; 	// PD15(파란색 내장 LED)를 켜서 부팅 완료를 알림

	while(1)  //메인 무한 루프 시작
	{

		GPIOD->ODR |= 0x8000; 	// 보드 동작 상태 표시용 LED 켜기 (PD15)
		
		temp_cmd = UART1_Read_Byte(); // 블루투스 데이터 읽기(앱에서 보낸 데이터 확인)
		Read_RPI_Data(); //라즈베리파이 데이터 읽어오기
		
        if (temp_cmd != -1) // 데이터(새로운 명령)가 들어왔을 때만 실행
        {
			// 로봇의 전체적인 속도 튜닝 명령 (앱에서 설정)
			if (temp_cmd >= 6 && temp_cmd <= 8) 
			{	
				if (temp_cmd == 6) // 6: 회전 속도 감소
				{
					TURN_speed -= 5;
					if (TURN_speed < 30) TURN_speed = 30;  // 하한선 안전장치
				}
				else if (temp_cmd == 7) // 7: 전후진 속도 감소
				{
					SLOW_speed -= 5;
					if (SLOW_speed < 30) SLOW_speed = 30;
				}
				else if (temp_cmd == 8) // 8: 전체 속도 증가
				{
					SLOW_speed += 5;
					TURN_speed += 5;
					if (SLOW_speed > 100) SLOW_speed = 100;  // PWM 상한선 (100%)
					if (TURN_speed > 100) TURN_speed = 100;
				}
			}
			else
			{
				cmd = temp_cmd; // 유효한 명령이 들어왔을 때만 업데이트

				if (cmd == 1) // 앱에서 정지버튼(1) 수신 시, 무조건 전체 초기화 및 정지
				{
					//ODR:   0000 0000 0000 0000
					//       8421 8421 8421 8421
					//0x2000 0010 0000 0000 0000 -> 13번 핀 키기
					GPIOD->ODR |= 0x2000; 	// GPIOG->ODR.0 'H' PD13 LED 켜기 (디버깅용)


					STOP_ROBOT();
					auto_move = 0; //순찰기동 정지
					manual = 0;  // 수동 정지
					end_line = 0;  // 라인트레이싱 초기화
					m_em = 0;  // 수동비상 종료
					blink_flag = 0;  //점멸동작 종료

					// Active-Low 부저 및 외장 LED 모두 OFF (1이 꺼짐, 0이 켜짐인 부품들 고려)
					GPIOA->ODR &= ~((1<<4) | (1<<5) | (1<<6) | (1<<7)); // 외부 LED 끄기 4번, 5번 6번 7번자리애 1 집어넣고 0으로 반들어라
					GPIOD->ODR &= ~(1<<0); // 레이저(PD0) OFF
            		GPIOA->ODR |= (1<<8);  // 부저(PA8) 끄기 (Active-Low 이므로 1을 주어야 꺼짐)
					// 속도 설정 초기화
					SLOW_speed = 80;
    				TURN_speed = 70;
					// 과거 기억 리셋 (안전한 재시작 위함)
					prev_cmd = 1;              // 이전 수동 명령 기억 리셋
					prev_situation_data = -1;  // 이전 상황 기억 리셋 (그래야 다음 상황 때 다시 LED가 제대로 켜짐)
					temp_cmd = -1;             // 임시 명령 버퍼 비우기
				}
				else if(cmd == 2)  // 임시 정지
				{
					STOP_ROBOT();
				}
				else if(cmd == 3)  // 수동제어모드 해제
				{
					manual = 0; // 수동조작 OFF
				}
				else if(cmd == 4)  // 수동비상모드
				{
					STOP_ROBOT();
					auto_move = 0;
					end_line = 0;
					GPIOA->ODR &= ~((1<<4) | (1<<5) | (1<<6) | (1<<7)); 
					GPIOD->ODR &= ~(1<<0); // 레이저(PD0) OFF
            		GPIOA->ODR |= (1<<8); // 부저 OFF 처
					m_em = 1;  // 수동비상 모드(핸들러에서 동작)
					blink_flag = 1;	 // 인터럽트에서 깜빡이도록 플래그 세팅
				}
				else if(cmd == 5) // 수동 제압 (레이저 발사)
				{
					GPIOD->ODR |= (1<<0);  // PD0번 핀 HIGH -> 레이저 ON
				}
				else if (cmd == 16) // 앱에서 16 수신 시, 수동조작모드
				{
					GPIOD->ODR |= 0x1000; 	// GPIOG->ODR.0 'H' PD12 LED 켜서 수동 모드 표시
					// 순찰은 안끔
					manual = 1;  // 수동 조작 ON
				}
				else if (cmd == 17) // 앱에서 17 수신 시, 순찰 시작 on, 수동 OFF 
				{
					GPIOD->ODR &= ~0x1000; 	// 수동 모드 LED 끄기
					GPIOD->ODR |= 0x4000; 	// GPIOG->ODR.0 'H' // PD14 LED 켜서 순찰 모드 표시

					manual = 0; // 수동 조작 OFF (순찰중에 키기 가능)
					auto_move = 1;  // 자동 순찰 ON
				}
				// 직진성 보정 튜닝 명령 (모터의 좌우 편차를 소프트웨어로 맞춤)
				else if (cmd == 20) // [앱에서 20 수신] 우측 바퀴가 빠를 때 -> 우측 속도 1 감소
                {
                    right_offset += 1; // 빼줄 값을 1 증가시킴
                }
                else if (cmd == 21) // [앱에서 21 수신] 좌측 바퀴가 빠를 때 -> 좌측 속도 1 감소
                {
                    left_offset += 1; 
                }
                else if (cmd == 22) // [앱에서 22 수신] 튜닝 초기화 (영점 리셋)
                {
                    right_offset = 0;
                    left_offset = 0;
					turn_pulse_ms = 200;
                }
				else if (cmd == 23)
				{
					turn_pulse_ms -= 10; // 10ms씩 감소
                    if (turn_pulse_ms < 50) turn_pulse_ms = 50; // 너무 짧아지는 것 방지 (안전장치)
				}
				
			}
		}  // 앱 데이터 읽기 끝

		/*
		* 동작 Start
		*/
		// 수동 모드일 때 모터 제어 실행
		if(manual == 1)  
		{
			MANUAL_CON(); // 수동조작, manual이 1이면 동작함
		}

		// 자동 순찰 모드이고 동작 허가(enable_run)가 떨어졌을 때
		if((auto_move == 1) && enable_run == 1)  // enable_run은 라즈베리파이 동작비트
		{
			// 아직 타겟(situation_data)이 없고, 선을 벗어나지도 않았다면
			if((situation_data == 0) && (end_line == 0))
			{
				LINE_TRACING();  // 라인 트레이싱
			}
			// 라인 트레이싱은 끝났는데, 타겟은 없는 방황 상태
			else if((situation_data == 0) && (end_line == 1) )
			{
				// 로봇이 라인트레이싱은 종료했는데, 객체를 잃은 상황
				STOP_ROBOT();
			}
			else
			{
				end_line = 1; //라인 트레이싱 종료 (타겟 발견 등)
				// 수동조작 모드가 아닐때 라즈베리파이의 지시(drive_data)를 따름
				if( manual != 1)
				{
					switch (drive_data)
					{
						case 0:  // 정지해야 할 때
							STOP_ROBOT();
							DelayMS(20);
							break;
						case 1:  // 전진해야 할 때
							FRONT(SLOW_speed);
							DelayMS(20);
							break;
						case 2:  // 후진해야 할 때
							BACK(SLOW_speed);
							DelayMS(20);
							break;
						case 3:  // 좌회전 해야 할 때
							LEFT_TURN(TURN_speed);
							DelayMS(turn_pulse_ms);  // 지정된 시간만큼 동안 확실하게 돌기
							STOP_ROBOT();
							DelayMS(300);  // 모터 관성 대기 및 객체 찾을 시간 부여
							break;
						case 4:  // 우회전 해야 할 때
							RIGHT_TURN(TURN_speed);
							DelayMS(turn_pulse_ms);  // 지정된 시간만큼 동안 확실하게 돌기
							STOP_ROBOT();
							DelayMS(300);  // 모터 관성 대기 및 객체 찾을 시간 부여
							break;
					}
				}

				// 1 상태를 받아오다가 또 1을 받아오면 -> 기존에 킨건 끄고 끄면 키야하는데 1을 계속 받아오는동안 led를 껐다 키면 안되서 유지시키기. 기존 상황과 변경되었을따/
				// 상황(객체 인식 결과)이 변경되었을 때만 LED/부저 제어 (처음 찾았을 때도 제어)
				if((situation_data != prev_situation_data))
				{
					// 1. 상태가 바뀌었으므로 모든 경고 장치를 초기화 (끄기)
					GPIOA->ODR &= ~((1<<4) | (1<<5) | (1<<6) | (1<<7)); 
					GPIOD->ODR &= ~(1<<0); // 레이저(PD0) OFF
            		GPIOA->ODR |= (1<<8);  // 부저 OFF

					// 2. 라즈베리파이가 알려준 새로운 상황에 맞게 LED, 부저, 레이저 제어
					switch (situation_data)
					{
						case 0:  // 라인트레이싱하며 

							blink_flag = 0; //점멸 핸들러 OFF
							break;
						case 1: // 사람을 찾았을 때 ( 의심 )
							// 점멸이므로 
							blink_flag = 1; // GREEN 점멸
							break;
						case 2: // 사람 찾았을 때 (확정)
							blink_flag = 0; // 점멸 아니므로 끄기
							GPIOA->ODR |= (1<<4);  // GREEN LED ON			
							break;
						case 3: // 쓰러짐 의심
							blink_flag = 1; // BLUE 점멸
							break;
						case 4: // 쓰러짐 확정
							GPIOA->ODR &= ~(1<<8); // [수정] 부저 ON (Active-Low이므로 0을 할당)
							GPIOA->ODR |= (1<<5);  // BLUE LED ON
							break;
						case 5:  // 담배 의심
							blink_flag = 1; // yellow 점멸
							
							break;
						case 6:  //담배 확정
							blink_flag = 1;  // 부저 점멸
							GPIOA->ODR |= (1<<6);  // yellow LED ON
							break;
						case 7:  // 공격 의심
							blink_flag = 1;  // red 점멸
							break;
						case 8:  // 공격 확정
							blink_flag = 1;  // 부저 점멸
							GPIOA->ODR |= (1<<7);  // RED LED ON
							GPIOD->ODR |= (1<<0);  // 레이저(PD0) ON
							break;
					} // switch (situation_data)
					if (m_em == 1) // 비상 모드면 무조건 점멸
                    {
                        blink_flag = 1; 
                    }
					prev_situation_data = situation_data; // 현재 상태 기억
				}  //if((situation_data != prev_situation_data))
			} // else
		} // if((auto_move == 1) && enable_run == 1)
		
	}  // while
}
void LINE_TRACING(void)
{
	// PC8(왼쪽), PC9(오른쪽) 센서 값 읽기 (검은선 감지 시 1, 아니면 0)
	int left_sensor = (GPIOC->IDR & (1<<8)) ? 1 : 0;
	int right_sensor = (GPIOC->IDR & (1<<9)) ? 1 : 0;

	if (left_sensor == 1 && right_sensor == 1)
	{
		// 양쪽 모두 검은선 감지 (정지선 도착) -> 180도 턴 로직 시작
		STOP_ROBOT();
		DelayMS(200); // 멈출 때 관성에 의해 튀어나가지 않도록 대기
		
		// 1. 제자리 우회전 시작
		RIGHT_TURN(TURN_speed);   
		
		// 2.현재 밟고 있는 선에서 완전히 빠져나갈 때까지 잠시 대기
		// 이 딜레이가 없으면 돌자마자 바로 밑에 있는 선을 인식하고 멈춤
		// 모터 속도와 센서 위치에 따라 0.3초 ~ 0.5초 사이로 조절
		DelayMS(500); 
		
		// 3. 다시 선을 만날 때까지 무한 반복하며 회전 유지 (센서 피드백)
		int timeout_ms = 0; // 무한 루프 갇힘 방지를 위한 타임아웃 변수 추가
		while (1)
		{
			// 돌고 있는 동안 오른쪽 센서(PC9) 값을 실시간으로 다시 읽어옵니다.
			int current_right_sensor = (GPIOC->IDR & (1<<9)) ? 1 : 0;
			
			// 오른쪽으로 돌고 있으므로, 오른쪽 센서가 가장 먼저 선에 닿음
			
			if (current_right_sensor == 1)
			{
				// 센서가 다시 검은 선을 밟았다면 180도 회전 완료
				break; // while 문 탈출
			}
			DelayMS(1); // 1ms 대기 (정확한 시간 지연)
            timeout_ms++; // 1ms마다 1씩 증가
			// 4초(4000ms) 동안 선을 못 찾으면 강제 탈출
            if (timeout_ms > 4000) 
            {
                break; 
            }
		}
		
		// 4. 회전 완료 후 정지
		STOP_ROBOT();
		DelayMS(200); // 회전 후 몸체가 흔들리지 않게 안정화
	}
	else if (left_sensor == 1 && right_sensor == 0)
	{
		// 왼쪽 센서만 검은선을 밟음 (로봇이 우측으로 치우침) -> 좌회전으로 복귀
		LEFT_TURN(TURN_speed);
		DelayMS(20);
	}
	else if (left_sensor == 0 && right_sensor == 1)
	{
		// 오른쪽 센서만 검은선을 밟음 (로봇이 좌측으로 치우침) -> 우회전으로 복귀
		RIGHT_TURN(TURN_speed);
		DelayMS(20);
	}
	else // (left_sensor == 0 && right_sensor == 0)
	{
		// 양쪽 모두 흰색 배경 -> 직진
		FRONT(SLOW_speed); 
		DelayMS(20);
	}
}

void Read_RPI_Data(void)
{
    // 1. GPIOC의 IDR 레지스터에서 전체 값을 읽은 뒤, 
    //    쓸데없는 상위 8비트(PC8~15)를 0으로 날려버리고 PC0~7만 가져오기
    //    (0x00FF는 이진수로 0000 0000 1111 1111)
    RPI_data = GPIOC->IDR & 0x00FF;
	
    // --------------------------------------------------------
    // 2. 주행 데이터 추출 (PC0 ~ PC2)
    //    필요한 하위 3비트만 남김 (이진수 0000 0111 = 0x07)
    drive_data = RPI_data & 0x07; 
    
	// 0부터 7 까지의 값이 들어감
	
    // --------------------------------------------------------
    // 3. 상황 데이터 추출 (PC3 ~ PC6)
    //    오른쪽으로 3칸 밀어서(>> 3) PC3을 0번 자리로 맞춘 뒤, 
    //    4비트만 남김 (이진수 0000 1111 = 0x0F)
    situation_data = (RPI_data >> 3) & 0x0F;
	
	// 이제 situation_data 안에 0 부터 15까지 
	
	// 4. 동작 결정 비트 추출 (PC7)
    //    오른쪽으로 7칸 밀어서(>> 7) PC7을 0번 자리로 맞춘 뒤,
    //    1비트만 남김(이진수 0000 0001 = 0x01)
    enable_run = (RPI_data >> 7) & 0x01;
}

void _GPIO_Init(void)
{
	// LED (GPIO D) 설정 : Output mode
	// 포트 D 클럭 활성화
	RCC->AHB1ENR	|=  0x00000008;	// RCC_AHB1ENR : GPIOD(bit#3) Enable (수정 완료)						
	// 2. 기존 보드 내장 LED (GPIOD 8~15번 핀) 설정 유지
	GPIOD->MODER 	|=  0x55550000;	// GPIOD 8~15 : Output mode (0b01) (출력 모드)
	GPIOD->OTYPER	&= ~0xFF00;	    // GPIOD 8~15 : Push-pull (확실한 1과 0 출력)
	GPIOD->OSPEEDR 	|=  0x55550000;	// GPIOD 8~15 : Output speed 25MHZ Medium speed
	// PUPDR : Default (floating) 유지
	
	// ==========================================================
	// 3. [추가] 센서/제어용 핀 (GPIOD 0~5번 핀) 입출력 분할 설정
	// PD0~3: Output(출력), PD4~5: Input(입력)
	
	// 먼저 PD0~PD5 영역(비트 11:0)을 모두 00(Input)으로 깨끗하게 지움
	GPIOD->MODER 	&= ~0x00000FFF;
	
	// 지워진 상태에서 PD0~PD3 핀만 Output(01)으로 덮어쓰기
	GPIOD->MODER 	|=  0x00000055;

	// PD0~PD3 핀을 Push-pull 타입과 Medium 속도로 맞춰주기
	GPIOD->OTYPER	&= ~0x000F;  // PD0~PD3 Push-pull 설정
	GPIOD->OSPEEDR 	&= ~0x000000FF; // 속도 레지스터 초기화
	GPIOD->OSPEEDR 	|=  0x00000055; // Medium speed 덮어쓰기
	
	
	// 출력 설정 GPIOA (모터 방향 및 LED 등)  PA 0~ 12번 총 13개 핀 사용가능 
	// 모터 방향에 총 4핀 필요, 9핀 남음 
	// LED, 부저, 레이저 모듈, 라즈베리파이 통신 등 
	RCC->AHB1ENR    |=  0x00000001;	// RCC_AHB1ENR : GPIOH(bit#7)에 Clock Enable							
	GPIOA->MODER 	|=  0x01555555;	// GPIOA 0~12 : Output mode (0b01) 출력모드
	GPIOA->OTYPER	&= ~0x1FFF;	// GPIOA 0~12 : Push-pull  (GP0 ~ 12:reset state)
	GPIOA->OSPEEDR 	|=  0x15555555;	// GPIOA 0~12 : Output speed 25MHZ Medium speed
	// PUPDR : Default (floating)
	
	GPIOA->ODR |= (1<<8); // [추가] 부저 핀(PA8) 초기 상태를 OFF(HIGH)로 묶어두기
	
	// SW, 센서 입력 등(GPIO C) 설정 (C로 수정)
	RCC->AHB1ENR    |=  0x00000004;	// RCC_AHB1ENR : GPIOC(bit#2) Enable							
	GPIOC->MODER 	&= ~0xFFFFFFFF;	// GPIOC 0~15 : Input mode (reset state)
	GPIOC->PUPDR 	&= ~0xFFFFFFFF;	// GPIOC 0~15 : Floating input (No Pull-up, pull-down) :reset state
	GPIOC->PUPDR    |=  0x0000AAAA; // GPIOC 0 ~ 7 : pull-down // 라즈베리파이와 통신
	

}	
// 비어있는 GPIOE의 0번 핀(PE0)을 LED 출력으로 설정
void BLINK_LED_Init(void)
{
    // 1. GPIOE 클럭 활성화 (AHB1ENR 비트 4)
    RCC->AHB1ENR |= (1 << 4);
    
    // 2. PE0 모드 설정 (Output: 01)
    GPIOE->MODER &= ~(3 << 0); // 초기화
    GPIOE->MODER |= (1 << 0);  // Output 설정
    
    // 3. 출력 타입(Push-Pull) 및 속도(Medium) 설정
    GPIOE->OTYPER &= ~(1 << 0); 
    GPIOE->OSPEEDR |= (1 << 0);
}


// 중요 중요 중요 중요 중요
// main과 별개인, 이게 켜지면 main은 잠시 멈춤
// 500ms 주기로 인터럽트를 발생시키는 TIM3 초기화
void TIMER3_Interrupt_Init(void)
{
    // 1. TIM3 클럭 활성화 (APB1ENR 비트 1)
    RCC->APB1ENR |= (1 << 1);
    
    // 2. 타이머 주기 설정 (STM32F4 APB1 클럭 84MHz 기준)
    TIM3->PSC = 8400 - 1;   // 84MHz / 8400 = 10,000Hz (0.1ms 속도) (0.1ms마다 카운트 1 증가)
    TIM3->ARR = 4000 - 1;   // 0.1ms * 4000 = 400ms (0.4초 마다 인터럽트 발생)
    
    // 3. 업데이트 인터럽트 활성화 (인터럽트 발생 허용 레지스터 세팅)
    TIM3->DIER |= (1 << 0); // UIE (Update Interrupt Enable)
    
    // 4. NVIC 인터럽트 컨트롤러 켜기 (TIM3_IRQn은 29번) 
	// (NVIC 컨트롤 타워에서 TIM3 인터럽트 승인)
    // stm32f4xx.h 에 정의된 표준 함수 사용
    NVIC_EnableIRQ(TIM3_IRQn); 
    
    // 5. 타이머 시작 (카운터 Enable)
    TIM3->CR1 |= (1 << 0);
}
// 여기서 인터업트뭐할건지
// TIM3 인터럽트 핸들러 
void TIM3_IRQHandler(void)
{	
	//인터럽트 발생하면 sr레지스터에 1 참
    // TIM3 업데이트 인터럽트(Update Interrupt) 플래그 확인
    if (TIM3->SR & (1 << 0)) 
    {
        // 1. 인터럽트 플래그 클리어 (다음 인터럽트를 위해 무조건 해줘야 함)
        TIM3->SR &= ~(1 << 0); //직접 초기화
        
        // 2. 메인 루프에서 특정 조건일 때 (깜빡임 ON)

        if (blink_flag == 1) 
        {
            // GPIOA 5, 6, 7, 8번 핀의 출력 상태를 반전(Toggle)
			switch (situation_data)
			{
				case 1: // 사람 찾기
					GPIOA->ODR ^= (1<<4);  //  그린 점멸
					break;
				case 3: // 쓰러짐 의심
					GPIOA->ODR ^= (1<<5);  // 파란색 점멸
					break;
				case 5: // 담배 의심
					GPIOA->ODR ^= (1<<6);  //노란색 점멸
					break;
				case 6: // 담배 확정
					GPIOA->ODR ^= (1<<8);  //부저 점멸
					break;
				case 7:  // 공격 의심
					GPIOA->ODR ^= (1<<7);  // 빨간색 점멸
					break;
				case 8:  // 공격 확정
					GPIOA->ODR ^= (1<<8);  // 부저 점멸
					break;
			}
			if(m_em == 1)
			{
				GPIOA->ODR ^= (1<<7);  // 빨간색 점멸
				GPIOA->ODR ^= (1<<8);  // 부저 점멸	
			}
			
        }
    }
}
// 타이머 4를 이용한 PWM 출력 초기화 (모터 속도 조절)
// 가장 중요!!!!!!!!!!!! 면접용
/*




*/
void TIMER4_PWM_Init(void)
{   
// TIM CH3 : PB8 (167번 핀)
// Clock Enable : GPIOB & TIMER4

	// 1. 포트 B와 타이머 4 클럭 활성화
	RCC->AHB1ENR	|= (1<<1);	// GPIOB CLOCK Enable, 포트 B (PB8, PB9핀 사용)
	RCC->APB1ENR 	|= (1<<2);	// TIMER4 CLOCK Enable 
							
	GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4)); // PB8, PB9 자리 0000으로 초기화
	
// PB8을 출력설정하고 Alternate function(TIM4_CH3)으로 사용 선언 : PWM 출력
	GPIOB->MODER 	|= (2<<16);	// 0x00020000 PB8 Output Alternate function mode(AF 모드) -> 출력 부가가능모드로 사용하라(timer, 등, 아래에서 설정)
	//----------
	GPIOB->OSPEEDR 	|= (3<<16);	// 0x00030000 PB8 Output speed (100MHz High speed)
	GPIOB->OTYPER	&= ~(1<<8);	// PB8 Output type push-pull (reset state)
	GPIOB->PUPDR	|= (1<<16);	// 0x00010000 PB8 Pull-up(pull up 기본값 1)
	// Push-Pull 회로는 MCU 및 IC 의 내부 전원을 이용하여 출력포트의 값을 결정하는 것을 의미
	GPIOB->AFR[1]	|= (2<<0);	// 0x00000002 (AFR[1].(3~0)=0b0010): Connect TIM4 pins(PB8) to AF2(TIM3..5)
	// PB8을 AF2(TIM4 매핑)로 연결

// PB9를 출력설정하고 Alternate function(TIM4_CH4)으로 사용 선언 : PWM 출력
	GPIOB->MODER 	|= (2<<18);	// 0x00020000 PB9 Output Alternate function mode\
	//--------		
	GPIOB->OSPEEDR 	|= (3<<18);	// 0x00030000 PB9 Output speed (100MHz High speed)
	GPIOB->OTYPER	&= ~(1<<9);	// PB9 Output type push-pull (reset state)
	GPIOB->PUPDR	|= (1<<18);	// 0x00010000 PB9 Pull-up
	GPIOB->AFR[1]	|= (2<<4);	// 0x00000002 (AFR[1].(3~0)=0b0010): Connect TIM4 pins(PB8) to AF2(TIM3..5)
	

// 가장 중요!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// pwm 생성에 있어서 주기가
// TIM4 Channel 3 : PWM 1 mode
// PWM 주기 및 주파수 설정 (APB1 타이머 클럭 84MHz 기준)
	// Assign 'PWM Pulse Period'
	// 목표 가청주파수 넘기려고 20khz 넘기기

	// Prescaler: 84MHz를 42로 나누어 2MHz(0.5us) 속도로 톱니바퀴를 돌림
	TIM4->PSC = 42 - 1;  // 84MHz / 42 = 2000,000Hz (2000kHz) -1값은 규칙?? psc 분주기 설정시 -1 해야함

	// Auto-Reload: 카운터가 100이 되면 리셋. 즉, 주기 = 0.5us * 100 = 50us (20kHz 주파수 완성)
	TIM4->ARR	= 100-1;	// Auto reload  (0.0005ms * 100 = 0.05ms : PWM Period)  20Khz만들기
	// 0부터 100까지 새다가 100 넘으면 값을 0으로 내리기???? 100을 새기


	// 타이머 컨트롤 레지스터 설정 (Up counting 등 기본 설정)
	// Setting CR1 : 0x0000 (Up counting)
	TIM4->CR1 &= ~(1<<4);	// DIR=0(Up counter)(reset state) Up counter: (0에서 ARR까지 증가)
	TIM4->CR1 &= ~(1<<1);	// UDIS=0(Update event Enabled)
	TIM4->CR1 &= ~(1<<2);	// URS=0(Update event source Selection)g events
	TIM4->CR1 &= ~(1<<3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM4->CR1 |= (1<<7);	// ARPE=1(ARR is buffered): ARR Preload Enable (작동 중에 부드럽게 주기 변경 가능)
	TIM4->CR1 &= ~(3<<8); 	// CKD(Clock division)=00(reset state)
	TIM4->CR1 &= ~(3<<5); 	// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)
				
	// Define the corresponding pin by 'Output'  
	// CCER(Capture/Compare Enable Register) : Enable "Channel 3" 
	// 출력 채널(3, 4번) 활성화 및 극성 설정

	// 채널 3(PB8) 출력 허용
	TIM4->CCER	|= (1<<8);	// CC3E=1: OC3(TIM4_CH3) Active(Capture/Compare 3 output enable)
					// 해당핀(167번)을 통해 신호출력
	// 극성 HIGH (보통 상태 유지)
	TIM4->CCER	&= ~(1<<9);	// CC3P=0: CC3 Output Polarity (OCPolarity_High : OC3으로 반전없이 출력)

	
	// 채널 4(PB9) 출력 허용
	TIM4->CCER	|= (1<<12);	// CC4E=1: OC4(TIM4_CH4) Active(Capture/Compare 3 output enable)
	TIM4->CCER	&= ~(1<<13);	// CC4P=0: CC3 Output Polarity (OCPolarity_High : OC3으로 반전없이 출력)
	
	// 초기 듀티비(PWM 속도) 세팅 0~100 사이
	// Duty Ratio 
	TIM4->CCR4	= 60;		// CCR4 value (오리지날) arr 을 100까지 새다가 60까지 새면 신호 주기
	TIM4->CCR3	= 60;		// CCR3 value (오리지날)
	
	
	// PWM 모드 설정
	// 'Mode' Selection : Output mode, PWM 1
	// CCMR2(Capture/Compare Mode Register 2) : Setting the MODE of Ch3 or Ch4
	TIM4->CCMR2 &= ~(3<<0); // CC3S(CC3 channel)= '0b00' : Output 
	TIM4->CCMR2 |= (1<<3); 	// OC3PE=1: Output Compare 3 preload Enable
	
	TIM4->CCMR2	|= (6<<4);	// OC3M=0b110: Output compare 3 mode: PWM 1 mode //오리지날
	// 채널 3을 PWM Mode 1(110)로 설정 (카운터가 CCR보다 작을 때 HIGH 출력)
	//TIM4->CCMR2	|= (7<<4);	// OC3M=0b110: Output compare 3 mode: PWM 2 mode  //모드2로 바꾸기 
	
	TIM4->CCMR2	|= (1<<7);	// OC3CE=1: Output compare 3 Clear enable
	
	//졸작 추가 (채널4) PB9
	TIM4->CCMR2 &= ~(3<<8);  // CC4S = '0b00' : 채널 4를 출력(Output) 모드로 설정
	TIM4->CCMR2 |= (1<<11);  // OC4PE = 1 : Output Compare 4 Preload Enable (타이머 작동 중 부드러운 속도 변경 허용)
	
	TIM4->CCMR2	|= (6<<12);	 // OC4M = 0b110 : Output Compare 4 Mode를 PWM Mode 1로 설정
	
	TIM4->CCMR2	|= (1<<15);	// OC4CE = 1 : Output Compare 4 Clear Enable
	
	
	// Counter TIM4 enable (TIM4 메인 카운터 시작)
	TIM4->CR1	|= (1<<0);	// CEN: Counter TIM4 enable
}
// UART1 초기화 (HC-06 블루투스 통신용, 9600 bps)
void UART1_Bluetooth_Init(void)
{
    // 1. 클럭 활성화 (GPIOB, USART1)
    RCC->AHB1ENR |= (1 << 1);    // GPIOB Clock Enable
    RCC->APB2ENR |= (1 << 4);    // USART1 Clock Enable (APB2 영역)

    // 2. PB6(TX), PB7(RX) 핀을 Alternate Function(AF) 모드로 설정
    GPIOB->MODER &= ~((3 << 12) | (3 << 14)); 
    GPIOB->MODER |=  ((2 << 12) | (2 << 14)); // 0b10: AF 모드
    
    // 3. PB6, PB7을 AF7(USART1)로 매핑
    // STM32 매뉴얼을 보면 USART1은 AF7이라는 고유 번호를 가짐. 그래서 0x7이라는 값을 
	// PB6(24번째 비트 시작)과 PB7(28번째 비트 시작) 위치에 밀어 넣는 것
	
    GPIOB->AFR[0] &= ~(0xFF << 24);           // PB6, PB7 자리 초기화
    GPIOB->AFR[0] |=  (0x77 << 24);           // AF7(USART1) 설정

    // 4. 통신 속도(Baudrate) 설정 - 9600 bps
    // (STM32F407의 APB2 클럭이 84MHz일 경우 기준)
    // 84,000,000(클럭속도) / 9600 = 8750 = 0x222E
    USART1->BRR = 0x222E;  // 속도 조절 레지스터인 BRR

    // 5. USART1 활성화 (TX, RX, USART Enable)
    // 비트 3(TE: 송신 활성), 비트 2(RE: 수신 활성), 비트 13(UE: USART 활성)
    USART1->CR1 = (1 << 3) | (1 << 2) | (1 << 13); 
}

// 1바이트 수신 함수 (Non-blocking 방식)
// 데이터가 없으면 -1을 반환하여 메인 루프가 멈추지 않도록 함
int UART1_Read_Byte(void)
{
    // SR 레지스터의 비트 5 (RXNE: 데이터 수신 완료 플래그) 확인
    if (USART1->SR & (1 << 5)) 
    {
        return (USART1->DR & 0xFF); // 받은 데이터 반환
    }
    
    return -1; // 수신된 데이터가 없으면 -1 반환
}

void FRONT(int speed_front1)
{
	GPIOA->ODR &= ~0x000F;	// PA 0~3 LOW //모터 초기화
	
	int right_pwm = speed_front1 - right_offset;
    int left_pwm = speed_front1 - left_offset;
	
	if (right_pwm < 50) right_pwm = 50;
    if (left_pwm < 50) left_pwm = 50;
	
	
	TIM4->CCR3 = right_pwm;	 // PB6 (우)속도  
    TIM4->CCR4 = left_pwm;   // PB7 (좌)속도
	
	GPIOA->ODR |= 0x0002; 	// (PA0 OFF, PA1 ON) //1번모터 전진 L H
	GPIOA->ODR |= 0x0004; 	// (PA2 ON, PA3 OFF) //2번 모터 전진 H L

}

void BACK(int speed_BACK)
{
	GPIOA->ODR &= ~0x000F;	// PA 0~3 LOW //모터 초기화
	
	int right_pwm = speed_BACK - right_offset;
	int left_pwm = speed_BACK - left_offset;
	
	if (right_pwm < 50) right_pwm = 50;
	if (left_pwm < 50) left_pwm = 50;
	
	TIM4->CCR3 = right_pwm;	 // PB6 (우)속도
	TIM4->CCR4 = left_pwm;   // PB7 (좌)속도
	
	GPIOA->ODR |= 0x0001; 	// (PA0 On, PA1 OFF) //1번모터 후진 H L
	GPIOA->ODR |= 0x0008; 	// (PA2 Off, PA3 ON) //2번 모터 후진 L H

}
void FRONT_SLOW(int speed_front2)
{
	GPIOA->ODR &= ~0x000F;	// PA 0~3 LOW //모터 초기화
	
	TIM4->CCR3 = speed_front2;	 //PB6 (우 앞)속도  0 ~ 100
	TIM4->CCR4 = speed_front2;  //PB7 (우 뒤)속도  0 ~ 100
	GPIOA->ODR |= 0x0002; 	// (PA0 OFF, PA1 ON) //1번모터 전진 L H
	GPIOA->ODR |= 0x0004; 	// (PA2 ON, PA3 OFF) //2번 모터 전진 H L

}

void LEFT_TURN(int speed_left_turn)
{
	GPIOA->ODR &= ~0x000F;	// PA 0~3 LOW //모터 초기화
	
	TIM4->CCR3 = speed_left_turn;	//PB6 (우 앞)속도	// DR:0 ~ 100
	TIM4->CCR4 = (speed_left_turn);  //PB7 (좌)속도
	
	GPIOA->ODR |= 0x0002; 	// (PA0 OFF, PA1 ON) PA HIGH  //우측모터 전진 L H
	GPIOA->ODR |= 0x0008; 	// (PA2 OFF, PA3 ON) PA HIGH //좌측모터 후진 L H 
}


void RIGHT_TURN(int speed_right_turn)
{
	GPIOA->ODR &= ~0x000F;	// PA 0~3 LOW //모터 초기화
	
	TIM4->CCR3 = speed_right_turn;	//PB6 (우 앞)속도	// DR:0 ~ 100
	TIM4->CCR4 = (speed_right_turn);  //PB7 (좌)속도
	
	GPIOA->ODR |= 0x0001; 	// (PA0 ON, PA1 off) //우측모터 후진 H L
	GPIOA->ODR |= 0x0004; 	// (PA2 ON, PA3 off) //좌측 모터 전진 H L
}

void FRONT_LEFT(int speed1)
{
	GPIOA->ODR &= ~0x000F;	// PA 0~3 LOW //모터 초기화
	
	TIM4->CCR3 = speed1;	//PB6 (우)속도	// DR:0 ~ 100
	TIM4->CCR4 = (speed1-20);  //PB7 (좌)속도
	
	GPIOA->ODR |= 0x0002; 	// (PA0 OFF, PA1 ON) PA HIGH  //우측모터 전진 L H
	GPIOA->ODR |= 0x0004; 	// (PA2 ON, PA3 OFF) PA HIGH //좌측모터 전진 H L
}

void FRONT_RIGHT(int speed2)
{
	GPIOA->ODR &= ~0x000F;	// PA 0~3 LOW //모터 초기화
	
	TIM4->CCR3 = (speed2-20);	//PB6 (우)속도	// DR:0 ~ 100
	TIM4->CCR4 = speed2;  //PB7 (좌)속도
	
	GPIOA->ODR |= 0x0002; 	// (PA0 OFF, PA1 ON) PA HIGH  //우측모터 전진 L H
	GPIOA->ODR |= 0x0004; 	// (PA2 ON, PA3 OFF) PA HIGH //좌측모터 전진 H L
}

void STOP_ROBOT(void)
{
	GPIOA->ODR &= ~0x000F;	// PA 0~3 LOW //모터 초기화
	
	TIM4->CCR3 = 0;
	TIM4->CCR4 = 0;
}

void MANUAL_CON(void)
{
	if(cmd != prev_cmd)
	{
		// 방향 전환을 위해 새 명령을 내리기 전, 
        // 이전 상태가 '정지'가 아니었다면(즉, 달리고 있었다면) 먼저 브레이크를 밟음
		if (prev_cmd != 1 && cmd != 1 && manual == 1) 
		{
			STOP_ROBOT();
			DelayMS(100); // 0.1초 대기 (관성이 줄어들 시간)
		}
		// 안전한 새로운 동작
		if ((cmd == 10) && (manual == 1)) // 전진
		{
			FRONT(basic_speed);
		}
		else if ((cmd == 11) && (manual == 1)) // 
		{
			BACK(basic_speed);

		}
		else if ((cmd == 12) && (manual == 1)) // 
		{
			LEFT_TURN(basic_speed);

		}
		else if ((cmd == 13) && (manual == 1)) // 
		{
			RIGHT_TURN(basic_speed);

		}
		else if ((cmd == 14) && (manual == 1)) // 
		{
			FRONT_LEFT(basic_speed);

		}
		else if ((cmd == 15) && (manual == 1)) // 
		{
			FRONT_RIGHT(basic_speed);
		}
		prev_cmd = cmd;
	}
	// 필요한 숫자(명령어)만큼 else if 추가
}
void PI_FRONT(void)
{
	FRONT(SLOW_speed);
}
void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i=0; i<wMS; i++)
		DelayUS(1000);	// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS*17;
	for(; Dly; Dly--);
}
//실습에 쓴 코드

/*
#define u32_t unsigned int

int main()
{
	*(u32_t*)0x40023830 = *(u32_t*)0x40023830 | 0x00000009;
	*(u32_t*)0x40020C00 = *(u32_t*)0x40020C00 | 0x55000000;
	*(u32_t*)0x40020C08 = *(u32_t*)0x40020C08 | 0xff000000;
	*(u32_t*)0x40020C0C = *(u32_t*)0x40020C0C | 0xAA000000;
	*(u32_t*)0x40020C14 = *(u32_t*)0x40020C14 | 0x0000F000;

		
	while(1)
		;
	
  return 0;
}
*/

/***********************************************************************
// 보충 설명자료
// 다음은 stm32f4xx.h에 있는 RCC관련 주요 선언문임 
#define HSE_STARTUP_TIMEOUT    ((uint16_t)0x05000)   // Time out for HSE start up 
typedef enum {RESET = 0, SET = !RESET} FlagStatus, ITStatus;

#define FLASH_BASE            ((uint32_t)0x08000000) // FLASH(up to 1 MB) base address in the alias region                          
#define CCMDATARAM_BASE       ((uint32_t)0x10000000) // CCM(core coupled memory) data RAM(64 KB) base address in the alias region   3
#define SRAM1_BASE            ((uint32_t)0x20000000) // SRAM1(112 KB) base address in the alias region                              

#define SRAM2_BASE            ((uint32_t)0x2001C000) // SRAM2(16 KB) base address in the alias region                               
#define SRAM3_BASE            ((uint32_t)0x20020000) // SRAM3(64 KB) base address in the alias region                               

#define PERIPH_BASE           ((uint32_t)0x40000000) // Peripheral base address in the alias region                                 
#define BKPSRAM_BASE          ((uint32_t)0x40024000) // Backup SRAM(4 KB) base address in the alias region                          

// Peripheral memory map  
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000)
#define AHB2PERIPH_BASE       (PERIPH_BASE + 0x10000000)

// AHB1 peripherals  
#define GPIOA_BASE            (AHB1PERIPH_BASE + 0x0000)
#define GPIOB_BASE            (AHB1PERIPH_BASE + 0x0400)
#define GPIOC_BASE            (AHB1PERIPH_BASE + 0x0800)
#define GPIOD_BASE            (AHB1PERIPH_BASE + 0x0C00)
#define GPIOE_BASE            (AHB1PERIPH_BASE + 0x1000)
#define GPIOF_BASE            (AHB1PERIPH_BASE + 0x1400)
#define GPIOG_BASE            (AHB1PERIPH_BASE + 0x1800)
#define GPIOH_BASE            (AHB1PERIPH_BASE + 0x1C00)
#define GPIOI_BASE            (AHB1PERIPH_BASE + 0x2000)
#define GPIOJ_BASE            (AHB1PERIPH_BASE + 0x2400)
#define GPIOK_BASE            (AHB1PERIPH_BASE + 0x2800)
#define CRC_BASE              (AHB1PERIPH_BASE + 0x3000)
#define RCC_BASE              (AHB1PERIPH_BASE + 0x3800)
#define FLASH_R_BASE          (AHB1PERIPH_BASE + 0x3C00)
#define SYSCFG_BASE           (APB2PERIPH_BASE + 0x3800)
#define EXTI_BASE             (APB2PERIPH_BASE + 0x3C00)

typedef struct
{
  __IO uint32_t CR;            // RCC clock control register,                                  Address offset: 0x00  
  __IO uint32_t PLLCFGR;       // RCC PLL configuration register,                              Address offset: 0x04  
  __IO uint32_t CFGR;          // RCC clock configuration register,                            Address offset: 0x08  
  __IO uint32_t CIR;           // RCC clock interrupt register,                                Address offset: 0x0C  
  __IO uint32_t AHB1RSTR;      // RCC AHB1 peripheral reset register,                          Address offset: 0x10  
  __IO uint32_t AHB2RSTR;      // RCC AHB2 peripheral reset register,                          Address offset: 0x14  
  __IO uint32_t AHB3RSTR;      // RCC AHB3 peripheral reset register,                          Address offset: 0x18  
  __IO uint32_t APB1RSTR;      // RCC APB1 peripheral reset register,                          Address offset: 0x20  
  __IO uint32_t APB2RSTR;      // RCC APB2 peripheral reset register,                          Address offset: 0x24  
  __IO uint32_t AHB1ENR;       // RCC AHB1 peripheral clock register,                          Address offset: 0x30  
  __IO uint32_t AHB2ENR;       // RCC AHB2 peripheral clock register,                          Address offset: 0x34  
  __IO uint32_t AHB3ENR;       // RCC AHB3 peripheral clock register,                          Address offset: 0x38  
  __IO uint32_t APB1ENR;       // RCC APB1 peripheral clock enable register,                   Address offset: 0x40  
  __IO uint32_t APB2ENR;       // RCC APB2 peripheral clock enable register,                   Address offset: 0x44  
  __IO uint32_t AHB1LPENR;     // RCC AHB1 peripheral clock enable in low power mode register, Address offset: 0x50  
  __IO uint32_t AHB2LPENR;     // RCC AHB2 peripheral clock enable in low power mode register, Address offset: 0x54  
  __IO uint32_t AHB3LPENR;     // RCC AHB3 peripheral clock enable in low power mode register, Address offset: 0x58  
  __IO uint32_t APB1LPENR;     // RCC APB1 peripheral clock enable in low power mode register, Address offset: 0x60  
  __IO uint32_t APB2LPENR;     // RCC APB2 peripheral clock enable in low power mode register, Address offset: 0x64  
  __IO uint32_t BDCR;          // RCC Backup domain control register,                          Address offset: 0x70  
  __IO uint32_t CSR;           // RCC clock control & status register,                         Address offset: 0x74  
  __IO uint32_t SSCGR;         // RCC spread spectrum clock generation register,               Address offset: 0x80  
  __IO uint32_t PLLI2SCFGR;    // RCC PLLI2S configuration register,                           Address offset: 0x84  
  __IO uint32_t PLLSAICFGR;    // RCC PLLSAI configuration register,                           Address offset: 0x88  
  __IO uint32_t DCKCFGR;       // RCC Dedicated Clocks configuration register,                 Address offset: 0x8C  
  __IO uint32_t CKGATENR;      // RCC Clocks Gated Enable Register,                            Address offset: 0x90   // Only for STM32F412xG, STM32413_423xx and STM32F446xx devices  
  __IO uint32_t DCKCFGR2;      // RCC Dedicated Clocks configuration register 2,               Address offset: 0x94   // Only for STM32F410xx, STM32F412xG, STM32413_423xx and STM32F446xx devices  

} RCC_TypeDef;
	

typedef struct
{
  __IO uint32_t ACR;      // FLASH access control register,   Address offset: 0x00  
  __IO uint32_t KEYR;     // FLASH key register,              Address offset: 0x04  
  __IO uint32_t OPTKEYR;  // FLASH option key register,       Address offset: 0x08  
  __IO uint32_t SR;       // FLASH status register,           Address offset: 0x0C  
  __IO uint32_t CR;       // FLASH control register,          Address offset: 0x10  
  __IO uint32_t OPTCR;    // FLASH option control register ,  Address offset: 0x14  
  __IO uint32_t OPTCR1;   // FLASH option control register 1, Address offset: 0x18  
} FLASH_TypeDef;

typedef struct
{
  __IO uint32_t MODER;    // GPIO port mode register,               Address offset: 0x00       
  __IO uint32_t OTYPER;   // GPIO port output type register,        Address offset: 0x04       
  __IO uint32_t OSPEEDR;  // GPIO port output speed register,       Address offset: 0x08       
  __IO uint32_t PUPDR;    // GPIO port pull-up/pull-down register,  Address offset: 0x0C       
  __IO uint32_t IDR;      // GPIO port input data register,         Address offset: 0x10       
  __IO uint32_t ODR;      // GPIO port output data register,        Address offset: 0x14       
  __IO uint16_t BSRRL;    // GPIO port bit set/reset low register,  Address offset: 0x18       
  __IO uint16_t BSRRH;    // GPIO port bit set/reset high register, Address offset: 0x1A       
  __IO uint32_t LCKR;     // GPIO port configuration lock register, Address offset: 0x1C       
  __IO uint32_t AFR[2];   // GPIO alternate function registers,     Address offset: 0x20-0x24  
} GPIO_TypeDef;

typedef struct
{
  __IO uint32_t IMR;    // EXTI Interrupt mask register, Address offset: 0x00 
  __IO uint32_t EMR;    // EXTI Event mask register, Address offset: 0x04 
  __IO uint32_t RTSR;   // EXTI Rising trigger selection register,  Address offset: 0x08
  __IO uint32_t FTSR;   // EXTI Falling trigger selection register, Address offset: 0x0C
  __IO uint32_t SWIER;  // EXTI Software interrupt event register,  Address offset: 0x10 
  __IO uint32_t PR;     // EXTI Pending register, Address offset: 0x14 
} EXTI_TypeDef;

typedef struct
{
  __IO uint32_t MEMRMP;   // SYSCFG memory remap register, Address offset: 0x00 
  __IO uint32_t PMC;          // SYSCFG peripheral mode configuration register, Address offset: 0x04
  __IO uint32_t EXTICR[4];    // SYSCFG external interrupt configuration registers, Address offset: 0x08-0x14 
  __IO uint32_t CMPCR;        // SYSCFG Compensation cell control register,Address offset: 0x20

} SYSCFG_TypeDef;

#define GPIOA 	((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB	((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD   ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE  ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOF   ((GPIO_TypeDef *) GPIOF_BASE)
#define GPIOG   ((GPIO_TypeDef *) GPIOG_BASE)
#define GPIOH   ((GPIO_TypeDef *) GPIOH_BASE)
#define GPIOI   ((GPIO_TypeDef *) GPIOI_BASE)
#define GPIOJ   ((GPIO_TypeDef *) GPIOJ_BASE)
#define GPIOK   ((GPIO_TypeDef *) GPIOK_BASE)

#define CRC             ((CRC_TypeDef *) CRC_BASE)
#define RCC             ((RCC_TypeDef *) RCC_BASE)
#define FLASH           ((FLASH_TypeDef *) FLASH_R_BASE)

#define SYSCFG              ((SYSCFG_TypeDef *) SYSCFG_BASE)
#define EXTI                ((EXTI_TypeDef *) EXTI_BASE)

#define FLASH_ACR_PRFTEN             ((uint32_t)0x00000100)
#define FLASH_ACR_ICEN               ((uint32_t)0x00000200)
#define FLASH_ACR_DCEN               ((uint32_t)0x00000400)
#define FLASH_ACR_ICRST              ((uint32_t)0x00000800)
#define FLASH_ACR_DCRST              ((uint32_t)0x00001000)
#define FLASH_ACR_BYTE0_ADDRESS      ((uint32_t)0x40023C00)
#define FLASH_ACR_BYTE2_ADDRESS      ((uint32_t)0x40023C03)

#define FLASH_ACR_LATENCY_5WS        ((uint32_t)0x00000005)

typedef struct {
  __IO uint32_t ISER[8];  // Interrupt Set Enable Register    
  __IO uint32_t ICER[8];  // Interrupt Clear Enable Register  
  __IO uint32_t ISPR[8];  //  Interrupt Set Pending Register   
  __IO uint32_t ICPR[8];  //  Interrupt Clear Pending Register
  __IO uint32_t IABR[8];  //  Interrupt Active bit Register      
  __IO uint8_t  IP[240];  //  Interrupt Priority Register (8Bit) 
  __O  uint32_t STIR;  //  Software Trigger Interrupt Register    
}  NVIC_Type;    

// Memory mapping of Cortex-M4 Hardware 
#define SCS_BASE     (0xE000E000)    // System Control Space Base Address 
#define NVIC_BASE   (SCS_BASE +  0x0100)  // NVIC Base Address  
#define NVIC        ((NVIC_Type *)  NVIC_BASE) // NVIC configuration struct                                           

*/ 