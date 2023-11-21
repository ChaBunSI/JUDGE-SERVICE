# JUDGE-SERVICE
마지막으로 수정한 날짜: 2023-11-19 13:49
## 진행 상황  
* SQS에서 제출된 코드 받아오기 완료
* 받은 메세지를 rapidjson을 이용해, 파싱하여 제공된 정보를 사용, 채점 완료
* 채점 결과를 다시 json의 형태로 만들어 SNS로 보내기 완료

## 채점 과정  
1) 채점 큐 (SQS)에서 채점 정보를 빼냅니다.
2) 빼낸 정보를 바탕으로 채점을 진행합니다.  
   주어진 코드를 언어에 맞게 컴파일 후, 테스트케이스의 입력을 코드에 넣어 샌드박스 환경에서 실행합니다.  
   (샌드박스 환경: [isolate](https://github.com/ioi/isolate), [isolate docs](https://www.ucw.cz/moe/isolate.1.html))  
3) 샌드박스 내 저장된 출력 파일과 테스트케이스의 출력 파일을 비교하여 채점합니다.  
4) 채점 결과를 SNS로 보냅니다.  
   이때 채점 결과의 형식(body)는 다음과 같습니다.  
   |이름|설명|  
   |---|---|
   |id|**제출** 번호|  
   |user_id|사용자 아이디|
   |problem_id|문제 번호|
   |language_code|언어 코드|
   |memory_limited|메모리 제한(MB)|
   |time_limited|시간 제한(ms)|
   |judge_result|채점 결과|
   |error_message|에러 메세지: 컴파일 에러, 런타임 에러 등|  
이때 에러 메세지는 컴파일 에러, 런타임 에러일 때만 존재합니다.  
다른 경우엔 null로 보냅니다.

### SNS로 보내는 데이터 예시
```json
{
    "id": 12485120,
    "user_id": 124125,
    "problem_id": 30461,
    "language_code": 1,       // CPP
    "memory_limited": 1024,   // MB
    "time_limited": 1000,     // ms
    "judge_result": 1,        // AC
    "error_message": null
}
```
AC나 WA같이 에러가 따로 발생하지 않았다면,  
error_message는 **null**로 보냅니다.  
- judge_result: 정수의 형태로 주어집니다.
```cpp
enum judge_result {
    NJ, AC, WA, CE, RE, TLE, MLE, OLE, SE
};
```
|채점 결과| 값 | 설명|
|---|---|---|
|NJ|0|Not Judged, 채점되지 않음. |
|AC|1|Accepted, 정답. 출력 결과가 일치함|
|WA|2|Wrong Answer, 오답. 출력 결과가 일치하지 않음|
|CE|3|Compile Error, 컴파일 에러|
|RE|4|Runtime Error, 런타임 에러|
|TLE|5|Time Limit Exceeded, 시간 초과|
|MLE|6|Memory Limit Exceeded, 메모리 초과|
|OLE|7|Output Limit Exceeded, 출력 초과|
|SE|8|Sandbox Error, 샌드박스 에러|

- language_code: 정수의 형태로 주어집니다.
```cpp
enum language_code {
    C, JAVA, PYTHON, CPP
};
```
|언어| 값 |
|---|---|
|C|0|
|CPP|1|
|Java|2|
|Python|3|
|DEFAULT|9|

## 예상 구조도  
![예상 구조도](./images/architecture.png)
추후 그림은 언제든지 수정될 수 있습니다.  

## Dependency  
* [isolate](https://www.github.com/ioi/isolate)
* [AWS SQS](https://aws.amazon.com/ko/sqs/)
* [AWS SNS](https://aws.amazon.com/ko/sns/)
* [rapidjson](https://github.com/Tencent/rapidjson/)
  ```bash
  sudo apt-get install rapidjson-dev
  ```

## TODO
* 채점
  - 제출된 코드의 시간 / 메모리 사용량 측정  
    정확한 측정이 가능하면, 측정한 값 또한 **채점 결과**에 포함시키기
  - 컴파일 에러 / 런타임 에러 발생 시, 가능하면 에러 메세지 얻어오기
  - 폴링 방식 개선 필요:  
    현재는 SQS로 받은 메세지가 없으면, 10초간 sleep하는 방식.  
    더 나은 방식으로 개선하여야 한다.
  - **채점 서버 성능 최적화**
* 확장성
  - 컨테이너를 사용하여 배포 필요
  - 채점 서버의 수를 늘릴 수 있어야 한다.
    + 채점 서버: 제출 수신 (SQS) -> 처리 (채점) -> 발신 (SNS)
    + 확장성을 위해서 수신하는 부분과 처리하는 부분을 분리할 필요가 있어 보인다.  
* 동기화
  - 문제 관리 서비스와의 동기화 필요
  - 문제 CRUD와 채점 직렬화 필요
    + 문제가 추가되면, 채점 서버에도 해당 문제의 테스트케이스를 추가해야 한다.
    + 위의 경우와 같이 문제와 함께 테스트케이스 CRUD가 가능하여야 함.
    + 현재 채점 서버의 기능을 분리하여, 문제 관리 서비스와 동기화할 수 있도록 해야 한다.  
    + SQS로 제출 받아와 채점 작업을 나눠주는 브로커와 채점만 하는 워커로 나누기..?

## 조금 더 자세한 설명
 
[Notion](https://dripbox.notion.site/88eaba989d5e4a36a45771e835cb836f?pvs=4)
