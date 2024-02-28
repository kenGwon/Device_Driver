# Device_Driver

## 평가문항
입출력 다중화(Poll)와 블로킹I/O를 구현한 디바이스 드라이버

## 구현 조건
1. 디바이스 드라이버는 입출력 다중화(Poll), blocking I/O를 구현하여 처리할 데이터가 없을 시 프로세스를 대기 상태(Sleep on)로 전환하고 key 인터럽트 발생 시 wake up 하여 준비/실행 상태로 전환하여 처리한다.

2. 타이머 시간과 LED 값은 App 실행시 argument로 전달하여 처리한다.(미입력 시 사용법 출력 후 종료)<br>
   (사용법) Usage: keyled_app [led_val(0x00~0xff)] [time_val(1/100)]
   
3. ioctl() 함수를 사용하여 다음 명령어를 구현한다.
   TIMER_START: 커널 타이머 시작
   TIMER_STOP: 커널 타이머 정지
   TIMER_VALUE: led on/off 주기 변경

4. key값 읽기: read(), led값 쓰기: write()

5. key1를 입력 시 커널 타이머를 정지한다.<br>
   key2를 입력 시 키보드로 커널 타이머 시간을 100분의 1초 단위로 입력 받아 동작시킨다.<br>
   key3를 입력 시 키보드로 led값을 입력받아(0x00~0xff) 변경된 값으로 led on/off를 동작시킨다.<br>
   key4를 입력 시 커널 타이머를 동작시킨다.<br>

6. ioctl에 사용할 구조체 arg는 다음과 같은 형태로 제한한다.
    ```c
    typedef struct {
        unsigned long timer_val
    } __attribute__ ((packed__ )) keyled_data;
    ```

7. 디바이스 파일은 `/dev/keyled_dev`로 만들고, 주번호는 `230` 부번호는 `0`으로 지정한다.

<br><br>

## 구현 결과

### 어플리케이션 동작화면
![어플리케이션 동작화면](./동작%20이미지/어플리케이션%20동작화면.png)

### 커널 메시지
![커널 메시지](./동작%20이미지/커널메시지.png)
