# `clampedExpSerial` 함수 벡터화
다음과 같이 코드를 수정했다. 
```cpp
void clampedExpVector(float* values, int* exponents, float* output, int N) {
  __cs149_vec_float x;
  __cs149_vec_int y;
  __cs149_vec_float result;
  __cs149_vec_int zero_int = _cs149_vset_int(0);
  __cs149_vec_int one_int = _cs149_vset_int(1);
  __cs149_vec_float clamping_float = _cs149_vset_float(9.999999f);
  __cs149_vec_int count;
  __cs149_mask maskAll, maskIsZero, maskIsNotZero, maskIsClamped;
  for(int i=0;i<N;i+=VECTOR_WIDTH)
  {
    if(i+VECTOR_WIDTH > N) maskAll = _cs149_init_ones(N-i);
    else maskAll = _cs149_init_ones();
    _cs149_vload_float(x, values+i, maskAll);
    _cs149_vload_int(y, exponents+i, maskAll);
    
    _cs149_veq_int(maskIsZero, y, zero_int, maskAll); // if(y == 0)
    _cs149_vset_float(result, 1.f, maskIsZero); // result = 1.f
    _cs149_vgt_int(maskIsNotZero, y, zero_int, maskAll);
    // else
    _cs149_vmove_float(result, x, maskIsNotZero); // result = x
    _cs149_vsub_int(count, y, one_int, maskIsNotZero); // count = y - 1
    while(_cs149_cntbits(maskIsNotZero))
    {
      _cs149_vgt_int(maskIsNotZero, count, zero_int, maskIsNotZero);
      _cs149_vmult_float(result, result, x, maskIsNotZero); // result *= y
      _cs149_vsub_int(count, count, one_int, maskIsNotZero); // count -= 1
    }
    _cs149_vgt_float(maskIsClamped, result, clamping_float, maskAll); // if (result > 9.99999f)
    _cs149_vmove_float(result, clamping_float, maskIsClamped); // result = 9.999999f
    _cs149_vstore_float(output+i, result, maskAll); // output[i] = result
  }
}
```
N이 VECTOR_WIDTH로 나누어떨어지지 않는 경우에는 `maskAll`의 나머지를 0으로 설정하는 `_cs149_init_ones()_` 함수를 이용했다. 

```cpp
__cs149_mask _cs149_init_ones(int first) {
  __cs149_mask mask;
  for (int i=0; i<VECTOR_WIDTH; i++) {
    mask.value[i] = (i<first) ? true : false;
  }
  return mask;
}
```
N % VECTOR_WIDTH를 `first` 변수로 넘겨주면 남은 mask는 그대로 비운다. 이때 나머지 연산은 cost가 크므로 빼기 연산을 이용해 값을 구해주었다. 
```cpp
if(i+VECTOR_WIDTH > N) maskAll = _cs149_init_ones(N-i);
else maskAll = _cs149_init_ones();
```
다음으로 while문을 구현하기 위해 `_cs149_cntbits()` 함수를 이용하였다. 
```cpp
int _cs149_cntbits(__cs149_mask &maska) {
  int count = 0;
  for (int i=0; i<VECTOR_WIDTH; i++) {
    if (maska.value[i]) count++;
  }
  CS149Logger.addLog("cntbits", _cs149_init_ones(), VECTOR_WIDTH);
  return count;
}
```
이 함수는 마스크의 1의 개수를 센다. 현재 실행중인 vector의 각 원소가 전부 완료되어야 하므로 `maskIsNotZero` 마스크를 `_cs149_cntbits()` 함수의 입력으로 주었다. 
```cpp
while(_cs149_cntbits(maskIsNotZero))
{
    _cs149_vgt_int(maskIsNotZero, count, zero_int, maskIsNotZero);
    _cs149_vmult_float(result, result, x, maskIsNotZero); // result *= y
    _cs149_vsub_int(count, count, one_int, maskIsNotZero); // count -= 1
}
```
```
Results matched with answer!
****************** Printing Vector Unit Statistics *******************
Vector Width:              4
Total Vector Instructions: 163
Vector Utilization:        72.4%
Utilized Vector Lanes:     472
Total Vector Lanes:        652
************************ Result Verification *************************
Passed!!!
```
결과가 잘 나옴을 확인할 수 있다. 

한편 N = 5와 같이 `VECTOR_WIDTH`로 나누어떨어지지 않는 경우에도
```
Results matched with answer!
****************** Printing Vector Unit Statistics *******************
Vector Width:              4
Total Vector Instructions: 53
Vector Utilization:        63.7%
Utilized Vector Lanes:     135
Total Vector Lanes:        212
************************ Result Verification *************************
Passed!!!
```
결과가 잘 나옴을 확인할 수 있다. 

# `VECTOR_WIDTH` 변경
`./myexp -s 10000`을 실행하고, `VECTOR_WIDTH`를 2, 4, 8, 16으로 변경하였을 때의 결과는 아래와 같다. 

|`VECTOR_WIDTH`|vector utilization|
|:---:|:---:|
|2|76.3%|
|4|67%|
|8|63.7%|
|16|62.2%|

`VECTOR_WIDTH`가 증가할 때 vector utilization이 감소함을 확인할 수 있다. 이는 `VECTOR_WIDTH`가 증가할수록 모든 벡터의 원소가 같은 방식으로 동작하지 않기 때문이다. 특히 while()문의 경우 하나의 원소라도 연산이 끝나지 않으면 계속 동작하는데, 이는 vector utilization을 감소시키는 주요한 원인이 된다. 

# `arraySumSerial` 함수 벡터화
다음과 같이 코드를 수정했다. 
```cpp
float arraySumVector(float* values, int N) {
  
  //
  // CS149 STUDENTS TODO: Implement your vectorized version of arraySumSerial here
  //
  __cs149_vec_float partial_sum = _cs149_vset_float(0.f);
  __cs149_mask maskAll;
  float total_sum = 0.f;
  for (int i=0; i<N; i+=VECTOR_WIDTH) {
    // All ones
    if(i+VECTOR_WIDTH > N) maskAll = _cs149_init_ones(N-i);
    else maskAll = _cs149_init_ones();
    _cs149_vload_float(partial_sum, values+i, maskAll);
    for(int k=0;k<log2(VECTOR_WIDTH);k++)
    {
      _cs149_hadd_float(partial_sum, partial_sum);
      _cs149_interleave_float(partial_sum, partial_sum);
    }
    total_sum += partial_sum.value[0];
  }
  return total_sum;
}
```
우선 함수 `_cs149_hadd_float()` 함수를 보면
```cpp
void _cs149_hadd(__cs149_vec<T> &vecResult, __cs149_vec<T> &vec) {
  for (int i=0; i<VECTOR_WIDTH/2; i++) {
    T result = vec.value[2*i] + vec.value[2*i+1];
    vecResult.value[2 * i] = result;
    vecResult.value[2 * i + 1] = result;
  }
}
```
vector에서 인접한 원소들의 합을 구해 다시 해당 원소에 값을 넣음을 확인할 수 있다. 이를 $\log_2 \text{VECTOR\_WIDTH}$번 반복하면 `partial_sum`을 구할 수 있을 것이다. 
($\log_2$ 함수는 `math.h` 헤더에 존재한다. )
이때 vector에서 인접한 원소들의 합을 구해야 하므로 이를 재배열하는 과정이 필요하다. 여기서 `_cs149_interleave_float()` 함수를 보면
```cpp
void _cs149_interleave(__cs149_vec<T> &vecResult, __cs149_vec<T> &vec) {
  for (int i=0; i<VECTOR_WIDTH; i++) {
    int index = i < VECTOR_WIDTH/2 ? (2 * i) : (2 * (i - VECTOR_WIDTH/2) + 1);
    vecResult.value[i] = vec.value[index];
  }
}
```
위와 같이 vector를 `_cs149_hadd_float` 함수에 넣기 좋게 재배열해주고 있음을 알 수 있다. 예를 들어 `VECTOR_WIDTH`가 4일 때 for문 안은 아래와 같이 펼쳐진다. 
```
vecResult.value[0] = vec.value[0];
vecResult.value[1] = vec.value[2];
vecResult.value[2] = vec.value[1];
vecResult.value[3] = vec.value[3];
```
이때
```
ARRAY SUM (bonus) 
Passed!!!
```
구현이 잘 되었음을 확인할 수 있다. 
여기서 주의할 점으로는 위의 펼쳐진 for문에서 보듯 
```
vecResult.value[1] = vec.value[2];
vecResult.value[2] = vec.value[1];
```
함수 안에 같은 벡터 `partial_sum`을 넣어 `partial_sum[2]`의 값은 사라진다. 이때 합한 값은 앞쪽으로 모이고 있음을 알 수 있는데, 따라서 `partial_sum[0]`의 값만을 사용해야 한다. 