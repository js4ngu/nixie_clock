# Nixie Clock

ESP32 DevKit V1 기반의 닉시 클럭 펌웨어입니다.  
GPS와 DS3231 RTC를 함께 사용하고, ESP32 SoftAP 웹 UI로 상태 확인과 설정을 할 수 있습니다.

## 주요 기능

- GPS 시간 수신
- DS3231 RTC 유지 및 GPS 기준 동기화
- 전원 재인가 후에도 RTC 시간 유지
- SoftAP 기반 웹 제어 페이지
- 디밍 값 저장
- 다이버전스미터 모드 주기 설정
- 다이버전스 이펙트 시간 설정
- 웹에서 RTC 시간 동기화
  - 브라우저 현재 시간으로 동기화
  - 수동 날짜/시간 입력
- 웹에서 RTC LOG / DEBUG LOG 확인

## 하드웨어 기준

- 보드: `ESP32 DevKit V1`
- RTC: `DS3231`
- GPS: UART 출력 가능한 GPS 모듈
- 4자리 닉시 표시부 또는 4채널 숫자 표시 출력 회로

## 핀맵

### GPS

- `GPIO19`: GPS RX 입력
- `GPIO18`: GPS TX 출력
- Baud: `9600`

코드 기준 연결:

- `ESP32 GPIO19 <- GPS TX`
- `ESP32 GPIO18 -> GPS RX`

### RTC (I2C)

- `GPIO5`: SDA
- `GPIO17`: SCL

### Display

- `GPIO32`: DATA
- `GPIO33`: DATA2
- `GPIO25`: CLK
- `GPIO14`: LATCH
- `GPIO26`: OE
- `GPIO27`: CLR

## 동작 방식

### 시간 소스

- GPS lock이 잡히면 GPS 시간을 사용합니다.
- GPS lock이 없으면 RTC 시간을 사용합니다.
- GPS 시간으로 RTC를 KST 기준으로 동기화합니다.

### RTC 초기화 정책

- DS3231이 정상 상태면 기존 RTC 시간을 그대로 유지합니다.
- `rtc.lostPower()` 상태일 때만 컴파일 시각으로 RTC를 복구합니다.
- 웹 UI에서 RTC 시간을 수동으로 다시 맞출 수 있습니다.

### 다이버전스미터 이펙트

- 설정한 주기마다 잠깐 랜덤 숫자를 표시합니다.
- 이펙트 지속 시간도 웹에서 조절할 수 있습니다.
- 현재 기본값:
  - 주기: `3600초`
  - 효과 시간: `2500ms`

## 웹 UI

ESP32는 부팅 후 SoftAP를 띄웁니다.

- SSID: `NixieClock`
- Password: `12345678`

접속 후 브라우저에서 아래 주소로 접근합니다.

- `http://192.168.4.1`

### 웹에서 할 수 있는 것

- GPS 상태 확인
- 현재 디밍 값 확인 및 변경
- 현재 다이버전스 주기 확인 및 변경
- 현재 다이버전스 효과 시간 확인 및 변경
- 브라우저 시간으로 RTC 동기화
- 수동으로 RTC 시간 입력
- RTC LOG 확인
- DEBUG LOG 확인

## 저장되는 설정

ESP32 `Preferences`에 아래 값이 저장됩니다.

- 디밍 값
- 다이버전스 주기
- 다이버전스 효과 시간

전원 재시작 후에도 유지됩니다.

## 프로젝트 구조

```text
firmware/
  firmware.ino         전체 초기화 진입점
  app_state.*          전역 상태 저장
  gps_task.*           GPS 수신 태스크
  rtc_task.*           DS3231 RTC 태스크
  display_task.*       표시부 출력 태스크
  web_server.*         SoftAP + 웹 UI + API
  settings_store.*     Preferences 저장/로드
  debug_log.*          링버퍼 로그 저장
  debug_config.h       태스크별 디버그 스위치
```

## 빌드 환경

확인된 환경:

- `arduino-cli 1.4.1`
- `esp32:esp32 3.3.7`

## 필수 라이브러리

코드에서 사용하는 주요 라이브러리:

- `WiFi.h`
- `WebServer.h`
- `TinyGPSPlus`
- `RTClib`
- `Preferences`

Arduino IDE 또는 `arduino-cli lib install`로 설치합니다.

예:

```bash
arduino-cli lib install "TinyGPSPlus"
arduino-cli lib install "RTClib"
```

## 빌드

프로젝트 루트에서:

```bash
arduino-cli compile \
  --build-path /tmp/nixie-build \
  --fqbn esp32:esp32:esp32doit-devkit-v1 \
  firmware
```

## 업로드

포트를 먼저 확인합니다.

```bash
arduino-cli board list
```

예시 업로드:

```bash
arduino-cli upload \
  -p /dev/cu.usbserial-0001 \
  --fqbn esp32:esp32:esp32doit-devkit-v1 \
  --input-dir /tmp/nixie-build \
  firmware
```

주의:

- Arduino IDE 시리얼 모니터가 열려 있으면 업로드가 실패할 수 있습니다.
- 포트가 busy 상태면 시리얼 모니터를 먼저 닫아야 합니다.

## 시리얼 모니터

```bash
arduino-cli monitor -p /dev/cu.usbserial-0001 -c baudrate=115200
```

로그 예시:

- `RTC INIT`
- `RTC TASK`
- `GPS RAW`
- `GPS TASK`
- `SETTINGS`

## 디버그 설정

`firmware/debug_config.h`에서 태스크별 로그를 켜고 끌 수 있습니다.

예:

- `DEBUG_GPS_TASK`
- `DEBUG_RTC_TASK`
- `DEBUG_DISPLAY_TASK`
- `DEBUG_WEB_SERVER_TASK`
- `DEBUG_WEB_API`

## 현재 알려진 점

- GPS는 위치 정보까지 valid여야 lock으로 처리합니다.
- 디스플레이 숫자 매핑은 현재 `digit & 0x0F` 기반의 단순 구현입니다.
- 닉시 드라이버 회로에 따라 `digitToByte()`는 추후 별도 매핑 테이블로 바꿔야 할 수 있습니다.

## 추천 테스트 순서

1. 전원 인가 후 `NixieClock` AP 접속
2. `http://192.168.4.1` 접속
3. RTC 시간이 맞지 않으면 `브라우저 시간으로 동기화` 실행
4. 디밍, 다이버전스 주기, 효과 시간을 변경해 동작 확인
5. GPS 수신 환경에서 GPS lock 및 RTC 동기화 확인
6. 전원 차단 후 RTC 유지 여부 확인
